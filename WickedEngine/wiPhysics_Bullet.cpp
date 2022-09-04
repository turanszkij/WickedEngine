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
		int ACCURACY = 1;
		int softbodyIterationCount = 5;
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

	void AddRigidBody(
		wi::scene::Scene& scene,
		Entity entity,
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const wi::scene::TransformComponent& transform,
		const wi::scene::MeshComponent* mesh
	)
	{
		RigidBody& physicsobject = GetRigidBody(physicscomponent);

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
				btVector3 S(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
				physicsobject.shape->setLocalScaling(S);
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
				btVector3 S(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
				physicsobject.shape->setLocalScaling(S);
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

			//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(btVector3(transform.translation_local.x, transform.translation_local.y, transform.translation_local.z));
			shapeTransform.setRotation(btQuaternion(transform.rotation_local.x, transform.rotation_local.y, transform.rotation_local.z, transform.rotation_local.w));
			physicsobject.motionState = btDefaultMotionState(shapeTransform);

			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, &physicsobject.motionState, physicsobject.shape.get(), localInertia);
			//rbInfo.m_friction = physicscomponent.friction;
			//rbInfo.m_restitution = physicscomponent.restitution;
			//rbInfo.m_linearDamping = physicscomponent.damping;
			//rbInfo.m_angularDamping = physicscomponent.damping;

			physicsobject.rigidBody = std::make_unique<btRigidBody>(rbInfo);
			physicsobject.rigidBody->setUserIndex(entity);

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
			softbody->setUserIndex(entity);

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

		// System will register rigidbodies to objects, and update physics engine state for kinematics:
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

				// For kinematic object, system updates physics state, else the physics updates system state:
				if ((physicscomponent.IsKinematic() || !IsSimulationEnabled()) && scene.transforms.Contains(entity))
				{
					TransformComponent& transform = *scene.transforms.GetComponent(entity);

					btMotionState* motionState = rigidbody->getMotionState();
					btTransform physicsTransform;

					XMFLOAT3 position = transform.GetPosition();
					XMFLOAT4 rotation = transform.GetRotation();
					btVector3 T(position.x, position.y, position.z);
					btQuaternion R(rotation.x, rotation.y, rotation.z, rotation.w);
					physicsTransform.setOrigin(T);
					physicsTransform.setRotation(R);
					motionState->setWorldTransform(physicsTransform);

					if (!IsSimulationEnabled())
					{
						// This is a more direct way of manipulating rigid body:
						rigidbody->setWorldTransform(physicsTransform);
					}

					btCollisionShape* shape = rigidbody->getCollisionShape();
					XMFLOAT3 scale = transform.GetScale();
					btVector3 S(scale.x, scale.y, scale.z);
					shape->setLocalScaling(S);
				}
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

		wi::jobsystem::Wait(ctx);

		// Perform internal simulation step:
		if (IsSimulationEnabled())
		{
			dynamicsWorld.stepSimulation(dt, ACCURACY);
		}

		// Feedback physics engine state to system:
		for (int i = 0; i < dynamicsWorld.getCollisionObjectArray().size(); ++i)
		{
			btCollisionObject* collisionobject = dynamicsWorld.getCollisionObjectArray()[i];
			Entity entity = (Entity)collisionobject->getUserIndex();

			btRigidBody* rigidbody = btRigidBody::upcast(collisionobject);
			if (rigidbody != nullptr)
			{
				RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);

				// Feedback non-kinematic objects to system:
				if (IsSimulationEnabled() && !physicscomponent->IsKinematic() && scene.transforms.Contains(entity))
				{
					TransformComponent& transform = *scene.transforms.GetComponent(entity);
	
					btTransform physicsTransform = rigidbody->getWorldTransform();
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

					// Soft body simulation nodes will update graphics mesh:
					for (size_t ind = 0; ind < physicscomponent->vertex_positions_simulation.size(); ++ind)
					{
						uint32_t physicsInd = physicscomponent->graphicsToPhysicsVertexMapping[ind];

						btSoftBody::Node& node = softbody->m_nodes[physicsInd];

						MeshComponent::Vertex_POS& vertex = physicscomponent->vertex_positions_simulation[ind];
						vertex.pos.x = node.m_x.getX();
						vertex.pos.y = node.m_x.getY();
						vertex.pos.z = node.m_x.getZ();

						XMFLOAT3 normal;
						normal.x = -node.m_n.getX();
						normal.y = -node.m_n.getY();
						normal.z = -node.m_n.getZ();
						vertex.MakeFromParams(normal);
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

								const XMFLOAT3 v0 = physicscomponent->vertex_positions_simulation[i0].pos;
								const XMFLOAT3 v1 = physicscomponent->vertex_positions_simulation[i1].pos;
								const XMFLOAT3 v2 = physicscomponent->vertex_positions_simulation[i2].pos;

								const XMFLOAT2 u0 = mesh.vertex_uvset_0[i0];
								const XMFLOAT2 u1 = mesh.vertex_uvset_0[i1];
								const XMFLOAT2 u2 = mesh.vertex_uvset_0[i2];

								const XMVECTOR nor0 = physicscomponent->vertex_positions_simulation[i0].LoadNOR();
								const XMVECTOR nor1 = physicscomponent->vertex_positions_simulation[i1].LoadNOR();
								const XMVECTOR nor2 = physicscomponent->vertex_positions_simulation[i2].LoadNOR();

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
}
