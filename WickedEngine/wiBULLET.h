#pragma once
#include "wiPHYSICS.h"

struct BulletPhysicsWorld;

struct RAY;

class wiBULLET:public PHYSICS
{
private:
	BulletPhysicsWorld * bulletPhysics = nullptr;

	void deleteObject(int id);
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
	void addConvexHull(const std::vector<XMFLOAT4>& vertices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic);
	void addTriangleMesh(const std::vector<XMFLOAT4>& vertices, const std::vector<unsigned int>& indices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic);
	
	void addSoftBodyTriangleMesh(const wiSceneComponents::Mesh* mesh, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping);

	void addBone(float rad, float hei);
	
	void connectVerticesToSoftBody(wiSceneComponents::Mesh* const mesh, int objectI);
	void connectSoftBodyToVertices(const wiSceneComponents::Mesh*  const mesh, int objectI);
	void transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI);

	PhysicsTransform* getObject(int index);

	void registerObject(wiSceneComponents::Object* object);
	void removeObject(wiSceneComponents::Object* object);

	void Update(float dt);
	void MarkForRead();
	void UnMarkForRead();
	void MarkForWrite();
	void UnMarkForWrite();
	void ClearWorld();
	void CleanUp();

};

