#pragma once
#include "wiPHYSICS.h"

#include "BULLET/btBulletDynamicsCommon.h"

class btCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDynamicsWorld;
class btCollisionShape;
class btSoftBodySolver;
class btSoftBodySolverOutput;

struct RAY;

class wiBULLET:public PHYSICS
{
private:
	btCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDynamicsWorld* dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*> collisionShapes;

	btSoftBodySolver* softBodySolver;
	btSoftBodySolverOutput* softBodySolverOutput;

	btVector3 wind;

	static bool grab;
	static RAY grabRay;
public:
	wiBULLET();
	~wiBULLET();

	void addWind(const XMFLOAT3& wind);

	void addBox(const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic);
	void addSphere(float rad, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic);
	void addCapsule(float rad, float hei, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic);
	void addConvexHull(const vector<SkinnedVertex>& vertices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic);
	void addTriangleMesh(const vector<SkinnedVertex>& vertices, const vector<unsigned int>& indices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic);
	
	void addSoftBodyTriangleMesh(const Mesh* mesh, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping);

	void addBone(float rad, float hei);
	
	void connectVerticesToSoftBody(Mesh* const mesh, int objectI);
	void connectSoftBodyToVertices(const Mesh*  const mesh, int objectI);
	void transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI);

	PhysicsTransform* getObject(int index);

	void registerObject(Object* object);

	void Update();
	void MarkForRead();
	void UnMarkForRead();
	void MarkForWrite();
	void UnMarkForWrite();
	void ClearWorld();
	void CleanUp();

	void setGrab(bool val, const RAY& ray);
	static void pickingPreTickCallback (btDynamicsWorld *world, btScalar timeStep);
	static void soundTickCallback(btDynamicsWorld *world, btScalar timeStep);

	ALIGN_16
};

