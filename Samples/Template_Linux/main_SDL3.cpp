#include "WickedEngine.h"
#include <SDL3/SDL.h>

wi::Application application;

int main(int argc, char *argv[])
{
	// SDL window setup:
	// SDL3 dropped SDL_INIT_EVERYTHING; list the subsystems the app actually needs instead.
    sdl3::sdlsystem_ptr_t system = sdl3::make_sdlsystem(
        SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
    sdl3::window_ptr_t window = sdl3::make_window(
            "WickedEngineApplicationTemplate",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1920, 1080,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    SDL_Event event;

	// set SDL window to engine:
    application.SetWindow(window.get());

	// process command line string:
	wi::arguments::Parse(argc, argv);

	// just show some basic info:
    application.infoDisplay.active = true;
    application.infoDisplay.watermark = true;
    application.infoDisplay.resolution = true;
    application.infoDisplay.fpsinfo = true;

	bool quit = false;
	while (!quit)
	{
		SDL_PumpEvents();
		application.Run();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EVENT_QUIT:
				quit = true;
				break;
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				quit = true;
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				application.SetWindow(application.window);
				break;
			default:
				break;
			}
			wi::input::sdlinput::ProcessEvent(event);
		}
	}

	wi::jobsystem::ShutDown(); // waits for jobs to finish before shutdown

	// Must run before SDL_Quit() (triggered below by the `system` RAII wrapper going out of
	// scope): wi::audio's internal state is otherwise only destroyed as a static at process
	// exit, which runs after SDL_Quit() has already torn down the audio subsystem -- the
	// FAudio SDL3 platform backend's device teardown then dereferences an already-invalidated
	// SDL3 audio device handle and crashes.
	wi::audio::Deinitialize();

    SDL_Quit();

    return 0;
}
