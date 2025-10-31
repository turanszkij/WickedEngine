#include "wiInitializer.h"
#include "WickedEngine.h"

#include <thread>
#include <atomic>

#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#include "Utility/cpuinfo.hpp"
#endif // defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)

namespace wi::initializer
{
	static std::atomic_bool initializationStarted{ false };
	static wi::jobsystem::context ctx;
	static wi::Timer timer;
	static std::atomic_bool systems[INITIALIZED_SYSTEM_COUNT]{};

	void InitializeComponentsImmediate()
	{
		if (IsInitializeFinished())
			return;
		if (!initializationStarted.load())
		{
			InitializeComponentsAsync();
		}
		WaitForInitializationsToFinish();
	}
	void InitializeComponentsAsync()
	{
		if (IsInitializeFinished())
			return;
		timer.record();

		initializationStarted.store(true);

#if defined(PLATFORM_WINDOWS_DESKTOP)
		static constexpr const char* platform_string = "Windows";
#elif defined(PLATFORM_LINUX)
		static constexpr const char* platform_string = "Linux";
#elif defined(PLATFORM_MACOS)
		static constexpr const char* platform_string = "macOS";
#elif defined(PLATFORM_PS5)
		static constexpr const char* platform_string = "PS5";
#elif defined(PLATFORM_XBOX)
		static constexpr const char* platform_string = "Xbox";
#endif // PLATFORM

		wilog("\n[wi::initializer] Initializing Wicked Engine, please wait...\nVersion: %s\nPlatform: %s", wi::version::GetVersionString(), platform_string);

		StackString<1024> cpustring;
#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
		CPUInfo cpuinfo;
		cpustring.push_back("\nCPU: ");
		cpustring.push_back(cpuinfo.model().c_str());
		cpustring.push_back("\n\tFeatures available: ");
		if (cpuinfo.haveSSE())
		{
			cpustring.push_back("SSE; ");
		}
		if (cpuinfo.haveSSE2())
		{
			cpustring.push_back("SSE 2; ");
		}
		if (cpuinfo.haveSSE3())
		{
			cpustring.push_back("SSE 3; ");
		}
		if (cpuinfo.haveSSE41())
		{
			cpustring.push_back("SSE 4.1; ");
		}
		if (cpuinfo.haveSSE42())
		{
			cpustring.push_back("SSE 4.2; ");
		}
		if (cpuinfo.haveAVX())
		{
			cpustring.push_back("AVX; ");
		}
		if (cpuinfo.haveFMA3())
		{
			cpustring.push_back("FMA3; ");
		}
		if (cpuinfo.haveF16C())
		{
			cpustring.push_back("F16C; ");
		}
		if (cpuinfo.haveAVX2())
		{
			cpustring.push_back("AVX 2; ");
		}
		if (cpuinfo.haveAVX512F())
		{
			cpustring.push_back("AVX 512; ");
		}
#endif // defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)

// DirectXMath feature detection:
		cpustring.push_back("\n\tFeatures used: ");
#ifdef _XM_SSE_INTRINSICS_
		cpustring.push_back("SSE; ");
		cpustring.push_back("SSE 2; ");
#endif // _XM_SSE_INTRINSICS_
#ifdef _XM_SSE3_INTRINSICS_
		cpustring.push_back("SSE 3; ");
#endif // _XM_SSE3_INTRINSICS_
#ifdef _XM_SSE4_INTRINSICS_
		cpustring.push_back("SSE 4.1; ");
#endif // _XM_SSE4_INTRINSICS_
#ifdef _XM_AVX_INTRINSICS_
		cpustring.push_back("AVX; ");
#endif // _XM_AVX_INTRINSICS_
#ifdef _XM_FMA3_INTRINSICS_
		cpustring.push_back("FMA3; ");
#endif // _XM_FMA3_INTRINSICS_
#ifdef _XM_F16C_INTRINSICS_
		cpustring.push_back("F16C; ");
#endif // _XM_F16C_INTRINSICS_
#ifdef _XM_AVX2_INTRINSICS_
		cpustring.push_back("AVX 2; ");
#endif // _XM_AVX2_INTRINSICS_
#ifdef _XM_ARM_NEON_INTRINSICS_
		cpustring.push_back("NEON; ");
#endif // _XM_ARM_NEON_INTRINSICS_

		wi::backlog::post(cpustring.c_str());

		if (!XMVerifyCPUSupport())
		{
			wilog_messagebox("XMVerifyCPUSupport() failed! This means that your CPU doesn't support a required feature! %s", cpustring.c_str());
		}

		wilog("\nRAM: %s", wi::helper::GetMemorySizeText(wi::helper::GetMemoryUsage().total_physical).c_str());

		size_t shaderdump_count = wi::renderer::GetShaderDumpCount();
		if (shaderdump_count > 0)
		{
			wilog("\nEmbedded shaders found: %d", (int)shaderdump_count);
		}
		else
		{
			wilog("\nNo embedded shaders found, shaders will be compiled at runtime if needed.\n\tShader source path: %s\n\tShader binary path: %s", wi::renderer::GetShaderSourcePath().c_str(), wi::renderer::GetShaderPath().c_str());
		}

		wi::backlog::post("");
		wi::jobsystem::Initialize();

		wi::backlog::post("");
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
		wi::jobsystem::Execute(ctx, [](wi::jobsystem::JobArgs args) { wi::TrailRenderer::Initialize(); systems[INITIALIZED_SYSTEM_TRAILRENDERER].store(true); });

		// Initialize these immediately:
		wi::lua::Initialize(); systems[INITIALIZED_SYSTEM_LUA].store(true);
		wi::audio::Initialize(); systems[INITIALIZED_SYSTEM_AUDIO].store(true);
		wi::font::Initialize(); systems[INITIALIZED_SYSTEM_FONT].store(true);

		std::thread([] {
			wi::jobsystem::Wait(ctx);
			wilog("\n[wi::initializer] Wicked Engine Initialized (%d ms)", (int)std::round(timer.elapsed()));
		}).detach();

	}

	bool IsInitializeFinished(INITIALIZED_SYSTEM system)
	{
		if (system == INITIALIZED_SYSTEM_COUNT)
		{
			return initializationStarted.load() && !wi::jobsystem::IsBusy(ctx);
		}
		else
		{
			return systems[system].load();
		}
	}

	void WaitForInitializationsToFinish()
	{
		wi::jobsystem::Wait(ctx);
	}
}
