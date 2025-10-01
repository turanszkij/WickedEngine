/**
 * This file includes platform, os specific libraries and supplies common
 * platform specific resources.
 */

#pragma once

///////////////////////////////////////////////////////////////////////////////
// Define current platform
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
    #if WINAPI_FAMILY == WINAPI_FAMILY_GAMES
		#define PLATFORM_XBOX
	#else
		#define PLATFORM_WINDOWS_DESKTOP
	#endif
#elif defined(__SCE__)
    #define PLATFORM_PS5
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
#else
    #define PLATFORM_LINUX
#endif

///////////////////////////////////////////////////////////////////////////////
// Platform specific includes and definitions
///////////////////////////////////////////////////////////////////////////////

// Windows and Xbox
#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_XBOX)
    #ifndef NOMINMAX
    	#define NOMINMAX
    #endif

    #ifndef WIN32_LEAN_AND_MEAN
    	#define WIN32_LEAN_AND_MEAN
    #endif

    #include <SDKDDKVer.h>
    #include <windows.h>

    using HMODULE_t = HMODULE;

    static inline HMODULE_t wiLoadLibrary(const char* name)
	{
		return LoadLibraryA(name);
	}

    static inline void* wiGetProcAddress(HMODULE_t h, const char* name)
	{
		return (void*)GetProcAddress(h, name);
	}

// PS5
#elif defined(PLATFORM_PS5)
    // EMPTY ON PURPOSE

// POSIX (Linux + macOS)
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
    #include <dlfcn.h>

    using HMODULE_t = void*;

    static inline HMODULE_t wiLoadLibrary(const char* name)
	{
		return dlopen(name, RTLD_LAZY);
	}

    static inline void* wiGetProcAddress(HMODULE_t h, const char* name)
	{
		return dlsym(h, name);
	}

#else
    #error "Unsupported platform!"
#endif

///////////////////////////////////////////////////////////////////////////////
// Include SDL2 if available
///////////////////////////////////////////////////////////////////////////////

#ifdef SDL2
	#include <SDL2/SDL.h>
	#include <SDL_vulkan.h>
	#include "sdl2.h"
#endif


namespace wi::platform
{
#ifdef _WIN32
	using window_type = HWND;
	using error_type = HRESULT;
#elif defined(SDL2)
	using window_type = SDL_Window*;
	using error_type = int;
#else
	using window_type = void*;
	using error_type = int;
#endif // _WIN32

	inline void Exit()
	{
#ifdef _WIN32
		PostQuitMessage(0);
#endif // _WIN32
#ifdef SDL2
		SDL_Event quit_event;
		quit_event.type = SDL_QUIT;
		SDL_PushEvent(&quit_event);
#endif
	}

	struct WindowProperties
	{
		int width = 0;
		int height = 0;
		float dpi = 96;
	};
	inline void GetWindowProperties(window_type window, WindowProperties* dest)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		dest->dpi = (float)GetDpiForWindow(window);
#endif // WINDOWS_DESKTOP

#ifdef PLATFORM_XBOX
		dest->dpi = 96.f;
#endif // PLATFORM_XBOX

#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_XBOX)
		RECT rect;
		GetClientRect(window, &rect);
		dest->width = int(rect.right - rect.left);
		dest->height = int(rect.bottom - rect.top);
#endif // PLATFORM_WINDOWS_DESKTOP || PLATFORM_XBOX

// On POSIX platforms with SDL (Linux, macOS) use SDL to query drawable and DPI
#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	int window_width = 0, window_height = 0;

	SDL_GetWindowSize(window, &window_width, &window_height);
	SDL_Vulkan_GetDrawableSize(window, &dest->width, &dest->height);

	if (window_width != 0)
	{
	    dest->dpi = ((float)dest->width / (float)window_width) * 96.f;
	}
#endif // PLATFORM_LINUX || PLATFORM_MACOS
	}
}
