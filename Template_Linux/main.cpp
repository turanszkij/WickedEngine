#include "sdl2.h"
#include "stdafx.h"
#include "Icon.h"

#include <iostream>
#include <SDL2/SDL.h>

using std::cout;

int sdl_loop(App &exampleApp)
{
    SDL_Event event;

    bool quit = false;
    while (!quit)
    {
        SDL_PumpEvents();
        exampleApp.Run();

        while( SDL_PollEvent(&event)) 
        {
            switch (event.type) 
            {
                case SDL_QUIT:      
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) 
                    {
                    case SDL_WINDOWEVENT_CLOSE:   // exit game
                        quit = true;
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        // Tells the engine to reload window configuration (size and dpi)
                        exampleApp.SetWindow(exampleApp.window);
                        break;
                    default:
                        break;
                    }
                default:
                    break;
            }
        }
    }
}

void set_window_icon(SDL_Window *window) {
  // these masks are necessary to tell SDL_CreateRGBSurface(From)
  // to assume the data it gets is byte-wise RGB(A) data
  Uint32 rmask, gmask, bmask, amask;
  
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int shift = (gimp_image.bytes_per_pixel == 3) ? 8 : 0;
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x000000ff >> shift;
    #else // little endian, like x86
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = (gimp_image.bytes_per_pixel == 3) ? 0 : 0xff000000;
    #endif

    SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(
        (void *)gimp_image.pixel_data, gimp_image.width, gimp_image.height,
        gimp_image.bytes_per_pixel * 8,
        gimp_image.bytes_per_pixel * gimp_image.width, rmask, gmask, bmask,
        amask);

  SDL_SetWindowIcon(window, icon);

  SDL_FreeSurface(icon);
}

int main(int argc, char *argv[])
{
    App exampleApp;

    wi::arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (!system) 
    {
        throw sdl2::SDLError("Error creating SDL2 system!");
    }

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Engine exampleApp",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            2560, 1440,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
            
    if (!window) 
    {
        throw sdl2::SDLError("Error creating window");
    }

    set_window_icon(window.get());

    exampleApp.SetWindow(window.get());

    int appReturn = sdl_loop(exampleApp);

    SDL_Quit();
    
    return appReturn;
}
