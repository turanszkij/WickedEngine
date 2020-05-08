#pragma once
// This file includes platform, os specific libraries and supplies common platform specific resources

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>

#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#define PLATFORM_UWP
#include <Windows.UI.Core.h>
#include <agile.h>
#endif // UWP

#endif // _WIN32


namespace wiPlatform
{
#ifdef _WIN32
#ifndef PLATFORM_UWP
	using window_type = HWND;
#else
	using window_type = Platform::Agile<Windows::UI::Core::CoreWindow>;
#endif // PLATFORM_UWP
#endif // _WIN32

	inline window_type& GetWindow()
	{
		static window_type window;
		return window;
	}
	inline bool IsWindowActive()
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		return GetForegroundWindow() == GetWindow();
#else
		return true;
#endif // PLATFORM_UWP
#else
		return true;
#endif // _WIN32
	}
	inline int GetDPI()
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		return (int)GetDpiForWindow(GetWindow());
#else
		return (int)Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
#endif // PLATFORM_UWP
#else
		return 96;
#endif // _WIN32
	}
	inline float GetDPIScaling()
	{
		return (float)GetDPI() / 96.0f;
	}
}
