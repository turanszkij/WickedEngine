#include "LoadingScreen.h"
#include "MainComponent.h"

#include <thread>

bool LoadingScreen::isActive()
{
	return wiJobSystem::IsBusy(ctx);
}

void LoadingScreen::addLoadingFunction(std::function<void(wiJobArgs)> loadingFunction)
{
	if (loadingFunction != nullptr)
	{
		tasks.push_back(loadingFunction);
	}
}

void LoadingScreen::addLoadingComponent(RenderPath* component, MainComponent* main, float fadeSeconds, wiColor fadeColor)
{
	addLoadingFunction([=](wiJobArgs args) {
		component->Load();
	});
	onFinished([=] {
		main->ActivatePath(component, fadeSeconds, fadeColor);
	});
}

void LoadingScreen::onFinished(std::function<void()> finishFunction)
{
	if (finishFunction != nullptr)
		finish = finishFunction;
}

void LoadingScreen::Start()
{
	for (auto& x : tasks)
	{
		wiJobSystem::Execute(ctx, x);
	}
	std::thread([this]() {
		wiJobSystem::Wait(ctx);
		finish();
	}).detach();

	RenderPath2D::Start();
}

void LoadingScreen::Stop()
{
	tasks.clear();
	finish = nullptr;

	RenderPath2D::Stop();
}

