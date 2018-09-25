#pragma once
#include "wiECS.h"
#include "wiSceneSystem_Decl.h"

namespace wiPhysicsEngine
{
	void Initialize();
	void CleanUp();

	void RunPhysicsUpdateSystem(
		const wiSceneSystem::WeatherComponent& weather,
		wiECS::ComponentManager<wiSceneSystem::TransformComponent>& transforms,
		wiECS::ComponentManager<wiSceneSystem::MeshComponent>& meshes,
		wiECS::ComponentManager<wiSceneSystem::ObjectComponent>& objects,
		wiECS::ComponentManager<wiSceneSystem::RigidBodyPhysicsComponent>& rigidbodies,
		wiECS::ComponentManager<wiSceneSystem::SoftBodyPhysicsComponent>& softbodies,
		float dt
	);
}
