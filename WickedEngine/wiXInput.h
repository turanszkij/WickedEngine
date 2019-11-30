#pragma once
#include "CommonInclude.h"

#if __has_include("xinput.h")
#define WICKEDENGINE_BUILD_XINPUT
#endif

#ifdef WICKEDENGINE_BUILD_XINPUT

#include <windows.h>
#include <xinput.h>

namespace wiXInput
{
	// Call once per frame to read and update controller states
	void Update();

	// Returns how many gamepads can Xinput handle
	short GetMaxGamepadCount();

	// Returns if the specified gamepad is connected or not
	bool IsGamepadConnected(short index);

	// Returns the specified gamepad's state
	XINPUT_STATE GetGamepadData(short index);
}

#endif // WICKEDENGINE_BUILD_XINPUT
