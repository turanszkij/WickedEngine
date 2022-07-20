#include "stdafx.h"
#include <SDL2/SDL.h>

int main(void)
{
    wi::Application application;

    application.infoDisplay.active = true;
    application.infoDisplay.watermark = true;
    application.infoDisplay.resolution = true;
    application.infoDisplay.fpsinfo = true;

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);

    sdl2::window_ptr_t window = sdl2::make_window(
            "Template",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            2560, 1440,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);

    application.SetWindow(window.get());

    while(true) 
    {
        application.Run(); 
    }

    return 0;
}