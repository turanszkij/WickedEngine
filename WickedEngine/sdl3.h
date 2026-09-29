// SDL3 counterpart of sdl2.h -- same RAII-wrapper approach (see
// https://eb2.co/blog/2014/04/c-plus-plus-14-and-sdl2-managing-resources/), adapted for
// SDL3's API (SDL_Init returns bool, not an int where 0 means success).

#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <sstream>

namespace sdl3 {

template <typename Creator, typename Destructor, typename... Arguments>
inline auto make_resource(Creator c, Destructor d, Arguments&&... args)
{
    auto r = c(std::forward<Arguments>(args)...);
    return std::unique_ptr<std::decay_t<decltype(*r)>, decltype(d)>(r, d);
}

// The "internal type" of the SDL System
using SDL_System = int;

// SDL_CreateSDL initiates the use of SDL.
// The given flags are passed to SDL_Init.
// The returned value contains the exit code (0 = success, matching sdl2.h's
// convention, even though SDL3's own SDL_Init returns a bool where true = success).
inline SDL_System* SDL_CreateSDL(Uint32 flags)
{
    auto init_status = new SDL_System;
    *init_status = SDL_Init(flags) ? 0 : 1;
    return init_status;
}

// SDL_DestroySDL ends the use of SDL
inline void SDL_DestroySDL(SDL_System* init_status)
{
    delete init_status; // Delete the int that contains the return value from SDL_Init
    SDL_Quit();
}

using sdlsystem_ptr_t = std::unique_ptr<SDL_System, decltype(&SDL_DestroySDL)>;
using window_ptr_t = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

// Initialize SDL (the returned int* contains the return value from SDL_Init)
inline sdlsystem_ptr_t make_sdlsystem(Uint32 flags)
{
    return make_resource(SDL_CreateSDL, SDL_DestroySDL, flags);
}

// Create a window (that contains both a SDL_Window and the destructor for SDL_Windows).
// SDL3's SDL_CreateWindow() dropped the x/y position parameters SDL2 had -- positioning is
// now a separate call, so this makes it afterward to keep the same call signature as sdl2.h.
inline window_ptr_t make_window(const char* title, int x, int y, int w, int h, Uint64 flags)
{
    window_ptr_t window(SDL_CreateWindow(title, w, h, flags), SDL_DestroyWindow);
    if (window)
    {
        SDL_SetWindowPosition(window.get(), x, y);
    }
    return window;
}

// Easy to raise exceptions with this class
class SDLError : public std::exception
{
	std::string error_message;
	public:
	SDLError(const char *err_msg)
	{
		std::stringstream msg;
        msg << err_msg << ": " << SDL_GetError();
        this->error_message = msg.str();
	}
	SDLError(const std::string &err_msg)
        : SDLError(err_msg.c_str())
    {}

    const char*
    what() const noexcept override
    {
        return error_message.c_str();
    }

};

} // namespace sdl3
