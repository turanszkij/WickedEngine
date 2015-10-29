#include "MainComponent.h"
#include "RenderableComponent.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiCpuInfo.h"
#include "wiInputManager.h"
#include "wiBackLog.h"
#include "MainComponent_BindLua.h"
#include "WickedEngine.h"

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
}


MainComponent::~MainComponent()
{
	activeComponent->Unload();
}

void MainComponent::Initialize()
{
	wiLua::GetGlobal()->RegisterObject(MainComponent_BindLua::className, "main", new MainComponent_BindLua(this));
}

void MainComponent::activateComponent(RenderableComponent* component)
{
	if (component == nullptr)
	{
		return;
	}

	activeComponent->Stop();
	component->Start();

	activeComponent = component;
}

void MainComponent::run()
{
	static bool startupScriptProcessed = false;
	if (!startupScriptProcessed) {
		wiLua::GetGlobal()->RunFile("startup.lua");
		startupScriptProcessed = true;
	}

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

	Render();

	wiRenderer::Present(bind(&MainComponent::Compose, this));
}

void MainComponent::Update()
{
	wiInputManager::Update();
	wiCpuInfo::Frame();
	wiBackLog::Update();

	wiLua::GetGlobal()->Update();

	getActiveComponent()->Update();
}

void MainComponent::Render()
{
	wiLua::GetGlobal()->Render();

	getActiveComponent()->Render();
}

void MainComponent::Compose()
{
	getActiveComponent()->Compose();

	// Draw the information display
	if (infoDisplay.active)
	{
		stringstream ss("");
		if (infoDisplay.watermark)
		{
			ss << string("Wicked Engine ") + string(WICKED_ENGINE_VERSION) << endl;
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
		wiFont(ss.str(), wiFontProps(0, 0, infoDisplay.size, WIFALIGN_LEFT, WIFALIGN_TOP, infoDisplay.size)).Draw();
	}

	// Draw the color grading palette
	if (colorGradingPaletteDisplayEnabled)
	{
		wiImage::BatchBegin();
		wiImage::Draw(wiTextureHelper::getInstance()->getColorGradeDefault(), wiImageEffects(0, 0, 256, 16));
		wiImage::Draw(wiRenderer::GetColorGrading(), wiImageEffects(screenW-256, 0, 256, 16));
	}

	wiBackLog::Draw();
}

#ifndef WINSTORE_SUPPORT
bool MainComponent::setWindow(HWND hWnd, HINSTANCE hInst)
{
	window = hWnd;
	instance = hInst;

	if (screenH == 0 && screenW == 0)
	{
		RECT rect = RECT();
		GetClientRect(hWnd, &rect);
		screenW = rect.right - rect.left;
		screenH = rect.bottom - rect.top;
	}

	if (FAILED(wiRenderer::InitDevice(window, screenW, screenH, !fullscreen)))
	{
		wiHelper::messageBox("Could not initialize D3D device!", "Fatal Error!", window);
		return false;
	}

	return true;
}
#else
bool MainComponent::setWindow(Windows::UI::Core::CoreWindow^ window)
{
	screenW = (int)window->Bounds.Width;
	screenH = (int)window->Bounds.Height;
	if (FAILED(wiRenderer::InitDevice(window)))
	{
		wiHelper::messageBox("Could not initialize D3D device!", "Fatal Error!");
		return false;
	}

	return true;
}
#endif

