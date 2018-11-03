#include "LoadingScreen.h"
#include "MainComponent.h"

using namespace std;

LoadingScreen::LoadingScreen() : RenderPath2D()
{
	loaders.clear();
	finish = nullptr;
}


LoadingScreen::~LoadingScreen()
{
}

bool LoadingScreen::isActive()
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

void LoadingScreen::addLoadingFunction(function<void()> loadingFunction)
{
	if (loadingFunction != nullptr)
	{
		loaders.push_back(LoaderTask(loadingFunction));
	}
}

void LoadingScreen::addLoadingComponent(RenderPath* component, MainComponent* main)
{
	addLoadingFunction([=] {
		component->Load();
	});
	onFinished([=] {
		main->ActivatePath(component);
	});
	//addLoadingFunction(bind(&RenderPath::Load, this, component));
	//onFinished(bind(&RenderPath::Start, this, component));
}

void LoadingScreen::onFinished(function<void()> finishFunction)
{
	if (finishFunction != nullptr)
		finish = finishFunction;
}

void LoadingScreen::waitForFinish()
{
	worker.join();
	if (finish != nullptr)
		finish();
}

void LoadingScreen::doLoadingTasks()
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

int LoadingScreen::getPercentageComplete()
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

void LoadingScreen::Unload()
{
	RenderPath2D::Unload();
}

void LoadingScreen::Start()
{
	worker = thread(&LoadingScreen::doLoadingTasks, this);
	thread(&LoadingScreen::waitForFinish, this).detach();

	RenderPath2D::Start();
}

void LoadingScreen::Stop()
{
	loaders.clear();
	finish = nullptr;

	RenderPath2D::Stop();
}

