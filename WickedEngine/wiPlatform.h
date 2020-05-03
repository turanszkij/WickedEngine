#pragma once
// This file includes platform, os specific libraries and supplies common platform specific resources

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>

#ifdef WINSTORE_SUPPORT
#include <Windows.UI.Core.h>
#endif // WINSTORE_SUPPORT

#endif // _WIN32


namespace wiPlatform
{
#ifdef _WIN32
#ifndef WINSTORE_SUPPORT
	typedef HWND window_type;
#else
	typedef Windows::UI::Core::CoreWindow^ window_type;
#endif // WINSTORE_SUPPORT
#endif // _WIN32

	inline window_type& GetWindow()
	{
		static window_type window;
		return window;
	}
	inline bool IsWindowActive()
	{
#ifdef _WIN32
#ifndef WINSTORE_SUPPORT
		return GetForegroundWindow() == GetWindow();
#else
		return true;
#endif // WINSTORE_SUPPORT
#else
		return true;
#endif // _WIN32
	}
	inline uint32_t GetDPI()
	{
#ifdef _WIN32
#ifndef WINSTORE_SUPPORT
		return GetDpiForWindow(GetWindow());
#else
		return 96;
#endif // WINSTORE_SUPPORT
#else
		return 96;
#endif // _WIN32
	}
	inline float GetDPIScaling()
	{
		return (float)GetDPI() / 96.0f;
	}
}
