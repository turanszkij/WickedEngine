#include "LoadingScreenComponent.h"
#include "MainComponent.h"

using namespace std;

LoadingScreenComponent::LoadingScreenComponent() : Renderable2DComponent()
{
	loaders.clear();
	finish = nullptr;
}


LoadingScreenComponent::~LoadingScreenComponent()
{
}

bool LoadingScreenComponent::isActive()
{
	for (LoaderTask& x : loaders)
	{
		if (x.active.load())
		{
			return true;
		}
	}
	return false;
}

void LoadingScreenComponent::addLoadingFunction(function<void()> loadingFunction)
{
	if (loadingFunction != nullptr)
	{
		loaders.push_back(LoaderTask(loadingFunction));
	}
}

void LoadingScreenComponent::addLoadingComponent(RenderableComponent* component, MainComponent* main)
{
	addLoadingFunction([=] {
		component->Load();
	});
	onFinished([=] {
		main->activateComponent(component);
	});
	//addLoadingFunction(bind(&RenderableComponent::Load, this, component));
	//onFinished(bind(&RenderableComponent::Start, this, component));
}

void LoadingScreenComponent::onFinished(function<void()> finishFunction)
{
	if (finishFunction != nullptr)
		finish = finishFunction;
}

void LoadingScreenComponent::waitForFinish()
{
	worker.join();
	if (finish != nullptr)
		finish();
}

void LoadingScreenComponent::doLoadingTasks()
{
	std::vector<thread> loaderThreads(0);

	for (LoaderTask& x : loaders)
	{
		x.active.store(true);
		loaderThreads.push_back(thread(x.functionBody));
	}

	int i = 0;
	for (thread& x : loaderThreads)
	{
		x.join();
		loaders[i].active.store(false);
		i++;
	}
}

int LoadingScreenComponent::getPercentageComplete()
{
	const int numberOfLoaders = (int)loaders.size();
	int completed = 0;

	for (LoaderTask& x : loaders)
	{
		if (!x.active.load())
		{
			completed++;
		}
	}

	return (int)(((float)completed / (float)numberOfLoaders)*100.f);
}

void LoadingScreenComponent::Unload()
{
	Renderable2DComponent::Unload();
}

void LoadingScreenComponent::Start()
{
	worker = thread(&LoadingScreenComponent::doLoadingTasks, this);
	thread(&LoadingScreenComponent::waitForFinish, this).detach();

	Renderable2DComponent::Start();
}

void LoadingScreenComponent::Stop()
{
	loaders.clear();
	finish = nullptr;

	Renderable2DComponent::Stop();
}

