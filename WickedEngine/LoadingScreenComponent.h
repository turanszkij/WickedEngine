#pragma once
#include "Renderable2DComponent.h"

#include <thread>
#include <functional>
#include <atomic>

class MainComponent;

class LoadingScreenComponent :
	public Renderable2DComponent
{
private:
	struct LoaderTask
	{
		std::function< void() > functionBody;
		std::atomic_bool active;

		LoaderTask(std::function< void() > functionBody) :functionBody(functionBody)
		{
			active.store(false);
		}
		LoaderTask(const LoaderTask& l)
		{
			functionBody = l.functionBody;
			active.store(l.active.load());
		}
	};
	std::vector< LoaderTask > loaders;
	void doLoadingTasks();

	void waitForFinish();
	std::function<void()> finish;
	std::thread worker;
public:
	LoadingScreenComponent();
	virtual ~LoadingScreenComponent();

	//Add a loading task which should be executed
	//use std::bind( YourFunctionPointer )
	void addLoadingFunction(std::function<void()> loadingFunction);
	//Helper for loading a whole renderable component
	void addLoadingComponent(RenderableComponent* component, MainComponent* main);
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

