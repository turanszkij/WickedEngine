#include "wiBULLET.h"
#include "wiSceneSystem.h"
#include "wiRenderer.h"

#include "btBulletDynamicsCommon.h"

#include "LinearMath/btHashMap.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "BulletSoftBody/btSoftBodySolvers.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

using namespace std;
using namespace wiSceneSystem;
using namespace wiECS;

int PHYSICS::softBodyIterationCount=5;
bool PHYSICS::rigidBodyPhysicsEnabled = true, PHYSICS::softBodyPhysicsEnabled = true;

struct BulletPhysicsWorld
{
	btCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDynamicsWorld* dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*> collisionShapes;

	btSoftBodySolver* softBodySolver;
	btSoftBodySolverOutput* softBodySolverOutput;

	btVector3 wind;
};

wiBULLET::wiBULLET()
{
	bulletPhysics = new BulletPhysicsWorld;

	registeredObjects=-1;

	///-----initialization_start-----
	//softBodySolver = new btDefaultSoftBodySolver();
	bulletPhysics->softBodySolver = nullptr;

	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	//collisionConfiguration = new btDefaultCollisionConfiguration();
	bulletPhysics->collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	bulletPhysics->dispatcher = new	btCollisionDispatcher(bulletPhysics->collisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	bulletPhysics->overlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	bulletPhysics->solver = new btSequentialImpulseConstraintSolver;

	//dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
	bulletPhysics->dynamicsWorld = new btSoftRigidDynamicsWorld(bulletPhysics->dispatcher, bulletPhysics->overlappingPairCache, bulletPhysics->solver, bulletPhysics->collisionConfiguration, bulletPhysics->softBodySolver);
	

	bulletPhysics->dynamicsWorld->getSolverInfo().m_solverMode|=SOLVER_RANDMIZE_ORDER;
	bulletPhysics->dynamicsWorld->getDispatchInfo().m_enableSatConvex = true;
	bulletPhysics->dynamicsWorld->getSolverInfo().m_splitImpulse=true;

	bulletPhysics->dynamicsWorld->setGravity(btVector3(0,-11,0));
	((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->getWorldInfo().air_density			=	(btScalar)1.0;
	((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->getWorldInfo().water_density		=	0;
	((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->getWorldInfo().water_offset			=	0;
	((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->getWorldInfo().water_normal			=	btVector3(0,0,0);
	((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->getWorldInfo().m_gravity.setValue(0,-11,0);
	((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->getWorldInfo().m_sparsesdf.Initialize(); //???

	//dynamicsWorld->setInternalTickCallback(soundTickCallback,this,true);
	//dynamicsWorld->setInternalTickCallback(pickingPreTickCallback,this,true);


#ifdef BACKLOG
	wiBackLog::post("wiBULLET physics Initialized");
#endif

	///-----initialization_end-----
}

wiBULLET::~wiBULLET()
{
	//cleanup in the reverse order of creation/initialization
	
	///-----cleanup_start-----

	ClearWorld();

	//delete dynamics world
	delete bulletPhysics->dynamicsWorld;

	//delete solver
	delete bulletPhysics->solver;

	//delete broadphase
	delete bulletPhysics->overlappingPairCache;

	//delete dispatcher
	delete bulletPhysics->dispatcher;

	delete bulletPhysics->collisionConfiguration;

	//next line is optional: it will be cleared by the destructor when the array goes out of scope
	bulletPhysics->collisionShapes.clear();

	///-----cleanup_end-----
	CleanUp();

	delete bulletPhysics;
}


void wiBULLET::setWind(const XMFLOAT3& wind){
	this->bulletPhysics->wind = btVector3(btScalar(wind.x),btScalar(wind.y),btScalar(wind.z));
}


void wiBULLET::connectVerticesToSoftBody(PhysicsComponent& physicscomponent)
{
	if (!softBodyPhysicsEnabled)
		return;

	btCollisionObject* obj = bulletPhysics->dynamicsWorld->getCollisionObjectArray()[physicscomponent.physicsObjectID];
	btSoftBody* softBody = btSoftBody::upcast(obj);

	MeshComponent& mesh = wiRenderer::GetScene().meshes.GetComponent(physicscomponent.mesh_ref);

	if(softBody){
		btVector3 min, max;
		softBody->getAabb(min, max);
		mesh.aabb.create(XMFLOAT3(min.x(), min.y(), min.z()), XMFLOAT3(max.x(), max.y(), max.z()));

		softBody->setWindVelocity(bulletPhysics->wind);

		btSoftBody::tNodeArray&   nodes(softBody->m_nodes);
		
		int gvg = physicscomponent.goalVG;
		for (unsigned int i = 0; i<mesh.vertices_POS.size(); ++i)
		{
			int indexP = physicscomponent.physicalmapGP[i];
			float weight = mesh.vertexGroups[gvg].vertex_weights[indexP];

			MeshComponent::Vertex_POS& vert = mesh.vertices_Transformed_POS[i];

			mesh.vertices_Transformed_PRE[i] = vert;
			vert.pos.x = nodes[indexP].m_x.getX();
			vert.pos.y = nodes[indexP].m_x.getY();
			vert.pos.z = nodes[indexP].m_x.getZ();
			mesh.vertices_Transformed_POS[i].MakeFromParams(XMFLOAT3(-nodes[indexP].m_n.getX(), -nodes[indexP].m_n.getY(), -nodes[indexP].m_n.getZ())/*, vert.GetWind(), vert.GetMaterialIndex()*/);
		}
	}
}
void wiBULLET::connectSoftBodyToVertices(PhysicsComponent& physicscomponent){
	if (!softBodyPhysicsEnabled)
		return;

	if(!firstRunWorld){
		btCollisionObject* obj = bulletPhysics->dynamicsWorld->getCollisionObjectArray()[physicscomponent.physicsObjectID];
		btSoftBody* softBody = btSoftBody::upcast(obj);

		MeshComponent& mesh = wiRenderer::GetScene().meshes.GetComponent(physicscomponent.mesh_ref);

		if(softBody){
			btSoftBody::tNodeArray&   nodes(softBody->m_nodes);
		
			int gvg = physicscomponent.goalVG;
			if(gvg>=0){
				int j=0;
				for (auto it = mesh.vertexGroups[gvg].vertex_weights.begin(); it != mesh.vertexGroups[gvg].vertex_weights.end(); ++it) {
					int vi = (*it).first;
					int index = physicscomponent.physicalmapGP[vi];
					float weight = (*it).second;
					nodes[index].m_x = nodes[index].m_x.lerp(btVector3(physicscomponent.goalPositions[j].x, physicscomponent.goalPositions[j].y, physicscomponent.goalPositions[j].z), weight);
					++j;
				}
			}
		}
	}
}
void wiBULLET::transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI){
	if (objectI < 0)
	{
		return;
	}

	btCollisionObject* obj = bulletPhysics->dynamicsWorld->getCollisionObjectArray()[objectI];
	btRigidBody* rigidBody = btRigidBody::upcast(obj);
	if(rigidBody){
		btTransform transform;
		transform.setIdentity();
		transform.setRotation(btQuaternion(rot.x,rot.y,rot.z,rot.w));
		transform.setOrigin(btVector3(pos.x,pos.y,pos.z));
		rigidBody->getMotionState()->setWorldTransform(transform);
	}
}

PHYSICS::PhysicsTransform* wiBULLET::getObject(int index){
	
	btCollisionObject* obj = bulletPhysics->dynamicsWorld->getCollisionObjectArray()[index];
	btTransform trans;

	btRigidBody* body = btRigidBody::upcast(obj);
	if (body && body->getMotionState())
	{
		body->getMotionState()->getWorldTransform(trans);
	}

	btSoftBody* softBody = btSoftBody::upcast(obj);
	if(softBody)
	{
		trans = softBody->getWorldTransform();
	}
	
	btQuaternion rot = trans.getRotation();
	btVector3 pos = trans.getOrigin();
	transforms[index]->rotation=XMFLOAT4(rot.getX(),rot.getY(),rot.getZ(),rot.getW());
	transforms[index]->position=XMFLOAT3(pos.getX(),pos.getY(),pos.getZ());
	return transforms[index];
}

void wiBULLET::registerObject(PhysicsComponent& physicscomponent)
{
	MeshComponent& mesh = wiRenderer::GetScene().meshes.GetComponent(physicscomponent.mesh_ref);
	TransformComponent& transform = wiRenderer::GetScene().transforms.GetComponent(physicscomponent.transform_ref);

	btVector3 S = btVector3(transform.scale_local.x, transform.scale_local.y, transform.scale_local.z);
	btQuaternion R = btQuaternion(transform.rotation_local.x, transform.rotation_local.y, transform.rotation_local.z, transform.rotation_local.z);
	btVector3 T = btVector3(transform.translation_local.x, transform.translation_local.y, transform.translation_local.z);

	if(physicscomponent.rigidBody&& rigidBodyPhysicsEnabled){

		if(!physicscomponent.collisionShape.compare("BOX"))
		{
			btCollisionShape* shape = new btBoxShape(S);
			shape->setMargin(btScalar(0.05));
			bulletPhysics->collisionShapes.push_back(shape);

			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(T);
			shapeTransform.setRotation(R);
			{
				btScalar mass(physicscomponent.mass);

				//rigidbody is dynamic if and only if mass is non zero, otherwise static
				bool isDynamic = (mass != 0.f && !physicscomponent.kinematic);

				btVector3 localInertia(0, 0, 0);
				if (isDynamic)
					shape->calculateLocalInertia(mass, localInertia);
				else
					mass = 0;

				//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
				rbInfo.m_friction = physicscomponent.friction;
				rbInfo.m_restitution = physicscomponent.restitution;
				rbInfo.m_linearDamping = physicscomponent.damping;
				rbInfo.m_angularDamping = physicscomponent.damping;
				btRigidBody* body = new btRigidBody(rbInfo);
				if (physicscomponent.kinematic) 
					body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				body->setActivationState(DISABLE_DEACTIVATION);

				//add the body to the dynamics world
				bulletPhysics->dynamicsWorld->addRigidBody(body);


				if (body->getMotionState())
				{
					btTransform trans;
					body->getMotionState()->getWorldTransform(trans);
					btQuaternion nRot = trans.getRotation();
					btVector3 nPos = trans.getOrigin();
					transforms.push_back(new PhysicsTransform(
						XMFLOAT4(nRot.getX(), nRot.getY(), nRot.getZ(), nRot.getW()), XMFLOAT3(nPos.getX(), nPos.getY(), nPos.getZ()))
					);
				}
			}
		}
		if(!physicscomponent.collisionShape.compare("SPHERE"))
		{
			btCollisionShape* shape = new btSphereShape(btScalar(S.x()));
			shape->setMargin(btScalar(0.05));
			bulletPhysics->collisionShapes.push_back(shape);

			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(T);
			{
				btScalar mass(physicscomponent.mass);

				//rigidbody is dynamic if and only if mass is non zero, otherwise static
				bool isDynamic = (mass != 0.f && !physicscomponent.kinematic);

				btVector3 localInertia(0, 0, 0);
				if (isDynamic)
					shape->calculateLocalInertia(mass, localInertia);
				else
					mass = 0;

				//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
				rbInfo.m_friction = physicscomponent.friction;
				rbInfo.m_restitution = physicscomponent.restitution;
				rbInfo.m_linearDamping = physicscomponent.damping;
				rbInfo.m_angularDamping = physicscomponent.damping;
				btRigidBody* body = new btRigidBody(rbInfo);
				if (physicscomponent.kinematic)
					body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				body->setActivationState(DISABLE_DEACTIVATION);

				//add the body to the dynamics world
				bulletPhysics->dynamicsWorld->addRigidBody(body);


				if (body->getMotionState())
				{
					btTransform trans;
					body->getMotionState()->getWorldTransform(trans);
					btQuaternion nRot = trans.getRotation();
					btVector3 nPos = trans.getOrigin();
					transforms.push_back(new PhysicsTransform(
						XMFLOAT4(nRot.getX(), nRot.getY(), nRot.getZ(), nRot.getW()), XMFLOAT3(nPos.getX(), nPos.getY(), nPos.getZ()))
					);
				}
			}

		}
		if(!physicscomponent.collisionShape.compare("CAPSULE"))
		{
			btCollisionShape* shape = new btCapsuleShape(btScalar(S.x()), btScalar(S.y()));
			shape->setMargin(btScalar(0.05));
			bulletPhysics->collisionShapes.push_back(shape);

			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(T);
			shapeTransform.setRotation(R);
			{
				btScalar mass(physicscomponent.mass);

				//rigidbody is dynamic if and only if mass is non zero, otherwise static
				bool isDynamic = (mass != 0.f && !physicscomponent.kinematic);

				btVector3 localInertia(0, 0, 0);
				if (isDynamic)
					shape->calculateLocalInertia(mass, localInertia);
				else
					mass = 0;

				//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
				rbInfo.m_friction = physicscomponent.friction;
				rbInfo.m_restitution = physicscomponent.restitution;
				rbInfo.m_linearDamping = physicscomponent.damping;
				rbInfo.m_angularDamping = physicscomponent.damping;
				btRigidBody* body = new btRigidBody(rbInfo);
				if (physicscomponent.kinematic)
					body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				body->setActivationState(DISABLE_DEACTIVATION);

				//add the body to the dynamics world
				bulletPhysics->dynamicsWorld->addRigidBody(body);


				if (body->getMotionState())
				{
					btTransform trans;
					body->getMotionState()->getWorldTransform(trans);
					btQuaternion nRot = trans.getRotation();
					btVector3 nPos = trans.getOrigin();
					transforms.push_back(new PhysicsTransform(
						XMFLOAT4(nRot.getX(), nRot.getY(), nRot.getZ(), nRot.getW()), XMFLOAT3(nPos.getX(), nPos.getY(), nPos.getZ()))
					);
				}
			}
		}
		if(!physicscomponent.collisionShape.compare("CONVEX_HULL"))
		{
			btCollisionShape* shape = new btConvexHullShape();
			for (auto& x : mesh.vertices_POS)
			{
				((btConvexHullShape*)shape)->addPoint(btVector3(x.pos.x, x.pos.y, x.pos.z));
			}
			shape->setLocalScaling(S);
			shape->setMargin(btScalar(0.05));

			bulletPhysics->collisionShapes.push_back(shape);

			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(T);
			shapeTransform.setRotation(R);
			{
				btScalar mass(physicscomponent.mass);

				//rigidbody is dynamic if and only if mass is non zero, otherwise static
				bool isDynamic = (mass != 0.f && !physicscomponent.kinematic);

				btVector3 localInertia(0, 0, 0);
				if (isDynamic)
					shape->calculateLocalInertia(mass, localInertia);
				else
					mass = 0;

				//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
				rbInfo.m_friction = physicscomponent.friction;
				rbInfo.m_restitution = physicscomponent.restitution;
				rbInfo.m_linearDamping = physicscomponent.damping;
				rbInfo.m_angularDamping = physicscomponent.damping;
				btRigidBody* body = new btRigidBody(rbInfo);
				if (physicscomponent.kinematic)
					body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				body->setActivationState(DISABLE_DEACTIVATION);

				//add the body to the dynamics world
				bulletPhysics->dynamicsWorld->addRigidBody(body);


				if (body->getMotionState())
				{
					btTransform trans;
					body->getMotionState()->getWorldTransform(trans);
					btQuaternion nRot = trans.getRotation();
					btVector3 nPos = trans.getOrigin();
					transforms.push_back(new PhysicsTransform(
						XMFLOAT4(nRot.getX(), nRot.getY(), nRot.getZ(), nRot.getW()), XMFLOAT3(nPos.getX(), nPos.getY(), nPos.getZ()))
					);
				}
			}
		}
		if(!physicscomponent.collisionShape.compare("MESH"))
		{
			int totalVerts = (int)mesh.vertices_POS.size();
			int totalTriangles = (int)mesh.indices.size() / 3;

			btVector3* btVerts = new btVector3[totalVerts];
			size_t i = 0;
			for (auto& x : mesh.vertices_POS)
			{
				btVerts[i++] = btVector3(x.pos.x, x.pos.y, x.pos.z);
			}

			int* btInd = new int[mesh.indices.size()];
			for (unsigned int i = 0; i < mesh.indices.size(); ++i)
			{
				btInd[i] = mesh.indices[i];
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

			btCollisionShape* shape = new btBvhTriangleMeshShape(indexVertexArrays, useQuantizedAabbCompression);
			shape->setMargin(btScalar(0.05));
			shape->setLocalScaling(S);

			bulletPhysics->collisionShapes.push_back(shape);

			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(T);
			shapeTransform.setRotation(R);
			{
				btScalar mass(physicscomponent.mass);

				//rigidbody is dynamic if and only if mass is non zero, otherwise static
				bool isDynamic = (mass != 0.f && !physicscomponent.kinematic);

				btVector3 localInertia(0, 0, 0);
				if (isDynamic)
					shape->calculateLocalInertia(mass, localInertia);
				else
					mass = 0;

				//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
				rbInfo.m_friction = physicscomponent.friction;
				rbInfo.m_restitution = physicscomponent.restitution;
				rbInfo.m_linearDamping = physicscomponent.damping;
				rbInfo.m_angularDamping = physicscomponent.damping;
				btRigidBody* body = new btRigidBody(rbInfo);
				if (physicscomponent.kinematic) 
					body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				body->setActivationState(DISABLE_DEACTIVATION);

				//add the body to the dynamics world
				bulletPhysics->dynamicsWorld->addRigidBody(body);


				if (body->getMotionState())
				{
					btTransform trans;
					body->getMotionState()->getWorldTransform(trans);
					btQuaternion nRot = trans.getRotation();
					btVector3 nPos = trans.getOrigin();
					transforms.push_back(new PhysicsTransform(
						XMFLOAT4(nRot.getX(), nRot.getY(), nRot.getZ(), nRot.getW()), XMFLOAT3(nPos.getX(), nPos.getY(), nPos.getZ()))
					);
				}
			}
		}
	}

	if(physicscomponent.softBody && softBodyPhysicsEnabled)
	{
		const int vCount = (int)physicscomponent.physicsverts.size();
		btScalar* btVerts = new btScalar[vCount*3];
		for(int i=0;i<vCount*3;i+=3){
			const int vindex = i/3;
			btVerts[i] = btScalar(physicscomponent.physicsverts[vindex].x);
			btVerts[i+1] = btScalar(physicscomponent.physicsverts[vindex].y);
			btVerts[i+2] = btScalar(physicscomponent.physicsverts[vindex].z);
		}
		const int iCount = (int)physicscomponent.physicsindices.size();
		const int tCount = iCount/3;
		int* btInd = new int[iCount];
		for(int i=0;i<iCount;++i){
			btInd[i] = physicscomponent.physicsindices[i];
		}

		
		btSoftBody* softBody = btSoftBodyHelpers::CreateFromTriMesh(
				((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->getWorldInfo()
				,&btVerts[0]
				,&btInd[0]
				,tCount
				,false
		);

		
		delete[] btVerts;
		delete[] btInd;
		
		if(softBody){
			btSoftBody::Material* pm=softBody->appendMaterial();
			pm->m_kLST = 0.5;
			pm->m_kVST = 0.5;
			pm->m_kAST = 0.5;
			pm->m_flags = 0;
			softBody->generateBendingConstraints(2,pm);
			softBody->randomizeConstraints();
			
			btTransform shapeTransform;
			shapeTransform.setIdentity();
			shapeTransform.setOrigin(T);
			shapeTransform.setRotation(R);
			softBody->scale(S);
			softBody->transform(shapeTransform);


			softBody->m_cfg.piterations	=	softBodyIterationCount;
			softBody->m_cfg.aeromodel=btSoftBody::eAeroModel::F_TwoSidedLiftDrag;

			softBody->m_cfg.kAHR		=btScalar(.69); //0.69		Anchor hardness  [0,1]
			softBody->m_cfg.kCHR		=btScalar(1.0); //1			Rigid contact hardness  [0,1]
			softBody->m_cfg.kDF			=btScalar(0.2); //0.2		Dynamic friction coefficient  [0,1]
			softBody->m_cfg.kDG			=btScalar(0.01); //0			Drag coefficient  [0,+inf]
			softBody->m_cfg.kDP			=btScalar(0.0); //0			Damping coefficient  [0,1]
			softBody->m_cfg.kKHR		=btScalar(0.1); //0.1		Kinetic contact hardness  [0,1]
			softBody->m_cfg.kLF			=btScalar(0.1); //0			Lift coefficient  [0,+inf]
			softBody->m_cfg.kMT			=btScalar(0.0); //0			Pose matching coefficient  [0,1]
			softBody->m_cfg.kPR			=btScalar(0.0); //0			Pressure coefficient  [-1,1]
			softBody->m_cfg.kSHR		=btScalar(1.0); //1			Soft contacts hardness  [0,1]
			softBody->m_cfg.kVC			=btScalar(0.0); //0			Volume conseration coefficient  [0,+inf]
			softBody->m_cfg.kVCF		=btScalar(1.0); //1			Velocities correction factor (Baumgarte)

			softBody->m_cfg.kSKHR_CL	=btScalar(1.0); //1			Soft vs. kinetic hardness   [0,1]
			softBody->m_cfg.kSK_SPLT_CL	=btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]
			softBody->m_cfg.kSRHR_CL	=btScalar(0.1); //0.1		Soft vs. rigid hardness  [0,1]
			softBody->m_cfg.kSR_SPLT_CL	=btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]
			softBody->m_cfg.kSSHR_CL	=btScalar(0.5); //0.5		Soft vs. soft hardness  [0,1]
			softBody->m_cfg.kSS_SPLT_CL	=btScalar(0.5); //0.5		Soft vs. rigid impulse split  [0,1]
			

			btScalar mass = btScalar(physicscomponent.mass);
			softBody->setTotalMass(mass);
			
			int mvg = physicscomponent.massVG;
			if(mvg>=0){
				for(auto it=mesh.vertexGroups[mvg].vertex_weights.begin();it!=mesh.vertexGroups[mvg].vertex_weights.end();++it){
					int vi = (*it).first;
					float wei = (*it).second;
					int index= physicscomponent.physicalmapGP[vi];
					softBody->setMass(index,softBody->getMass(index)*btScalar(wei));
				}
			}

			
			int gvg = physicscomponent.goalVG;
			if(gvg>=0){
				for(auto it=mesh.vertexGroups[gvg].vertex_weights.begin();it!=mesh.vertexGroups[gvg].vertex_weights.end();++it){
					int vi = (*it).first;
					int index= physicscomponent.physicalmapGP[vi];
					float weight = (*it).second;
					if(weight==1)
						softBody->setMass(index,0);
				}
			}

			softBody->getCollisionShape()->setMargin(btScalar(0.2));

			softBody->setWindVelocity(bulletPhysics->wind);
			
			softBody->setPose(true,true);

			softBody->setActivationState(DISABLE_DEACTIVATION);

			((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->addSoftBody(softBody);

			transforms.push_back(new PhysicsTransform);
		}
	}

	physicscomponent.physicsObjectID = ++registeredObjects;
}
void wiBULLET::removeObject(PhysicsComponent& physicscomponent)
{

	if (physicscomponent.physicsObjectID < 0)
	{
		return;
	}

	deleteObject(physicscomponent.physicsObjectID);
	physicscomponent.physicsObjectID = -1;
}

void wiBULLET::Update(float dt){
	if (rigidBodyPhysicsEnabled || softBodyPhysicsEnabled)
		bulletPhysics->dynamicsWorld->stepSimulation(dt, 6);
}
void wiBULLET::ClearWorld(){
	for(int i= bulletPhysics->dynamicsWorld->getNumCollisionObjects()-1;i>=0;i--)
	{
		deleteObject(i);
	}

	//delete collision shapes
	for (int j=0;j<bulletPhysics->collisionShapes.size();j++)
	{
		btCollisionShape* shape = bulletPhysics->collisionShapes[j];
		bulletPhysics->collisionShapes[j] = 0;
		delete shape;
	}

	//delete transfom interface
	for (unsigned int i = 0; i<transforms.size(); ++i)
		delete transforms[i];
	transforms.clear();
	registeredObjects=-1;
}
void wiBULLET::CleanUp(){
	for (unsigned int i = 0; i<transforms.size(); ++i)
		delete transforms[i];
	transforms.clear();
}

void wiBULLET::deleteObject(int id)
{
	btCollisionObject*	obj = bulletPhysics->dynamicsWorld->getCollisionObjectArray()[id];
	btRigidBody*		body = btRigidBody::upcast(obj);

	if (body && body->getMotionState())
	{
		delete body->getMotionState();
	}
	while (bulletPhysics->dynamicsWorld->getNumConstraints())
	{
		btTypedConstraint*	pc = bulletPhysics->dynamicsWorld->getConstraint(0);
		bulletPhysics->dynamicsWorld->removeConstraint(pc);
		delete pc;
	}

	btSoftBody* softBody = btSoftBody::upcast(obj);
	if (softBody)
	{
		((btSoftRigidDynamicsWorld*)bulletPhysics->dynamicsWorld)->removeSoftBody(softBody);
	}
	else
	{
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body)
			bulletPhysics->dynamicsWorld->removeRigidBody(body);
		else
			bulletPhysics->dynamicsWorld->removeCollisionObject(obj);
	}
	delete obj;

	registeredObjects--;
}

//
//void wiBULLET::soundTickCallback(btDynamicsWorld *world, btScalar timeStep) {
//	int numManifolds = world->getDispatcher()->getNumManifolds();
//    for (int i=0;i<numManifolds;i++)
//    {
//
//        btPersistentManifold* contactManifold =  world->getDispatcher()->getManifoldByIndexInternal(i);
//        btCollisionObject* obA = (btCollisionObject*)(contactManifold->getBody0());
//        btCollisionObject* obB = (btCollisionObject*)(contactManifold->getBody1());
//
//        int numContacts = contactManifold->getNumContacts();
//        for (int j=0;j<numContacts;j++)
//        {
//            btManifoldPoint& pt = contactManifold->getContactPoint(j);
//            if (pt.getDistance()<0.f)
//            {
//				if(pt.getAppliedImpulse()>10.f){
//					//TODO: play some sound
//				}
//                const btVector3& ptA = pt.getPositionWorldOnA();
//                const btVector3& ptB = pt.getPositionWorldOnB();
//                const btVector3& normalOnB = pt.m_normalWorldOnB;
//            }
//        }
//    }
//}
//
//void wiBULLET::pickingPreTickCallback (btDynamicsWorld *world, btScalar timeStep)
//{
////	world->getWorldUserInfo();
////
////	if(grab)
////	{
////		/*const int				x=softDemo->m_lastmousepos[0];
////		const int				y=softDemo->m_lastmousepos[1];
////		const btVector3			rayFrom=softDemo->getCameraPosition();
////		const btVector3			rayTo=softDemo->getRayTo(x,y);
////		const btVector3			rayDir=(rayTo-rayFrom).normalized();
////		const btVector3			N=(softDemo->getCameraTargetPosition()-softDemo->getCameraPosition()).normalized();
////		const btScalar			O=btDot(softDemo->m_impact,N);*/
////		const btVector3			rayFrom=btVector3(grabRay.origin.x,grabRay.origin.y,grabRay.origin.z);
////		const btVector3			rayDir=btVector3(grabRay.direction.x,grabRay.direction.y,grabRay.direction.z);
////		//const btScalar			den=btDot(N,rayDir);
////		//if((den*den)>0)
/////*		{
////			const btScalar			num=O-btDot(N,rayFrom);
////			const btScalar			hit=num/den;
////			if((hit>0)&&(hit<1500))
////			{				
////				softDemo->m_goal=rayFrom+rayDir*hit;
////			}				
////		}*/		
////		btVector3				delta=softDemo->m_goal-softDemo->m_node->m_x;
////		static const btScalar	maxdrag=10;
////		if(delta.length2()>(maxdrag*maxdrag))
////		{
////			delta=delta.normalized()*maxdrag;
////		}
////		softDemo->m_node->m_v+=delta/timeStep;
////	}
////
//}
//void wiBULLET::setGrab(bool value, const RAY& ray){
//	grab=value;
//	grabRay=ray;
//}
