// WickedEngineTests.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "sdl2.h"
#include "Wayland/wiWaylandBackend.h"
#include "Wayland/wiWaylandWindow.h"
#include <wiPlatform.h>

int sdl_loop(Tests &tests)
{
    bool quit = false;
    while (!quit)
    {
        tests.Run();
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE: // exit tests
                            quit = true;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            // Tells the engine to reload window configuration (size and dpi)
                            tests.SetWindow(tests.window);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            tests.is_window_active = false;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            tests.is_window_active = true;
                            if (wi::shadercompiler::GetRegisteredShaderCount() > 0)
                            {
                                std::thread([] {
                                    wi::backlog::post("[Shader check] Started checking " + std::to_string(wi::shadercompiler::GetRegisteredShaderCount()) + " registered shaders for changes...");
                                    if (wi::shadercompiler::CheckRegisteredShadersOutdated())
                                    {
                                        wi::backlog::post("[Shader check] Changes detected, initiating reload...");
                                        wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
                                            wi::renderer::ReloadShaders();
                                            });
                                    }
                                    else
                                    {
                                        wi::backlog::post("[Shader check] All up to date");
                                    }
                                    }).detach();
                            }
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

    return 0;
}

int main_sdl2(int argc, char *argv[])
{
    Tests tests;
    // TODO: Place code here.

    wi::arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (*system) {
        throw sdl2::SDLError("Error creating SDL2 system");
    }

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Engine Tests",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 800,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        throw sdl2::SDLError("Error creating window");
    }

    tests.SetWindow(window.get());

    int ret = sdl_loop(tests);

    SDL_Quit();
    return ret;
}


void wayland_loop(wi::wayland::Backend &wwbackend, Tests &tests)
{
	bool quit = false;
	wwbackend.window.on_close = [&quit](wi::wayland::Window*w) { quit = true; };

	while(!quit)
	{
		tests.Run();
		if (wwbackend.dispatch() < 0)
			break;
		if (wwbackend.window.AcceptDesiredDimensions())
			tests.SetWindow(&wwbackend.window);
	}
}


bool main_wayland(int argc, char *argv[])
{
	wi::wayland::Backend wwbackend;
	if (!wwbackend.Init())
		return false;
	if (!wwbackend.CreateWindow("Wicked Engine Tests"))
		return false;

	Tests tests;
	tests.SetWindow(&wwbackend.window);
	wayland_loop(wwbackend, tests);

	wwbackend.window.Deinit();
	wwbackend.DeInit();

	return true;
}

int main(int argc, char *argv[])
{
	// Attempt to initialize wayland backend
	if (!main_wayland(argc, argv))
		main_sdl2(argc, argv);
}
