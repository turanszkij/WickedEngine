#include "wiInitializer.h"
#include "WickedEngine.h"

#include <string>
#include <thread>

namespace wiInitializer
{
	bool initializationStarted = false;
	wiJobSystem::context ctx;
	wiTimer timer;

	void InitializeComponentsImmediate()
	{
		InitializeComponentsAsync();
		wiJobSystem::Wait(ctx);
	}
	void InitializeComponentsAsync()
	{
		timer.record();

		initializationStarted = true;

		std::string ss;
		ss += "\n[wiInitializer] Initializing Wicked Engine, please wait...\n";
		ss += "Version: ";
		ss += wiVersion::GetVersionString();
		ss += "\n";
		wiBackLog::post(ss.c_str());

		wiJobSystem::Initialize();
		wiShaderCompiler::Initialize();

		size_t shaderdump_count = wiRenderer::GetShaderDumpCount();
		if (shaderdump_count > 0)
		{
			wiBackLog::post(("Embedded shaders found: " + std::to_string(shaderdump_count)).c_str());
		}

		wiBackLog::post("");
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

		std::thread([] {
			wiJobSystem::Wait(ctx);
			wiBackLog::post("\n[wiInitializer] Wicked Engine Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		}).detach();

	}

	bool IsInitializeFinished()
	{
		return initializationStarted && !wiJobSystem::IsBusy(ctx);
	}
}
