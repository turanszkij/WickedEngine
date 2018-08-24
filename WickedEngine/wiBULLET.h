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


	virtual void addSoftBodyTriangleMesh(wiSceneSystem::PhysicsComponent& physicscomponent) override;

	virtual void connectVerticesToSoftBody(wiSceneSystem::PhysicsComponent& physicscomponent) override;
	virtual void connectSoftBodyToVertices(wiSceneSystem::PhysicsComponent& physicscomponent) override;
	virtual void transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI) override;

	virtual PhysicsTransform* getObject(int index) override;

	// add object to the simulation
	virtual void registerObject(wiSceneSystem::PhysicsComponent& physicscomponent) override;
	// remove object from simulation
	virtual void removeObject(wiSceneSystem::PhysicsComponent& physicscomponent) override;

	virtual void Update(float dt) override;
	virtual void ClearWorld() override;
	virtual void CleanUp() override;

};

