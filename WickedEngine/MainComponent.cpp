#include "MainComponent.h"
#include "RenderableComponent.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiCpuInfo.h"
#include "wiInputManager.h"
#include "wiBackLog.h"
#include "MainComponent_BindLua.h"
#include "wiVersion.h"
#include "wiImageEffects.h"
#include "wiTextureHelper.h"
#include "wiFrameRate.h"
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

MainComponent::MainComponent()
{
	// This call also saves the current working dir as the original one on this first call
	wiHelper::GetOriginalWorkingDirectory();

	activeComponent = new RenderableComponent();
}


MainComponent::~MainComponent()
{
	wiRenderer::GetDevice()->WaitForGPU();
}

void MainComponent::Initialize()
{

	// User can also create a graphics device if custom logic is desired, but he must do before this function!
	if (wiRenderer::GetDevice() == nullptr)
	{

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
		else
		{
			wiRenderer::SetDevice(new GraphicsDevice_DX11(window, fullscreen, debugdevice));
		}

	}


	wiInitializer::InitializeComponentsAsync();

	wiLua::GetGlobal()->RegisterObject(MainComponent_BindLua::className, "main", new MainComponent_BindLua(this));
}

void MainComponent::activateComponent(RenderableComponent* component, float fadeSeconds, const wiColor& fadeColor)
{
	if (component == nullptr)
	{
		return;
	}

	// Fade manager will activate on fadeout
	fadeManager.Clear();
	fadeManager.Start(fadeSeconds, fadeColor, [this,component]() {
		if (component == nullptr)
			return;
		activeComponent->Stop();
		component->Start();
		activeComponent = component;
	});
}

void MainComponent::Run()
{
	if (!wiInitializer::IsInitializeFinished())
	{
		// Until engine is not loaded, present initialization screen...
		wiRenderer::GetDevice()->PresentBegin();
		wiFont::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
		wiFont(wiBackLog::getText(), wiFontProps(4, 4, infoDisplay.size)).Draw(GRAPHICSTHREAD_IMMEDIATE);
		wiRenderer::GetDevice()->PresentEnd();
		return;
	}

	wiProfiler::GetInstance().BeginFrame();
	wiProfiler::GetInstance().BeginRange("CPU Frame", wiProfiler::DOMAIN_CPU);

	wiInputManager::GetInstance()->Update();

	static wiTimer timer = wiTimer();
	static double accumulator = 0.0;
	const double elapsedTime = max(0, timer.elapsed() / 1000.0);
	timer.record();

	fadeManager.Update((float)elapsedTime);

	// Fixed time update:
	wiProfiler::GetInstance().BeginRange("Fixed Update", wiProfiler::DOMAIN_CPU);
	if (frameskip)
	{
		accumulator += elapsedTime;
		if (!wiWindowRegistration::GetInstance()->IsWindowActive() || accumulator > applicationControlLostThreshold) //application probably lost control
			accumulator = 0;

		while (accumulator >= targetFrameRateInv)
		{
			FixedUpdate();
			accumulator -= targetFrameRateInv;
		}
	}
	else
	{
		FixedUpdate();
	}
	wiProfiler::GetInstance().EndRange(); // Fixed Update

	wiLua::GetGlobal()->SetDeltaTime(elapsedTime);

	// Variable-timed update:
	wiProfiler::GetInstance().BeginRange("Update", wiProfiler::DOMAIN_CPU);
	Update((float)elapsedTime);
	wiProfiler::GetInstance().EndRange(); // Update


	wiProfiler::GetInstance().BeginRange("Render", wiProfiler::DOMAIN_CPU);
	Render();
	wiProfiler::GetInstance().EndRange(); // Render

	wiProfiler::GetInstance().EndRange(); // CPU Frame


	wiProfiler::GetInstance().BeginRange("Compose", wiProfiler::DOMAIN_CPU);
	wiRenderer::GetDevice()->PresentBegin();
	Compose();
	wiRenderer::GetDevice()->PresentEnd();
	wiProfiler::GetInstance().EndRange(); // Compose

	wiRenderer::EndFrame();

	static bool startupScriptProcessed = false;
	if (!startupScriptProcessed) {
		wiLua::GetGlobal()->RunFile("startup.lua");
		startupScriptProcessed = true;
	}

	wiProfiler::GetInstance().EndFrame();
}

void MainComponent::Update(float dt)
{
	wiCpuInfo::UpdateFrame();
	getActiveComponent()->Update(dt);

	wiLua::GetGlobal()->Update();
}

void MainComponent::FixedUpdate()
{
	wiBackLog::Update();
	wiLua::GetGlobal()->FixedUpdate();

	getActiveComponent()->FixedUpdate();
}

void MainComponent::Render()
{
	wiLua::GetGlobal()->Render();

	wiProfiler::GetInstance().BeginRange("GPU Frame", wiProfiler::DOMAIN_GPU, GRAPHICSTHREAD_IMMEDIATE);
	wiImage::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	wiFont::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	getActiveComponent()->Render();
	wiProfiler::GetInstance().EndRange(GRAPHICSTHREAD_IMMEDIATE); // GPU Frame
}

void MainComponent::Compose()
{
	getActiveComponent()->Compose();

	if (fadeManager.IsActive())
	{
		// display fade rect
		static wiImageEffects fx;
		fx.siz.x = (float)wiRenderer::GetDevice()->GetScreenWidth();
		fx.siz.y = (float)wiRenderer::GetDevice()->GetScreenHeight();
		fx.opacity = fadeManager.opacity;
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(fadeManager.color), fx, GRAPHICSTHREAD_IMMEDIATE);
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
		if (infoDisplay.initstats)
		{
			ss << "System init time: " << wiInitializer::GetInitializationTimeInSeconds() << " sec" << endl;
		}
		if (infoDisplay.resolution)
		{
			ss << "Resolution: " << wiRenderer::GetDevice()->GetScreenWidth() << " x " << wiRenderer::GetDevice()->GetScreenHeight() << endl;
		}
		if (infoDisplay.fpsinfo)
		{
			ss.precision(2);
			ss << fixed << wiFrameRate::GetFPS() << " FPS" << endl;
		}
		if (infoDisplay.cpuinfo)
		{
			float cpupercentage = wiCpuInfo::GetCPUPercentage();
			ss << "CPU: ";
			if (cpupercentage >= 0)
			{
				ss << cpupercentage << "%";
			}
			else
			{
				ss << "Query failed";
			}
			ss << endl;
		}
		ss.precision(2);
		wiFont(ss.str(), wiFontProps(4, 4, infoDisplay.size, WIFALIGN_LEFT, WIFALIGN_TOP, 2, 1, wiColor(255,255,255,255), wiColor(0,0,0,255))).Draw(GRAPHICSTHREAD_IMMEDIATE);
	}

	wiProfiler::GetInstance().DrawData(4, 120, GRAPHICSTHREAD_IMMEDIATE);

	wiBackLog::Draw();
}

#ifndef WINSTORE_SUPPORT
bool MainComponent::SetWindow(wiWindowRegistration::window_type window, HINSTANCE hInst)
{
	this->window = window;
	this->instance = hInst;

	if (screenW == 0 || screenH == 0)
	{
		RECT rect = RECT();
		GetClientRect(window, &rect);
		screenW = rect.right - rect.left;
		screenH = rect.bottom - rect.top;
	}

	wiWindowRegistration::GetInstance()->RegisterWindow(window);

	return true;
}
#else
bool MainComponent::SetWindow(wiWindowRegistration::window_type window)
{
	screenW = (int)window->Bounds.Width;
	screenH = (int)window->Bounds.Height;

	this->window = window;

	wiWindowRegistration::GetInstance()->RegisterWindow(window);

	return true;
}
#endif

