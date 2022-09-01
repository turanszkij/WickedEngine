#include "wiInitializer.h"
#include "WickedEngine.h"

#include <string>
#include <thread>
#include <atomic>

namespace wi::initializer
{
	bool initializationStarted = false;
	wi::jobsystem::context ctx;
	wi::Timer timer;
	static std::atomic_bool systems[INITIALIZED_SYSTEM_COUNT]{};

	void InitializeComponentsImmediate()
	{
		InitializeComponentsAsync();
		wi::jobsystem::Wait(ctx);
	}
	void InitializeComponentsAsync()
	{
		timer.record();

		initializationStarted = true;

		std::string ss;
		ss += "\n[wi::initializer] Initializing Wicked Engine, please wait...\n";
		ss += "Version: ";
		ss += wi::version::GetVersionString();
		wi::backlog::post(ss);

		size_t shaderdump_count = wi::renderer::GetShaderDumpCount();
		if (shaderdump_count > 0)
		{
			wi::backlog::post("Embedded shaders found: " + std::to_string(shaderdump_count));
		}

		wi::backlog::post("");
		wi::jobsystem::Initialize();

		wi::backlog::post("");
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::font::Initialize(); systems[INITIALIZED_SYSTEM_FONT].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::image::Initialize(); systems[INITIALIZED_SYSTEM_IMAGE].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::input::Initialize(); systems[INITIALIZED_SYSTEM_INPUT].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::renderer::Initialize(); systems[INITIALIZED_SYSTEM_RENDERER].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::texturehelper::Initialize(); systems[INITIALIZED_SYSTEM_TEXTUREHELPER].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::HairParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_HAIRPARTICLESYSTEM].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::EmittedParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_EMITTEDPARTICLESYSTEM].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::Ocean::Initialize(); systems[INITIALIZED_SYSTEM_OCEAN].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::gpusortlib::Initialize(); systems[INITIALIZED_SYSTEM_GPUSORTLIB].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::GPUBVH::Initialize(); systems[INITIALIZED_SYSTEM_GPUBVH].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::physics::Initialize(); systems[INITIALIZED_SYSTEM_PHYSICS].store(true); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::audio::Initialize(); systems[INITIALIZED_SYSTEM_AUDIO].store(true); });

		// Initialize this immediately:
		wi::lua::Initialize(); systems[INITIALIZED_SYSTEM_LUA].store(true);

		std::thread([] {
			wi::jobsystem::Wait(ctx);
			wi::backlog::post("\n[wi::initializer] Wicked Engine Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		}).detach();

	}

	bool IsInitializeFinished(INITIALIZED_SYSTEM system)
	{
		if (system == INITIALIZED_SYSTEM_COUNT)
		{
			return initializationStarted && !wi::jobsystem::IsBusy(ctx);
		}
		else
		{
			return systems[system].load();
		}
	}
}
