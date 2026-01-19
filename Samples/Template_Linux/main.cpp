#include "WickedEngine.h"
#include <SDL2/SDL.h>

wi::Application* application;

int main(int argc, char *argv[])
{
	application = new wi::Application();
	// SDL window setup:
    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    sdl2::window_ptr_t window = sdl2::make_window(
            "WickedEngineApplicationTemplate",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1920, 1080,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_Event event;

	// set SDL window to engine:
    application->SetWindow(window.get());

	// process command line string:
	wi::arguments::Parse(argc, argv);

	// just show some basic info:
    application->infoDisplay.active = true;
    application->infoDisplay.watermark = true;
    application->infoDisplay.resolution = true;
    application->infoDisplay.fpsinfo = true;

	bool quit = false;
	while (!quit)
	{
		SDL_PumpEvents();
		application->Run();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_CLOSE:
					quit = true;
					break;
				case SDL_WINDOWEVENT_RESIZED:
					application->SetWindow(application->window);
					break;
				default:
					break;
				}
			default:
				break;
			}
			wi::input::sdlinput::ProcessEvent(event);
		}
	}

	wi::jobsystem::ShutDown(); // waits for jobs to finish before shutdown

    // explicitly deleting prevents issues with Vulkan debug layers (debugdevice)
    // which don't like vulkan calls happening during C++ application shutdown
    delete application;

    SDL_Quit();

    return 0;
}
