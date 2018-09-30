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
	btVector3 gravity(0, -110, 0);
	int softbodyIterationCount = 5;
	btCollisionConfiguration* collisionConfiguration = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* overlappingPairCache = nullptr;
	btSequentialImpulseConstraintSolver* solver = nullptr;
	btDynamicsWorld* dynamicsWorld = nullptr;

	btSoftBodySolver* softBodySolver = nullptr;
	btSoftBodySolverOutput* softBodySolverOutput = nullptr;


	unordered_map<Entity, int> lookup;


	void Initialize()
	{
		// collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
		collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();

		// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		dispatcher = new btCollisionDispatcher(collisionConfiguration);

		// btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
		overlappingPairCache = new btDbvtBroadphase();

		// the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		solver = new btSequentialImpulseConstraintSolver;

		//dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
		dynamicsWorld = new btSoftRigidDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration, softBodySolver);

		dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER;
		dynamicsWorld->getDispatchInfo().m_enableSatConvex = true;
		dynamicsWorld->getSolverInfo().m_splitImpulse = true;

		dynamicsWorld->setGravity(gravity);

		((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().air_density = (btScalar)0.0;
		((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().water_density = 0;
		((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().water_offset = 0;
		((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().water_normal = btVector3(0, 0, 0);
		((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().m_gravity.setValue(gravity.x(), gravity.y(), gravity.z());
		((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().m_sparsesdf.Initialize();

	}
	void CleanUp()
	{
		delete dynamicsWorld;
		delete solver;
		delete overlappingPairCache;
		delete dispatcher;
		delete collisionConfiguration;
	}


	void Remove(Entity entity)
	{
		auto it = lookup.find(entity);
		if (it != lookup.end())
		{
			int id = it->second;
			lookup.erase(it);

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
	}
	int AddRigidBody(Entity entity, wiSceneSystem::RigidBodyPhysicsComponent& physicscomponent, const wiSceneSystem::MeshComponent& mesh, const wiSceneSystem::TransformComponent& transform)
	{
		btVector3 S(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);

		btCollisionShape* shape = nullptr;

		switch (physicscomponent.shape)
		{
		case RigidBodyPhysicsComponent::CollisionShape::BOX:
			shape = new btBoxShape(S * 0.5f);
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
			shape->setMargin(btScalar(0.01));

			btScalar mass(physicscomponent.mass);

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
			rbInfo.m_friction = physicscomponent.friction;
			rbInfo.m_restitution = physicscomponent.restitution;
			rbInfo.m_linearDamping = physicscomponent.damping;
			rbInfo.m_angularDamping = physicscomponent.damping;

			btRigidBody* body = new btRigidBody(rbInfo);
			if (physicscomponent.IsKinematic())
			{
				body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			}
			if (physicscomponent.IsDisableDeactivation())
			{
				body->setActivationState(DISABLE_DEACTIVATION);
			}

			int id = dynamicsWorld->getCollisionObjectArray().size();
			lookup[entity] = id;
			dynamicsWorld->addRigidBody(body);
			return id;
		}

		return -1;
	}
	int AddSoftBody(Entity entity, wiSceneSystem::SoftBodyPhysicsComponent& physicscomponent, const wiSceneSystem::MeshComponent& mesh, const wiSceneSystem::TransformComponent& transform)
	{
		btVector3 S = btVector3(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
		btQuaternion R = btQuaternion(transform.rotation_local.x, transform.rotation_local.y, transform.rotation_local.z, transform.rotation_local.z);
		btVector3 T = btVector3(transform.translation_local.x, transform.translation_local.y, transform.translation_local.z);

		const int vCount = (int)physicscomponent.physicsvertices.size();
		btScalar* btVerts = new btScalar[vCount * 3];
		for (int i = 0; i < vCount * 3; i += 3) {
			const int vindex = i / 3;
			btVerts[i] = btScalar(physicscomponent.physicsvertices[vindex].x);
			btVerts[i + 1] = btScalar(physicscomponent.physicsvertices[vindex].y);
			btVerts[i + 2] = btScalar(physicscomponent.physicsvertices[vindex].z);
		}
		const int iCount = (int)physicscomponent.physicsindices.size();
		const int tCount = iCount / 3;
		int* btInd = new int[iCount];
		for (int i = 0; i < iCount; ++i) {
			btInd[i] = physicscomponent.physicsindices[i];
		}


		btSoftBody* softBody = btSoftBodyHelpers::CreateFromTriMesh(
			((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo()
			, &btVerts[0]
			, &btInd[0]
			, tCount
			, false
		);


		delete[] btVerts;
		delete[] btInd;

		if (softBody)
		{
			btSoftBody::Material* pm = softBody->appendMaterial();
			pm->m_kLST = 0.5;
			pm->m_kVST = 0.5;
			pm->m_kAST = 0.5;
			pm->m_flags = 0;
			softBody->generateBendingConstraints(2, pm);
			softBody->randomizeConstraints();

			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(T);
			shapeTransform.setRotation(R);
			softBody->scale(S);
			softBody->transform(shapeTransform);


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

			softBody->getCollisionShape()->setMargin(btScalar(0.2));

			//softBody->setWindVelocity(wind);

			softBody->setPose(true, true);

			softBody->setActivationState(DISABLE_DEACTIVATION);

			int id = dynamicsWorld->getCollisionObjectArray().size();
			lookup[entity] = id;
			((btSoftRigidDynamicsWorld*)dynamicsWorld)->addSoftBody(softBody);
			return id;
		}

		return -1;
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
		wiProfiler::GetInstance().BeginRange("Physics", wiProfiler::DOMAIN_CPU);

		btVector3 wind = btVector3(weather.windDirection.x, weather.windDirection.y, weather.windDirection.z);


		// Rigid bodies - Objects:
		for (size_t i = 0; i < rigidbodies.GetCount(); ++i)
		{
			Entity entity = rigidbodies.GetEntity(i);
			RigidBodyPhysicsComponent& rigidbody = rigidbodies[i];
			TransformComponent& transform = *transforms.GetComponent(entity);
			int id = -1;

			auto it = lookup.find(entity);
			if (it == lookup.end())
			{
				const ObjectComponent& object = *objects.GetComponent(entity);
				const MeshComponent& mesh = *meshes.GetComponent(object.meshID);
				id = AddRigidBody(entity, rigidbody, mesh, transform);
			}
			else
			{
				id = it->second;
			}
			assert(id >= 0 && id <= dynamicsWorld->getCollisionObjectArray().size());

			btCollisionObject*	obj = dynamicsWorld->getCollisionObjectArray()[id];
			btRigidBody*		body = btRigidBody::upcast(obj);

			if (body != nullptr)
			{
				btMotionState* motionState = body->getMotionState();
				btTransform physicsTransform;

				if (rigidbody.IsKinematic())
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


		}

		// Soft bodies - Meshes:
		for (size_t i = 0; i < softbodies.GetCount(); ++i)
		{
			Entity entity = softbodies.GetEntity(i);
			SoftBodyPhysicsComponent& softbody = softbodies[i];
			MeshComponent& mesh = *meshes.GetComponent(entity);
			mesh.SetDynamic(true); 
			int id = -1;

			auto it = lookup.find(entity);
			if (it == lookup.end())
			{
				TransformComponent transform;
				id = AddSoftBody(entity, softbody, mesh, transform);
			}
			else
			{
				id = it->second;
			}
			assert(id >= 0 && id <= dynamicsWorld->getCollisionObjectArray().size());

			btCollisionObject*	obj = dynamicsWorld->getCollisionObjectArray()[id];
			btSoftBody* body = btSoftBody::upcast(obj);
			if (body != nullptr)
			{
				body->setWindVelocity(wind);
			}

		}

		// Tidy:
		for (auto it : lookup)
		{
			Entity entity = it.first;
			bool exists = rigidbodies.Contains(entity);
			if (!exists)
			{
				exists = softbodies.Contains(entity);
			}

			if (!exists)
			{
				Remove(entity);
				break;
			}
		}

		dynamicsWorld->stepSimulation(dt, 10);

		wiProfiler::GetInstance().EndRange(); // Physics
	}
}
