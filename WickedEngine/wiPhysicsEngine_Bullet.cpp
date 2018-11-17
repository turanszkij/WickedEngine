#include "wiPhysicsEngine.h"
#include "wiSceneSystem.h"
#include "wiProfiler.h"
#include "wiBackLog.h"
#include "wiJobSystem.h"

#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

#include <mutex>

using namespace std;
using namespace wiECS;
using namespace wiSceneSystem;


namespace wiPhysicsEngine
{
	bool ENABLED = true;
	std::mutex physicsLock;

	btVector3 gravity(0, -10, 0);
	int softbodyIterationCount = 5;
	btCollisionConfiguration* collisionConfiguration = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* overlappingPairCache = nullptr;
	btSequentialImpulseConstraintSolver* solver = nullptr;
	btDynamicsWorld* dynamicsWorld = nullptr;


	void Initialize()
	{
		// collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
		collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration;

		// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		dispatcher = new btCollisionDispatcher(collisionConfiguration);

		// btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
		overlappingPairCache = new btDbvtBroadphase;

		// the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		solver = new btSequentialImpulseConstraintSolver;

		//dynamicsWorld = new btSimpleDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
		//dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
		dynamicsWorld = new btSoftRigidDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

		dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER;
		dynamicsWorld->getDispatchInfo().m_enableSatConvex = true;
		dynamicsWorld->getSolverInfo().m_splitImpulse = true;

		dynamicsWorld->setGravity(gravity);

		btSoftRigidDynamicsWorld* softRigidWorld = (btSoftRigidDynamicsWorld*)dynamicsWorld;
		btSoftBodyWorldInfo& softWorldInfo = softRigidWorld->getWorldInfo();
		softWorldInfo.air_density = btScalar(1.2f);
		softWorldInfo.water_density = 0;
		softWorldInfo.water_offset = 0;
		softWorldInfo.water_normal = btVector3(0, 0, 0);
		softWorldInfo.m_gravity.setValue(gravity.x(), gravity.y(), gravity.z());
		softWorldInfo.m_sparsesdf.Initialize();

		wiBackLog::post("wiPhysicsEngine_Bullet Initialized");
	}
	void CleanUp()
	{
		delete dynamicsWorld;
		delete solver;
		delete overlappingPairCache;
		delete dispatcher;
		delete collisionConfiguration;
	}

	bool IsEnabled() { return ENABLED; }
	void SetEnabled(bool value) { ENABLED = value; }

	void AddRigidBody(Entity entity, wiSceneSystem::RigidBodyPhysicsComponent& physicscomponent, const wiSceneSystem::MeshComponent& mesh, const wiSceneSystem::TransformComponent& transform)
	{
		btVector3 S(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);

		btCollisionShape* shape = nullptr;

		switch (physicscomponent.shape)
		{
		case RigidBodyPhysicsComponent::CollisionShape::BOX:
			shape = new btBoxShape(S);
			break;

		case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
			shape = new btSphereShape(btScalar(S.x()));
			break;

		case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
			shape = new btCapsuleShape(btScalar(S.x()), btScalar(S.y()));
			break;

		case RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL:
			{
				shape = new btConvexHullShape();
				for (auto& pos : mesh.vertex_positions)
				{
					((btConvexHullShape*)shape)->addPoint(btVector3(pos.x, pos.y, pos.z));
				}
				shape->setLocalScaling(S);
			}
			break;

		case RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH:
			{
				int totalVerts = (int)mesh.vertex_positions.size();
				int totalTriangles = (int)mesh.indices.size() / 3;

				btVector3* btVerts = new btVector3[totalVerts];
				size_t i = 0;
				for (auto& pos : mesh.vertex_positions)
				{
					btVerts[i++] = btVector3(pos.x, pos.y, pos.z);
				}

				int* btInd = new int[mesh.indices.size()];
				i = 0;
				for (auto& ind : mesh.indices)
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
				shape->setLocalScaling(S);
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
			rigidbody->setUserIndex(*(int*)&entity);

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
	void AddSoftBody(Entity entity, wiSceneSystem::SoftBodyPhysicsComponent& physicscomponent, const wiSceneSystem::MeshComponent& mesh)
	{
		if (physicscomponent.physicsToGraphicsVertexMapping.empty())
		{
			physicscomponent.CreateFromMesh(mesh);
		}

		physicscomponent.saved_vertex_positions = mesh.vertex_positions;

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
			((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo()
			, btVerts
			, btInd
			, tCount
			, false
		);
		delete[] btVerts;
		delete[] btInd;

		if (softbody)
		{
			softbody->setUserIndex(*(int*)&entity);

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

			((btSoftRigidDynamicsWorld*)dynamicsWorld)->addSoftBody(softbody);
			physicscomponent.physicsobject = softbody;
		}
	}

	void RunPhysicsUpdateSystem(
		const WeatherComponent& weather,
		const ComponentManager<ArmatureComponent>& armatures,
		ComponentManager<TransformComponent>& transforms,
		ComponentManager<MeshComponent>& meshes,
		ComponentManager<ObjectComponent>& objects,
		ComponentManager<RigidBodyPhysicsComponent>& rigidbodies,
		ComponentManager<SoftBodyPhysicsComponent>& softbodies,
		float dt
	)
	{
		if (!IsEnabled() || dt <= 0)
		{
			return;
		}

		wiProfiler::BeginRange("Physics", wiProfiler::DOMAIN_CPU);

		btVector3 wind = btVector3(weather.windDirection.x, weather.windDirection.y, weather.windDirection.z);

		// System will register rigidbodies to objects, and update physics engine state for kinematics:
		wiJobSystem::Dispatch((uint32_t)rigidbodies.GetCount(), 256, [&](wiJobDispatchArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = rigidbodies[args.jobIndex];
			Entity entity = rigidbodies.GetEntity(args.jobIndex);

			if (physicscomponent.physicsobject == nullptr)
			{
				TransformComponent& transform = *transforms.GetComponent(entity);
				const ObjectComponent& object = *objects.GetComponent(entity);
				const MeshComponent& mesh = *meshes.GetComponent(object.meshID);
				physicsLock.lock();
				AddRigidBody(entity, physicscomponent, mesh, transform);
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

				// For kinematic object, system updates physics state, else the physics updates system state:
				if (physicscomponent.IsKinematic())
				{
					TransformComponent& transform = *transforms.GetComponent(entity);

					btMotionState* motionState = rigidbody->getMotionState();
					btTransform physicsTransform;

					XMFLOAT3 position = transform.GetPosition();
					XMFLOAT4 rotation = transform.GetRotation();
					btVector3 T(position.x, position.y, position.z);
					btQuaternion R(rotation.x, rotation.y, rotation.z, rotation.w);
					physicsTransform.setOrigin(T);
					physicsTransform.setRotation(R);
					motionState->setWorldTransform(physicsTransform);
				}
			}
		});

		// System will register softbodies to meshes and update physics engine state:
		wiJobSystem::Dispatch((uint32_t)softbodies.GetCount(), 1, [&](wiJobDispatchArgs args) {

			SoftBodyPhysicsComponent& physicscomponent = softbodies[args.jobIndex];
			Entity entity = softbodies.GetEntity(args.jobIndex);
			MeshComponent& mesh = *meshes.GetComponent(entity);
			mesh.SetDynamic(true);

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
						XMFLOAT3 position = physicscomponent.saved_vertex_positions[graphicsInd];
						XMVECTOR P = XMLoadFloat3(&position);
						P = XMVector3Transform(P, worldMatrix);
						// todo: here goes skinning...
						XMStoreFloat3(&position, P);
						node.m_x = btVector3(position.x, position.y, position.z);
					}
				}
			}
		});

		wiJobSystem::Wait();

		// Perform internal simulation step:
		dynamicsWorld->stepSimulation(dt, 10);

		// Feedback physics engine state to system:
		for (int i = 0; i < dynamicsWorld->getCollisionObjectArray().size(); ++i)
		{
			btCollisionObject* collisionobject = dynamicsWorld->getCollisionObjectArray()[i];
			int userIndex = collisionobject->getUserIndex();
			Entity entity = *(Entity*)&userIndex;

			btRigidBody* rigidbody = btRigidBody::upcast(collisionobject);
			if (rigidbody != nullptr)
			{
				RigidBodyPhysicsComponent* physicscomponent = rigidbodies.GetComponent(entity);
				if (physicscomponent == nullptr)
				{
					dynamicsWorld->removeRigidBody(rigidbody);
					i--;
					continue;
				}

				// Feedback non-kinematic objects to system:
				if(!physicscomponent->IsKinematic())
				{
					TransformComponent& transform = *transforms.GetComponent(entity);

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
					SoftBodyPhysicsComponent* physicscomponent = softbodies.GetComponent(entity);
					if (physicscomponent == nullptr)
					{
						((btSoftRigidDynamicsWorld*)dynamicsWorld)->removeSoftBody(softbody);
						i--;
						continue;
					}

					MeshComponent& mesh = *meshes.GetComponent(entity);

					// System mesh aabb will be queried from physics engine soft body:
					btVector3 aabb_min;
					btVector3 aabb_max;
					softbody->getAabb(aabb_min, aabb_max);
					mesh.aabb = AABB(XMFLOAT3(aabb_min.x(), aabb_min.y(), aabb_min.z()), XMFLOAT3(aabb_max.x(), aabb_max.y(), aabb_max.z()));

					// Soft body simulation nodes will update graphics mesh:
					for (size_t ind = 0; ind < mesh.vertex_positions.size(); ++ind)
					{
						uint32_t physicsInd = physicscomponent->graphicsToPhysicsVertexMapping[ind];
						float weight = physicscomponent->weights[physicsInd];

						btSoftBody::Node& node = softbody->m_nodes[physicsInd];

						XMFLOAT3& position = mesh.vertex_positions[ind];
						position.x = node.m_x.getX();
						position.y = node.m_x.getY();
						position.z = node.m_x.getZ();

						if (!mesh.vertex_normals.empty())
						{
							XMFLOAT3& normal = mesh.vertex_normals[ind];
							normal.x = -node.m_n.getX();
							normal.y = -node.m_n.getY();
							normal.z = -node.m_n.getZ();
						}
					}
				}
			}
		}

		wiProfiler::EndRange(); // Physics
	}
}
