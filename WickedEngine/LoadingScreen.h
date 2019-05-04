#pragma once
#include "RenderPath2D.h"
#include "wiColor.h"

#include <thread>
#include <functional>
#include <atomic>

class MainComponent;

class LoadingScreen :
	public RenderPath2D
{
private:
	struct LoaderTask
	{
		std::function<void()> functionBody;
		std::atomic_bool active;

		LoaderTask(std::function<void()> functionBody) :functionBody(functionBody)
		{
			active.store(false);
		}
		LoaderTask(const LoaderTask& l)
		{
			functionBody = l.functionBody;
			active.store(l.active.load());
		}
	};
	std::vector<LoaderTask> loaders;
	void doLoadingTasks();

	void waitForFinish();
	std::function<void()> finish;
	std::thread worker;
public:
	LoadingScreen();
	virtual ~LoadingScreen();

	//Add a loading task which should be executed
	//use std::bind( YourFunctionPointer )
	void addLoadingFunction(std::function<void()> loadingFunction);
	//Helper for loading a whole renderable component
	void addLoadingComponent(RenderPath* component, MainComponent* main, float fadeSeconds = 0, const wiColor& fadeColor = wiColor(0, 0, 0, 255));
	//Set a function that should be called when the loading finishes
	//use std::bind( YourFunctionPointer )
	void onFinished(std::function<void()> finishFunction);
	//Get percentage of finished loading tasks (values 0-100)
	int getPercentageComplete();
	//See if the loading is currently running
	bool isActive();

	//Start Executing the tasks and mark the loading as active
	virtual void Start() override;
	//Clear all tasks
	virtual void Stop() override;

	virtual void Unload() override;
};

