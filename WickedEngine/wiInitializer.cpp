#include "wiInitializer.h"
#include "WickedEngine.h"

#include <string>
#include <thread>

namespace wi::initializer
{
	bool initializationStarted = false;
	wi::jobsystem::context ctx;
	wi::Timer timer;

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
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::font::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::image::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::input::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::renderer::Initialize(); wi::gui::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::texturehelper::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::HairParticleSystem::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::EmittedParticleSystem::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::Ocean::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::gpusortlib::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::GPUBVH::Initialize(); });
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::physics::Initialize(); });

		// Initialize this immediately:
		wi::lua::Initialize();

		std::thread([] {
			wi::jobsystem::Wait(ctx);
			wi::backlog::post("\n[wi::initializer] Wicked Engine Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		}).detach();

	}

	bool IsInitializeFinished()
	{
		return initializationStarted && !wi::jobsystem::IsBusy(ctx);
	}
}
