#pragma once
#include "RenderPath2D.h"
#include "wiColor.h"
#include "wiJobSystem.h"

#include <functional>

class MainComponent;

class LoadingScreen :
	public RenderPath2D
{
private:
	wiJobSystem::context ctx_main;
	wiJobSystem::context ctx_finish;
	std::vector<std::function<void(wiJobArgs)>> tasks;
	std::function<void()> finish;
public:

	//Add a loading task which should be executed
	//use std::bind( YourFunctionPointer )
	void addLoadingFunction(std::function<void(wiJobArgs)> loadingFunction);
	//Helper for loading a whole renderable component
	void addLoadingComponent(RenderPath* component, MainComponent* main, float fadeSeconds = 0, wiColor fadeColor = wiColor(0, 0, 0, 255));
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
};

