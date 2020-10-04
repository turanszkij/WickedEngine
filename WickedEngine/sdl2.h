// From https://github.com/xyproto/sdl2-examples/blob/master/include/sdl2.h
// MIT License

#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <sstream>

namespace sdl2 {

// Very useful function from Eric Scott Barr:
// https://eb2.co/blog/2014/04/c-plus-plus-14-and-sdl2-managing-resources/
template <typename Creator, typename Destructor, typename... Arguments>
auto make_resource(Creator c, Destructor d, Arguments&&... args)
{
    using std::decay_t;
    using std::forward;
    using std::unique_ptr;

    auto r = c(forward<Arguments>(args)...);
    return unique_ptr<decay_t<decltype(*r)>, decltype(d)>(r, d);
}

// The "internal type" of the SDL System
using SDL_System = int;

// SDL_CreateSDL initiates the use of SDL.
// The given flags are passed to SDL_Init.
// The returned value contains the exit code.
inline SDL_System* SDL_CreateSDL(Uint32 flags)
{
    auto init_status = new SDL_System;
    *init_status = SDL_Init(flags);
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
using renderer_ptr_t = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using surf_ptr_t = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
using texture_ptr_t = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

// Initialize SDL (the returned int* contains the return value from SDL_Init)
inline sdlsystem_ptr_t make_sdlsystem(Uint32 flags)
{
    return make_resource(SDL_CreateSDL, SDL_DestroySDL, flags);
}

// Create a window (that contains both a SDL_Window and the destructor for SDL_Windows)
inline window_ptr_t make_window(const char* title, int x, int y, int w, int h, Uint32 flags)
{
    return make_resource(SDL_CreateWindow, SDL_DestroyWindow, title, x, y, w, h, flags);
}

// Create a renderer given a window, containing both the renderer and the destructor
inline renderer_ptr_t make_renderer(SDL_Window* win, int x, Uint32 flags)
{
    return make_resource(SDL_CreateRenderer, SDL_DestroyRenderer, win, x, flags);
}

// Create a surface from a bmp file, containing both the surface and the destructor
inline surf_ptr_t make_bmp(SDL_RWops* sdlfile)
{
    // May throw an exception if sdlfile is nullptr
    return make_resource(SDL_LoadBMP_RW, SDL_FreeSurface, sdlfile, 1);
}

// Create a texture from a renderer and a surface
inline texture_ptr_t make_texture(SDL_Renderer* ren, SDL_Surface* surf)
{
    return make_resource(SDL_CreateTextureFromSurface, SDL_DestroyTexture, ren, surf);
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

} // namespace sdl2
