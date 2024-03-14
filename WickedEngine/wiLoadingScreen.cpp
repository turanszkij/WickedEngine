#include "wiLoadingScreen.h"
#include "wiApplication.h"
#include "wiEventHandler.h"

#include <thread>

using namespace wi::graphics;

namespace wi
{

	bool LoadingScreen::isActive() const
	{
		return wi::jobsystem::IsBusy(ctx);
	}

	bool LoadingScreen::isFinished() const
	{
		return tasks.empty();
	}

	int LoadingScreen::getProgress() const
	{
		if (launchedTasks == 0)
			return 100;
		uint32_t counter = ctx.counter.load();
		float percent = 1 - float(counter) / float(launchedTasks);
		return (int)std::round(percent * 100);
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
		launchedTasks = (uint32_t)tasks.size();
		for (auto& x : tasks)
		{
			wi::jobsystem::Execute(ctx, x);
		}
		std::thread([this]() {
			wi::jobsystem::Wait(ctx);
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [this](uint64_t) {
				if (finish != nullptr)
					finish();
				tasks.clear();
				launchedTasks = 0;
				finish = nullptr;
			});
		}).detach();

		RenderPath2D::Start();
	}

	void LoadingScreen::Compose(wi::graphics::CommandList cmd) const
	{
		if (backgroundTexture.IsValid())
		{
			wi::image::Params fx;
			fx.enableFullScreen();
			fx.blendFlag = wi::enums::BLENDMODE_PREMULTIPLIED;
			if (colorspace != ColorSpace::SRGB)
			{
				// Convert the regular SRGB result of the render path to linear space for HDR compositing:
				fx.enableLinearOutputMapping(hdr_scaling);
			}
			const Texture& tex = backgroundTexture.GetTexture();
			wi::image::Draw(&tex, fx, cmd);
		}

		RenderPath2D::Compose(cmd);
	}

}
