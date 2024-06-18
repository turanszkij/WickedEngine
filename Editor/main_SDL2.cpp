#include "stdafx.h"
#include "Editor.h"

#include "sdl2.h"
#include <fstream>

#include "icon.c"

using namespace std;

int sdl_loop(Editor &editor)
{
    bool quit = false;
    while (!quit)
    {
        editor.Run();
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            bool textinput_action_delete = false;
            switch(event.type){
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE: // exit editor
                            quit = true;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            // Tells the engine to reload window configuration (size and dpi)
                            editor.SetWindow(editor.window);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            editor.is_window_active = false;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            editor.is_window_active = true;
							editor.HotReload();
                            break;
                        default:
                            break;
                    }
                case SDL_KEYDOWN:
                    if(event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE 
                        || event.key.keysym.scancode == SDL_SCANCODE_DELETE
                        || event.key.keysym.scancode == SDL_SCANCODE_KP_BACKSPACE){
                            wi::gui::TextInputField::DeleteFromInput();
                            textinput_action_delete = true;
                        }
                    break;
                case SDL_TEXTINPUT:
                    if(!textinput_action_delete){
                        if(event.text.text[0] >= 21){
                            wi::gui::TextInputField::AddInput(event.text.text[0]);
                        }
                    }
                    break;
                default:
                    break;
            }
            wi::input::sdlinput::ProcessEvent(event);
        }
    }

    return 0;
}

void set_window_icon(SDL_Window *window) {
    // these masks are needed to tell SDL_CreateRGBSurface(From)
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
    SDL_Surface* icon = SDL_CreateRGBSurfaceFrom((void*)gimp_image.pixel_data, gimp_image.width,
        gimp_image.height, gimp_image.bytes_per_pixel*8, gimp_image.bytes_per_pixel*gimp_image.width,
        rmask, gmask, bmask, amask);

    SDL_SetWindowIcon(window, icon);
 
    SDL_FreeSurface(icon);
}

int main(int argc, char *argv[])
{
    Editor editor;

    wi::arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (!system) {
        throw sdl2::SDLError("Error creating SDL2 system");
    }

	int width = 1920;
	int height = 1080;
	bool fullscreen = false;

	wi::Timer timer;
	if (editor.config.Open("config.ini"))
	{
		if (editor.config.Has("width"))
		{
			width = editor.config.GetInt("width");
			height = editor.config.GetInt("height");
		}
		fullscreen = editor.config.GetBool("fullscreen");
		editor.allow_hdr = editor.config.GetBool("allow_hdr");

		wi::backlog::post("config.ini loaded in " + std::to_string(timer.elapsed_milliseconds()) + " milliseconds\n");
	}

	width = std::max(100, width);
	height = std::max(100, height);

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Engine Editor",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        throw sdl2::SDLError("Error creating window");
    }

    set_window_icon(window.get());

	if (fullscreen)
	{
		//SDL_SetWindowFullscreen(window.get(), SDL_TRUE);
		//SDL_SetWindowFullscreen(window.get(), SDL_WINDOW_FULLSCREEN);
		SDL_SetWindowFullscreen(window.get(), SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

    editor.SetWindow(window.get());

    int ret = sdl_loop(editor);

    return ret;
}
