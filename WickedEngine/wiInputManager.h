#pragma once
#include "CommonInclude.h"

#include <vector>

enum INPUT_TYPE
{
	INPUT_TYPE_KEYBOARD,
	INPUT_TYPE_GAMEPAD,
};
enum GAMEPAD_BUTTON
{
	GAMEPAD_BUTTON_UP,
	GAMEPAD_BUTTON_LEFT,
	GAMEPAD_BUTTON_DOWN,
	GAMEPAD_BUTTON_RIGHT,
	GAMEPAD_BUTTON_1,
	GAMEPAD_BUTTON_2,
	GAMEPAD_BUTTON_3,
	GAMEPAD_BUTTON_4,
	GAMEPAD_BUTTON_5,
	GAMEPAD_BUTTON_6,
	GAMEPAD_BUTTON_7,
	GAMEPAD_BUTTON_8,
	GAMEPAD_BUTTON_9,
	GAMEPAD_BUTTON_10,
	GAMEPAD_BUTTON_11,
	GAMEPAD_BUTTON_12,
	GAMEPAD_BUTTON_13,
	GAMEPAD_BUTTON_14,
};

namespace wiInputManager
{
	// call once at app start
	void Initialize();

	// call once per frame
	void Update();
	
	// check if a button is down
	bool down(uint32_t button, INPUT_TYPE inputType = INPUT_TYPE::INPUT_TYPE_KEYBOARD, short playerindex = 0);
	// check if a button is pressed once
	bool press(uint32_t button, INPUT_TYPE inputType = INPUT_TYPE::INPUT_TYPE_KEYBOARD, short playerindex = 0);
	// check if a button is held down
	bool hold(uint32_t button, uint32_t frames = 30, bool continuous = false, INPUT_TYPE inputType = INPUT_TYPE::INPUT_TYPE_KEYBOARD, short playerIndex = 0);
	// get pointer position (eg. mouse pointer) (.xy) + scroll delta (.z) + 1 unused (.w)
	XMFLOAT4 getpointer();
	// set pointer position (eg. mouse pointer) + scroll delta (.z)
	void setpointer(const XMFLOAT4& props);
	// hide pointer
	void hidepointer(bool value);

	struct Touch
	{
		enum TouchState
		{
			TOUCHSTATE_PRESSED,
			TOUCHSTATE_RELEASED,
			TOUCHSTATE_MOVED,
			TOUCHSTATE_COUNT,
		} state;
		// current position of touch
		XMFLOAT2 pos;
	};
	const std::vector<Touch>& getTouches();

};

