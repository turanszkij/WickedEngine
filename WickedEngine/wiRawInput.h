#pragma once
#include "CommonInclude.h"
#include "wiInput.h"

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

	struct KeyboardState
	{
		bool buttons[128] = {};
	};
	struct MouseState
	{
		XMINT2 position = XMINT2(0, 0);
		XMINT2 delta_position = XMINT2(0, 0);
		float delta_wheel = 0;
	};

	enum POV
	{
		POV_UP = 0,
		POV_UPRIGHT = 1,
		POV_RIGHT = 2,
		POV_RIGHTDOWN = 3,
		POV_DOWN = 4,
		POV_DOWNLEFT = 5,
		POV_LEFT = 6,
		POV_LEFTUP = 7,
		POV_IDLE = 8,
	};
	struct ControllerState
	{
		bool buttons[32] = {};
		POV pov = POV_IDLE;
		XMFLOAT2 thumbstick_L = XMFLOAT2(0, 0);
		XMFLOAT2 thumbstick_R = XMFLOAT2(0, 0);
		float trigger_L = 0;
		float trigger_R = 0;
	};

	KeyboardState GetKeyboardState();

	MouseState GetMouseState();

	short GetControllerCount();
	ControllerState GetControllerState(short index);
	void SetControllerFeedback(const wiInput::ControllerFeedback data, short index);
}

#endif // WICKEDENGINE_BUILD_RAWINPUT
