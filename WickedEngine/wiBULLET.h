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

	virtual void addWind(const XMFLOAT3& wind) override;

	virtual void addBox(const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic) override;
	virtual void addSphere(float rad, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic) override;
	virtual void addCapsule(float rad, float hei, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic) override;
	virtual void addConvexHull(const std::vector<XMFLOAT4>& vertices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic) override;
	virtual void addTriangleMesh(const std::vector<XMFLOAT4>& vertices, const std::vector<unsigned int>& indices, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass, float newFriction, float newRestitution, float newDamping, bool kinematic) override;


	virtual void addSoftBodyTriangleMesh(wiECS::ComponentManager<wiSceneSystem::Mesh>::ref mesh_ref, const XMFLOAT3& sca, const XMFLOAT4& rot, const XMFLOAT3& pos
		, float newMass = 1, float newFriction = 1, float newRestitution = 1, float newDamping = 1) override;

	virtual void connectVerticesToSoftBody(wiECS::ComponentManager<wiSceneSystem::Mesh>::ref mesh_ref, int objectI) override;
	virtual void connectSoftBodyToVertices(wiECS::ComponentManager<wiSceneSystem::Mesh>::ref mesh_ref, int objectI) override;
	virtual void transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI) override;

	virtual PhysicsTransform* getObject(int index) override;

	// add object to the simulation
	virtual void registerObject(wiECS::ComponentManager<wiSceneSystem::Object>::ref object_ref) override;
	// remove object from simulation
	virtual void removeObject(wiECS::ComponentManager<wiSceneSystem::Object>::ref object_ref) override;

	void Update(float dt);
	void MarkForRead();
	void UnMarkForRead();
	void MarkForWrite();
	void UnMarkForWrite();
	void ClearWorld();
	void CleanUp();

};

