#pragma once
#include "CommonInclude.h"

#ifndef WINSTORE_SUPPORT
#define WICKEDENGINE_BUILD_RAWINPUT
#endif // WINSTORE_SUPPORT

#ifdef WICKEDENGINE_BUILD_RAWINPUT

namespace wiRawInput
{
	// Call this once to initialize raw input devices
	void Initialize();

	// Updates the state of raw input devices
	void Update();

	// Returns relative mouse pointer movement
	XMINT2 GetMouseDelta_XY();

	// Returns the mouse wheel delta.
	//	0: not scrolled
	//	1: scrolled 1 detent (positive direction)
	//	-1: scrolled 1 detent (negative direction)
	//	other: scrolled via more fine grained method (such as a touchpad for example)
	float GetMouseDelta_Wheel();

	// Returns whether a specific keycode is currently pressed or not
	bool IsKeyDown(unsigned char keycode);
}

#endif // WICKEDENGINE_BUILD_RAWINPUT
