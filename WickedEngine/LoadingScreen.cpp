#include "LoadingScreen.h"
#include "MainComponent.h"

using namespace std;

bool LoadingScreen::isActive()
{
	return wiJobSystem::IsBusy(ctx_main) || wiJobSystem::IsBusy(ctx_finish);
}

void LoadingScreen::addLoadingFunction(function<void(wiJobArgs)> loadingFunction)
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

void LoadingScreen::onFinished(function<void()> finishFunction)
{
	if (finishFunction != nullptr)
		finish = finishFunction;
}

void LoadingScreen::Start()
{
	for (auto& x : tasks)
	{
		wiJobSystem::Execute(ctx_main, x);
	}
	wiJobSystem::Execute(ctx_finish, [this](wiJobArgs args) {
		wiJobSystem::Wait(ctx_main);
		finish();
	});

	RenderPath2D::Start();
}

void LoadingScreen::Stop()
{
	tasks.clear();
	finish = nullptr;

	RenderPath2D::Stop();
}

