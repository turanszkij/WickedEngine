#include "wiInitializer.h"
#include "WickedEngine.h"

#include <thread>
#include <sstream>

using namespace std;

namespace wiInitializer
{
	bool initializationStarted = false;
	wiJobSystem::context ctx;

	void InitializeComponentsImmediate()
	{
		InitializeComponentsAsync();
		wiJobSystem::Wait(ctx);
	}
	void InitializeComponentsAsync()
	{
		initializationStarted = true;

		wiBackLog::post("\n[wiInitializer] Initializing Wicked Engine, please wait...\n");

		wiJobSystem::Initialize();

		wiJobSystem::Execute(ctx, [] { wiFont::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiImage::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiInputManager::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiRenderer::Initialize(); wiWidget::LoadShaders(); });
		wiJobSystem::Execute(ctx, [] { wiAudio::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiNetwork::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiTextureHelper::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiSceneSystem::wiHairParticle::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiSceneSystem::wiEmittedParticle::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiOcean::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiGPUSortLib::LoadShaders(); });
		wiJobSystem::Execute(ctx, [] { wiGPUBVH::LoadShaders(); });
		wiJobSystem::Execute(ctx, [] { wiPhysicsEngine::Initialize(); });

	}

	bool IsInitializeFinished()
	{
		return initializationStarted && !wiJobSystem::IsBusy(ctx);
	}
}