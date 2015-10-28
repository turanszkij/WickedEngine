#pragma once
#include "CommonInclude.h"
#include "wiResourceManager.h"
#include "wiCVars.h"

class RenderableComponent;

class MainComponent
{
private:
	RenderableComponent* activeComponent;
	bool frameskip;
	int targetFrameRate;
	double targetFrameRateInv;
	int applicationControlLostThreshold;
public:
	MainComponent();
	~MainComponent();

	int screenW, screenH;
	bool fullscreen;

	void run();

	void activateComponent(RenderableComponent* component);
	RenderableComponent* getActiveComponent(){ return activeComponent; }

	wiCVars Props;
	wiResourceManager Content;

	void	setFrameSkip(bool value){ frameskip = value; }
	bool	getFrameSkip(){ return frameskip; }
	void	setTargetFrameRate(int value){ targetFrameRate = value; targetFrameRateInv = 1.0 / (double)targetFrameRate; }
	int		getTargetFrameRate(){ return targetFrameRate; }
	void	setApplicationControlLostThreshold(int value){ applicationControlLostThreshold = value; }

	virtual void Initialize();
	virtual void Update();
	virtual void Render();
	virtual void Compose();


#ifndef WINSTORE_SUPPORT
	HWND window;
	HINSTANCE instance;
	bool setWindow(HWND hWnd, HINSTANCE hInst);
#else
	Windows::UI::Core::CoreWindow^ window;
	bool setWindow(Windows::UI::Core::CoreWindow^ window);
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
		// text size
		float size;

		InfoDisplayer() :active(false), watermark(true), fpsinfo(true), cpuinfo(true), size(-7.f)
		{}
	};
	// display all-time engine information text
	InfoDisplayer infoDisplay;

	// display the default color grading palette
	bool colorGradingPaletteDisplayEnabled;
};

