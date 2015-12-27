#include "LoadingScreenComponent.h"


LoadingScreenComponent::LoadingScreenComponent()
{
	Renderable2DComponent::Renderable2DComponent();
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

void LoadingScreenComponent::addLoadingComponent(RenderableComponent* component)
{
	//TODO
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
	vector<thread> loaderThreads(0);

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
	const int numberOfLoaders = loaders.size();
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
	loaders.clear();
	finish = nullptr;
}

void LoadingScreenComponent::Start()
{
	worker = thread(&LoadingScreenComponent::doLoadingTasks, this);
	thread(&LoadingScreenComponent::waitForFinish, this).detach();

	Renderable2DComponent::Start();
}

