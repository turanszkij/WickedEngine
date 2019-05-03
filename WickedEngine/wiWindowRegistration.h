#pragma once
#include "CommonInclude.h"

namespace wiWindowRegistration
{
#ifndef WINSTORE_SUPPORT
	typedef HWND window_type;
	typedef HINSTANCE instance_type;
#else
	typedef Windows::UI::Core::CoreWindow^ window_type;
	typedef void* instance_type;
#endif

	instance_type GetRegisteredInstance();
	window_type GetRegisteredWindow();
	void RegisterInstance(instance_type wnd);
	void RegisterWindow(window_type wnd);
	bool IsWindowActive();
};
