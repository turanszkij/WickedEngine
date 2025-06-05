#include "stdafx.h"
#include "Editor.h"

#include "sdl2.h"
#include <fstream>

#include "icon.c"

#ifdef __linux__
#  include <execinfo.h>
#  include <csignal>
#  include <cstdio>
#  include <unistd.h>
#endif

using namespace std;

class EditorWithDevInfo : public Editor
{
public:
	const char* GetAdapterName() const
	{
		return graphicsDevice == nullptr ? "(no device)" : graphicsDevice->GetAdapterName().c_str();
	}

	const char* GetDriverDescription() const
	{
		return graphicsDevice == nullptr ? "(no device)" : graphicsDevice->GetDriverDescription().c_str();
	}
} editor;

int sdl_loop()
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
                case SDL_DROPFILE:
				    editor.renderComponent.Open(event.drop.file);
                    editor.is_window_active = true;
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

#ifdef __linux__

void crash_handler(int sig)
{
	static bool already_handled = false;

	void* btbuf[100];

	char outbuf[512];

	if (already_handled) return;

	already_handled = true;

	size_t size = backtrace(btbuf, 100);

	snprintf(
		outbuf, sizeof(outbuf),
		"Signal: %i (%s)\n"
		"Version: %s\n"
		"Adapter: %s\n"
		"Driver: %s\n"
		"Stacktrace:\n",
		sig, sigdescr_np(sig),
		wi::version::GetVersionString(),
		editor.GetAdapterName(),
		editor.GetDriverDescription()
	);

	fprintf(
		stderr,
		"\e[31m"  // red
		"The editor just crashed, sorry about that! If you make a bug report, please include the following information:\n\n%s",
		outbuf
	);
	backtrace_symbols_fd(btbuf, size, STDERR_FILENO);  // backtrace_symbols does a malloc which could crash, backtrace_symbols_fd does not.
	fprintf(stderr, "\e[m");  // back to normal

	// finally, we also try to write the crash data to a file
	// this might fail because we're in a weird state right now, but there's no harm done if it doesn't work

	// Use C interface because some stuff in the c++ stdlib could be calling malloc

	const char* filename = "wicked-editor-crash-log.txt";

	FILE* logfile = fopen(filename, "w");

	if (logfile != nullptr)
	{
		fputs(outbuf, logfile);
		fflush(logfile);
		backtrace_symbols_fd(btbuf, size, fileno(logfile));
		fputs("\nBacklog:\n", logfile);
		wi::backlog::_forEachLogEntry_unsafe([&] (auto&& entry) {
			fputs(entry.text.c_str(), logfile);
			fflush(logfile);
		});
		fclose(logfile);
		char cwdbuf[200];
		fprintf(stderr, "\e[1mcrash log written to %s/%s\e[m\n", getcwd(cwdbuf, sizeof(cwdbuf)), filename);
	}
	abort();
}

#endif

int main(int argc, char *argv[])
{

#ifdef __linux__
	// dummy backtrace() call to force libgcc to be loaded ahead of time.
	// Otherwise it might lead to malloc calls in the crash_handler, which we want to avoid
	void* dummy[1];
	backtrace(dummy, 1);

	for (int sig : std::array{SIGABRT, SIGBUS, SIGILL, SIGFPE, SIGSEGV})
	{
		signal(sig, crash_handler);
	}
#endif

    wi::arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (*system) {
        throw sdl2::SDLError("Error creating SDL2 system");
    }

	int width = 1920;
	int height = 1080;
	bool fullscreen = false;

	wi::Timer timer;
	if (wi::helper::FileExists("config.ini") && editor.config.Open("config.ini"))
	{
		if (editor.config.Has("width"))
		{
			width = editor.config.GetInt("width");
			height = editor.config.GetInt("height");
		}
		fullscreen = editor.config.GetBool("fullscreen");
		editor.allow_hdr = editor.config.GetBool("allow_hdr");

		wilog("config.ini loaded in %.2f milliseconds\n", (float)timer.elapsed_milliseconds());
	}

	width = std::max(100, width);
	height = std::max(100, height);

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Editor",
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

    int ret = sdl_loop();

	wi::jobsystem::ShutDown();

    return ret;
}
