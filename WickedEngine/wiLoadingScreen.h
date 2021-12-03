#pragma once
#include "wiRenderPath2D.h"
#include "wiColor.h"
#include "wiJobSystem.h"
#include "wiVector.h"

#include <functional>

namespace wi
{

	class Application;

	class LoadingScreen :
		public RenderPath2D
	{
	private:
		wi::jobsystem::context ctx;
		wi::vector<std::function<void(wi::jobsystem::JobArgs)>> tasks;
		std::function<void()> finish;
	public:

		//Add a loading task which should be executed
		//use std::bind( YourFunctionPointer )
		void addLoadingFunction(std::function<void(wi::jobsystem::JobArgs)> loadingFunction);
		//Helper for loading a whole renderable component
		void addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds = 0, wi::Color fadeColor = wi::Color(0, 0, 0, 255));
		//Set a function that should be called when the loading finishes
		//use std::bind( YourFunctionPointer )
		void onFinished(std::function<void()> finishFunction);
		//See if the loading is currently running
		bool isActive();

		//Start Executing the tasks and mark the loading as active
		virtual void Start() override;
		//Clear all tasks
		virtual void Stop() override;
	};

}
