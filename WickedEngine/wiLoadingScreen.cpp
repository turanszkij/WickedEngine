#include "wiLoadingScreen.h"
#include "wiApplication.h"

#include <thread>

namespace wi
{

	bool LoadingScreen::isActive()
	{
		return wi::jobsystem::IsBusy(ctx);
	}

	void LoadingScreen::addLoadingFunction(std::function<void(wi::jobsystem::JobArgs)> loadingFunction)
	{
		if (loadingFunction != nullptr)
		{
			tasks.push_back(loadingFunction);
		}
	}

	void LoadingScreen::addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds, wi::Color fadeColor)
	{
		addLoadingFunction([=](wi::jobsystem::JobArgs args) {
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
			wi::jobsystem::Execute(ctx, x);
		}
		std::thread([this]() {
			wi::jobsystem::Wait(ctx);
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

}
