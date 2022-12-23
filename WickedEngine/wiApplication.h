#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"
#include "wiResourceManager.h"
#include "wiColor.h"
#include "wiFadeManager.h"
#include "wiGraphics.h"
#include "wiEventHandler.h"
#include "wiCanvas.h"

#include <memory>
#include <string>

namespace wi
{
	class RenderPath;

	// This can be used as a Wicked Engine application
	//	- SetWindow() will set a window from operating system and initialize graphics device
	//	- Run() will run the main application logic. It needs to be called every frame
	class Application
	{
	protected:
		std::unique_ptr<wi::graphics::GraphicsDevice> graphicsDevice;
		wi::eventhandler::Handle swapChainVsyncChangeEvent;

		RenderPath* activePath = nullptr;
		float targetFrameRate = 60;
		bool frameskip = true;
		bool framerate_lock = false;
		bool initialized = false;

		wi::FadeManager fadeManager;

		float deltaTime = 0;
		float deltaTimeAccumulator = 0;
		wi::Timer timer;

		float deltatimes[20] = {};
		int fps_avg_counter = 0;

		// These are used when HDR10 color space is active:
		//	Because we want to blend in linear color space, but HDR10 is non-linear
		wi::graphics::Texture rendertarget;

		std::string infodisplay_str;

	public:
		virtual ~Application() = default;

		bool is_window_active = true;
		bool allow_hdr = true;
		wi::graphics::SwapChain swapChain;
		wi::Canvas canvas;
		wi::platform::window_type window;

		// Runs the main engine loop
		void Run();

		// This will activate a RenderPath as the active one, so it will run its Update, FixedUpdate, Render and Compose functions
		//	You can set a fade time and fade screen color so that switching components will happen when the screen is faded out. Then it will fade back to the new component
		void ActivatePath(RenderPath* component, float fadeSeconds = 0, wi::Color fadeColor = wi::Color(0, 0, 0, 255));
		inline RenderPath* GetActivePath() { return activePath; }

		// Set the desired target framerate for the FixedUpdate() loop (default = 60)
		void setTargetFrameRate(float value) { targetFrameRate = value; }
		// Get the desired target framerate for the FixedUpdate() loop
		float getTargetFrameRate() const { return targetFrameRate; }
		// Set the desired behaviour of the FixedUpdate() loop (default = true)
		//	enabled		: the FixedUpdate() loop will run at targetFrameRate frequency
		//	disabled	: the FixedUpdate() loop will run every frame only once.
		void setFrameSkip(bool enabled) { frameskip = enabled; }
		void setFrameRateLock(bool enabled) { framerate_lock = enabled; }

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
		virtual void Compose(wi::graphics::CommandList cmd);

		// You need to call this before calling Run() or Initialize() if you want to render
		void SetWindow(wi::platform::window_type);


		struct InfoDisplayer
		{
			// activate the whole display
			bool active = false;
			// display engine version number
			bool watermark = true;
			// display framerate
			bool fpsinfo = false;
			// display graphics device name
			bool device_name = false;
			// display resolution info
			bool resolution = false;
			// window's size in logical (DPI scaled) units
			bool logical_size = false;
			// HDR status and color space
			bool colorspace = false;
			// display number of heap allocations per frame
			bool heap_allocation_counter = false;
			// display the active graphics pipeline count
			bool pipeline_count = false;
			// display video memory usage and budget
			bool vram_usage = false;
			// text size
			int size = 16;
			// display default color grading helper texture in top left corner of the screen
			bool colorgrading_helper = false;
		};
		// display all-time engine information text
		InfoDisplayer infoDisplay;

	};

}
