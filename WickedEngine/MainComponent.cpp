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
	FRAMESKIP_START(getFrameSkip(), 10)

	Update();

	FRAMESKIP_END

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

