#include "MainComponent.h"
#include "RenderableComponent.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiCpuInfo.h"
#include "wiInputManager.h"
#include "wiBackLog.h"


MainComponent::MainComponent()
{
	screenW = 800;
	screenH = 600;

	activeComponent = new RenderableComponent();

	setFrameSkip(true);
	setTargetFrameRate(60);
	setApplicationControlLostThreshold(10);
}


MainComponent::~MainComponent()
{
	activeComponent->Unload();
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

	static wiTimer timer = wiTimer();
	static double accumulator = 0.0;

	accumulator += timer.elapsed() / 1000.0;
	if (accumulator>applicationControlLostThreshold) //application probably lost control
		accumulator = 0;
	timer.record();


	while (accumulator >= targetFrameRateInv || !frameskip)
	{

		Update();

		accumulator -= targetFrameRateInv;

		if (!frameskip)
			break;

	}

	Render();

	wiRenderer::Present(bind(&MainComponent::Compose, this));
}

void MainComponent::Update()
{
	wiInputManager::Update();
	wiCpuInfo::Frame();
	wiBackLog::Update();

	getActiveComponent()->Update();
}

void MainComponent::Render()
{
	getActiveComponent()->Render();
}

void MainComponent::Compose()
{
	getActiveComponent()->Compose();
}

#ifndef WINSTORE_SUPPORT
bool MainComponent::setWindow(HWND hWnd, HINSTANCE hInst)
{
	window = hWnd;
	instance = hInst;

	RECT rect = RECT();
	GetClientRect(hWnd, &rect);
	screenW = rect.right - rect.left;
	screenH = rect.bottom - rect.top;

	if (FAILED(wiRenderer::InitDevice(window, screenW, screenH, true)))
	{
		wiHelper::messageBox("Could not initialize D3D device!", "Fatal Error!", window);
		return false;
	}

	return true;
}
#endif

