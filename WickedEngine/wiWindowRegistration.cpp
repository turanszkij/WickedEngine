#include "wiWindowRegistration.h"

namespace wiWindowRegistration
{
	window_type window = nullptr;

	window_type GetRegisteredWindow() {
		return window;
	}
	void RegisterWindow(window_type wnd) {
		window = wnd;
	}
	bool IsWindowActive() {
#ifndef WINSTORE_SUPPORT
		HWND fgw = GetForegroundWindow();
		return fgw == window;
#else
		return true;
#endif
	}
}
