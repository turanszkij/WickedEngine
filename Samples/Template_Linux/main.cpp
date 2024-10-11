#include "stdafx.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

int sdl_loop(wi::Application &application)
{
    SDL_Event event;

    bool quit = false;
    while (!quit)
    {
        SDL_PumpEvents();
        application.Run();

        while( SDL_PollEvent(&event))
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
        }
    }

    return 0;

}

int main(int argc, char *argv[])
{
    wi::Application application;
    #ifdef WickedEngine_SHADER_DIR
    wi::renderer::SetShaderSourcePath(WickedEngine_SHADER_DIR);
    #endif

    application.infoDisplay.active = true;
    application.infoDisplay.watermark = true;
    application.infoDisplay.resolution = true;
    application.infoDisplay.fpsinfo = true;

    sdl3::sdlsystem_ptr_t system = sdl3::make_sdlsystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    sdl3::window_ptr_t window = sdl3::make_window(
            "Template",
            2560, 1440,
            SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    SDL_Event event;

    application.SetWindow(window.get());

    int ret = sdl_loop(application);

    SDL_Quit();

    return ret;
}
