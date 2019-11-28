#pragma once
#include "CommonInclude.h"
#include "Platform.h"

#ifndef WINSTORE_SUPPORT
#define WICKEDENGINE_BUILD_DIRECTINPUT

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

	void Initialize();
	void CleanUp();
	void Update();

	short GetConnectedJoystickCount();
	bool GetJoystickData(DIJOYSTATE2* result, short index);
}
#endif // WINSTORE_SUPPORT
