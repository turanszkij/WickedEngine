#include "MainComponent.h"
#include "RenderableComponent.h"
#include "frameskipDEF.h"
#include "wiRenderer.h"
#include "wiHelper.h"


MainComponent::MainComponent()
{
	screenW = 800;
	screenH = 600;

	activeComponent = new RenderableComponent();
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

	activeComponent = component;
	activeComponent->Start();
}

void MainComponent::run()
{

	static Timer timer = Timer();
	static const double dt = 1.0 / 60.0;
	static double accumulator = 0.0;

	accumulator += timer.elapsed() / 1000.0;
	if (accumulator>10) //application probably lost control
		accumulator = 0;
	timer.record();


	while (accumulator >= dt)
	{

		Update();

		accumulator -= dt;

		if (!getFrameSkip())
			break;

	}

	Render();

	wiRenderer::Present(bind(&MainComponent::Compose, this));
}

void MainComponent::Update()
{
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

