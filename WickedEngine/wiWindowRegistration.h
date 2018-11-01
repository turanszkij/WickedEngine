#pragma once
#include "CommonInclude.h"

namespace wiWindowRegistration
{
#ifndef WINSTORE_SUPPORT
	typedef HWND window_type;
#else
	typedef Windows::UI::Core::CoreWindow^ window_type;
#endif

	window_type GetRegisteredWindow();
	void RegisterWindow(window_type wnd);
	bool IsWindowActive();
};
