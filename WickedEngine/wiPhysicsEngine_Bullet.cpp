#include "wiPhysicsEngine.h"
#include "wiSceneSystem.h"
#include "wiProfiler.h"


#include "btBulletDynamicsCommon.h"

#include "LinearMath/btHashMap.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "BulletSoftBody/btSoftBodySolvers.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

#include <unordered_map>

using namespace std;
using namespace wiECS;
using namespace wiSceneSystem;

namespace wiPhysicsEngine
{
	bool ENABLED = true;

	btVector3 gravity(0, -10, 0);
	int softbodyIterationCount = 5;
	btCollisionConfiguration* collisionConfiguration = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* overlappingPairCache = nullptr;
	btSequentialImpulseConstraintSolver* solver = nullptr;
	btDynamicsWorld* dynamicsWorld = nullptr;

	btSoftBodySolver* softBodySolver = nullptr;
	btSoftBodySolverOutput* softBodySolverOutput = nullptr;


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
		dynamicsWorld = new btSoftRigidDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration, softBodySolver);

		dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER;
		dynamicsWorld->getDispatchInfo().m_enableSatConvex = true;
		dynamicsWorld->getSolverInfo().m_splitImpulse = true;

		dynamicsWorld->setGravity(gravity);

		btSoftRigidDynamicsWorld* softRigidWorld = (btSoftRigidDynamicsWorld*)dynamicsWorld;
		btSoftBodyWorldInfo& softWorldInfo = softRigidWorld->getWorldInfo();
		softWorldInfo.air_density = (btScalar)0.0;
		softWorldInfo.water_density = 0;
		softWorldInfo.water_offset = 0;
		softWorldInfo.water_normal = btVector3(0, 0, 0);
		softWorldInfo.m_gravity.setValue(gravity.x(), gravity.y(), gravity.z());
		softWorldInfo.m_sparsesdf.Initialize();

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

	void Remove(int id)
	{
		btCollisionObject* collisionobject = dynamicsWorld->getCollisionObjectArray()[id];

		btRigidBody* rigidbody = btRigidBody::upcast(collisionobject);
		if (rigidbody != nullptr)
		{
			dynamicsWorld->removeRigidBody(rigidbody);
		}
		else
		{
			btSoftBody* softbody = btSoftBody::upcast(collisionobject);

			if (softbody != nullptr)
			{
				((btSoftRigidDynamicsWorld*)dynamicsWorld)->removeSoftBody(softbody);
			}
		}

		dynamicsWorld->removeCollisionObject(collisionobject);
	}
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

			btRigidBody* body = new btRigidBody(rbInfo);
			body->setUserIndex(*(int*)&entity);

			if (physicscomponent.IsKinematic())
			{
				body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			}
			if (physicscomponent.IsDisableDeactivation())
			{
				body->setActivationState(DISABLE_DEACTIVATION);
			}

			dynamicsWorld->addRigidBody(body);
			physicscomponent.physicsobject = body;
		}
	}
	void AddSoftBody(Entity entity, wiSceneSystem::SoftBodyPhysicsComponent& physicscomponent, const wiSceneSystem::MeshComponent& mesh, const wiSceneSystem::TransformComponent& transform)
	{
		btVector3 S = btVector3(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
		btQuaternion R = btQuaternion(transform.rotation_local.x, transform.rotation_local.y, transform.rotation_local.z, transform.rotation_local.z);
		btVector3 T = btVector3(transform.translation_local.x, transform.translation_local.y, transform.translation_local.z);

		if (physicscomponent.physicsvertices.empty())
		{
			physicscomponent.CreateFromMesh(mesh);
		}

		const int vCount = (int)physicscomponent.physicsvertices.size();
		btScalar* btVerts = new btScalar[vCount * 3];
		for (int i = 0; i < vCount; ++i) 
		{
			btVerts[i * 3 + 0] = btScalar(physicscomponent.physicsvertices[i].x);
			btVerts[i * 3 + 1] = btScalar(physicscomponent.physicsvertices[i].y);
			btVerts[i * 3 + 2] = btScalar(physicscomponent.physicsvertices[i].z);
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


		btSoftBody* softBody = btSoftBodyHelpers::CreateFromTriMesh(
			((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo()
			, btVerts
			, btInd
			, tCount
			, false
		);


		delete[] btVerts;
		delete[] btInd;

		if (softBody)
		{
			softBody->setUserIndex(*(int*)&entity);

			btSoftBody::Material* pm = softBody->appendMaterial();
			pm->m_kLST = 0.5;
			pm->m_kVST = 0.5;
			pm->m_kAST = 0.5;
			pm->m_flags = 0;
			softBody->generateBendingConstraints(2, pm);
			softBody->randomizeConstraints();

			//btTransform shapeTransform;
			//shapeTransform.setIdentity();
			//shapeTransform.setOrigin(T);
			//shapeTransform.setRotation(R);
			//softBody->scale(S);
			//softBody->transform(shapeTransform);


			softBody->m_cfg.piterations = softbodyIterationCount;
			softBody->m_cfg.aeromodel = btSoftBody::eAeroModel::F_TwoSidedLiftDrag;

			softBody->m_cfg.kAHR = btScalar(.69); //0.69		Anchor hardness  [0,1]
			softBody->m_cfg.kCHR = btScalar(1.0); //1			Rigid contact hardness  [0,1]
			softBody->m_cfg.kDF = btScalar(physicscomponent.friction); //0.2			Dynamic friction coefficient  [0,1]
			softBody->m_cfg.kDG = btScalar(0.01); //0			Drag coefficient  [0,+inf]
			softBody->m_cfg.kDP = btScalar(0.0); //0			Damping coefficient  [0,1]
			softBody->m_cfg.kKHR = btScalar(0.1); //0.1			Kinetic contact hardness  [0,1]
			softBody->m_cfg.kLF = btScalar(0.1); //0			Lift coefficient  [0,+inf]
			softBody->m_cfg.kMT = btScalar(0.0); //0			Pose matching coefficient  [0,1]
			softBody->m_cfg.kPR = btScalar(0.0); //0			Pressure coefficient  [-1,1]
			softBody->m_cfg.kSHR = btScalar(1.0); //1			Soft contacts hardness  [0,1]
			softBody->m_cfg.kVC = btScalar(0.0); //0			Volume conseration coefficient  [0,+inf]
			softBody->m_cfg.kVCF = btScalar(1.0); //1			Velocities correction factor (Baumgarte)

			softBody->m_cfg.kSKHR_CL = btScalar(1.0); //1			Soft vs. kinetic hardness   [0,1]
			softBody->m_cfg.kSK_SPLT_CL = btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]
			softBody->m_cfg.kSRHR_CL = btScalar(0.1); //0.1			Soft vs. rigid hardness  [0,1]
			softBody->m_cfg.kSR_SPLT_CL = btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]
			softBody->m_cfg.kSSHR_CL = btScalar(0.5); //0.5			Soft vs. soft hardness  [0,1]
			softBody->m_cfg.kSS_SPLT_CL = btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]


			btScalar mass = btScalar(physicscomponent.mass);
			softBody->setTotalMass(mass);

			//int mvg = physicscomponent.massVG;
			//if (mvg >= 0) {
			//	for (auto it = mesh.vertexGroups[mvg].vertex_weights.begin(); it != mesh.vertexGroups[mvg].vertex_weights.end(); ++it) {
			//		int vi = (*it).first;
			//		float wei = (*it).second;
			//		int index = physicscomponent.physicalmapGP[vi];
			//		softBody->setMass(index, softBody->getMass(index)*btScalar(wei));
			//	}
			//}


			//int gvg = physicscomponent.goalVG;
			//if (gvg >= 0) {
			//	for (auto it = mesh.vertexGroups[gvg].vertex_weights.begin(); it != mesh.vertexGroups[gvg].vertex_weights.end(); ++it) {
			//		int vi = (*it).first;
			//		int index = physicscomponent.physicalmapGP[vi];
			//		float weight = (*it).second;
			//		if (weight == 1)
			//			softBody->setMass(index, 0);
			//	}
			//}

			for (size_t i = 0; i < physicscomponent.physicsvertices.size(); ++i)
			{
				softBody->setMass((int)i, physicscomponent.mass);
			}

			softBody->getCollisionShape()->setMargin(btScalar(0.2));

			//softBody->setWindVelocity(wind);

			softBody->setPose(true, true);

			softBody->setActivationState(DISABLE_DEACTIVATION);

			((btSoftRigidDynamicsWorld*)dynamicsWorld)->addSoftBody(softBody);
			physicscomponent.physicsobject = softBody;
		}
	}

	void RunPhysicsUpdateSystem(
		const WeatherComponent& weather,
		ComponentManager<TransformComponent>& transforms,
		ComponentManager<MeshComponent>& meshes,
		ComponentManager<ObjectComponent>& objects,
		ComponentManager<RigidBodyPhysicsComponent>& rigidbodies,
		ComponentManager<SoftBodyPhysicsComponent>& softbodies,
		float dt
	)
	{
		if (!IsEnabled())
		{
			return;
		}

		wiProfiler::GetInstance().BeginRange("Physics", wiProfiler::DOMAIN_CPU);

		btVector3 wind = btVector3(weather.windDirection.x, weather.windDirection.y, weather.windDirection.z);

		// Try to register rigidbodies to Objects:
		for (size_t i = 0; i < rigidbodies.GetCount(); ++i)
		{
			RigidBodyPhysicsComponent& physicscomponent = rigidbodies[i];

			if (physicscomponent.physicsobject == nullptr)
			{
				Entity entity = rigidbodies.GetEntity(i);
				TransformComponent& transform = *transforms.GetComponent(entity);
				const ObjectComponent& object = *objects.GetComponent(entity);
				const MeshComponent& mesh = *meshes.GetComponent(object.meshID);
				AddRigidBody(entity, physicscomponent, mesh, transform);
			}
		}

		// Try to register softbodies to Meshes:
		for (size_t i = 0; i < softbodies.GetCount(); ++i)
		{
			Entity entity = softbodies.GetEntity(i);
			SoftBodyPhysicsComponent& physicscomponent = softbodies[i];
			MeshComponent& mesh = *meshes.GetComponent(entity);
			mesh.SetDynamic(true);

			if (physicscomponent.physicsobject == nullptr)
			{
				TransformComponent transform;
				AddSoftBody(entity, physicscomponent, mesh, transform);
			}
		}

		// Update all physics components and remove from simulation if components no longer exist:
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
					Remove(i);
					break;
				}

				TransformComponent& transform = *transforms.GetComponent(entity);

				btMotionState* motionState = rigidbody->getMotionState();
				btTransform physicsTransform;

				if (physicscomponent->IsKinematic())
				{
					btVector3 T(transform.translation_local.x, transform.translation_local.y, transform.translation_local.z);
					btQuaternion R(transform.rotation_local.x, transform.rotation_local.y, transform.rotation_local.z, transform.rotation_local.w);
					physicsTransform.setOrigin(T);
					physicsTransform.setRotation(R);
					motionState->setWorldTransform(physicsTransform);
				}
				else
				{
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
						Remove(i);
						break;
					}

					softbody->setWindVelocity(wind);

					MeshComponent& mesh = *meshes.GetComponent(entity);

					btVector3 aabb_min;
					btVector3 aabb_max;
					softbody->getAabb(aabb_min, aabb_max);
					mesh.aabb = AABB(XMFLOAT3(aabb_min.x(), aabb_min.y(), aabb_min.z()), XMFLOAT3(aabb_max.x(), aabb_max.y(), aabb_max.z()));

					btSoftBody::tNodeArray& nodes(softbody->m_nodes);
					for (size_t ind = 0; ind < mesh.vertex_positions.size(); ++ind)
					{
						uint32_t physicsInd = physicscomponent->graphicsToPhysicsVertexMapping[ind];
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

		dynamicsWorld->stepSimulation(dt, 10);

		wiProfiler::GetInstance().EndRange(); // Physics
	}
}
