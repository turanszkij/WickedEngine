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

		wiJobSystem::Execute(ctx, [] { wiFont::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiImage::Initialize(); });
		wiJobSystem::Execute(ctx, [] { wiInput::Initialize(); });
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

	void CleanUp()
	{
		wiFont::CleanUp();
		wiRenderer::CleanUp();
		wiAudio::CleanUp();
		wiNetwork::CleanUp();
		wiPhysicsEngine::CleanUp();
		wiInput::CleanUp();
	}
}