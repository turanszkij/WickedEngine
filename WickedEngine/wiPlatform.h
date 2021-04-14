#pragma once
#include "CommonInclude.h"
// This file includes platform, os specific libraries and supplies common platform specific resources

#include <vector>
#include <mutex>
#include <string>
#include <iostream>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>
#include <tchar.h>

#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#define PLATFORM_UWP
#define wiLoadLibrary(name) LoadPackagedLibrary(_T(name),0)
#define wiGetProcAddress(handle,name) GetProcAddress(handle, name)
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#else
#define PLATFORM_WINDOWS_DESKTOP
#define wiLoadLibrary(name) LoadLibrary(_T(name))
#define wiGetProcAddress(handle,name) GetProcAddress(handle, name)
#endif // UWP

#else

#define PLATFORM_LINUX
#define wiLoadLibrary(name) dlopen(name, RTLD_LAZY)
#define wiGetProcAddress(handle,name) dlsym(handle, name)
#define HMODULE (void*)

#endif // _WIN32

#ifdef SDL2
#include <SDL2/SDL.h>
#include "sdl2.h"
#endif


namespace wiPlatform
{
#ifdef _WIN32
#ifdef PLATFORM_UWP
	using window_type = const winrt::Windows::UI::Core::CoreWindow&;
#else
	using window_type = HWND;
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

	inline XMUINT2 GetWindowSize(window_type window)
	{
		XMUINT2 size = {};

#ifdef PLATFORM_WINDOWS_DESKTOP
		RECT rect;
		GetClientRect(window, &rect);
		size.x = uint32_t(rect.right - rect.left);
		size.y = uint32_t(rect.bottom - rect.top);
#endif // WINDOWS_DESKTOP

#ifdef PLATFORM_UWP
		float dpi = GetWindowDPI(window);
		float dpiscale = dpi / 96.f;
		size.x = uint32_t(window.Bounds().Width * dpiscale);
		size.y = uint32_t(window.Bounds().Height * dpiscale);
#endif // PLATFORM_UWP

#ifdef PLATFORM_LINUX
		int width, height;
		SDL_GetWindowSize(window, &width, &height);
		size.x = (uint32_t)width;
		size.y = (uint32_t)height;
#endif // PLATFORM_LINUX

		return size;
	}

	inline float GetWindowDPI(window_type window)
	{
		float dpi = 96;

#ifdef PLATFORM_WINDOWS_DESKTOP
		dpi = (float)GetDpiForWindow(window);
#endif // WINDOWS_DESKTOP

#ifdef PLATFORM_UWP
		dpi = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView().LogicalDpi();
#endif // PLATFORM_UWP

#ifdef PLATFORM_LINUX
		dpi = 96; // todo
#endif // PLATFORM_LINUX

		return dpi;
	}
}
