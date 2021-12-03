#include "stdafx.h"
#include "Editor.h"

#include <SDL_events.h>
#include <SDL_scancode.h>
#include <fstream>
#include <thread>

#include "stdafx.h"
#include <SDL2/SDL.h>
#include "sdl2.h"

#include "Assets/Icon.c"

using namespace std;

//Convert SDL scancode to character input
//Keyboard layout is based on the standard US keyboard layout
char sdl_character(){
    int numkeys;
	const uint8_t *state = SDL_GetKeyboardState(&numkeys);

    char result = -1;

    if(state[SDL_SCANCODE_BACKSPACE] || state[SDL_SCANCODE_DELETE]) return 127; //127 = Delete ascii code

    bool mod = (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]); //Keyboard shift behavior
    mod = (state[SDL_SCANCODE_CAPSLOCK]) ? !mod : mod; //Keyboard capslock behavior

    //Keycode to character coversion code
    if(state[SDL_SCANCODE_1]) result = (mod) ? '!' : '1';
    if(state[SDL_SCANCODE_2]) result = (mod) ? '@' : '2';
    if(state[SDL_SCANCODE_3]) result = (mod) ? '#' : '3';
    if(state[SDL_SCANCODE_4]) result = (mod) ? '#' : '4';
    if(state[SDL_SCANCODE_5]) result = (mod) ? '%' : '5';
    if(state[SDL_SCANCODE_6]) result = (mod) ? '^' : '6';
    if(state[SDL_SCANCODE_7]) result = (mod) ? '&' : '7';
    if(state[SDL_SCANCODE_8]) result = (mod) ? '*' : '8';
    if(state[SDL_SCANCODE_9]) result = (mod) ? '(' : '9';
    if(state[SDL_SCANCODE_0]) result = (mod) ? ')' : '0';
    if(state[SDL_SCANCODE_A]) result = (mod) ? 'A' : 'a';
    if(state[SDL_SCANCODE_B]) result = (mod) ? 'B' : 'b';
    if(state[SDL_SCANCODE_C]) result = (mod) ? 'C' : 'c';
    if(state[SDL_SCANCODE_D]) result = (mod) ? 'D' : 'd';
    if(state[SDL_SCANCODE_E]) result = (mod) ? 'E' : 'e';
    if(state[SDL_SCANCODE_F]) result = (mod) ? 'F' : 'f';
    if(state[SDL_SCANCODE_G]) result = (mod) ? 'G' : 'g';
    if(state[SDL_SCANCODE_H]) result = (mod) ? 'H' : 'h';
    if(state[SDL_SCANCODE_I]) result = (mod) ? 'I' : 'i';
    if(state[SDL_SCANCODE_J]) result = (mod) ? 'J' : 'j';
    if(state[SDL_SCANCODE_K]) result = (mod) ? 'K' : 'k';
    if(state[SDL_SCANCODE_L]) result = (mod) ? 'L' : 'l';
    if(state[SDL_SCANCODE_M]) result = (mod) ? 'M' : 'm';
    if(state[SDL_SCANCODE_N]) result = (mod) ? 'N' : 'n';
    if(state[SDL_SCANCODE_O]) result = (mod) ? 'O' : 'o';
    if(state[SDL_SCANCODE_P]) result = (mod) ? 'P' : 'p';
    if(state[SDL_SCANCODE_Q]) result = (mod) ? 'Q' : 'q';
    if(state[SDL_SCANCODE_R]) result = (mod) ? 'R' : 'r';
    if(state[SDL_SCANCODE_S]) result = (mod) ? 'S' : 's';
    if(state[SDL_SCANCODE_T]) result = (mod) ? 'T' : 't';
    if(state[SDL_SCANCODE_U]) result = (mod) ? 'U' : 'u';
    if(state[SDL_SCANCODE_V]) result = (mod) ? 'V' : 'v';
    if(state[SDL_SCANCODE_W]) result = (mod) ? 'W' : 'w';
    if(state[SDL_SCANCODE_X]) result = (mod) ? 'X' : 'x';
    if(state[SDL_SCANCODE_Y]) result = (mod) ? 'Y' : 'y';
    if(state[SDL_SCANCODE_Z]) result = (mod) ? 'Z' : 'z';
    if(state[SDL_SCANCODE_MINUS]) result = (mod) ? '_' : '-';
    if(state[SDL_SCANCODE_EQUALS]) result = (mod) ? '+' : '=';
    if(state[SDL_SCANCODE_GRAVE]) result = (mod) ? '~' : '`';
    if(state[SDL_SCANCODE_LEFTBRACKET]) result = (mod) ? '{' : '[';
    if(state[SDL_SCANCODE_RIGHTBRACKET]) result = (mod) ? '}' : ']';
    if(state[SDL_SCANCODE_BACKSLASH]) result = (mod) ? '|' : '\\';
    if(state[SDL_SCANCODE_SEMICOLON]) result = (mod) ? ':' : ';';
    if(state[SDL_SCANCODE_APOSTROPHE]) result = (mod) ? '\"' : '\'';
    if(state[SDL_SCANCODE_COMMA]) result = (mod) ? '<' : ',';
    if(state[SDL_SCANCODE_PERIOD]) result = (mod) ? '>' : '.';
    if(state[SDL_SCANCODE_SLASH]) result = (mod) ? '?' : '/';

    return result;
}

int sdl_loop(Editor &editor)
{
    SDL_Event event;

    //Buffer for key input comparison, to make sure when a key is pressed the input is done once
    char cbuf = -1;

    bool quit = false;
    while (!quit)
    {
        SDL_PumpEvents();
        editor.Run();

        while( SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYUP: //Flush key input comparison because key is up
                cbuf = -1;
                break;
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
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    editor.is_window_active = true;
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
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    editor.is_window_active = false;
                    break;
                default:
                    break;
                }
            default:
                break;
            }
        }

        //Text input code
        char c = sdl_character();
        if(c != cbuf) //Accept the same key only once, can accept the same key only when key press status is up
        {
            if(c == 127)
            {
                if (wi::backlog::isActive())
	        		wi::backlog::deletefromInput();
	        	wi::gui::TextInputField::DeleteFromInput();
            }
            else if (c != -1)
            {
                if (wi::backlog::isActive())
	        		wi::backlog::input(c);
	        	wi::gui::TextInputField::AddInput(c);
            }
        }
        cbuf = c;

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
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        throw sdl2::SDLError("Error creating window");
    }

    set_window_icon( window.get());

    if(fullscreen) SDL_SetWindowFullscreen(window.get(), SDL_TRUE);

    editor.SetWindow(window.get());

    int ret = sdl_loop(editor);

    SDL_Quit();
    return ret;
}
