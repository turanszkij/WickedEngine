#include "MainComponent.h"
#include "RenderPath.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiInput.h"
#include "wiBackLog.h"
#include "MainComponent_BindLua.h"
#include "wiVersion.h"
#include "wiEnums.h"
#include "wiTextureHelper.h"
#include "wiProfiler.h"
#include "wiInitializer.h"
#include "wiStartupArguments.h"
#include "wiFont.h"
#include "wiImage.h"

#include "wiGraphicsDevice_DX11.h"
#include "wiGraphicsDevice_DX12.h"
#include "wiGraphicsDevice_Vulkan.h"

#include "Utility/replace_new.h"

#include <sstream>
#include <algorithm>

using namespace std;
using namespace wiGraphics;

void MainComponent::Initialize()
{
	if (initialized)
	{
		return;
	}
	initialized = true;

	wiHelper::GetOriginalWorkingDirectory();

	// User can also create a graphics device if custom logic is desired, but he must do before this function!
	if (wiRenderer::GetDevice() == nullptr)
	{
		auto window = wiPlatform::GetWindow();

		bool debugdevice = wiStartupArguments::HasArgument("debugdevice");

		if (wiStartupArguments::HasArgument("vulkan"))
		{
#ifdef WICKEDENGINE_BUILD_VULKAN
			wiRenderer::SetShaderPath(wiRenderer::GetShaderPath() + "spirv/");
			wiRenderer::SetDevice(std::make_shared<GraphicsDevice_Vulkan>(window, fullscreen, debugdevice));
#else
			wiHelper::messageBox("Vulkan SDK not found during building the application! Vulkan API disabled!", "Error");
#endif
		}
		else if (wiStartupArguments::HasArgument("dx12"))
		{
			if (wiStartupArguments::HasArgument("hlsl6"))
			{
				wiRenderer::SetShaderPath(wiRenderer::GetShaderPath() + "hlsl6/");
			}
			wiRenderer::SetDevice(std::make_shared<GraphicsDevice_DX12>(window, fullscreen, debugdevice));
		}

		// default graphics device:
		if (wiRenderer::GetDevice() == nullptr)
		{
			wiRenderer::SetDevice(std::make_shared<GraphicsDevice_DX11>(window, fullscreen, debugdevice));
		}

	}


	wiInitializer::InitializeComponentsAsync();
}

void MainComponent::ActivatePath(RenderPath* component, float fadeSeconds, wiColor fadeColor)
{
	// Fade manager will activate on fadeout
	fadeManager.Clear();
	fadeManager.Start(fadeSeconds, fadeColor, [this, component]() {

		if (GetActivePath() != nullptr)
		{
			GetActivePath()->Stop();
		}

		if (component != nullptr)
		{
			component->Start();
		}
		activePath = component;
	});

	fadeManager.Update(0); // If user calls ActivatePath without fadeout, it will be instant
}

void MainComponent::Run()
{
	if (!initialized)
	{
		// Initialize in a lazy way, so the user application doesn't have to call this explicitly
		Initialize();
		initialized = true;
	}
	if (!wiInitializer::IsInitializeFinished())
	{
		// Until engine is not loaded, present initialization screen...
		CommandList cmd = wiRenderer::GetDevice()->BeginCommandList();
		wiRenderer::GetDevice()->PresentBegin(cmd);
		wiFont(wiBackLog::getText(), wiFontParams(4, 4, infoDisplay.size)).Draw(cmd);
		wiRenderer::GetDevice()->PresentEnd(cmd);
		return;
	}

	static bool startup_script = false;
	if (!startup_script)
	{
		startup_script = true;
		wiLua::GetGlobal()->RegisterObject(MainComponent_BindLua::className, "main", new MainComponent_BindLua(this));
		wiLua::GetGlobal()->RunFile("startup.lua");
	}

	wiProfiler::BeginFrame();

	deltaTime = float(std::max(0.0, timer.elapsed() / 1000.0));
	timer.record();

	if (wiPlatform::IsWindowActive())
	{
		// If the application is active, run Update loops:

		const float dt = framerate_lock ? (1.0f / targetFrameRate) : deltaTime;

		fadeManager.Update(dt);

		// Fixed time update:
		auto range = wiProfiler::BeginRangeCPU("Fixed Update");
		{
			if (frameskip)
			{
				deltaTimeAccumulator += dt;
				if (deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would take too long
					deltaTimeAccumulator = 0;
				}

				const float targetFrameRateInv = 1.0f / targetFrameRate;
				while (deltaTimeAccumulator >= targetFrameRateInv)
				{
					FixedUpdate();
					deltaTimeAccumulator -= targetFrameRateInv;
				}
			}
			else
			{
				FixedUpdate();
			}
		}
		wiProfiler::EndRange(range); // Fixed Update

		// Variable-timed update:
		Update(dt);

		wiInput::Update();

		Render();
	}
	else
	{
		// If the application is not active, disable Update loops:
		deltaTimeAccumulator = 0;
	}

	CommandList cmd = wiRenderer::GetDevice()->BeginCommandList();
	wiRenderer::GetDevice()->PresentBegin(cmd);
	{
		Compose(cmd);
		wiProfiler::EndFrame(cmd); // End before Present() so that GPU queries are properly recorded
	}
	wiRenderer::GetDevice()->PresentEnd(cmd);

	wiRenderer::EndFrame();
}

void MainComponent::Update(float dt)
{
	auto range = wiProfiler::BeginRangeCPU("Update");

	wiLua::GetGlobal()->SetDeltaTime(double(dt));
	wiLua::GetGlobal()->Update();

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Update(dt);
	}

	wiProfiler::EndRange(range); // Update
}

void MainComponent::FixedUpdate()
{
	wiBackLog::Update();
	wiLua::GetGlobal()->FixedUpdate();

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->FixedUpdate();
	}
}

void MainComponent::Render()
{
	auto range = wiProfiler::BeginRangeCPU("Render");

	wiLua::GetGlobal()->Render();

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Render();
	}

	wiProfiler::EndRange(range); // Render
}

void MainComponent::Compose(CommandList cmd)
{
	auto range = wiProfiler::BeginRangeCPU("Compose");

	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Compose(cmd);
	}

	if (fadeManager.IsActive())
	{
		// display fade rect
		static wiImageParams fx;
		fx.siz.x = (float)wiRenderer::GetDevice()->GetScreenWidth();
		fx.siz.y = (float)wiRenderer::GetDevice()->GetScreenHeight();
		fx.opacity = fadeManager.opacity;
		wiImage::Draw(wiTextureHelper::getColor(fadeManager.color), fx, cmd);
	}

	// Draw the information display
	if (infoDisplay.active)
	{
		stringstream ss("");
		if (infoDisplay.watermark)
		{
			ss << "Wicked Engine " << wiVersion::GetVersionString() << " ";

#if defined(_ARM)
			ss << "[ARM]";
#elif defined(_WIN64)
			ss << "[64-bit]";
#elif defined(_WIN32)
			ss << "[32-bit]";
#endif

			if (dynamic_cast<GraphicsDevice_DX11*>(wiRenderer::GetDevice()))
			{
				ss << "[DX11]";
			}
			else if (dynamic_cast<GraphicsDevice_DX12*>(wiRenderer::GetDevice()))
			{
				ss << "[DX12]";
			}
#ifdef WICKEDENGINE_BUILD_VULKAN
			else if (dynamic_cast<GraphicsDevice_Vulkan*>(wiRenderer::GetDevice()))
			{
				ss << "[Vulkan]";
			}
#endif

#ifdef _DEBUG
			ss << "[DEBUG]";
#endif
			if (wiRenderer::GetDevice()->IsDebugDevice())
			{
				ss << "[debugdevice]";
			}
			ss << endl;
		}
		if (infoDisplay.resolution)
		{
			ss << "Resolution: " << wiRenderer::GetDevice()->GetScreenWidth() << " x " << wiRenderer::GetDevice()->GetScreenHeight() << endl;
		}
		if (infoDisplay.fpsinfo)
		{
			deltatimes[fps_avg_counter++ % arraysize(deltatimes)] = deltaTime;
			float displaydeltatime = deltaTime;
			if (fps_avg_counter > arraysize(deltatimes))
			{
				float avg_time = 0;
				for (int i = 0; i < arraysize(deltatimes); ++i)
				{
					avg_time += deltatimes[i];
				}
				displaydeltatime = avg_time / arraysize(deltatimes);
			}

			ss.precision(2);
			ss << fixed << 1.0f / displaydeltatime << " FPS" << endl;
		}
		if (infoDisplay.heap_allocation_counter)
		{
			ss << "Heap allocations per frame: " << number_of_allocs.load() << endl;
			number_of_allocs.store(0);
		}

#ifdef _DEBUG
		ss << "Warning: This is a [DEBUG] build, performance will be slow!" << endl;
#endif
		if (wiRenderer::GetDevice()->IsDebugDevice())
		{
			ss << "Warning: Graphics is in [debugdevice] mode, performance will be slow!" << endl;
		}

		ss.precision(2);
		wiFont(ss.str(), wiFontParams(4, 4, infoDisplay.size, WIFALIGN_LEFT, WIFALIGN_TOP, 0, 0, wiColor(255,255,255,255), wiColor(0,0,0,255))).Draw(cmd);
	}

	wiProfiler::DrawData(4, 120, cmd);

	wiBackLog::Draw(cmd);

	wiProfiler::EndRange(range); // Compose
}

void MainComponent::SetWindow(wiPlatform::window_type window)
{
	wiPlatform::GetWindow() = window;
}

