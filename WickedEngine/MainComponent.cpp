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

MainComponent::MainComponent()
{
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
	activeComponent->Unload();
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
	static wiTimer timer = wiTimer();
	static double accumulator = 0.0;
	const double elapsedTime = timer.elapsed() / 1000.0;
	timer.record();

	wiLua::GetGlobal()->SetDeltaTime(elapsedTime);

	if (frameskip)
	{
		accumulator += elapsedTime;
		if (accumulator > applicationControlLostThreshold) //application probably lost control
			accumulator = 0;

		while (accumulator >= targetFrameRateInv)
		{

			Update();

			accumulator -= targetFrameRateInv;

		}

	}
	else
	{
		Update();
	}

	getActiveComponent()->FixedUpdate((float)elapsedTime);

	Render();

	wiRenderer::Present(bind(&MainComponent::Compose, this));

	static bool startupScriptProcessed = false;
	if (!startupScriptProcessed) {
		wiLua::GetGlobal()->RunFile("startup.lua");
		startupScriptProcessed = true;
	}
}

void MainComponent::Update()
{
	wiCpuInfo::Frame();
	wiBackLog::Update();

	wiLua::GetGlobal()->Update();

	getActiveComponent()->Update();


	fadeManager.Update();
}

void MainComponent::Render()
{
	wiLua::GetGlobal()->Render();

	wiRenderer::GetDevice()->LOCK();
	getActiveComponent()->Render();
	wiRenderer::GetDevice()->UNLOCK();
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
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(fadeManager.color), fx);
	}

	// Draw the information display
	if (infoDisplay.active)
	{
		stringstream ss("");
		if (infoDisplay.watermark)
		{
			ss << string("Wicked Engine ") + wiVersion::GetVersionString() << endl;
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
			ss << "CPU: " << wiCpuInfo::GetCpuPercentage() << "%" << endl;
		}
		wiFont(ss.str(), wiFontProps(0, 0, infoDisplay.size, WIFALIGN_LEFT, WIFALIGN_TOP, -2)).Draw();
	}

	// Draw the color grading palette
	if (colorGradingPaletteDisplayEnabled)
	{
		//wiImage::BatchBegin();
		wiImage::Draw(wiTextureHelper::getInstance()->getColorGradeDefault(), wiImageEffects(0, 0, 256, 16));
		wiImage::Draw(wiRenderer::GetColorGrading(), wiImageEffects(screenW-256.f, 0, 256, 16));
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

