#pragma once
#include "CommonInclude.h"
#include "wiResourceManager.h"
#include "wiColor.h"
#include "wiFadeManager.h"
#include "wiWindowRegistration.h"

class RenderableComponent;


class MainComponent
{
private:
	RenderableComponent* activeComponent;
	bool frameskip;
	int targetFrameRate;
	double targetFrameRateInv;
	int applicationControlLostThreshold;

	wiFadeManager fadeManager;
public:
	MainComponent();
	virtual ~MainComponent();

	int screenW, screenH;
	bool fullscreen;

	// Runs the main engine loop
	void Run();

	void activateComponent(RenderableComponent* component, int fadeFrames = 0, const wiColor& fadeColor = wiColor(0,0,0,255));
	RenderableComponent* getActiveComponent(){ return activeComponent; }

	wiResourceManager Content;

	void	setFrameSkip(bool value){ frameskip = value; }
	bool	getFrameSkip(){ return frameskip; }
	void	setTargetFrameRate(int value){ targetFrameRate = value; targetFrameRateInv = 1.0 / (double)targetFrameRate; }
	int		getTargetFrameRate(){ return targetFrameRate; }
	void	setApplicationControlLostThreshold(int value){ applicationControlLostThreshold = value; }

	// Initializes all engine components
	virtual void Initialize();
	virtual void Update(float dt);
	virtual void FixedUpdate();
	virtual void Render();
	virtual void Compose();

	wiWindowRegistration::window_type window;

#ifndef WINSTORE_SUPPORT
	HINSTANCE instance;
	bool SetWindow(wiWindowRegistration::window_type window, HINSTANCE hInst);
#else
	bool SetWindow(wiWindowRegistration::window_type window);
#endif


	struct InfoDisplayer
	{
		// activate the whole display
		bool active;
		// display engine version number
		bool watermark;
		// display framerate
		bool fpsinfo;
		// display cpu utilization
		bool cpuinfo;
		// display resolution info
		bool resolution;
		// text size
		int size;

		InfoDisplayer() :active(false), watermark(true), fpsinfo(false), cpuinfo(false), 
			resolution(false), size(-1)
		{}
	};
	// display all-time engine information text
	InfoDisplayer infoDisplay;
};

