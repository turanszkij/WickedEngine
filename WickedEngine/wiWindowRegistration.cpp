#include "wiWindowRegistration.h"

namespace wiWindowRegistration
{
	window_type window = nullptr;
	instance_type instance = nullptr;

	window_type GetRegisteredWindow() {
		return window;
	}
	instance_type GetRegisteredInstance() {
		return instance;
	}
	void RegisterWindow(window_type wnd) {
		window = wnd;
	}
	void RegisterInstance(instance_type inst) {
		instance = inst;
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
