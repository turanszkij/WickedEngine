#pragma once
#include "CommonInclude.h"
#include "Platform.h"

#if __has_include("dinput.h") && !defined(WINSTORE_SUPPORT)
#define WICKEDENGINE_BUILD_DIRECTINPUT
#endif


#ifdef WICKEDENGINE_BUILD_DIRECTINPUT

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace wiDirectInput
{
	// Point of view hat values (direction pad)
	enum POV
	{
		POV_UP = 0,
		POV_UPRIGHT = 4500,
		POV_RIGHT = 9000,
		POV_RIGHTDOWN = 13500,
		POV_DOWN = 18000,
		POV_DOWNLEFT = 22500,
		POV_LEFT = 27000,
		POV_LEFTUP = 31500,
		POV_IDLE = 0xFFFFFFFF
	};

	// Create the DirectInput device, call this before calling any other functions here
	void Initialize();

	// Destroy the DirectInput device
	void CleanUp();

	// Update the connected devices
	void Update();

	// Returns how many joystick devices are connected
	short GetConnectedJoystickCount();

	// Returns the state of the specified joystick
	DIJOYSTATE2 GetJoystickData(short index);
}
#endif // WICKEDENGINE_BUILD_DIRECTINPUT
