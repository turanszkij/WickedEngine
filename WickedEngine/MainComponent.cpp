#include "MainComponent.h"
#include "RenderPath.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiInputManager.h"
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

#include <sstream>

using namespace std;
using namespace wiGraphicsTypes;

MainComponent::~MainComponent()
{
	// This usually means appllication is terminating. Wait for GPU to finish rendering.
	wiRenderer::GetDevice()->WaitForGPU();
}

void MainComponent::Initialize()
{
	if (initialized)
	{
		return;
	}
	initialized = true;

	// User can also create a graphics device if custom logic is desired, but he must do before this function!
	if (wiRenderer::GetDevice() == nullptr)
	{
		auto window = wiWindowRegistration::GetRegisteredWindow();

		bool debugdevice = wiStartupArguments::HasArgument("debugdevice");

		if (wiStartupArguments::HasArgument("vulkan"))
		{
#ifdef WICKEDENGINE_BUILD_VULKAN
			wiRenderer::GetShaderPath() += "spirv/";
			wiRenderer::SetDevice(new GraphicsDevice_Vulkan(window, fullscreen, debugdevice));
#else
			wiHelper::messageBox("Vulkan SDK not found during building the application! Vulkan API disabled!", "Error");
#endif
		}
		else if (wiStartupArguments::HasArgument("dx12"))
		{
			if (wiStartupArguments::HasArgument("hlsl6"))
			{
				wiRenderer::GetShaderPath() += "hlsl6/";
			}
			wiRenderer::SetDevice(new GraphicsDevice_DX12(window, fullscreen, debugdevice));
		}

		// default graphics device:
		if (wiRenderer::GetDevice() == nullptr)
		{
			wiRenderer::SetDevice(new GraphicsDevice_DX11(window, fullscreen, debugdevice));
		}

	}


	wiInitializer::InitializeComponentsAsync();

	wiLua::GetGlobal()->RegisterObject(MainComponent_BindLua::className, "main", new MainComponent_BindLua(this));
}

void MainComponent::ActivatePath(RenderPath* component, float fadeSeconds, const wiColor& fadeColor)
{
	if (component == nullptr)
	{
		return;
	}

	// Fade manager will activate on fadeout
	fadeManager.Clear();
	fadeManager.Start(fadeSeconds, fadeColor, [this,component]() {

		if (component == nullptr)
		{
			return;
		}

		if (activePath != nullptr)
		{
			activePath->Stop();
		}

		component->Start();
		activePath = component;
	});
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
		wiRenderer::GetDevice()->PresentBegin();
		wiFont::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
		wiFont(wiBackLog::getText(), wiFontParams(4, 4, infoDisplay.size)).Draw(GRAPHICSTHREAD_IMMEDIATE);
		wiRenderer::GetDevice()->PresentEnd();
		return;
	}

	wiProfiler::BeginFrame();
	wiProfiler::BeginRange("CPU Frame", wiProfiler::DOMAIN_CPU);

	wiInputManager::Update();

	deltaTime = float(max(0, timer.elapsed() / 1000.0));
	timer.record();

	if (wiWindowRegistration::IsWindowActive())
	{
		// If the application is active, run Update loops:

		fadeManager.Update(deltaTime);

		// Fixed time update:
		wiProfiler::BeginRange("Fixed Update", wiProfiler::DOMAIN_CPU);
		{
			if (frameskip)
			{
				deltaTimeAccumulator += deltaTime;
				if (deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would be take long
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
		wiProfiler::EndRange(); // Fixed Update

		wiLua::GetGlobal()->SetDeltaTime(double(deltaTime));

		// Variable-timed update:
		wiProfiler::BeginRange("Update", wiProfiler::DOMAIN_CPU);
		Update(deltaTime);
		wiProfiler::EndRange(); // Update
	}
	else
	{
		// If the application is not active, disable Update loops:
		deltaTimeAccumulator = 0;
		wiLua::GetGlobal()->SetDeltaTime(0);
	}

	wiProfiler::BeginRange("Render", wiProfiler::DOMAIN_CPU);
	Render();
	wiProfiler::EndRange(); // Render

	wiProfiler::EndRange(); // CPU Frame


	wiProfiler::BeginRange("Compose", wiProfiler::DOMAIN_CPU);
	wiRenderer::GetDevice()->PresentBegin();
	Compose();
	wiRenderer::GetDevice()->PresentEnd();
	wiProfiler::EndRange(); // Compose

	wiRenderer::EndFrame();

	static bool startupScriptProcessed = false;
	if (!startupScriptProcessed) {
		wiLua::GetGlobal()->RunFile("startup.lua");
		startupScriptProcessed = true;
	}

	wiProfiler::EndFrame();
}

void MainComponent::Update(float dt)
{
	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Update(dt);
	}

	wiLua::GetGlobal()->Update();
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
	wiLua::GetGlobal()->Render();

	wiProfiler::BeginRange("GPU Frame", wiProfiler::DOMAIN_GPU, GRAPHICSTHREAD_IMMEDIATE);
	wiRenderer::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	wiImage::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	wiFont::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Render();
	}
	wiProfiler::EndRange(GRAPHICSTHREAD_IMMEDIATE); // GPU Frame
}

void MainComponent::Compose()
{
	if (GetActivePath() != nullptr)
	{
		GetActivePath()->Compose();
	}

	if (fadeManager.IsActive())
	{
		// display fade rect
		static wiImageParams fx;
		fx.siz.x = (float)wiRenderer::GetDevice()->GetScreenWidth();
		fx.siz.y = (float)wiRenderer::GetDevice()->GetScreenHeight();
		fx.opacity = fadeManager.opacity;
		wiImage::Draw(wiTextureHelper::getColor(fadeManager.color), fx, GRAPHICSTHREAD_IMMEDIATE);
	}

	// Draw the information display
	if (infoDisplay.active)
	{
		stringstream ss("");
		if (infoDisplay.watermark)
		{
			ss << string("Wicked Engine ") + wiVersion::GetVersionString() + " ";

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
			ss << endl;
		}
		if (infoDisplay.resolution)
		{
			ss << "Resolution: " << wiRenderer::GetDevice()->GetScreenWidth() << " x " << wiRenderer::GetDevice()->GetScreenHeight() << endl;
		}
		if (infoDisplay.fpsinfo)
		{
			ss.precision(2);
			ss << fixed << 1.0f / deltaTime << " FPS" << endl;
		}
		ss.precision(2);
		wiFont(ss.str(), wiFontParams(4, 4, infoDisplay.size, WIFALIGN_LEFT, WIFALIGN_TOP, 0, 0, wiColor(255,255,255,255), wiColor(0,0,0,255))).Draw(GRAPHICSTHREAD_IMMEDIATE);
	}

	wiProfiler::DrawData(4, 120, GRAPHICSTHREAD_IMMEDIATE);

	wiBackLog::Draw();
}

#ifndef WINSTORE_SUPPORT
bool MainComponent::SetWindow(wiWindowRegistration::window_type window, HINSTANCE hInst)
{
	this->hInst = hInst;

	wiWindowRegistration::RegisterWindow(window);

	return true;
}
#else
bool MainComponent::SetWindow(wiWindowRegistration::window_type window)
{
	wiWindowRegistration::RegisterWindow(window);

	return true;
}
#endif

