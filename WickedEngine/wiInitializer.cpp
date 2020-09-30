#include "wiInitializer.h"
#include "WickedEngine.h"

#include <sstream>

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

		std::stringstream ss;
		ss << std::endl << "[wiInitializer] Initializing Wicked Engine, please wait..." << std::endl;
		ss << "Version: " << wiVersion::GetVersionString() << std::endl;
		wiBackLog::post(ss.str().c_str());

		wiJobSystem::Initialize();

		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiFont::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiImage::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiInput::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiRenderer::Initialize(); wiWidget::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiAudio::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiNetwork::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiTextureHelper::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiScene::wiHairParticle::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiScene::wiEmittedParticle::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiOcean::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiGPUSortLib::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiGPUBVH::Initialize(); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { wiPhysicsEngine::Initialize(); });

		// Initialize this immediately:
		wiLua::Initialize();

	}

	bool IsInitializeFinished()
	{
		return initializationStarted && !wiJobSystem::IsBusy(ctx);
	}
}