// WickedEngineTests.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <SDL2/SDL.h>
#include "sdl2.h"

int sdl_loop(Tests &tests)
{
    bool quit = false;
    while (!quit)
    {
        tests.Run();
        for(auto& event : *wi::input::sdlinput::GetExternalEvents()){
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
                            //tests.is_window_active = false;
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
        }
        wi::input::sdlinput::FlushExternalEvents();
    }

    return 0;
}

int main(int argc, char *argv[])
{
    Tests tests;
    // TODO: Place code here.

    wi::arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (!system) {
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
