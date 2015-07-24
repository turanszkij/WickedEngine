#pragma once
#include "CommonInclude.h"

class RenderableComponent;

class MainComponent
{
private:
	RenderableComponent* activeComponent;
	bool frameskip;
public:
	MainComponent();
	~MainComponent();

	int screenW, screenH;

	void run();

	void activateComponent(RenderableComponent* component);
	RenderableComponent* getActiveComponent(){ return activeComponent; }

	void setFrameSkip(bool value){ frameskip = value; }
	bool getFrameSkip(){ return frameskip; }

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

