#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"
#include "wiResourceManager.h"
#include "wiColor.h"
#include "wiFadeManager.h"

class RenderPath;

class MainComponent
{
protected:
	RenderPath* activePath = nullptr;
	float targetFrameRate = 60;
	bool frameskip = true;
	bool framerate_lock = false;
	bool initialized = false;

	wiFadeManager fadeManager;

	float deltaTime = 0;
	float deltaTimeAccumulator = 0;
	wiTimer timer;

	float deltatimes[20] = {};
	int fps_avg_counter = 0;

public:
	bool fullscreen = false;

	// Runs the main engine loop
	void Run();

	// This will activate a RenderPath as the active one, so it will run its Update, FixedUpdate, Render and Compose functions
	//	You can set a fade time and fade screen color so that switching components will happen when the screen is faded out. Then it will fade back to the new component
	void ActivatePath(RenderPath* component, float fadeSeconds = 0, wiColor fadeColor = wiColor(0,0,0,255));
	inline RenderPath* GetActivePath(){ return activePath; }

	// Set the desired target framerate for the FixedUpdate() loop (default = 60)
	void	setTargetFrameRate(float value) { targetFrameRate = value; }
	// Get the desired target framerate for the FixedUpdate() loop
	float	getTargetFrameRate() const { return targetFrameRate; }
	// Set the desired behaviour of the FixedUpdate() loop (default = true)
	//	enabled		: the FixedUpdate() loop will run at targetFrameRate frequency
	//	disabled	: the FixedUpdate() loop will run every frame only once.
	void	setFrameSkip(bool enabled) { frameskip = enabled; }
	void	setFrameRateLock(bool enabled) { framerate_lock = enabled; }

	// This is where the critical initializations happen (before any rendering or anything else)
	virtual void Initialize();
	// This is where application-wide updates get executed once per frame. 
	//  RenderPath::Update is also called from here for the active component
	virtual void Update(float dt);
	// This is where application-wide updates get executed in a fixed timestep based manner. 
	//  RenderPath::FixedUpdate is also called from here for the active component
	virtual void FixedUpdate();
	// This is where application-wide rendering happens to offscreen buffers. 
	//  RenderPath::Render is also called from here for the active component
	virtual void Render();
	// This is where the application will render to the screen (backbuffer). It must render to the provided command list.
	virtual void Compose(wiGraphics::CommandList cmd);

	// You need to call this before calling Run() or Initialize() if you want to render
	void SetWindow(wiPlatform::window_type);


	struct InfoDisplayer
	{
		// activate the whole display
		bool active = false;
		// display engine version number
		bool watermark = true;
		// display framerate
		bool fpsinfo = false;
		// display resolution info
		bool resolution = false;
		// display number of heap allocations per frame
		bool heap_allocation_counter = false;
		// text size
		int size = 16;
	};
	// display all-time engine information text
	InfoDisplayer infoDisplay;
};

