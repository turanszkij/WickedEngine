#include "wiPhysics.h"
#include "wiScene.h"
#include "wiProfiler.h"
#include "wiBacklog.h"
#include "wiJobSystem.h"
#include "wiRenderer.h"
#include "wiTimer.h"

#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

#include <mutex>
#include <memory>

using namespace wi::ecs;
using namespace wi::scene;

namespace wi::physics
{
	namespace bullet
	{
		bool ENABLED = true;
		bool SIMULATION_ENABLED = true;
		bool DEBUGDRAW_ENABLED = false;
		int ACCURACY = 8;
		int softbodyIterationCount = 5;
		float TIMESTEP = 1.0f / 120.0f;
		std::mutex physicsLock;

		class DebugDraw final : public btIDebugDraw
		{
			void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
			{
				wi::renderer::RenderableLine line;
				line.start = XMFLOAT3(from.x(), from.y(), from.z());
				line.end = XMFLOAT3(to.x(), to.y(), to.z());
				line.color_start = line.color_end = XMFLOAT4(color.x(), color.y(), color.z(), 1.0f);
				wi::renderer::DrawLine(line);
			}
			void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override
			{
			}
			void reportErrorWarning(const char* warningString) override
			{
				wi::backlog::post(warningString);
			}
			void draw3dText(const btVector3& location, const char* textString) override
			{
				wi::renderer::DebugTextParams params;
				params.position.x = location.x();
				params.position.y = location.y();
				params.position.z = location.z();
				params.scaling = 0.6f;
				params.flags |= wi::renderer::DebugTextParams::CAMERA_FACING;
				params.flags |= wi::renderer::DebugTextParams::CAMERA_SCALING;
				wi::renderer::DrawDebugText(textString, params);
			}
			void setDebugMode(int debugMode) override
			{
			}
			int getDebugMode() const override
			{
				int retval = 0;
				retval |= DBG_DrawWireframe;
				retval |= DBG_DrawText;
				return retval;
			}
		};
		DebugDraw debugDraw;

		struct PhysicsScene
		{
			btSoftBodyRigidBodyCollisionConfiguration collisionConfiguration;
			btDbvtBroadphase overlappingPairCache;
			btSequentialImpulseConstraintSolver solver;
			btCollisionDispatcher dispatcher = btCollisionDispatcher(&collisionConfiguration);
			btSoftRigidDynamicsWorld dynamicsWorld = btSoftRigidDynamicsWorld(&dispatcher, &overlappingPairCache, &solver, &collisionConfiguration);
		};
		PhysicsScene& GetPhysicsScene(Scene& scene)
		{
			if (scene.physics_scene == nullptr)
			{
				auto physics_scene = std::make_shared<PhysicsScene>();

				btContactSolverInfo& solverInfo = physics_scene->dynamicsWorld.getSolverInfo();
				solverInfo.m_solverMode |= SOLVER_RANDMIZE_ORDER;
				solverInfo.m_splitImpulse = true;

				btDispatcherInfo& dispatcherInfo = physics_scene->dynamicsWorld.getDispatchInfo();
				dispatcherInfo.m_enableSatConvex = true;

				physics_scene->dynamicsWorld.setDebugDrawer(&debugDraw);

				btSoftBodyWorldInfo& softWorldInfo = physics_scene->dynamicsWorld.getWorldInfo();
				softWorldInfo.air_density = btScalar(1.2f);
				softWorldInfo.water_density = 0;
				softWorldInfo.water_offset = 0;
				softWorldInfo.water_normal = btVector3(0, 0, 0);
				softWorldInfo.m_sparsesdf.Initialize();

				scene.physics_scene = physics_scene;
			}
			return *(PhysicsScene*)scene.physics_scene.get();
		}

		struct RigidBody
		{
			std::shared_ptr<void> physics_scene;
			std::unique_ptr<btCollisionShape> shape;
			std::unique_ptr<btRigidBody> rigidBody;
			btDefaultMotionState motionState;
			btTriangleIndexVertexArray triangles;
			Entity entity = INVALID_ENTITY;

			// for trace hit reporting:
			wi::ecs::Entity humanoid_ragdoll_entity = wi::ecs::INVALID_ENTITY;
			wi::scene::HumanoidComponent::HumanoidBone humanoid_bone = wi::scene::HumanoidComponent::HumanoidBone::Count;

			// These are used to remap default shape orientations into ragdoll and back:
			btTransform additionalTransform;
			btTransform additionalTransformInverse;
			btTransform restBasis;
			btTransform restBasisInverse;

			RigidBody()
			{
				additionalTransform.setIdentity();
				additionalTransformInverse.setIdentity();
				restBasis.setIdentity();
				restBasisInverse.setIdentity();
			}
			~RigidBody()
			{
				if (physics_scene == nullptr)
					return;
				btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)physics_scene.get())->dynamicsWorld;
				dynamicsWorld.removeRigidBody(rigidBody.get());
			}
		};
		struct SoftBody
		{
			std::shared_ptr<void> physics_scene;
			std::unique_ptr<btSoftBody> softBody;
			Entity entity = INVALID_ENTITY;
			~SoftBody()
			{
				if (physics_scene == nullptr)
					return;
				btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)physics_scene.get())->dynamicsWorld;
				dynamicsWorld.removeSoftBody(softBody.get());
			}
		};

		RigidBody& GetRigidBody(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
		{
			if (physicscomponent.physicsobject == nullptr)
			{
				physicscomponent.physicsobject = std::make_shared<RigidBody>();
			}
			return *(RigidBody*)physicscomponent.physicsobject.get();
		}
		SoftBody& GetSoftBody(wi::scene::SoftBodyPhysicsComponent& physicscomponent)
		{
			if (physicscomponent.physicsobject == nullptr)
			{
				physicscomponent.physicsobject = std::make_shared<SoftBody>();
			}
			return *(SoftBody*)physicscomponent.physicsobject.get();
		}
	}
	using namespace bullet;

	void Initialize()
	{
		wi::Timer timer;

		wi::backlog::post("wi::physics Initialized [Bullet] (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	bool IsEnabled() { return ENABLED; }
	void SetEnabled(bool value) { ENABLED = value; }

	bool IsSimulationEnabled() { return ENABLED && SIMULATION_ENABLED; }
	void SetSimulationEnabled(bool value) { SIMULATION_ENABLED = value; }

	bool IsDebugDrawEnabled() { return DEBUGDRAW_ENABLED; }
	void SetDebugDrawEnabled(bool value) { DEBUGDRAW_ENABLED = value; }

	int GetAccuracy() { return ACCURACY; }
	void SetAccuracy(int value) { ACCURACY = value; }

	float GetFrameRate() { return 1.0f / TIMESTEP; }
	void SetFrameRate(float value) { TIMESTEP = 1.0f / value; }

	void AddRigidBody(
		wi::scene::Scene& scene,
		Entity entity,
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const wi::scene::TransformComponent& transform,
		const wi::scene::MeshComponent* mesh
	)
	{
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		physicsobject.entity = entity;

		switch (physicscomponent.shape)
		{
		case RigidBodyPhysicsComponent::CollisionShape::BOX:
			physicsobject.shape = std::make_unique<btBoxShape>(btVector3(physicscomponent.box.halfextents.x, physicscomponent.box.halfextents.y, physicscomponent.box.halfextents.z));
			break;
		case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
			physicsobject.shape = std::make_unique<btSphereShape>(btScalar(physicscomponent.sphere.radius));
			break;
		case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
			physicsobject.shape = std::make_unique<btCapsuleShape>(btScalar(physicscomponent.capsule.radius), btScalar(physicscomponent.capsule.height));
			break;

		case RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL:
			if(mesh != nullptr)
			{
				physicsobject.shape = std::make_unique<btConvexHullShape>();
				btConvexHullShape* convexHull = (btConvexHullShape*)physicsobject.shape.get();
				for (auto& pos : mesh->vertex_positions)
				{
					convexHull->addPoint(btVector3(pos.x, pos.y, pos.z));
				}
			}
			else
			{
				wi::backlog::post("Convex Hull physics requested, but no MeshComponent provided!");
				assert(0);
			}
			break;

		case RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH:
			if(mesh != nullptr)
			{
				int totalTriangles = 0;
				int* indices = nullptr;
				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh->GetLODSubsetRange(physicscomponent.mesh_lod, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
					if (indices == nullptr)
					{
						indices = (int*)(mesh->indices.data() + subset.indexOffset);
					}
					totalTriangles += int(subset.indexCount / 3);
				}

				physicsobject.triangles = btTriangleIndexVertexArray(
					totalTriangles,
					indices,
					3 * int(sizeof(int)),
					int(mesh->vertex_positions.size()),
					(btScalar*)mesh->vertex_positions.data(),
					int(sizeof(XMFLOAT3))
				);

				bool useQuantizedAabbCompression = true;
				physicsobject.shape = std::make_unique<btBvhTriangleMeshShape>(&physicsobject.triangles, useQuantizedAabbCompression);
			}
			else
			{
				wi::backlog::post("Triangle Mesh physics requested, but no MeshComponent provided!");
				assert(0);
			}
			break;
		}

		if (physicsobject.shape == nullptr)
		{
			physicscomponent.physicsobject = nullptr;
			return;
		}
		else
		{
			// Use default margin for now
			//shape->setMargin(btScalar(0.01));

			btVector3 S(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
			physicsobject.shape->setLocalScaling(S);

			btScalar mass = physicscomponent.mass;

			bool isDynamic = (mass != 0.f && !physicscomponent.IsKinematic());

			if (physicscomponent.shape == RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH)
			{
				isDynamic = false;
			}

			btVector3 localInertia(0, 0, 0);
			if (isDynamic)
			{
				physicsobject.shape->calculateLocalInertia(mass, localInertia);
			}
			else
			{
				mass = 0;
			}

			XMVECTOR SCA = {};
			XMVECTOR ROT = {};
			XMVECTOR TRA = {};
			XMMatrixDecompose(&SCA, &ROT, &TRA, XMLoadFloat4x4(&transform.world));
			XMFLOAT4 rot = {};
			XMFLOAT3 tra = {};
			XMStoreFloat4(&rot, ROT);
			XMStoreFloat3(&tra, TRA);

			//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(btVector3(tra.x, tra.y, tra.z));
			shapeTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
			physicsobject.motionState = btDefaultMotionState(shapeTransform);

			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, &physicsobject.motionState, physicsobject.shape.get(), localInertia);
			//rbInfo.m_friction = physicscomponent.friction;
			//rbInfo.m_restitution = physicscomponent.restitution;
			//rbInfo.m_linearDamping = physicscomponent.damping;
			//rbInfo.m_angularDamping = physicscomponent.damping;

			physicsobject.rigidBody = std::make_unique<btRigidBody>(rbInfo);
			physicsobject.rigidBody->setUserPointer(&physicsobject);
			physicsobject.rigidBody->setWorldTransform(shapeTransform); // immediate transform on first frame

			if (physicscomponent.IsKinematic())
			{
				physicsobject.rigidBody->setCollisionFlags(physicsobject.rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			}
			if (physicscomponent.IsDisableDeactivation())
			{
				physicsobject.rigidBody->setActivationState(DISABLE_DEACTIVATION);
			}

			physicsobject.physics_scene = scene.physics_scene;
			GetPhysicsScene(scene).dynamicsWorld.addRigidBody(physicsobject.rigidBody.get());

			if (isDynamic)
			{
				// We must detach dynamic objects, because their physics object is created in world space
				//	and attachment would apply double transformation to the transform
				scene.Component_Detach(entity);
			}
		}
	}
	void AddSoftBody(
		wi::scene::Scene& scene,
		Entity entity,
		wi::scene::SoftBodyPhysicsComponent& physicscomponent,
		const wi::scene::MeshComponent& mesh
	)
	{
		SoftBody& physicsobject = GetSoftBody(physicscomponent);
		physicsobject.entity = entity;
		physicscomponent.CreateFromMesh(mesh);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&physicscomponent.worldMatrix);

		const int vCount = (int)physicscomponent.physicsToGraphicsVertexMapping.size();
		wi::vector<btScalar> btVerts(vCount * 3);
		for (int i = 0; i < vCount; ++i) 
		{
			uint32_t graphicsInd = physicscomponent.physicsToGraphicsVertexMapping[i];

			XMFLOAT3 position = mesh.vertex_positions[graphicsInd];
			XMVECTOR P = XMLoadFloat3(&position);
			P = XMVector3Transform(P, worldMatrix);
			XMStoreFloat3(&position, P);

			btVerts[i * 3 + 0] = btScalar(position.x);
			btVerts[i * 3 + 1] = btScalar(position.y);
			btVerts[i * 3 + 2] = btScalar(position.z);
		}

		wi::vector<int> btInd;
		btInd.reserve(mesh.indices.size());
		uint32_t first_subset = 0;
		uint32_t last_subset = 0;
		mesh.GetLODSubsetRange(0, first_subset, last_subset);
		for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
		{
			const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
			const uint32_t* indices = mesh.indices.data() + subset.indexOffset;
			for (uint32_t i = 0; i < subset.indexCount; ++i)
			{
				btInd.push_back((int)physicscomponent.graphicsToPhysicsVertexMapping[indices[i]]);
			}
		}

		//// This function uses new to allocate btSoftbody internally:
		//btSoftBody* softbody = btSoftBodyHelpers::CreateFromTriMesh(
		//	GetPhysicsScene(scene).dynamicsWorld.getWorldInfo(),
		//	btVerts.data(),
		//	btInd.data(),
		//	int(btInd.size() / 3),
		//	false
		//);

		// Modified version of btSoftBodyHelpers::CreateFromTriMesh:
		//	This version does not allocate btSoftbody with new
		btSoftBody* softbody = nullptr;
		{
			btSoftBodyWorldInfo& worldInfo = GetPhysicsScene(scene).dynamicsWorld.getWorldInfo();
			const btScalar* vertices = btVerts.data();
			const int* triangles = btInd.data();
			int ntriangles = int(btInd.size() / 3);
			bool randomizeConstraints = false;

			int		maxidx = 0;
			int i, j, ni;

			for (i = 0, ni = ntriangles * 3; i < ni; ++i)
			{
				maxidx = btMax(triangles[i], maxidx);
			}
			++maxidx;
			btAlignedObjectArray<bool>		chks;
			btAlignedObjectArray<btVector3>	vtx;
			chks.resize(maxidx * maxidx, false);
			vtx.resize(maxidx);
			for (i = 0, j = 0, ni = maxidx * 3; i < ni; ++j, i += 3)
			{
				vtx[j] = btVector3(vertices[i], vertices[i + 1], vertices[i + 2]);
			}
			//btSoftBody* psb = new btSoftBody(&worldInfo, vtx.size(), &vtx[0], 0);
			physicsobject.softBody = std::make_unique<btSoftBody>(&worldInfo, vtx.size(), &vtx[0], nullptr);
			softbody = physicsobject.softBody.get();
			btSoftBody* psb = softbody;
			for (i = 0, ni = ntriangles * 3; i < ni; i += 3)
			{
				const int idx[] = { triangles[i],triangles[i + 1],triangles[i + 2] };
#define IDX(_x_,_y_) ((_y_)*maxidx+(_x_))
				for (int j = 2, k = 0; k < 3; j = k++)
				{
					if (!chks[IDX(idx[j], idx[k])])
					{
						chks[IDX(idx[j], idx[k])] = true;
						chks[IDX(idx[k], idx[j])] = true;
						psb->appendLink(idx[j], idx[k]);
					}
				}
#undef IDX
				psb->appendFace(idx[0], idx[1], idx[2]);
			}

			if (randomizeConstraints)
			{
				psb->randomizeConstraints();
			}
		}

		if (softbody)
		{
			softbody->setUserPointer(&physicsobject);

			//btSoftBody::Material* pm = softbody->appendMaterial();
			btSoftBody::Material* pm = softbody->m_materials[0];
			pm->m_kLST = btScalar(0.9f);
			pm->m_kVST = btScalar(0.9f);
			pm->m_kAST = btScalar(0.9f);
			pm->m_flags = 0;
			softbody->generateBendingConstraints(2, pm);
			softbody->randomizeConstraints();

			softbody->m_cfg.piterations = softbodyIterationCount;
			softbody->m_cfg.aeromodel = btSoftBody::eAeroModel::F_TwoSidedLiftDrag;

			softbody->m_cfg.kAHR = btScalar(.69); //0.69		Anchor hardness  [0,1]
			softbody->m_cfg.kCHR = btScalar(1.0); //1			Rigid contact hardness  [0,1]
			softbody->m_cfg.kDF = btScalar(0.2); //0.2			Dynamic friction coefficient  [0,1]
			softbody->m_cfg.kDG = btScalar(0.01); //0			Drag coefficient  [0,+inf]
			softbody->m_cfg.kDP = btScalar(0.0); //0			Damping coefficient  [0,1]
			softbody->m_cfg.kKHR = btScalar(0.1); //0.1			Kinetic contact hardness  [0,1]
			softbody->m_cfg.kLF = btScalar(0.1); //0			Lift coefficient  [0,+inf]
			softbody->m_cfg.kMT = btScalar(0.0); //0			Pose matching coefficient  [0,1]
			softbody->m_cfg.kPR = btScalar(0.0); //0			Pressure coefficient  [-1,1]
			softbody->m_cfg.kSHR = btScalar(1.0); //1			Soft contacts hardness  [0,1]
			softbody->m_cfg.kVC = btScalar(0.0); //0			Volume conseration coefficient  [0,+inf]
			softbody->m_cfg.kVCF = btScalar(1.0); //1			Velocities correction factor (Baumgarte)

			softbody->m_cfg.kSKHR_CL = btScalar(1.0); //1			Soft vs. kinetic hardness   [0,1]
			softbody->m_cfg.kSK_SPLT_CL = btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]
			softbody->m_cfg.kSRHR_CL = btScalar(0.1); //0.1			Soft vs. rigid hardness  [0,1]
			softbody->m_cfg.kSR_SPLT_CL = btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]
			softbody->m_cfg.kSSHR_CL = btScalar(0.5); //0.5			Soft vs. soft hardness  [0,1]
			softbody->m_cfg.kSS_SPLT_CL = btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]

			for (size_t i = 0; i < physicscomponent.physicsToGraphicsVertexMapping.size(); ++i)
			{
				float weight = physicscomponent.weights[i];
				softbody->setMass((int)i, weight);
			}
			softbody->setTotalMass(physicscomponent.mass); // this must be AFTER softbody->setMass(), so that weights will be averaged

			if (physicscomponent.IsDisableDeactivation())
			{
				softbody->setActivationState(DISABLE_DEACTIVATION);
			}

			softbody->setPose(true, true);

			physicsobject.physics_scene = scene.physics_scene;
			GetPhysicsScene(scene).dynamicsWorld.addSoftBody(softbody);
		}
	}

	struct Ragdoll
	{
		enum BODYPART
		{
			BODYPART_PELVIS = 0,
			BODYPART_SPINE,
			BODYPART_HEAD,

			BODYPART_LEFT_UPPER_LEG,
			BODYPART_LEFT_LOWER_LEG,

			BODYPART_RIGHT_UPPER_LEG,
			BODYPART_RIGHT_LOWER_LEG,

			BODYPART_LEFT_UPPER_ARM,
			BODYPART_LEFT_LOWER_ARM,

			BODYPART_RIGHT_UPPER_ARM,
			BODYPART_RIGHT_LOWER_ARM,

			BODYPART_COUNT
		};
		enum JOINT
		{
			JOINT_PELVIS_SPINE = 0,
			JOINT_SPINE_HEAD,

			JOINT_LEFT_HIP,
			JOINT_LEFT_KNEE,

			JOINT_RIGHT_HIP,
			JOINT_RIGHT_KNEE,

			JOINT_LEFT_SHOULDER,
			JOINT_LEFT_ELBOW,

			JOINT_RIGHT_SHOULDER,
			JOINT_RIGHT_ELBOW,

			JOINT_COUNT
		};

		std::shared_ptr<void> physics_scene;
		std::shared_ptr<RigidBody> rigidbodies[BODYPART_COUNT];
		btRigidBody* m_bodies[BODYPART_COUNT];
		btTypedConstraint* m_joints[JOINT_COUNT];
		bool state_active = false;
		Entity saved_parents[BODYPART_COUNT] = {};
		float scale = 1;

		Ragdoll(Scene& scene, HumanoidComponent& humanoid, Entity humanoidEntity, float scale)
		{
			physics_scene = scene.physics_scene;
			btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)physics_scene.get())->dynamicsWorld;

			// https://github.com/bulletphysics/bullet3/blob/39b8de74df93721add193e5b3d9ebee579faebf8/examples/Benchmarks/BenchmarkDemo.cpp#L647

			btVector3 roots[BODYPART_COUNT] = {};
			btTransform transforms[BODYPART_COUNT] = {};

#if 0
			// slow speed and visualizer to aid debugging:
			wi::renderer::SetGameSpeed(0.1f);
			SetDebugDrawEnabled(true);
#endif

			//Detect which way humanoid is facing in rest pose:
			float facing = 1;
			Entity left_shoulder = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperArm];
			Entity right_shoulder = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperArm];
			const XMVECTOR& left_shoulder_pos = scene.FindBoneRestPose(left_shoulder).r[3];
			const XMVECTOR& right_shoulder_pos = scene.FindBoneRestPose(right_shoulder).r[3];
			if (XMVectorGetX(right_shoulder_pos) < XMVectorGetX(left_shoulder_pos))
			{
				facing = -1;
			}

			// Whole ragdoll will take a uniform scaling:
			const XMMATRIX scaleMatrix = XMMatrixScaling(scale, scale, scale);
			this->scale = scale;

			// Calculate the bone lengths and radiuses in armature local space and create rigid bodies for bones:
			for (int c = 0; c < BODYPART_COUNT; ++c)
			{
				HumanoidComponent::HumanoidBone humanoid_bone = HumanoidComponent::HumanoidBone::Count;
				Entity entityA = INVALID_ENTITY;
				Entity entityB = INVALID_ENTITY;
				switch (c)
				{
				case BODYPART_PELVIS:
					humanoid_bone = HumanoidComponent::HumanoidBone::Hips;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Hips];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Spine];
					break;
				case BODYPART_SPINE:
					humanoid_bone = HumanoidComponent::HumanoidBone::Spine;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Spine];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Neck]; // prefer neck instead of head
					if (entityB == INVALID_ENTITY)
					{
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Head]; // fall back to head if neck not available
					}
					break;
				case BODYPART_HEAD:
					humanoid_bone = HumanoidComponent::HumanoidBone::Neck;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Neck]; // prefer neck instead of head
					if (entityA == INVALID_ENTITY)
					{
						humanoid_bone = HumanoidComponent::HumanoidBone::Head;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Head]; // fall back to head if neck not available
					}
					break;
				case BODYPART_LEFT_UPPER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftUpperLeg;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperLeg];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerLeg];
					break;
				case BODYPART_LEFT_LOWER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftLowerLeg;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerLeg];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftFoot];
					break;
				case BODYPART_RIGHT_UPPER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightUpperLeg;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperLeg];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerLeg];
					break;
				case BODYPART_RIGHT_LOWER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightLowerLeg;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerLeg];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightFoot];
					break;
				case BODYPART_LEFT_UPPER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftUpperArm;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperArm];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerArm];
					break;
				case BODYPART_LEFT_LOWER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftLowerArm;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerArm];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftHand];
					break;
				case BODYPART_RIGHT_UPPER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightUpperArm;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperArm];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerArm];
					break;
				case BODYPART_RIGHT_LOWER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightLowerArm;
					entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerArm];
					entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightHand];
					break;
				}
				assert(entityA != INVALID_ENTITY);

				// Calculations here will be done in armature local space.
				//	Unfortunately since humanoid can be separate from armature, we use a "find" utility to find bone rest matrix in armature
				//	Note that current scaling of character is applied here separately from rest pose
				XMMATRIX restA = scene.FindBoneRestPose(entityA) * scaleMatrix;
				XMMATRIX restB = scene.FindBoneRestPose(entityB) * scaleMatrix;
				XMVECTOR rootA = restA.r[3];
				XMVECTOR rootB = restB.r[3];

				// Every bone will be a rigid body:
				rigidbodies[c] = std::make_unique<RigidBody>();
				RigidBody& physicsobject = *rigidbodies[c];
				physicsobject.entity = entityA;

				float mass = scale;
				float capsule_height = scale;
				float capsule_radius = scale;

				if (c == BODYPART_HEAD)
				{
					// Head doesn't necessarily have a child, so make up something reasonable:
					capsule_height = 0.05f * scale;
					capsule_radius = 0.1f * scale;
				}
				else
				{
					// bone length:
					XMVECTOR len = XMVector3Length(XMVectorSubtract(rootB, rootA));
					capsule_height = XMVectorGetX(len);

					// capsule radius and length is tweaked per body part:
					switch (c)
					{
					case BODYPART_PELVIS:
						capsule_radius = 0.1f * scale;
						break;
					case BODYPART_SPINE:
						capsule_radius = 0.1f * scale;
						capsule_height -= capsule_radius * 2;
						break;
					case BODYPART_LEFT_LOWER_ARM:
					case BODYPART_RIGHT_LOWER_ARM:
						capsule_radius = capsule_height * 0.15f;
						capsule_height += capsule_radius;
						break;
					case BODYPART_LEFT_UPPER_LEG:
					case BODYPART_RIGHT_UPPER_LEG:
						capsule_radius = capsule_height * 0.15f;
						capsule_height -= capsule_radius * 2;
						break;
					case BODYPART_LEFT_LOWER_LEG:
					case BODYPART_RIGHT_LOWER_LEG:
						capsule_radius = capsule_height * 0.15f;
						capsule_height -= capsule_radius;
						break;
					default:
						capsule_radius = capsule_height * 0.2f;
						capsule_height -= capsule_radius * 2;
						break;
					}
				}

				switch (c)
				{
				case BODYPART_LEFT_UPPER_ARM:
				case BODYPART_LEFT_LOWER_ARM:
				case BODYPART_RIGHT_UPPER_ARM:
				case BODYPART_RIGHT_LOWER_ARM:
					// Capsule could be rotated, but it is easier to just make CapsuleX and offset it:
					physicsobject.shape = std::make_unique<btCapsuleShapeX>(capsule_radius, capsule_height);
					break;
				default:
					physicsobject.shape = std::make_unique<btCapsuleShape>(capsule_radius, capsule_height);
					break;
				}

				btVector3 localInertia(0, 0, 0);
				physicsobject.shape->calculateLocalInertia(mass, localInertia);

				btTransform shapeTransform;
				shapeTransform.setIdentity();

				// Get the translation and rotation part of rest matrix:
				XMVECTOR SCA = {};
				XMVECTOR ROT = {};
				XMVECTOR TRA = {};
				XMMatrixDecompose(&SCA, &ROT, &TRA, restA);
				XMFLOAT4 rot = {};
				XMFLOAT3 tra = {};
				XMStoreFloat4(&rot, ROT);
				XMStoreFloat3(&tra, TRA);
				shapeTransform.setOrigin(btVector3(tra.x, tra.y, tra.z)); // bone will be only translated initially

				// Rest basis is saved to make final correction between only-translated rest pose and final pose
				btTransform restBasis;
				restBasis.setIdentity();
				restBasis.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

				// capsule offset on axis is performed because otherwise capsule center would be in the bone root position
				//	which is not what we want. Instead the bone is moved on its axis so it resides between root and tail
				const float offset = capsule_height * 0.5f + capsule_radius;

				btTransform additionalTransform;
				additionalTransform.setIdentity();

				switch (c)
				{
				case BODYPART_PELVIS:
					break;
				case BODYPART_SPINE:
				case BODYPART_HEAD:
					additionalTransform.setOrigin(btVector3(0, offset, 0));
					break;
				case BODYPART_LEFT_UPPER_LEG:
				case BODYPART_LEFT_LOWER_LEG:
				case BODYPART_RIGHT_UPPER_LEG:
				case BODYPART_RIGHT_LOWER_LEG:
					additionalTransform.setOrigin(btVector3(0, -offset, 0));
					break;
				case BODYPART_LEFT_UPPER_ARM:
				case BODYPART_LEFT_LOWER_ARM:
					additionalTransform.setOrigin(btVector3(-offset * facing, 0, 0));
					break;
				case BODYPART_RIGHT_UPPER_ARM:
				case BODYPART_RIGHT_LOWER_ARM:
					additionalTransform.setOrigin(btVector3(offset * facing, 0, 0));
					break;
				}

				shapeTransform.mult(shapeTransform, additionalTransform);
				physicsobject.additionalTransform = additionalTransform;
				physicsobject.additionalTransformInverse = additionalTransform.inverse();
				physicsobject.restBasis = restBasis;
				physicsobject.restBasisInverse = restBasis.inverse();

				physicsobject.motionState = btDefaultMotionState(shapeTransform);

				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, &physicsobject.motionState, physicsobject.shape.get(), localInertia);

				physicsobject.rigidBody = std::make_unique<btRigidBody>(rbInfo);
				physicsobject.rigidBody->setUserPointer(&physicsobject);
				physicsobject.rigidBody->setWorldTransform(shapeTransform); // immediate transform on first frame

				// by default, whole ragdoll is kinematic:
				physicsobject.rigidBody->setCollisionFlags(physicsobject.rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				physicsobject.rigidBody->setActivationState(DISABLE_DEACTIVATION);

				physicsobject.humanoid_ragdoll_entity = humanoidEntity;
				physicsobject.humanoid_bone = humanoid_bone;

				physicsobject.physics_scene = scene.physics_scene;
				dynamicsWorld.addRigidBody(physicsobject.rigidBody.get());

				roots[c] = btVector3(XMVectorGetX(rootA), XMVectorGetY(rootA), XMVectorGetZ(rootA));
				m_bodies[c] = physicsobject.rigidBody.get();
				transforms[c] = physicsobject.rigidBody->getWorldTransform();
			}

			// Create all constraints below:
			btHingeConstraint* hingeC = nullptr;
			btConeTwistConstraint* coneC = nullptr;

			btTransform localA, localB;
			static float constraint_dbg = 10;
			static bool fixpose = false; // enable to fix the pose to rest pose, useful for debugging

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(0, -XM_PIDIV2 * facing, 0);
			localA.setOrigin(roots[BODYPART_SPINE] - transforms[BODYPART_PELVIS].getOrigin());
			localB.getBasis().setEulerZYX(0, -XM_PIDIV2 * facing, 0);
			localB.setOrigin(roots[BODYPART_SPINE] - transforms[BODYPART_SPINE].getOrigin());
			hingeC = new btHingeConstraint(*m_bodies[BODYPART_PELVIS], *m_bodies[BODYPART_SPINE], localA, localB);
			if (fixpose)
			{
				hingeC->setLimit(0, 0);
			}
			else
			{
				hingeC->setLimit(0, XM_PIDIV2);
			}
			hingeC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_PELVIS_SPINE] = hingeC;
			dynamicsWorld.addConstraint(m_joints[JOINT_PELVIS_SPINE], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(0, 0, XM_PIDIV2);
			localA.setOrigin(roots[BODYPART_HEAD] - transforms[BODYPART_SPINE].getOrigin());
			localB.getBasis().setEulerZYX(0, 0, XM_PIDIV2);
			localB.setOrigin(roots[BODYPART_HEAD] - transforms[BODYPART_HEAD].getOrigin());
			coneC = new btConeTwistConstraint(*m_bodies[BODYPART_SPINE], *m_bodies[BODYPART_HEAD], localA, localB);
			if (fixpose)
			{
				coneC->setLimit(0, 0, 0);
			}
			else
			{
				coneC->setLimit(XM_PIDIV4, XM_PIDIV2, XM_PIDIV4);
			}
			coneC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_SPINE_HEAD] = coneC;
			dynamicsWorld.addConstraint(m_joints[JOINT_SPINE_HEAD], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(0, 0, XM_PIDIV4);
			localA.setOrigin(roots[BODYPART_LEFT_UPPER_LEG] - transforms[BODYPART_PELVIS].getOrigin());
			localB.getBasis().setEulerZYX(0, 0, XM_PIDIV4);
			localB.setOrigin(roots[BODYPART_LEFT_UPPER_LEG] - transforms[BODYPART_LEFT_UPPER_LEG].getOrigin());
			coneC = new btConeTwistConstraint(*m_bodies[BODYPART_PELVIS], *m_bodies[BODYPART_LEFT_UPPER_LEG], localA, localB);
			if (fixpose)
			{
				coneC->setLimit(0, 0, 0);
			}
			else
			{
				coneC->setLimit(XM_PIDIV4, XM_PIDIV4, 0);
			}
			coneC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_LEFT_HIP] = coneC;
			dynamicsWorld.addConstraint(m_joints[JOINT_LEFT_HIP], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(0, -XM_PIDIV2 * facing, 0);
			localA.setOrigin(roots[BODYPART_LEFT_LOWER_LEG] - transforms[BODYPART_LEFT_UPPER_LEG].getOrigin());
			localB.getBasis().setEulerZYX(0, -XM_PIDIV2 * facing, 0);
			localB.setOrigin(roots[BODYPART_LEFT_LOWER_LEG] - transforms[BODYPART_LEFT_LOWER_LEG].getOrigin());
			hingeC = new btHingeConstraint(*m_bodies[BODYPART_LEFT_UPPER_LEG], *m_bodies[BODYPART_LEFT_LOWER_LEG], localA, localB);
			if (fixpose)
			{
				hingeC->setLimit(0, 0);
			}
			else
			{
				hingeC->setLimit(0, XM_PI * 0.8f);
			}
			hingeC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_LEFT_KNEE] = hingeC;
			dynamicsWorld.addConstraint(m_joints[JOINT_LEFT_KNEE], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(0, 0, -XM_PIDIV4);
			localA.setOrigin(roots[BODYPART_RIGHT_UPPER_LEG] - transforms[BODYPART_PELVIS].getOrigin());
			localB.getBasis().setEulerZYX(0, 0, -XM_PIDIV4);
			localB.setOrigin(roots[BODYPART_RIGHT_UPPER_LEG] - transforms[BODYPART_RIGHT_UPPER_LEG].getOrigin());
			coneC = new btConeTwistConstraint(*m_bodies[BODYPART_PELVIS], *m_bodies[BODYPART_RIGHT_UPPER_LEG], localA, localB);
			if (fixpose)
			{
				coneC->setLimit(0, 0, 0);
			}
			else
			{
				coneC->setLimit(XM_PIDIV4, XM_PIDIV4, 0);
			}
			coneC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_RIGHT_HIP] = coneC;
			dynamicsWorld.addConstraint(m_joints[JOINT_RIGHT_HIP], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(0, -XM_PIDIV2 * facing, 0);
			localA.setOrigin(roots[BODYPART_RIGHT_LOWER_LEG] - transforms[BODYPART_RIGHT_UPPER_LEG].getOrigin());
			localB.getBasis().setEulerZYX(0, -XM_PIDIV2 * facing, 0);
			localB.setOrigin(roots[BODYPART_RIGHT_LOWER_LEG] - transforms[BODYPART_RIGHT_LOWER_LEG].getOrigin());
			hingeC = new btHingeConstraint(*m_bodies[BODYPART_RIGHT_UPPER_LEG], *m_bodies[BODYPART_RIGHT_LOWER_LEG], localA, localB);
			if (fixpose)
			{
				hingeC->setLimit(0, 0);
			}
			else
			{
				hingeC->setLimit(0, XM_PI * 0.8f);
			}
			hingeC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_RIGHT_KNEE] = hingeC;
			dynamicsWorld.addConstraint(m_joints[JOINT_RIGHT_KNEE], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.setOrigin(roots[BODYPART_LEFT_UPPER_ARM] - transforms[BODYPART_SPINE].getOrigin());
			localB.setOrigin(roots[BODYPART_LEFT_UPPER_ARM] - transforms[BODYPART_LEFT_UPPER_ARM].getOrigin());
			coneC = new btConeTwistConstraint(*m_bodies[BODYPART_SPINE], *m_bodies[BODYPART_LEFT_UPPER_ARM], localA, localB);
			if (fixpose)
			{
				coneC->setLimit(0, 0, 0);
			}
			else
			{
				coneC->setLimit(XM_PIDIV2, XM_PIDIV2, 0);
			}
			coneC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_LEFT_SHOULDER] = coneC;
			dynamicsWorld.addConstraint(m_joints[JOINT_LEFT_SHOULDER], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(-XM_PIDIV2, 0, 0);
			localA.setOrigin(roots[BODYPART_LEFT_LOWER_ARM] - transforms[BODYPART_LEFT_UPPER_ARM].getOrigin());
			localB.getBasis().setEulerZYX(-XM_PIDIV2, 0, 0);
			localB.setOrigin(roots[BODYPART_LEFT_LOWER_ARM] - transforms[BODYPART_LEFT_LOWER_ARM].getOrigin());
			hingeC = new btHingeConstraint(*m_bodies[BODYPART_LEFT_UPPER_ARM], *m_bodies[BODYPART_LEFT_LOWER_ARM], localA, localB);
			if (fixpose)
			{
				hingeC->setLimit(0, 0);
			}
			else
			{
				hingeC->setLimit(-XM_PIDIV2, 0);
			}
			hingeC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_LEFT_ELBOW] = hingeC;
			dynamicsWorld.addConstraint(m_joints[JOINT_LEFT_ELBOW], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(0, 0, 0);
			localA.setOrigin(roots[BODYPART_RIGHT_UPPER_ARM] - transforms[BODYPART_SPINE].getOrigin());
			localB.setOrigin(roots[BODYPART_RIGHT_UPPER_ARM] - transforms[BODYPART_RIGHT_UPPER_ARM].getOrigin());
			coneC = new btConeTwistConstraint(*m_bodies[BODYPART_SPINE], *m_bodies[BODYPART_RIGHT_UPPER_ARM], localA, localB);
			if (fixpose)
			{
				coneC->setLimit(0, 0, 0);
			}
			else
			{
				coneC->setLimit(XM_PIDIV2, XM_PIDIV2, 0);
			}
			coneC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_RIGHT_SHOULDER] = coneC;
			dynamicsWorld.addConstraint(m_joints[JOINT_RIGHT_SHOULDER], true);

			localA.setIdentity();
			localB.setIdentity();
			localA.getBasis().setEulerZYX(XM_PIDIV2, 0, 0);
			localA.setOrigin(roots[BODYPART_RIGHT_LOWER_ARM] - transforms[BODYPART_RIGHT_UPPER_ARM].getOrigin());
			localB.getBasis().setEulerZYX(XM_PIDIV2, 0, 0);
			localB.setOrigin(roots[BODYPART_RIGHT_LOWER_ARM] - transforms[BODYPART_RIGHT_LOWER_ARM].getOrigin());
			hingeC = new btHingeConstraint(*m_bodies[BODYPART_RIGHT_UPPER_ARM], *m_bodies[BODYPART_RIGHT_LOWER_ARM], localA, localB);
			if (fixpose)
			{
				hingeC->setLimit(0, 0);
			}
			else
			{
				hingeC->setLimit(-XM_PIDIV2, 0);
			}
			hingeC->setDbgDrawSize(constraint_dbg);
			m_joints[JOINT_RIGHT_ELBOW] = hingeC;
			dynamicsWorld.addConstraint(m_joints[JOINT_RIGHT_ELBOW], true);

			// For all body parts, we now apply the current world space pose:
			for (auto& x : rigidbodies)
			{
				RigidBody& physicsobject = *x.get();
				Entity entity = physicsobject.entity;
				const TransformComponent* transform = scene.transforms.GetComponent(entity);
				if (transform == nullptr)
					continue;

				XMVECTOR SCA = {};
				XMVECTOR ROT = {};
				XMVECTOR TRA = {};
				XMMatrixDecompose(&SCA, &ROT, &TRA, XMLoadFloat4x4(&transform->world));
				XMFLOAT4 rot = {};
				XMFLOAT3 tra = {};
				XMStoreFloat4(&rot, ROT);
				XMStoreFloat3(&tra, TRA);

				btTransform shapeTransform;
				shapeTransform.setIdentity();
				shapeTransform.setOrigin(btVector3(tra.x, tra.y, tra.z));
				shapeTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
				shapeTransform.mult(shapeTransform, physicsobject.restBasisInverse);
				shapeTransform.mult(shapeTransform, physicsobject.additionalTransform);

				btMotionState* ms = physicsobject.rigidBody->getMotionState();
				ms->setWorldTransform(shapeTransform);
				physicsobject.rigidBody->setWorldTransform(shapeTransform); // immediate transform on first frame
			}
		}
		~Ragdoll()
		{
			if (physics_scene == nullptr)
				return;
			btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)physics_scene.get())->dynamicsWorld;
			for (auto& x : m_joints)
			{
				if (x == nullptr)
					continue;
				dynamicsWorld.removeConstraint(x);
				delete x;
			}
			for (auto& x : rigidbodies)
			{
				dynamicsWorld.removeRigidBody(x->rigidBody.get());
			}
		}

		// Activates ragdoll as dynamic physics object:
		void Activate(
			Scene& scene,
			Entity humanoidEntity
		)
		{
			if (state_active)
				return;
			state_active = true;

			const HumanoidComponent* humanoid = scene.humanoids.GetComponent(humanoidEntity);
			if (humanoid == nullptr)
				return;

			btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)scene.physics_scene.get())->dynamicsWorld;

			int c = 0;
			for (auto& x : rigidbodies)
			{
				// remove kinematic flag from bone:
				x->rigidBody->setCollisionFlags(x->rigidBody->getCollisionFlags() ^ btCollisionObject::CF_KINEMATIC_OBJECT);

				x->rigidBody->forceActivationState(ACTIVE_TAG);
				x->rigidBody->setDeactivationTime(btScalar(0.8));
				x->rigidBody->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
				x->rigidBody->setDamping(btScalar(0.05), btScalar(0.85));

				// If we don't remove and re-add rigid body, then it will not correctly switch from kinematic to dynamic it seems:
				dynamicsWorld.removeRigidBody(x->rigidBody.get());
				dynamicsWorld.addRigidBody(x->rigidBody.get());

				// Save parenting information to be able to restore it:
				const HierarchyComponent* hier = scene.hierarchy.GetComponent(x->entity);
				if (hier != nullptr)
				{
					saved_parents[c] = hier->parentID;
				}
				else
				{
					saved_parents[c] = INVALID_ENTITY;
				}

				// detach bone because it will be simulated in world space:
				scene.Component_Detach(x->entity);

				c++;
			}

			// Stop all anims that are children of humanoid:
			for (size_t i = 0; i < scene.animations.GetCount(); ++i)
			{
				Entity entity = scene.animations.GetEntity(i);
				if (!scene.Entity_IsDescendant(entity, humanoidEntity))
					continue;
				AnimationComponent& animation = scene.animations[i];
				animation.Stop();
			}
		}

		// Disables dynamic ragdoll and reattaches loose parts as they were:
		void Deactivate(
			Scene& scene
		)
		{
			if (!state_active)
				return;
			state_active = false;

			btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)scene.physics_scene.get())->dynamicsWorld;

			int c = 0;
			for (auto& x : rigidbodies)
			{
				// add kinematic flag from bone:
				x->rigidBody->setCollisionFlags(x->rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

				x->rigidBody->forceActivationState(DISABLE_DEACTIVATION);

				dynamicsWorld.removeRigidBody(x->rigidBody.get());
				dynamicsWorld.addRigidBody(x->rigidBody.get());

				if (saved_parents[c] != INVALID_ENTITY)
				{
					scene.Component_Attach(x->entity, saved_parents[c]);
				}
				c++;
			}
		}
	};


	void RunPhysicsUpdateSystem(
		wi::jobsystem::context& ctx,
		Scene& scene,
		float dt
	)
	{
		if (!IsEnabled() || dt <= 0)
			return;

		auto range = wi::profiler::BeginRangeCPU("Physics");

		btSoftRigidDynamicsWorld& dynamicsWorld = GetPhysicsScene(scene).dynamicsWorld;
		dynamicsWorld.setGravity(btVector3(scene.weather.gravity.x, scene.weather.gravity.y, scene.weather.gravity.z));

		btVector3 wind = btVector3(scene.weather.windDirection.x, scene.weather.windDirection.y, scene.weather.windDirection.z);

		// Ragdoll management:
		for (size_t i = 0; i < scene.humanoids.GetCount(); ++i)
		{
			HumanoidComponent& humanoid = scene.humanoids[i];
			Entity humanoidEntity = scene.humanoids.GetEntity(i);
			float scale = 1;
			if (scene.transforms.Contains(humanoidEntity))
			{
				scale = scene.transforms.GetComponent(humanoidEntity)->scale_local.x;
			}
			if (humanoid.ragdoll != nullptr)
			{
				Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
				if (!wi::math::float_equal(ragdoll.scale, scale))
				{
					humanoid.SetRagdollPhysicsEnabled(false); // while scaling ragdoll, it will be kinematic
					ragdoll.Deactivate(scene); // recreate attached skeleton hierarchy structure
					humanoid.ragdoll = {}; // delete ragdoll if scale changed, it will be recreated
				}
			}
			if (humanoid.ragdoll == nullptr)
			{
				humanoid.ragdoll = std::make_shared<Ragdoll>(scene, humanoid, humanoidEntity, scale);
			}
			Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
			if (humanoid.IsRagdollPhysicsEnabled())
			{
				ragdoll.Activate(scene, humanoidEntity);
			}
			else
			{
				ragdoll.Deactivate(scene);
			}
		}

		// System will register rigidbodies to objects:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), 256, [&](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);

			if (physicscomponent.physicsobject == nullptr && scene.transforms.Contains(entity))
			{
				TransformComponent& transform = *scene.transforms.GetComponent(entity);
				const ObjectComponent* object = scene.objects.GetComponent(entity);
				const MeshComponent* mesh = nullptr;
				if (object != nullptr)
				{
					mesh = scene.meshes.GetComponent(object->meshID);
				}
				physicsLock.lock();
				AddRigidBody(scene, entity, physicscomponent, transform, mesh);
				physicsLock.unlock();
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				btRigidBody* rigidbody = GetRigidBody(physicscomponent).rigidBody.get();

				rigidbody->setDamping(
					physicscomponent.damping_linear,
					physicscomponent.damping_angular
				);
				rigidbody->setFriction(physicscomponent.friction);
				rigidbody->setRestitution(physicscomponent.restitution);
			}
		});

		// System will register softbodies to meshes and update physics engine state:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.softbodies.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {

			SoftBodyPhysicsComponent& physicscomponent = scene.softbodies[args.jobIndex];
			Entity entity = scene.softbodies.GetEntity(args.jobIndex);
			if (!scene.meshes.Contains(entity))
				return;
			MeshComponent& mesh = *scene.meshes.GetComponent(entity);
			const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;
			mesh.SetDynamic(true);

			if (physicscomponent._flags & SoftBodyPhysicsComponent::FORCE_RESET)
			{
				physicscomponent._flags &= ~SoftBodyPhysicsComponent::FORCE_RESET;
				physicscomponent.physicsobject = nullptr;
			}
			if (physicscomponent._flags & SoftBodyPhysicsComponent::SAFE_TO_REGISTER && physicscomponent.physicsobject == nullptr)
			{
				physicsLock.lock();
				AddSoftBody(scene, entity, physicscomponent, mesh);
				physicsLock.unlock();
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				btSoftBody* softbody = GetSoftBody(physicscomponent).softBody.get();
				softbody->getWorldInfo()->m_gravity = dynamicsWorld.getGravity();
				softbody->m_cfg.kDF = physicscomponent.friction;
				softbody->setWindVelocity(wind);

				softbody->setFriction(physicscomponent.friction);
				softbody->setRestitution(physicscomponent.restitution);

				// This is different from rigid bodies, because soft body is a per mesh component (no TransformComponent). World matrix is propagated down from single mesh instance (ObjectUpdateSystem).
				XMMATRIX worldMatrix = XMLoadFloat4x4(&physicscomponent.worldMatrix);

				// System controls zero weight soft body nodes:
				for (size_t ind = 0; ind < physicscomponent.weights.size(); ++ind)
				{
					float weight = physicscomponent.weights[ind];

					if (weight == 0)
					{
						btSoftBody::Node& node = softbody->m_nodes[(uint32_t)ind];
						uint32_t graphicsInd = physicscomponent.physicsToGraphicsVertexMapping[ind];
						XMFLOAT3 position = mesh.vertex_positions[graphicsInd];
						XMVECTOR P = armature == nullptr ? XMLoadFloat3(&position) : wi::scene::SkinVertex(mesh, *armature, graphicsInd);
						P = XMVector3Transform(P, worldMatrix);
						XMStoreFloat3(&position, P);
						node.m_x = btVector3(position.x, position.y, position.z);
					}
				}
			}
		});

		// Feedback system kinematics to physics engine:
		for (int i = 0; i < dynamicsWorld.getCollisionObjectArray().size(); ++i)
		{
			btCollisionObject* collisionobject = dynamicsWorld.getCollisionObjectArray()[i];

			btRigidBody* rigidbody = btRigidBody::upcast(collisionobject);
			if (rigidbody != nullptr)
			{
				RigidBody* physicsobject = (RigidBody*)rigidbody->getUserPointer();
				Entity entity = physicsobject->entity;
				const bool kinematic = rigidbody->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT;

				TransformComponent& transform = *scene.transforms.GetComponent(entity);

				btMotionState* motionState = rigidbody->getMotionState();
				btTransform physicsTransform;

				XMFLOAT3 position = transform.GetPosition();
				XMFLOAT4 rotation = transform.GetRotation();
				btVector3 T(position.x, position.y, position.z);
				btQuaternion R(rotation.x, rotation.y, rotation.z, rotation.w);
				physicsTransform.setOrigin(T);
				physicsTransform.setRotation(R);
				physicsTransform.mult(physicsTransform, physicsobject->restBasisInverse);
				physicsTransform.mult(physicsTransform, physicsobject->additionalTransform);
				motionState->setWorldTransform(physicsTransform);
				rigidbody->setWorldTransform(physicsTransform);

				if (physicsobject->humanoid_ragdoll_entity == INVALID_ENTITY)
				{
					btCollisionShape* shape = rigidbody->getCollisionShape();
					XMFLOAT3 scale = transform.GetScale();
					btVector3 S(scale.x, scale.y, scale.z);
					shape->setLocalScaling(S);
				}
			}
		}

		wi::jobsystem::Wait(ctx);

		// Perform internal simulation step:
		if (IsSimulationEnabled())
		{
			dynamicsWorld.stepSimulation(dt, ACCURACY, TIMESTEP);
		}

		// Feedback physics engine state to system:
		for (int i = 0; i < dynamicsWorld.getCollisionObjectArray().size(); ++i)
		{
			btCollisionObject* collisionobject = dynamicsWorld.getCollisionObjectArray()[i];

			btRigidBody* rigidbody = btRigidBody::upcast(collisionobject);
			if (rigidbody != nullptr)
			{
				RigidBody* physicsobject = (RigidBody*)rigidbody->getUserPointer();
				Entity entity = physicsobject->entity;
				const bool kinematic = rigidbody->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT;

				// Feedback non-kinematic objects to system:
				if (IsSimulationEnabled() && !kinematic && scene.transforms.Contains(entity))
				{
					TransformComponent& transform = *scene.transforms.GetComponent(entity);
	
					btTransform physicsTransform;
					rigidbody->getMotionState()->getWorldTransform(physicsTransform);
					physicsTransform.mult(physicsTransform, physicsobject->additionalTransformInverse);
					physicsTransform.mult(physicsTransform, physicsobject->restBasis);
					btVector3 T = physicsTransform.getOrigin();
					btQuaternion R = physicsTransform.getRotation();

					transform.translation_local = XMFLOAT3(T.x(), T.y(), T.z());
					transform.rotation_local = XMFLOAT4(R.x(), R.y(), R.z(), R.w());
					transform.SetDirty();
				}
			}
			else
			{
				btSoftBody* softbody = btSoftBody::upcast(collisionobject);

				if (softbody != nullptr)
				{
					SoftBody* physicsobject = (SoftBody*)softbody->getUserPointer();
					Entity entity = physicsobject->entity;
					SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(entity);

					// If you need it, you can enable soft body node debug strings here:
#if 0
					if (IsDebugDrawEnabled())
					{
						btSoftBodyHelpers::DrawInfos(
							softbody,
							&debugDraw,
							false,	// masses
							true,	// areas
							false	// stress
						);
					}
#endif

					MeshComponent& mesh = *scene.meshes.GetComponent(entity);

					// System mesh aabb will be queried from physics engine soft body:
					btVector3 aabb_min;
					btVector3 aabb_max;
					softbody->getAabb(aabb_min, aabb_max);
					physicscomponent->aabb = wi::primitive::AABB(XMFLOAT3(aabb_min.x(), aabb_min.y(), aabb_min.z()), XMFLOAT3(aabb_max.x(), aabb_max.y(), aabb_max.z()));
					mesh.aabb = physicscomponent->aabb;

					// Soft body simulation nodes will update graphics mesh:
					for (size_t ind = 0; ind < mesh.vertex_positions.size(); ++ind)
					{
						uint32_t physicsInd = physicscomponent->graphicsToPhysicsVertexMapping[ind];

						btSoftBody::Node& node = softbody->m_nodes[physicsInd];

						physicscomponent->vertex_positions_simulation[ind].FromFULL(XMFLOAT3(node.m_x.getX(), node.m_x.getY(), node.m_x.getZ()));
						physicscomponent->vertex_normals_simulation[ind].FromFULL(XMFLOAT3(-node.m_n.getX(), -node.m_n.getY(), -node.m_n.getZ()));
					}

					// Update tangent vectors:
					if (!mesh.vertex_uvset_0.empty() && !mesh.vertex_normals.empty())
					{
						uint32_t first_subset = 0;
						uint32_t last_subset = 0;
						mesh.GetLODSubsetRange(0, first_subset, last_subset);
						for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
						{
							const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
							for (size_t i = 0; i < subset.indexCount; i += 3)
							{
								const uint32_t i0 = mesh.indices[i + 0];
								const uint32_t i1 = mesh.indices[i + 1];
								const uint32_t i2 = mesh.indices[i + 2];

								const XMFLOAT3 v0 = physicscomponent->vertex_positions_simulation[i0].GetPOS();
								const XMFLOAT3 v1 = physicscomponent->vertex_positions_simulation[i1].GetPOS();
								const XMFLOAT3 v2 = physicscomponent->vertex_positions_simulation[i2].GetPOS();

								const XMFLOAT2 u0 = mesh.vertex_uvset_0[i0];
								const XMFLOAT2 u1 = mesh.vertex_uvset_0[i1];
								const XMFLOAT2 u2 = mesh.vertex_uvset_0[i2];

								const XMVECTOR nor0 = physicscomponent->vertex_normals_simulation[i0].LoadNOR();
								const XMVECTOR nor1 = physicscomponent->vertex_normals_simulation[i1].LoadNOR();
								const XMVECTOR nor2 = physicscomponent->vertex_normals_simulation[i2].LoadNOR();

								const XMVECTOR facenormal = XMVector3Normalize(XMVectorAdd(XMVectorAdd(nor0, nor1), nor2));

								const float x1 = v1.x - v0.x;
								const float x2 = v2.x - v0.x;
								const float y1 = v1.y - v0.y;
								const float y2 = v2.y - v0.y;
								const float z1 = v1.z - v0.z;
								const float z2 = v2.z - v0.z;

								const float s1 = u1.x - u0.x;
								const float s2 = u2.x - u0.x;
								const float t1 = u1.y - u0.y;
								const float t2 = u2.y - u0.y;

								const float r = 1.0f / (s1 * t2 - s2 * t1);
								const XMVECTOR sdir = XMVectorSet((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
									(t2 * z1 - t1 * z2) * r, 0);
								const XMVECTOR tdir = XMVectorSet((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
									(s1 * z2 - s2 * z1) * r, 0);

								XMVECTOR tangent;
								tangent = XMVector3Normalize(XMVectorSubtract(sdir, XMVectorMultiply(facenormal, XMVector3Dot(facenormal, sdir))));
								float sign = XMVectorGetX(XMVector3Dot(XMVector3Cross(tangent, facenormal), tdir)) < 0.0f ? -1.0f : 1.0f;

								XMFLOAT3 t;
								XMStoreFloat3(&t, tangent);

								physicscomponent->vertex_tangents_tmp[i0].x += t.x;
								physicscomponent->vertex_tangents_tmp[i0].y += t.y;
								physicscomponent->vertex_tangents_tmp[i0].z += t.z;
								physicscomponent->vertex_tangents_tmp[i0].w = sign;

								physicscomponent->vertex_tangents_tmp[i1].x += t.x;
								physicscomponent->vertex_tangents_tmp[i1].y += t.y;
								physicscomponent->vertex_tangents_tmp[i1].z += t.z;
								physicscomponent->vertex_tangents_tmp[i1].w = sign;

								physicscomponent->vertex_tangents_tmp[i2].x += t.x;
								physicscomponent->vertex_tangents_tmp[i2].y += t.y;
								physicscomponent->vertex_tangents_tmp[i2].z += t.z;
								physicscomponent->vertex_tangents_tmp[i2].w = sign;
							}
						}

						for (size_t i = 0; i < physicscomponent->vertex_tangents_simulation.size(); ++i)
						{
							physicscomponent->vertex_tangents_simulation[i].FromFULL(physicscomponent->vertex_tangents_tmp[i]);
						}
					}

				}
			}
		}

		if (IsDebugDrawEnabled())
		{
			dynamicsWorld.debugDrawWorld();
		}

		wi::profiler::EndRange(range); // Physics
	}



	void SetLinearVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
		}
	}
	void SetAngularVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
		}
	}

	void ApplyForce(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
		}
	}
	void ApplyForceAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force,
		const XMFLOAT3& at
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(at.x, at.y, at.z));
		}
	}

	void ApplyImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
		}
	}
	void ApplyImpulse(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse
	)
	{
		ApplyImpulseAt(humanoid, bone, impulse, XMFLOAT3(0, 0, 0));
	}
	void ApplyImpulseAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(at.x, at.y, at.z));
		}
	}
	void ApplyImpulseAt(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at
	)
	{
		if (humanoid.ragdoll == nullptr)
			return;

		Ragdoll::BODYPART bodypart = Ragdoll::BODYPART_COUNT;
		switch (bone)
		{
		case HumanoidComponent::HumanoidBone::Hips:
			bodypart = Ragdoll::BODYPART_PELVIS;
			break;
		case HumanoidComponent::HumanoidBone::Spine:
			bodypart = Ragdoll::BODYPART_SPINE;
			break;
		case HumanoidComponent::HumanoidBone::Head:
		case HumanoidComponent::HumanoidBone::Neck:
			bodypart = Ragdoll::BODYPART_HEAD;
			break;
		case HumanoidComponent::HumanoidBone::RightUpperArm:
			bodypart = Ragdoll::BODYPART_RIGHT_UPPER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::RightLowerArm:
			bodypart = Ragdoll::BODYPART_RIGHT_LOWER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::LeftUpperArm:
			bodypart = Ragdoll::BODYPART_LEFT_UPPER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::LeftLowerArm:
			bodypart = Ragdoll::BODYPART_LEFT_LOWER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::RightUpperLeg:
			bodypart = Ragdoll::BODYPART_RIGHT_UPPER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::RightLowerLeg:
			bodypart = Ragdoll::BODYPART_RIGHT_LOWER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::LeftUpperLeg:
			bodypart = Ragdoll::BODYPART_LEFT_UPPER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::LeftLowerLeg:
			bodypart = Ragdoll::BODYPART_LEFT_LOWER_LEG;
			break;
		}
		if (bodypart == Ragdoll::BODYPART_COUNT)
			return;

		Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
		if (ragdoll.rigidbodies[bodypart] == nullptr || ragdoll.rigidbodies[bodypart]->rigidBody == nullptr)
			return;
		ragdoll.rigidbodies[bodypart]->rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(at.x, at.y, at.z));
	}

	void ApplyTorque(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
		}
	}
	void ApplyTorqueImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).rigidBody->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
		}
	}

	constexpr int to_internal(ActivationState state)
	{
		switch (state)
		{
		default:
		case wi::physics::ActivationState::Active:
			return ACTIVE_TAG;
		case wi::physics::ActivationState::Inactive:
			return DISABLE_SIMULATION;
		}
	}
	void SetActivationState(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		ActivationState state
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			//GetRigidBody(physicscomponent).rigidBody->setActivationState(to_internal(state));
			GetRigidBody(physicscomponent).rigidBody->forceActivationState(to_internal(state));
		}
	}
	void SetActivationState(
		wi::scene::SoftBodyPhysicsComponent& physicscomponent,
		ActivationState state
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			//GetSoftBody(physicscomponent).softBody->setActivationState(to_internal(state));
			GetSoftBody(physicscomponent).softBody->forceActivationState(to_internal(state));
		}
	}

	RayIntersectionResult Intersects(
		const wi::scene::Scene& scene,
		wi::primitive::Ray ray
	)
	{
		RayIntersectionResult result;
		btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)scene.physics_scene.get())->dynamicsWorld;
		float tmin = wi::math::Clamp(ray.TMin, 0, 1000000);
		float tmax = wi::math::Clamp(ray.TMax, 0, 1000000);
		btVector3 rayFrom = btVector3(
			ray.origin.x + ray.direction.x * tmin,
			ray.origin.y + ray.direction.y * tmin,
			ray.origin.z + ray.direction.z * tmin
		);
		btVector3 rayTo = btVector3(
			ray.origin.x + ray.direction.x * tmax,
			ray.origin.y + ray.direction.y * tmax,
			ray.origin.z + ray.direction.z * tmax
		);
		btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
		dynamicsWorld.rayTest(rayFrom, rayTo, rayCallback);
		if (rayCallback.hasHit())
		{
			result.physicsobject = rayCallback.m_collisionObject;
			result.position.x = rayCallback.m_hitPointWorld.getX();
			result.position.y = rayCallback.m_hitPointWorld.getY();
			result.position.z = rayCallback.m_hitPointWorld.getZ();
			result.normal.x = rayCallback.m_hitNormalWorld.getX();
			result.normal.y = rayCallback.m_hitNormalWorld.getY();
			result.normal.z = -rayCallback.m_hitNormalWorld.getZ();

			btVector3 position_local = rayCallback.m_hitPointWorld;

			const btRigidBody* rigidbody = btRigidBody::upcast(rayCallback.m_collisionObject);
			if (rigidbody != nullptr)
			{
				RigidBody* physicsobject = (RigidBody*)rigidbody->getUserPointer();
				result.entity = physicsobject->entity;
				result.humanoid_ragdoll_entity = physicsobject->humanoid_ragdoll_entity;
				result.humanoid_bone = physicsobject->humanoid_bone;
				position_local = physicsobject->rigidBody->getCenterOfMassTransform().inverse() * position_local;
			}
			const btSoftBody* softbody = btSoftBody::upcast(rayCallback.m_collisionObject);
			if (softbody != nullptr)
			{
				SoftBody* physicsobject = (SoftBody*)softbody->getUserPointer();
				result.entity = physicsobject->entity;
			}

			result.position_local.x = position_local.getX();
			result.position_local.y = position_local.getY();
			result.position_local.z = position_local.getZ();
		}
		return result;
	}

	struct PickDragOperation_Bullet
	{
		std::shared_ptr<void> physics_scene;
		std::unique_ptr<btGeneric6DofConstraint> constraint;
		float pick_distance = 0;
		~PickDragOperation_Bullet()
		{
			btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)physics_scene.get())->dynamicsWorld;
			dynamicsWorld.removeConstraint(constraint.get());
		}
	};
	void PickDrag(
		const wi::scene::Scene& scene,
		wi::primitive::Ray ray,
		PickDragOperation& op
	)
	{
		btSoftRigidDynamicsWorld& dynamicsWorld = ((PhysicsScene*)scene.physics_scene.get())->dynamicsWorld;
		float tmin = wi::math::Clamp(ray.TMin, 0, 1000000);
		float tmax = wi::math::Clamp(ray.TMax, 0, 1000000);
		btVector3 rayFrom = btVector3(
			ray.origin.x + ray.direction.x * tmin,
			ray.origin.y + ray.direction.y * tmin,
			ray.origin.z + ray.direction.z * tmin
		);
		btVector3 rayTo = btVector3(
			ray.origin.x + ray.direction.x * tmax,
			ray.origin.y + ray.direction.y * tmax,
			ray.origin.z + ray.direction.z * tmax
		);
		if (op.IsValid())
		{
			// Continue dragging:
			PickDragOperation_Bullet* internal_state = (PickDragOperation_Bullet*)op.internal_state.get();
			btVector3 oldPivotInB = internal_state->constraint->getFrameOffsetA().getOrigin();
			btVector3 newPivotB;
			btVector3 dir = (rayTo - rayFrom).normalize();
			newPivotB = rayFrom + dir * internal_state->pick_distance;
			internal_state->constraint->getFrameOffsetA().setOrigin(newPivotB);
		}
		else
		{
			// Begin picking:
			RayIntersectionResult result = Intersects(scene, ray);
			if (!result.IsValid())
				return;
			btCollisionObject* collisionobject = (btCollisionObject*)result.physicsobject;
			btRigidBody* rigidbody = btRigidBody::upcast(collisionobject);
			if (rigidbody == nullptr)
				return;

			auto internal_state = std::make_shared<PickDragOperation_Bullet>();
			internal_state->physics_scene = scene.physics_scene;
			internal_state->pick_distance = (btVector3(result.position.x, result.position.y, result.position.z) - rayFrom).length();

			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(btVector3(result.position_local.x, result.position_local.y, result.position_local.z));

			internal_state->constraint = std::make_unique<btGeneric6DofConstraint>(*rigidbody, transform, false);
			internal_state->constraint->setLinearLowerLimit(btVector3(0, 0, 0));
			internal_state->constraint->setLinearUpperLimit(btVector3(0, 0, 0));
			internal_state->constraint->setAngularLowerLimit(btVector3(0, 0, 0));
			internal_state->constraint->setAngularUpperLimit(btVector3(0, 0, 0));
			dynamicsWorld.addConstraint(internal_state->constraint.get());

			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 0);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 1);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 2);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 3);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 4);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 5);

			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 0);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 1);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 2);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 3);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 4);
			internal_state->constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 5);

			op.internal_state = internal_state;
		}
	}
}
