#include "wiPhysicsEngine.h"
#include "wiScene.h"
#include "wiProfiler.h"
#include "wiBackLog.h"
#include "wiJobSystem.h"
#include "wiRenderer.h"

#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

#include <mutex>
#include <memory>

using namespace std;
using namespace wiECS;
using namespace wiScene;


namespace wiPhysicsEngine
{
	bool ENABLED = true;
	bool SIMULATION_ENABLED = true;
	bool DEBUGDRAW_ENABLED = false;
	int ACCURACY = 10;
	std::mutex physicsLock;

	btVector3 gravity(0, -10, 0);
	int softbodyIterationCount = 5;
	btSoftBodyRigidBodyCollisionConfiguration collisionConfiguration;
	btDbvtBroadphase overlappingPairCache;
	btSequentialImpulseConstraintSolver solver;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btDynamicsWorld> dynamicsWorld;

	class DebugDraw : public btIDebugDraw
	{
		void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
		{
			wiRenderer::RenderableLine line;
			line.start = XMFLOAT3(from.x(), from.y(), from.z());
			line.end = XMFLOAT3(to.x(), to.y(), to.z());
			line.color_start = line.color_end = XMFLOAT4(color.x(), color.y(), color.z(), 1.0f);
			wiRenderer::DrawLine(line);
		}
		void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override
		{
		}
		void reportErrorWarning(const char* warningString) override
		{
			wiBackLog::post(warningString);
		}
		void draw3dText(const btVector3& location, const char* textString) override
		{
		}
		void setDebugMode(int debugMode) override
		{
		}
		int getDebugMode() const override
		{
			return DBG_DrawWireframe;
		}
	};
	DebugDraw debugDraw;

	void Initialize()
	{
		dispatcher = std::make_unique<btCollisionDispatcher>(&collisionConfiguration);
		dynamicsWorld = std::make_unique<btSoftRigidDynamicsWorld>(dispatcher.get(), &overlappingPairCache, &solver, &collisionConfiguration);

		dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER;
		dynamicsWorld->getDispatchInfo().m_enableSatConvex = true;
		dynamicsWorld->getSolverInfo().m_splitImpulse = true;

		dynamicsWorld->setGravity(gravity);

		btSoftRigidDynamicsWorld* softRigidWorld = (btSoftRigidDynamicsWorld*)dynamicsWorld.get();
		btSoftBodyWorldInfo& softWorldInfo = softRigidWorld->getWorldInfo();
		softWorldInfo.air_density = btScalar(1.2f);
		softWorldInfo.water_density = 0;
		softWorldInfo.water_offset = 0;
		softWorldInfo.water_normal = btVector3(0, 0, 0);
		softWorldInfo.m_gravity.setValue(gravity.x(), gravity.y(), gravity.z());
		softWorldInfo.m_sparsesdf.Initialize();

		softRigidWorld->setDebugDrawer(&debugDraw);

		wiBackLog::post("wiPhysicsEngine_Bullet Initialized");
	}

	bool IsEnabled() { return ENABLED; }
	void SetEnabled(bool value) { ENABLED = value; }

	bool IsSimulationEnabled() { return SIMULATION_ENABLED; }
	void SetSimulationEnabled(bool value) { SIMULATION_ENABLED = value; }

	bool IsDebugDrawEnabled() { return DEBUGDRAW_ENABLED; }
	void SetDebugDrawEnabled(bool value) { DEBUGDRAW_ENABLED = value; }

	int GetAccuracy() { return ACCURACY; }
	void SetAccuracy(int value) { ACCURACY = value; }

	void AddRigidBody(Entity entity, wiScene::RigidBodyPhysicsComponent& physicscomponent, const wiScene::TransformComponent& transform, const wiScene::MeshComponent* mesh)
	{
		btCollisionShape* shape = nullptr;

		switch (physicscomponent.shape)
		{
		case RigidBodyPhysicsComponent::CollisionShape::BOX:
		{
			shape = new btBoxShape(btVector3(physicscomponent.box.halfextents.x, physicscomponent.box.halfextents.y, physicscomponent.box.halfextents.z));
		}
		break;

		case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
		{
			shape = new btSphereShape(btScalar(physicscomponent.sphere.radius));
		}
		break;

		case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
			shape = new btCapsuleShape(btScalar(physicscomponent.capsule.radius), btScalar(physicscomponent.capsule.height));
			break;

		case RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL:
			if(mesh != nullptr)
			{
				shape = new btConvexHullShape();
				for (auto& pos : mesh->vertex_positions)
				{
					((btConvexHullShape*)shape)->addPoint(btVector3(pos.x, pos.y, pos.z));
				}
				btVector3 S(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
				shape->setLocalScaling(S);
			}
			else
			{
				wiBackLog::post("Convex Hull physics requested, but no MeshComponent provided!");
				assert(0);
			}
			break;

		case RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH:
			if(mesh != nullptr)
			{
				int totalVerts = (int)mesh->vertex_positions.size();
				int totalTriangles = (int)mesh->indices.size() / 3;

				btVector3* btVerts = new btVector3[totalVerts];
				size_t i = 0;
				for (auto& pos : mesh->vertex_positions)
				{
					btVerts[i++] = btVector3(pos.x, pos.y, pos.z);
				}

				int* btInd = new int[mesh->indices.size()];
				i = 0;
				for (auto& ind : mesh->indices)
				{
					btInd[i++] = ind;
				}

				int vertStride = sizeof(btVector3);
				int indexStride = 3 * sizeof(int);

				btTriangleIndexVertexArray* indexVertexArrays = new btTriangleIndexVertexArray(
					totalTriangles,
					btInd,
					indexStride,
					totalVerts,
					(btScalar*)&btVerts[0].x(),
					vertStride
				);

				bool useQuantizedAabbCompression = true;
				shape = new btBvhTriangleMeshShape(indexVertexArrays, useQuantizedAabbCompression);
				btVector3 S(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
				shape->setLocalScaling(S);
			}
			else
			{
				wiBackLog::post("Triangle Mesh physics requested, but no MeshComponent provided!");
				assert(0);
			}
			break;
		}

		if (shape != nullptr)
		{
			// Use default margin for now
			//shape->setMargin(btScalar(0.01));

			btScalar mass = physicscomponent.mass;

			bool isDynamic = (mass != 0.f && !physicscomponent.IsKinematic());

			btVector3 localInertia(0, 0, 0);
			if (isDynamic)
			{
				shape->calculateLocalInertia(mass, localInertia);
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
			btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);

			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
			//rbInfo.m_friction = physicscomponent.friction;
			//rbInfo.m_restitution = physicscomponent.restitution;
			//rbInfo.m_linearDamping = physicscomponent.damping;
			//rbInfo.m_angularDamping = physicscomponent.damping;

			btRigidBody* rigidbody = new btRigidBody(rbInfo);
			rigidbody->setUserIndex(entity);

			if (physicscomponent.IsKinematic())
			{
				rigidbody->setCollisionFlags(rigidbody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			}
			if (physicscomponent.IsDisableDeactivation())
			{
				rigidbody->setActivationState(DISABLE_DEACTIVATION);
			}

			dynamicsWorld->addRigidBody(rigidbody);
			physicscomponent.physicsobject = rigidbody;
		}
	}
	void AddSoftBody(Entity entity, wiScene::SoftBodyPhysicsComponent& physicscomponent, const wiScene::MeshComponent& mesh)
	{
		physicscomponent.CreateFromMesh(mesh);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&physicscomponent.worldMatrix);

		const int vCount = (int)physicscomponent.physicsToGraphicsVertexMapping.size();
		btScalar* btVerts = new btScalar[vCount * 3];
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

		const int iCount = (int)mesh.indices.size();
		const int tCount = iCount / 3;
		int* btInd = new int[iCount];
		for (int i = 0; i < iCount; ++i) 
		{
			uint32_t ind = mesh.indices[i];
			uint32_t mappedIndex = physicscomponent.graphicsToPhysicsVertexMapping[ind];
			btInd[i] = (int)mappedIndex;
		}

		btSoftBody* softbody = btSoftBodyHelpers::CreateFromTriMesh(
			((btSoftRigidDynamicsWorld*)dynamicsWorld.get())->getWorldInfo()
			, btVerts
			, btInd
			, tCount
			, false
		);
		delete[] btVerts;
		delete[] btInd;

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

			((btSoftRigidDynamicsWorld*)dynamicsWorld.get())->addSoftBody(softbody);
			physicscomponent.physicsobject = softbody;
		}
	}

	void RunPhysicsUpdateSystem(
		wiJobSystem::context& ctx,
		Scene& scene,
		float dt
	)
	{
		if (!IsEnabled() || dt <= 0)
			return;

		auto range = wiProfiler::BeginRangeCPU("Physics");

		btVector3 wind = btVector3(scene.weather.windDirection.x, scene.weather.windDirection.y, scene.weather.windDirection.z);

		// System will register rigidbodies to objects, and update physics engine state for kinematics:
		wiJobSystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), 256, [&](wiJobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);

			if (physicscomponent.physicsobject == nullptr)
			{
				TransformComponent& transform = *scene.transforms.GetComponent(entity);
				const ObjectComponent* object = scene.objects.GetComponent(entity);
				const MeshComponent* mesh = nullptr;
				if (object != nullptr)
				{
					mesh = scene.meshes.GetComponent(object->meshID);
				}
				physicsLock.lock();
				AddRigidBody(entity, physicscomponent, transform, mesh);
				physicsLock.unlock();
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				btRigidBody* rigidbody = (btRigidBody*)physicscomponent.physicsobject;

				int activationState = rigidbody->getActivationState();
				if (physicscomponent.IsDisableDeactivation())
				{
					activationState |= DISABLE_DEACTIVATION;
				}
				else
				{
					activationState &= ~DISABLE_DEACTIVATION;
				}
				rigidbody->setActivationState(activationState);

				rigidbody->setDamping(
					physicscomponent.damping_linear,
					physicscomponent.damping_angular
				);
				rigidbody->setFriction(physicscomponent.friction);
				rigidbody->setRestitution(physicscomponent.restitution);

				// For kinematic object, system updates physics state, else the physics updates system state:
				if (physicscomponent.IsKinematic() || !IsSimulationEnabled())
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
		wiJobSystem::Dispatch(ctx, (uint32_t)scene.softbodies.GetCount(), 1, [&](wiJobArgs args) {

			SoftBodyPhysicsComponent& physicscomponent = scene.softbodies[args.jobIndex];
			Entity entity = scene.softbodies.GetEntity(args.jobIndex);
			MeshComponent& mesh = *scene.meshes.GetComponent(entity);
			const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;
			mesh.SetDynamic(true);

			if (!mesh.vertexBuffer_PRE.IsValid())
			{
				using namespace wiGraphics;
				GraphicsDevice* device = wiRenderer::GetDevice();
				device->CreateBuffer(&mesh.vertexBuffer_POS.desc, nullptr, &mesh.streamoutBuffer_POS);
				device->CreateBuffer(&mesh.vertexBuffer_POS.desc, nullptr, &mesh.vertexBuffer_PRE);
				device->CreateBuffer(&mesh.vertexBuffer_TAN.desc, nullptr, &mesh.streamoutBuffer_TAN);
			}

			if (physicscomponent._flags & SoftBodyPhysicsComponent::FORCE_RESET)
			{
				physicscomponent._flags &= ~SoftBodyPhysicsComponent::FORCE_RESET;
				if (physicscomponent.physicsobject != nullptr)
				{
					((btSoftRigidDynamicsWorld*)dynamicsWorld.get())->removeSoftBody((btSoftBody*)physicscomponent.physicsobject);
					physicscomponent.physicsobject = nullptr;
				}
			}
			if (physicscomponent._flags & SoftBodyPhysicsComponent::SAFE_TO_REGISTER && physicscomponent.physicsobject == nullptr)
			{
				physicsLock.lock();
				AddSoftBody(entity, physicscomponent, mesh);
				physicsLock.unlock();
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				btSoftBody* softbody = (btSoftBody*)physicscomponent.physicsobject;
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
						XMVECTOR P = armature == nullptr ? XMLoadFloat3(&position) : wiScene::SkinVertex(mesh, *armature, graphicsInd);
						P = XMVector3Transform(P, worldMatrix);
						XMStoreFloat3(&position, P);
						node.m_x = btVector3(position.x, position.y, position.z);
					}
				}
			}
		});

		wiJobSystem::Wait(ctx);

		// Perform internal simulation step:
		if (IsSimulationEnabled())
		{
			dynamicsWorld->stepSimulation(dt, ACCURACY);
		}

		// Feedback physics engine state to system:
		for (int i = 0; i < dynamicsWorld->getCollisionObjectArray().size(); ++i)
		{
			btCollisionObject* collisionobject = dynamicsWorld->getCollisionObjectArray()[i];
			Entity entity = (Entity)collisionobject->getUserIndex();

			btRigidBody* rigidbody = btRigidBody::upcast(collisionobject);
			if (rigidbody != nullptr)
			{
				RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);
				if (physicscomponent == nullptr || physicscomponent->physicsobject != rigidbody)
				{
					dynamicsWorld->removeRigidBody(rigidbody);
					i--;
					continue;
				}

				// Feedback non-kinematic objects to system:
				if (IsSimulationEnabled() && !physicscomponent->IsKinematic())
				{
					TransformComponent& transform = *scene.transforms.GetComponent(entity);

					btMotionState* motionState = rigidbody->getMotionState();
					btTransform physicsTransform;

					motionState->getWorldTransform(physicsTransform);
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
					if (physicscomponent == nullptr || physicscomponent->physicsobject != softbody)
					{
						((btSoftRigidDynamicsWorld*)dynamicsWorld.get())->removeSoftBody(softbody);
						i--;
						continue;
					}

					MeshComponent& mesh = *scene.meshes.GetComponent(entity);

					// System mesh aabb will be queried from physics engine soft body:
					btVector3 aabb_min;
					btVector3 aabb_max;
					softbody->getAabb(aabb_min, aabb_max);
					physicscomponent->aabb = AABB(XMFLOAT3(aabb_min.x(), aabb_min.y(), aabb_min.z()), XMFLOAT3(aabb_max.x(), aabb_max.y(), aabb_max.z()));

					// Soft body simulation nodes will update graphics mesh:
					for (size_t ind = 0; ind < physicscomponent->vertex_positions_simulation.size(); ++ind)
					{
						uint32_t physicsInd = physicscomponent->graphicsToPhysicsVertexMapping[ind];
						float weight = physicscomponent->weights[physicsInd];

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
					if (!mesh.vertex_uvset_0.empty())
					{
						for (size_t i = 0; i < mesh.indices.size(); i += 3)
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

						for (size_t i = 0; i < physicscomponent->vertex_tangents_simulation.size(); ++i)
						{
							physicscomponent->vertex_tangents_simulation[i].FromFULL(physicscomponent->vertex_tangents_tmp[i]);
						}
					}

					if (scene.cmd != wiGraphics::INVALID_COMMANDLIST)
					{
						using namespace wiGraphics;
						GraphicsDevice* device = wiRenderer::GetDevice();
						scene.cmd_locker.lock();
						device->UpdateBuffer(&mesh.streamoutBuffer_POS, physicscomponent->vertex_positions_simulation.data(), scene.cmd);
						device->UpdateBuffer(&mesh.streamoutBuffer_TAN, physicscomponent->vertex_tangents_simulation.data(), scene.cmd);
						scene.cmd_locker.unlock();
					}

				}
			}
		}

		if (IsDebugDrawEnabled())
		{
			dynamicsWorld->debugDrawWorld();
		}

		wiProfiler::EndRange(range); // Physics
	}



	void ApplyForce(
		const wiScene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			btRigidBody* rigidbody = (btRigidBody*)physicscomponent.physicsobject;
			rigidbody->applyCentralForce(btVector3(force.x, force.y, force.z));
		}
	}
	void ApplyForceAt(
		const wiScene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force,
		const XMFLOAT3& at
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			btRigidBody* rigidbody = (btRigidBody*)physicscomponent.physicsobject;
			rigidbody->applyForce(btVector3(force.x, force.y, force.z), btVector3(at.x, at.y, at.z));
		}
	}

	void ApplyImpulse(
		const wiScene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			btRigidBody* rigidbody = (btRigidBody*)physicscomponent.physicsobject;
			rigidbody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
		}
	}
	void ApplyImpulseAt(
		const wiScene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			btRigidBody* rigidbody = (btRigidBody*)physicscomponent.physicsobject;
			rigidbody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(at.x, at.y, at.z));
		}
	}

}
