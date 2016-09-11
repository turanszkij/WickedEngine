#include "wiBULLET.h"
#include "wiLoader.h"



#include "LinearMath/btHashMap.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#include "BulletSoftBody/btSoftBodySolvers.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

int PHYSICS::softBodyIterationCount=5;
bool PHYSICS::rigidBodyPhysicsEnabled = true, PHYSICS::softBodyPhysicsEnabled = true;
bool wiBULLET::grab=false;
RAY wiBULLET::grabRay=RAY();

wiBULLET::wiBULLET()
{
	registeredObjects=-1;

	///-----initialization_start-----
	//softBodySolver = new btDefaultSoftBodySolver();
	softBodySolver=NULL;

	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	//collisionConfiguration = new btDefaultCollisionConfiguration();
	collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	dispatcher = new	btCollisionDispatcher(collisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	overlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	solver = new btSequentialImpulseConstraintSolver;

	//dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
	dynamicsWorld = new btSoftRigidDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration,softBodySolver);
	

	dynamicsWorld->getSolverInfo().m_solverMode|=SOLVER_RANDMIZE_ORDER;
	dynamicsWorld->getDispatchInfo().m_enableSatConvex = true;
	dynamicsWorld->getSolverInfo().m_splitImpulse=true;

	dynamicsWorld->setGravity(btVector3(0,-11,0));
	((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().air_density			=	(btScalar)1.0;
	((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().water_density		=	0;
	((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().water_offset			=	0;
	((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().water_normal			=	btVector3(0,0,0);
	((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().m_gravity.setValue(0,-11,0);
	((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo().m_sparsesdf.Initialize(); //???

	dynamicsWorld->setInternalTickCallback(soundTickCallback,this,true);
	dynamicsWorld->setInternalTickCallback(pickingPreTickCallback,this,true);


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
	delete dynamicsWorld;

	//delete solver
	delete solver;

	//delete broadphase
	delete overlappingPairCache;

	//delete dispatcher
	delete dispatcher;

	delete collisionConfiguration;

	//next line is optional: it will be cleared by the destructor when the array goes out of scope
	collisionShapes.clear();

	///-----cleanup_end-----
	CleanUp();
}


void wiBULLET::addWind(const XMFLOAT3& wind){
	this->wind = btVector3(btScalar(wind.x),btScalar(wind.y),btScalar(wind.z));
}


void wiBULLET::addBox(const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
					, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic){

	btCollisionShape* shape = new btBoxShape(btVector3(sca.x,sca.y,sca.z));
	shape->setMargin(btScalar(0.05));
	collisionShapes.push_back(shape);

	btTransform shapeTransform;
	shapeTransform.setIdentity();
	shapeTransform.setOrigin(btVector3(pos.x,pos.y,pos.z));
	shapeTransform.setRotation(btQuaternion(rot.x,rot.y,rot.z,rot.w));
	{
		btScalar mass(newMass);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f && !kinematic);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);
		else 
			mass=0;

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		rbInfo.m_friction=newFriction;
		rbInfo.m_restitution=newRestitution;
		rbInfo.m_linearDamping = newDamping;
		rbInfo.m_angularDamping = newDamping;
		btRigidBody* body = new btRigidBody(rbInfo);
		if(kinematic) body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		body->setActivationState(DISABLE_DEACTIVATION);

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);

		
		if (body && body->getMotionState())
		{
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);
			btQuaternion nRot = trans.getRotation();
			btVector3 nPos = trans.getOrigin();
			transforms.push_back(new PhysicsTransform(
				XMFLOAT4(nRot.getX(),nRot.getY(),nRot.getZ(),nRot.getW()),XMFLOAT3(nPos.getX(),nPos.getY(),nPos.getZ()))
				);
		}
	}

	
}

void wiBULLET::addSphere(float rad, const XMFLOAT3& pos
					, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic){
	///create a few basic rigid bodies
	/*btCollisionShape* groundShape = new btshape(btVector3(btScalar(50.),btScalar(50.),btScalar(50.)));

	collisionShapes.push_back(groundShape);

	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0,-56,0));

	{
		btScalar mass(0.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,groundShape,localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
	}


	{
		//create a dynamic rigidbody

		//btCollisionShape* colShape = new btshape(btVector3(1,1,1));
		btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);

		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar	mass(1.f);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass,localInertia);

			startTransform.setOrigin(btVector3(2,10,0));
		
			//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
			btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,colShape,localInertia);
			btRigidBody* body = new btRigidBody(rbInfo);

			dynamicsWorld->addRigidBody(body);
	}*/

	btCollisionShape* shape = new btSphereShape(btScalar(rad));
	shape->setMargin(btScalar(0.05));
	collisionShapes.push_back(shape);

	btTransform shapeTransform;
	shapeTransform.setIdentity();
	shapeTransform.setOrigin(btVector3(pos.x,pos.y,pos.z));
	{
		btScalar mass(newMass);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f && !kinematic);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);
		else 
			mass=0;

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		rbInfo.m_friction=newFriction;
		rbInfo.m_restitution=newRestitution;
		rbInfo.m_linearDamping=newDamping;
		rbInfo.m_angularDamping = newDamping;
		btRigidBody* body = new btRigidBody(rbInfo);
		if(kinematic) body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		body->setActivationState(DISABLE_DEACTIVATION);

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);

		
		if (body && body->getMotionState())
		{
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);
			btQuaternion nRot = trans.getRotation();
			btVector3 nPos = trans.getOrigin();
			transforms.push_back(new PhysicsTransform(
				XMFLOAT4(nRot.getX(),nRot.getY(),nRot.getZ(),nRot.getW()),XMFLOAT3(nPos.getX(),nPos.getY(),nPos.getZ()))
				);
		}
	}

	
}

void wiBULLET::addCapsule(float rad, float hei, const XMFLOAT4& rot, const XMFLOAT3& pos
					, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic){

	btCollisionShape* shape = new btCapsuleShape(btScalar(rad),btScalar(hei));
	shape->setMargin(btScalar(0.05));
	collisionShapes.push_back(shape);

	btTransform shapeTransform;
	shapeTransform.setIdentity();
	shapeTransform.setOrigin(btVector3(pos.x,pos.y,pos.z));
	shapeTransform.setRotation(btQuaternion(rot.x,rot.y,rot.z,rot.w));
	{
		btScalar mass(newMass);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f && !kinematic);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);
		else 
			mass=0;

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		rbInfo.m_friction=newFriction;
		rbInfo.m_restitution=newRestitution;
		rbInfo.m_linearDamping = newDamping;
		rbInfo.m_angularDamping = newDamping;
		btRigidBody* body = new btRigidBody(rbInfo);
		if(kinematic) body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		body->setActivationState(DISABLE_DEACTIVATION);

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);

		
		if (body && body->getMotionState())
		{
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);
			btQuaternion nRot = trans.getRotation();
			btVector3 nPos = trans.getOrigin();
			transforms.push_back(new PhysicsTransform(
				XMFLOAT4(nRot.getX(),nRot.getY(),nRot.getZ(),nRot.getW()),XMFLOAT3(nPos.getX(),nPos.getY(),nPos.getZ()))
				);
		}
	}

	
}

void wiBULLET::addConvexHull(const vector<SkinnedVertex>& vertices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
					, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic){
	btCollisionShape* shape = new btConvexHullShape();
	for (unsigned int i = 0; i<vertices.size(); ++i)
		((btConvexHullShape*)shape)->addPoint(btVector3(vertices[i].pos.x,vertices[i].pos.y,vertices[i].pos.z));
	shape->setLocalScaling(btVector3(sca.x,sca.y,sca.z));
	shape->setMargin(btScalar(0.05));

	collisionShapes.push_back(shape);

	btTransform shapeTransform;
	shapeTransform.setIdentity();
	shapeTransform.setOrigin(btVector3(pos.x,pos.y,pos.z));
	shapeTransform.setRotation(btQuaternion(rot.x,rot.y,rot.z,rot.w));
	{
		btScalar mass(newMass);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f && !kinematic);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);
		else
			mass=0;

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		rbInfo.m_friction=newFriction;
		rbInfo.m_restitution=newRestitution;
		rbInfo.m_linearDamping = newDamping;
		rbInfo.m_angularDamping = newDamping;
		btRigidBody* body = new btRigidBody(rbInfo);
		if(kinematic) {
			body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}
		body->setActivationState(DISABLE_DEACTIVATION);

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);

		
		if (body && body->getMotionState())
		{
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);
			btQuaternion nRot = trans.getRotation();
			btVector3 nPos = trans.getOrigin();
			transforms.push_back(new PhysicsTransform(
				XMFLOAT4(nRot.getX(),nRot.getY(),nRot.getZ(),nRot.getW()),XMFLOAT3(nPos.getX(),nPos.getY(),nPos.getZ()))
				);
		}
	}

	
}

void wiBULLET::addTriangleMesh(const vector<SkinnedVertex>& vertices, const vector<unsigned int>& indices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
					, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic){
	
	int totalVerts = (int)vertices.size();
	int totalTriangles = (int)indices.size() / 3;

	btVector3* btVerts = new btVector3[totalVerts];
	for (unsigned int i = 0; i<vertices.size(); ++i)
		btVerts[i] = (btVector3(vertices[i].pos.x,vertices[i].pos.y,vertices[i].pos.z));

	int* btInd = new int[indices.size()];
	for (unsigned int i = 0; i<indices.size(); ++i)
		btInd[i] = indices[i];
	
	int vertStride = sizeof(btVector3);
	int indexStride = 3*sizeof(int);

	btTriangleIndexVertexArray* indexVertexArrays = new btTriangleIndexVertexArray(
		totalTriangles,
		btInd,
		indexStride,
		totalVerts,
		(btScalar*) &btVerts[0].x(),
		vertStride
		);

	bool useQuantizedAabbCompression = true;
						
	btCollisionShape* shape = new btBvhTriangleMeshShape(indexVertexArrays,useQuantizedAabbCompression);
	shape->setMargin(btScalar(0.05));
	shape->setLocalScaling(btVector3(sca.x,sca.y,sca.z));

	collisionShapes.push_back(shape);

	btTransform shapeTransform;
	shapeTransform.setIdentity();
	shapeTransform.setOrigin(btVector3(pos.x,pos.y,pos.z));
	shapeTransform.setRotation(btQuaternion(rot.x,rot.y,rot.z,rot.w));
	{
		btScalar mass(newMass);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f && !kinematic);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);
		else 
			mass=0;

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(shapeTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
		rbInfo.m_friction=newFriction;
		rbInfo.m_restitution=newRestitution;
		rbInfo.m_linearDamping = newDamping;
		rbInfo.m_angularDamping = newDamping;
		btRigidBody* body = new btRigidBody(rbInfo);
		if(kinematic) body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		body->setActivationState( DISABLE_DEACTIVATION );

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);

		
		if (body && body->getMotionState())
		{
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);
			btQuaternion nRot = trans.getRotation();
			btVector3 nPos = trans.getOrigin();
			transforms.push_back(new PhysicsTransform(
				XMFLOAT4(nRot.getX(),nRot.getY(),nRot.getZ(),nRot.getW()),XMFLOAT3(nPos.getX(),nPos.getY(),nPos.getZ()))
				);
		}
	}

	
}


void wiBULLET::addSoftBodyTriangleMesh(const Mesh* mesh, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
	, float newMass, float newFriction, float newRestitution, float newDamping){

	const int vCount = (int)mesh->physicsverts.size();
	btScalar* btVerts = new btScalar[vCount*3];
	for(int i=0;i<vCount*3;i+=3){
		const int vindex = i/3;
		btVerts[i] = btScalar(mesh->physicsverts[vindex].x);
		btVerts[i+1] = btScalar(mesh->physicsverts[vindex].y);
		btVerts[i+2] = btScalar(mesh->physicsverts[vindex].z);
	}
	const int iCount = (int)mesh->physicsindices.size();
	const int tCount = iCount/3;
	int* btInd = new int[iCount];
	for(int i=0;i<iCount;++i){
		btInd[i] = mesh->physicsindices[i];
	}

	
	btSoftBody* softBody = btSoftBodyHelpers::CreateFromTriMesh(
			((btSoftRigidDynamicsWorld*)dynamicsWorld)->getWorldInfo()
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
		shapeTransform.setOrigin(btVector3(pos.x,pos.y,pos.z));
		shapeTransform.setRotation(btQuaternion(rot.x,rot.y,rot.z,rot.w));
		softBody->scale(btVector3(sca.x,sca.y,sca.z));
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
		

		btScalar mass = btScalar(newMass);
		softBody->setTotalMass(mass);
		
		int mvg = mesh->massVG;
		if(mvg>=0){
			for(map<int,float>::const_iterator it=mesh->vertexGroups[mvg].vertices.begin();it!=mesh->vertexGroups[mvg].vertices.end();++it){
				int vi = (*it).first;
				float wei = (*it).second;
				int index=mesh->physicalmapGP[vi];
				softBody->setMass(index,softBody->getMass(index)*btScalar(wei));
			}
		}

		
		int gvg = mesh->goalVG;
		if(gvg>=0){
			for(map<int,float>::const_iterator it=mesh->vertexGroups[gvg].vertices.begin();it!=mesh->vertexGroups[gvg].vertices.end();++it){
				int vi = (*it).first;
				int index=mesh->physicalmapGP[vi];
				float weight = (*it).second;
				if(weight==1)
					softBody->setMass(index,0);
			}
		}

		softBody->getCollisionShape()->setMargin(btScalar(0.2));

		softBody->setWindVelocity(wind);
		
		softBody->setPose(true,true);

		softBody->setActivationState(DISABLE_DEACTIVATION);

		((btSoftRigidDynamicsWorld*)dynamicsWorld)->addSoftBody(softBody);

		transforms.push_back(new PhysicsTransform);
	}

}


void wiBULLET::connectVerticesToSoftBody(Mesh* const mesh, int objectI){
	if (!softBodyPhysicsEnabled)
		return;

	btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[objectI];
	btSoftBody* softBody = btSoftBody::upcast(obj);


	if(softBody){
		btVector3 min, max;
		softBody->getAabb(min, max);
		mesh->aabb.create(XMFLOAT3(min.x(), min.y(), min.z()), XMFLOAT3(max.x(), max.y(), max.z()));

		softBody->setWindVelocity(wind);

		btSoftBody::tNodeArray&   nodes(softBody->m_nodes);
		
		int gvg = mesh->goalVG;
		for (unsigned int i = 0; i<mesh->vertices_Complete.size(); ++i)
		{
			int indexP = mesh->physicalmapGP[i];
			float weight = mesh->vertexGroups[gvg].vertices[indexP];
			mesh->vertices_Complete[i].pre=mesh->vertices_Complete[i].pos;
			mesh->vertices_Complete[i].pos = XMFLOAT4(nodes[indexP].m_x.getX(), nodes[indexP].m_x.getY(), nodes[indexP].m_x.getZ(), 1);
			mesh->vertices_Complete[i].nor.x = -nodes[indexP].m_n.getX();
			mesh->vertices_Complete[i].nor.y = -nodes[indexP].m_n.getY();
			mesh->vertices_Complete[i].nor.z = -nodes[indexP].m_n.getZ();
			mesh->vertices_Complete[i].tex=mesh->vertices[i].tex;
		}
	}
}
void wiBULLET::connectSoftBodyToVertices(const Mesh* const mesh, int objectI){
	if (!softBodyPhysicsEnabled)
		return;

	if(!firstRunWorld){
		btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[objectI];
		btSoftBody* softBody = btSoftBody::upcast(obj);

		if(softBody){
			btSoftBody::tNodeArray&   nodes(softBody->m_nodes);
		
			int gvg = mesh->goalVG;
			if(gvg>=0){
				int j=0;
				for(map<int,float>::const_iterator it=mesh->vertexGroups[gvg].vertices.begin();it!=mesh->vertexGroups[gvg].vertices.end();++it){
					int vi = (*it).first;
					int index=mesh->physicalmapGP[vi];
					float weight = (*it).second;
					nodes[index].m_x=nodes[index].m_x.lerp(btVector3(mesh->goalPositions[j].x,mesh->goalPositions[j].y,mesh->goalPositions[j].z),weight);
					++j;
				}
			}
		}
	}
}
void wiBULLET::transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI){
	btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[objectI];
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
	
	btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[index];
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

void wiBULLET::registerObject(Object* object){
	if(object->rigidBody && rigidBodyPhysicsEnabled){
		//XMVECTOR s,r,t;
		//XMMatrixDecompose(&s,&r,&t,XMLoadFloat4x4(&object->world));
		XMFLOAT3 S,T;
		XMFLOAT4 R;
		//XMStoreFloat3(&S,s);
		//XMStoreFloat4(&R,r);
		//XMStoreFloat3(&T,t);
		object->applyTransform();
		object->attachTo(object->GetRoot());

		S = object->scale;
		T = object->translation;
		R = object->rotation;

		if(!object->collisionShape.compare("BOX")){
			addBox(
				S,R,T
				,object->mass,object->friction,object->restitution
				,object->damping,object->kinematic
			);
			object->physicsObjectI=++registeredObjects;
		}
		if(!object->collisionShape.compare("SPHERE")){
			addSphere(
				S.x,T
				,object->mass,object->friction,object->restitution
				,object->damping,object->kinematic
			);
			object->physicsObjectI=++registeredObjects;
		}
		if(!object->collisionShape.compare("CAPSULE")){
			addCapsule(
				S.x,S.y,R,T
				,object->mass,object->friction,object->restitution
				,object->damping,object->kinematic
			);
			object->physicsObjectI=++registeredObjects;
		}
		if(!object->collisionShape.compare("CONVEX_HULL")){
			addConvexHull(
				object->mesh->vertices,
				S,R,T
				,object->mass,object->friction,object->restitution
				,object->damping,object->kinematic
			);
			object->physicsObjectI=++registeredObjects;
		}
		if(!object->collisionShape.compare("MESH")){
			addTriangleMesh(
				object->mesh->vertices,object->mesh->indices,
				S,R,T
				,object->mass,object->friction,object->restitution
				,object->damping,object->kinematic
			);
			object->physicsObjectI=++registeredObjects;
		}
	}

	if(object->mesh->softBody && softBodyPhysicsEnabled){
		XMFLOAT3 s,t;
		XMFLOAT4 r;
		if(object->mesh->hasArmature()){
			s=object->mesh->armature->scale;
			r=object->mesh->armature->rotation;
			t=object->mesh->armature->translation;
		}
		else{
			s=object->scale;
			r=object->rotation;
			t=object->translation;
		}
		addSoftBodyTriangleMesh(
			object->mesh
			,s,r,t
			,object->mass,object->mesh->friction,object->restitution,object->damping
		);
		object->physicsObjectI=++registeredObjects;
	}
}

void wiBULLET::Update(float dt){
	if (rigidBodyPhysicsEnabled || softBodyPhysicsEnabled)
		dynamicsWorld->stepSimulation(dt, 6);
}
void wiBULLET::MarkForRead(){

}
void wiBULLET::UnMarkForRead(){

}
void wiBULLET::MarkForWrite(){

}
void wiBULLET::UnMarkForWrite(){

}
void wiBULLET::ClearWorld(){
	for(int i=dynamicsWorld->getNumCollisionObjects()-1;i>=0;i--)
	{
		btCollisionObject*	obj=dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody*		body=btRigidBody::upcast(obj);
		if(body&&body->getMotionState())
		{
			delete body->getMotionState();
		}
		while(dynamicsWorld->getNumConstraints())
		{
			btTypedConstraint*	pc=dynamicsWorld->getConstraint(0);
			dynamicsWorld->removeConstraint(pc);
			delete pc;
		}
		btSoftBody* softBody = btSoftBody::upcast(obj);
		if (softBody)
		{
			((btSoftRigidDynamicsWorld*)dynamicsWorld)->removeSoftBody(softBody);
		} else
		{
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body)
				dynamicsWorld->removeRigidBody(body);
			else
				dynamicsWorld->removeCollisionObject(obj);
		}
		delete obj;
	}

	//delete collision shapes
	for (int j=0;j<collisionShapes.size();j++)
	{
		btCollisionShape* shape = collisionShapes[j];
		collisionShapes[j] = 0;
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



void wiBULLET::soundTickCallback(btDynamicsWorld *world, btScalar timeStep) {
	int numManifolds = world->getDispatcher()->getNumManifolds();
    for (int i=0;i<numManifolds;i++)
    {

        btPersistentManifold* contactManifold =  world->getDispatcher()->getManifoldByIndexInternal(i);
        btCollisionObject* obA = (btCollisionObject*)(contactManifold->getBody0());
        btCollisionObject* obB = (btCollisionObject*)(contactManifold->getBody1());

        int numContacts = contactManifold->getNumContacts();
        for (int j=0;j<numContacts;j++)
        {
            btManifoldPoint& pt = contactManifold->getContactPoint(j);
            if (pt.getDistance()<0.f)
            {
				if(pt.getAppliedImpulse()>10.f){
					//TODO: play some sound
				}
                const btVector3& ptA = pt.getPositionWorldOnA();
                const btVector3& ptB = pt.getPositionWorldOnB();
                const btVector3& normalOnB = pt.m_normalWorldOnB;
            }
        }
    }
}

void wiBULLET::pickingPreTickCallback (btDynamicsWorld *world, btScalar timeStep)
{
//	world->getWorldUserInfo();
//
//	if(grab)
//	{
//		/*const int				x=softDemo->m_lastmousepos[0];
//		const int				y=softDemo->m_lastmousepos[1];
//		const btVector3			rayFrom=softDemo->getCameraPosition();
//		const btVector3			rayTo=softDemo->getRayTo(x,y);
//		const btVector3			rayDir=(rayTo-rayFrom).normalized();
//		const btVector3			N=(softDemo->getCameraTargetPosition()-softDemo->getCameraPosition()).normalized();
//		const btScalar			O=btDot(softDemo->m_impact,N);*/
//		const btVector3			rayFrom=btVector3(grabRay.origin.x,grabRay.origin.y,grabRay.origin.z);
//		const btVector3			rayDir=btVector3(grabRay.direction.x,grabRay.direction.y,grabRay.direction.z);
//		//const btScalar			den=btDot(N,rayDir);
//		//if((den*den)>0)
///*		{
//			const btScalar			num=O-btDot(N,rayFrom);
//			const btScalar			hit=num/den;
//			if((hit>0)&&(hit<1500))
//			{				
//				softDemo->m_goal=rayFrom+rayDir*hit;
//			}				
//		}*/		
//		btVector3				delta=softDemo->m_goal-softDemo->m_node->m_x;
//		static const btScalar	maxdrag=10;
//		if(delta.length2()>(maxdrag*maxdrag))
//		{
//			delta=delta.normalized()*maxdrag;
//		}
//		softDemo->m_node->m_v+=delta/timeStep;
//	}
//
}
void wiBULLET::setGrab(bool value, const RAY& ray){
	grab=value;
	grabRay=ray;
}
