#pragma once
#include "CommonInclude.h"
#include "wiSceneSystem_Decl.h"

#include <vector>

class PHYSICS
{
public:
	struct PhysicsTransform
	{
		XMFLOAT4 rotation;
		XMFLOAT3 position;
		PhysicsTransform() {
			rotation = XMFLOAT4(0, 0, 0, 1);
			position = XMFLOAT3(0, 0, 0);
		}
		PhysicsTransform(const XMFLOAT4& newRot, const XMFLOAT3& newPos) {
			rotation = newRot;
			position = newPos;
		}
	};
	static int softBodyIterationCount;
	static bool rigidBodyPhysicsEnabled, softBodyPhysicsEnabled;
protected:
	std::vector<PhysicsTransform*> transforms;
	bool firstRunWorld;
	int registeredObjects;
public:
	void FirstRunWorld() { firstRunWorld = true; }
	void NextRunWorld() { firstRunWorld = false; }
	int getObjectCount() { return registeredObjects + 1; }

	virtual void Update(float dt)=0;
	virtual void ClearWorld()=0;
	virtual void CleanUp()=0;

	virtual void setWind(const XMFLOAT3& wind)=0;
	
	virtual void connectVerticesToSoftBody(wiSceneSystem::SoftBodyPhysicsComponent& physicscomponent, wiSceneSystem::MeshComponent& mesh) = 0;
	virtual void connectSoftBodyToVertices(wiSceneSystem::SoftBodyPhysicsComponent& physicscomponent, wiSceneSystem::MeshComponent& mesh) = 0;
	virtual void transformBody(const XMFLOAT4& rot, const XMFLOAT3& pos, int objectI) = 0;

	virtual PhysicsTransform* getObject(int index)=0;

	virtual void addRigidBody(wiSceneSystem::RigidBodyPhysicsComponent& physicscomponent, wiSceneSystem::MeshComponent& mesh, wiSceneSystem::TransformComponent& transform) = 0;
	virtual void addSoftBody(wiSceneSystem::SoftBodyPhysicsComponent& physicscomponent, wiSceneSystem::MeshComponent& mesh, wiSceneSystem::TransformComponent& transform) = 0;
	
	virtual void removeRigidBody(wiSceneSystem::RigidBodyPhysicsComponent& physicscomponent) = 0;
	virtual void removeSoftBody(wiSceneSystem::SoftBodyPhysicsComponent& physicscomponent) = 0;
};
