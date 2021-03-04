#include "stdafx.h"
#include "Editor.h"

#include <fstream>

#include "stdafx.h"
#include <SDL2/SDL.h>
#include "sdl2.h"

using namespace std;

int sdl_loop(Editor &editor)
{
    SDL_Event event;

    bool quit = false;
    while (!quit)
    {
        SDL_PumpEvents();
        editor.Run();

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
                        //editor.Quit();
                        quit = true;

                    default:
                        break;
                }
            }
        }

    }

    return 0;
}

int main(int argc, char *argv[])
{
    Editor editor;

    wiStartupArguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (!system) {
        throw sdl2::SDLError("Error creating SDL2 system");
    }

    int x = 0, y = 0, w = 1920, h = 1080;
	bool fullscreen = false;
    bool borderless = false;
    string voidStr = "";

    ifstream file("config.ini");
    if (file.is_open())
    {
        int enabled;
        file >> voidStr >> enabled;
        if (enabled != 0)
        {
            file >> voidStr >> x >> voidStr >> y >> voidStr >> w >> voidStr >> h >> voidStr >> fullscreen >> voidStr >> borderless;
        }
    }
    file.close();

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Engine Editor",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            w, h,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    if (!window) {
        throw sdl2::SDLError("Error creating window");
    }

    if(fullscreen) SDL_SetWindowFullscreen(window.get(), SDL_TRUE);

    editor.SetWindow(window.get());

    int ret = sdl_loop(editor);

    SDL_Quit();
    return ret;
}
