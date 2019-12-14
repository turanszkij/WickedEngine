#pragma once
#include "wiECS.h"
#include "wiScene_Decl.h"
#include "wiJobSystem.h"

namespace wiPhysicsEngine
{
	void Initialize();

	bool IsEnabled();
	void SetEnabled(bool value);

	void RunPhysicsUpdateSystem(
		wiJobSystem::context& ctx,
		const wiScene::WeatherComponent& weather,
		const wiECS::ComponentManager<wiScene::ArmatureComponent>& armatures,
		wiECS::ComponentManager<wiScene::TransformComponent>& transforms,
		wiECS::ComponentManager<wiScene::MeshComponent>& meshes,
		wiECS::ComponentManager<wiScene::ObjectComponent>& objects,
		wiECS::ComponentManager<wiScene::RigidBodyPhysicsComponent>& rigidbodies,
		wiECS::ComponentManager<wiScene::SoftBodyPhysicsComponent>& softbodies,
		float dt
	);
}
