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
		wiScene::Scene& scene,
		float dt
	);
}
