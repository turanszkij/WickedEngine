// WickedEngineTests.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <SDL2/SDL.h>
#include "sdl2.h"

int sdl_loop(Tests &tests)
{
    SDL_Event event;

    bool quit = false;
    while (!quit)
    {
        SDL_PumpEvents();
        tests.Run();

//        int ret = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
        int ret = SDL_PollEvent(&event);

        if (ret < 0) {
            std::cerr << "Error Peeping event: " << SDL_GetError() << std::endl;
            std::cerr << "Exiting now" << std::endl;
            return -1;
        }

        if (ret > 0) {
            if (event.type == SDL_WINDOWEVENT) {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:   // exit game
                        //tests.Quit();
                        quit = true;

                    default:
                        break;
                }
            }
//            else if (event.type == SDL_KEYDOWN) {
//                switch (event.key.keysym.scancode) {
//                    case SDL_SCANCODE_SPACE:
//                        tests.SetSelected(tests.GetSelected()+1);
//                }
//            }
        }

    }

    return 0;
}

int main(int argc, char *argv[])
{
    Tests tests;
    // TODO: Place code here.

    wiStartupArguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (!system) {
        throw sdl2::SDLError("Error creating SDL2 system");
    }

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Engine Tests",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 800,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    if (!window) {
        throw sdl2::SDLError("Error creating window");
    }

    tests.SetWindow(window.get());

    int ret = sdl_loop(tests);

    SDL_Quit();
    return ret;
}
