#pragma once
// This file includes platform, os specific libraries and supplies common platform specific resources

#include <vector>
#include <mutex>
#include <string>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <windows.h>

#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#define PLATFORM_UWP
#include <Windows.UI.Core.h>
#include <agile.h>
#endif // UWP

#else

#define PLATFORM_LINUX

#endif // _WIN32


namespace wiPlatform
{
#ifdef _WIN32
#ifndef PLATFORM_UWP
	using window_type = HWND;
#else
	using window_type = Platform::Agile<Windows::UI::Core::CoreWindow>;
#endif // PLATFORM_UWP
#else
	using window_type = int;
#endif // _WIN32

	struct DeferredMessageBox
	{
		std::wstring caption;
		std::wstring message;
	};

	struct WindowState
	{
		window_type window;
		int dpi = 96;
		std::vector<DeferredMessageBox> messages;
		std::mutex messagemutex;
	};
	inline WindowState& GetWindowState()
	{
		static WindowState state;
		return state;
	}

	inline window_type& GetWindow()
	{
		return GetWindowState().window;
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
	inline void InitDPI()
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		GetWindowState().dpi = (int)GetDpiForWindow(GetWindow());
#else
		GetWindowState().dpi = (int)Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
#endif // PLATFORM_UWP
#endif // _WIN32
	}
	inline int GetDPI()
	{
		return GetWindowState().dpi;
	}
	inline float GetDPIScaling()
	{
		return (float)GetDPI() / 96.0f;
	}
	inline void Exit()
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		PostQuitMessage(0);
#else
		Windows::ApplicationModel::Core::CoreApplication::Exit();
#endif // PLATFORM_UWP
#endif // _WIN32
	}
	inline void PopMessages()
	{
		auto& state = GetWindowState();
		state.messagemutex.lock();
		for (auto& x : state.messages)
		{
#ifdef _WIN32
#ifndef PLATFORM_UWP
			MessageBox(wiPlatform::GetWindow(), x.message.c_str(), x.caption.c_str(), 0);
#else
			Windows::UI::Popups::MessageDialog(ref new Platform::String(x.message.c_str()), ref new Platform::String(x.caption.c_str())).ShowAsync();
#endif // PLATFORM_UWP
#endif // _WIN32
		}
		state.messages.clear();
		state.messagemutex.unlock();
	}
}
