#pragma once
#include "wiECS.h"
#include "wiSceneSystem_Decl.h"

namespace wiPhysicsEngine
{
	void Initialize();
	void CleanUp();

	bool IsEnabled();
	void SetEnabled(bool value);

	void RunPhysicsUpdateSystem(
		const wiSceneSystem::WeatherComponent& weather,
		const wiECS::ComponentManager<wiSceneSystem::ArmatureComponent>& armatures,
		wiECS::ComponentManager<wiSceneSystem::TransformComponent>& transforms,
		wiECS::ComponentManager<wiSceneSystem::MeshComponent>& meshes,
		wiECS::ComponentManager<wiSceneSystem::ObjectComponent>& objects,
		wiECS::ComponentManager<wiSceneSystem::RigidBodyPhysicsComponent>& rigidbodies,
		wiECS::ComponentManager<wiSceneSystem::SoftBodyPhysicsComponent>& softbodies,
		float dt
	);
}
