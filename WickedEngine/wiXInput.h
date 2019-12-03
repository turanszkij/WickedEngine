#pragma once
#include "CommonInclude.h"
#include "wiInput.h"

#if __has_include("xinput.h")
#define WICKEDENGINE_BUILD_XINPUT
#endif

#ifdef WICKEDENGINE_BUILD_XINPUT

#define NOMINMAX
#include <windows.h>
#include <xinput.h>

namespace wiXInput
{
	// Call once per frame to read and update controller states
	void Update();

	// Returns how many gamepads can Xinput handle
	short GetMaxControllerCount();

	// Returns if the specified gamepad is connected or not
	bool IsControllerConnected(short index);

	// Returns the specified gamepad's state
	XINPUT_STATE GetControllerState(short index);

	void SetControllerFeedback(const wiInput::ControllerFeedback data, short index);
}

#endif // WICKEDENGINE_BUILD_XINPUT
