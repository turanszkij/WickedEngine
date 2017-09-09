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

using namespace std;

MainComponent::MainComponent()
{
	// This call also saves the current working dir as the original one on this first call
	wiHelper::GetOriginalWorkingDirectory();

	screenW = 0;
	screenH = 0;
	fullscreen = false;

	activeComponent = new RenderableComponent();

	setFrameSkip(true);
	setTargetFrameRate(60);
	setApplicationControlLostThreshold(10);

	infoDisplay = InfoDisplayer(); 
	colorGradingPaletteDisplayEnabled = false;

	fadeManager.Clear();
}


MainComponent::~MainComponent()
{
	if (activeComponent != nullptr)
	{
		activeComponent->Unload();
	}
}

void MainComponent::Initialize()
{
	wiLua::GetGlobal()->RegisterObject(MainComponent_BindLua::className, "main", new MainComponent_BindLua(this));
}

void MainComponent::activateComponent(RenderableComponent* component, int fadeFrames, const wiColor& fadeColor)
{
	if (component == nullptr)
	{
		return;
	}

	if (fadeFrames > 0)
	{
		// Fade
		fadeManager.Clear();
		fadeManager.Start(fadeFrames, fadeColor, [this,component]() {
			if (component == nullptr)
				return;
			activeComponent->Stop();
			component->Start();
			activeComponent = component;
		});
	}
	else
	{
		// No fade
		fadeManager.Clear();

		activeComponent->Stop();
		component->Start();
		activeComponent = component;
	}
}

void MainComponent::run()
{
	wiProfiler::GetInstance().BeginFrame();
	wiProfiler::GetInstance().BeginRange("CPU Frame", wiProfiler::DOMAIN_CPU);

	static wiTimer timer = wiTimer();
	static double accumulator = 0.0;
	const double elapsedTime = max(0, timer.elapsed() / 1000.0);
	timer.record();

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

	wiProfiler::GetInstance().BeginRange("Physics", wiProfiler::DOMAIN_CPU);
	wiRenderer::SynchronizeWithPhysicsEngine((float)elapsedTime);
	wiProfiler::GetInstance().EndRange(); // Physics

	wiLua::GetGlobal()->SetDeltaTime(elapsedTime);

	// Variable-timed update:
	wiProfiler::GetInstance().BeginRange("Update", wiProfiler::DOMAIN_CPU);
	Update((float)elapsedTime);
	wiProfiler::GetInstance().EndRange(); // Update

	wiProfiler::GetInstance().BeginRange("Render", wiProfiler::DOMAIN_CPU);
	Render();
	wiProfiler::GetInstance().EndRange(); // Render

	wiProfiler::GetInstance().EndRange(); // CPU Frame

	wiRenderer::Present(bind(&MainComponent::Compose, this));

	static bool startupScriptProcessed = false;
	if (!startupScriptProcessed) {
		wiLua::GetGlobal()->RunFile("startup.lua");
		startupScriptProcessed = true;
	}

	wiProfiler::GetInstance().EndFrame();
}

void MainComponent::Update(float dt)
{
	wiCpuInfo::Frame();
	getActiveComponent()->Update(dt);

	wiLua::GetGlobal()->Update();
}

void MainComponent::FixedUpdate()
{
	wiBackLog::Update();
	wiLua::GetGlobal()->FixedUpdate();

	getActiveComponent()->FixedUpdate();

	fadeManager.Update();
}

void MainComponent::Render()
{
	wiLua::GetGlobal()->Render();

	wiProfiler::GetInstance().BeginRange("GPU Frame", wiProfiler::DOMAIN_GPU, GRAPHICSTHREAD_IMMEDIATE);
	wiRenderer::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	wiImage::BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
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
			ss << string("Wicked Engine ") + wiVersion::GetVersionString();
#ifdef _DEBUG
			ss << " [DEBUG]";
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
			ss << fixed << wiFrameRate::FPS() << " FPS" << endl;
		}
		if (infoDisplay.cpuinfo)
		{
			int cpupercentage = wiCpuInfo::GetCpuPercentage();
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

	// Draw the color grading palette
	if (colorGradingPaletteDisplayEnabled)
	{
		//wiImage::BatchBegin();
		wiImage::Draw(wiTextureHelper::getInstance()->getColorGradeDefault(), wiImageEffects(0, 0, 256, 16), GRAPHICSTHREAD_IMMEDIATE);
		wiImage::Draw(wiRenderer::GetColorGrading(), wiImageEffects(screenW-256.f, 0, 256, 16), GRAPHICSTHREAD_IMMEDIATE);
	}

	wiBackLog::Draw();
}

#ifndef WINSTORE_SUPPORT
bool MainComponent::setWindow(wiWindowRegistration::window_type window, HINSTANCE hInst)
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

	wiRenderer::InitDevice(window, fullscreen);

	wiWindowRegistration::GetInstance()->RegisterWindow(window);

	return true;
}
#else
bool MainComponent::setWindow(wiWindowRegistration::window_type window)
{
	screenW = (int)window->Bounds.Width;
	screenH = (int)window->Bounds.Height;
	wiRenderer::InitDevice(window, fullscreen);

	this->window = window;

	wiWindowRegistration::GetInstance()->RegisterWindow(window);

	return true;
}
#endif

