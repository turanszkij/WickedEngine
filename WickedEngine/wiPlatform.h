#pragma once
// This file includes platform, os specific libraries and supplies common platform specific resources

#include <vector>
#include <mutex>
#include <string>
#include <iostream>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>

#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#define PLATFORM_UWP
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#endif // UWP

#else

#define PLATFORM_LINUX

#endif // _WIN32

#ifdef SDL2
#include <SDL2/SDL.h>
#include "sdl2.h"
#endif


namespace wiPlatform
{
#ifdef _WIN32
#ifndef PLATFORM_UWP
	using window_type = HWND;
#else
	using window_type = const winrt::Windows::UI::Core::CoreWindow&;
#endif // PLATFORM_UWP
#elif SDL2
	using window_type = SDL_Window*;
#else
	using window_type = int;
#endif // _WIN32

	inline void Exit()
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		PostQuitMessage(0);
#else
		winrt::Windows::ApplicationModel::Core::CoreApplication::Exit();
#endif // PLATFORM_UWP
#endif // _WIN32
#ifdef SDL2
		SDL_Quit();
#endif
	}
}
