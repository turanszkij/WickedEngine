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

	virtual void setWind(const XMFLOAT3& wind) override;

	virtual void connectVerticesToSoftBody(wiSceneSystem::PhysicsComponent& physicscomponent, wiSceneSystem::MeshComponent& mesh) override;
	virtual void connectSoftBodyToVertices(wiSceneSystem::PhysicsComponent& physicscomponent, wiSceneSystem::MeshComponent& mesh) override;
	virtual void transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI) override;

	virtual PhysicsTransform* getObject(int index) override;

	// add object to the simulation
	virtual void registerObject(wiSceneSystem::PhysicsComponent& physicscomponent, wiSceneSystem::MeshComponent& mesh, wiSceneSystem::TransformComponent& transform) override;
	// remove object from simulation
	virtual void removeObject(wiSceneSystem::PhysicsComponent& physicscomponent) override;

	virtual void Update(float dt) override;
	virtual void ClearWorld() override;
	virtual void CleanUp() override;

};

