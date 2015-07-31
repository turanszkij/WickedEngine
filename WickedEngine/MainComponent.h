#pragma once
#include "CommonInclude.h"
#include "wiResourceManager.h"

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

	void run();

	void activateComponent(RenderableComponent* component);
	RenderableComponent* getActiveComponent(){ return activeComponent; }

	wiResourceManager Content;

	void	setFrameSkip(bool value){ frameskip = value; }
	bool	getFrameSkip(){ return frameskip; }
	void	setTargetFrameRate(int value){ targetFrameRate = value; targetFrameRateInv = 1.0 / (double)targetFrameRate; }
	int		getTargetFrameRate(){ return targetFrameRate; }
	void	setApplicationControlLostThreshold(int value){ applicationControlLostThreshold = value; }

	virtual void Initialize(){};
	virtual void Update();
	virtual void Render();
	virtual void Compose();


#ifndef WINSTORE_SUPPORT
	HWND window;
	HINSTANCE instance;
	bool setWindow(HWND hWnd, HINSTANCE hInst);
#endif

};

