#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"
#include "wiColor.h"
#include "wiVector.h"
#include "wiCanvas.h"

namespace wi::input
{
	// Do not alter order as it is bound to lua manually!
	enum BUTTON
	{
		BUTTON_NONE = 0,

		MOUSE_BUTTON_LEFT,
		MOUSE_BUTTON_RIGHT,
		MOUSE_BUTTON_MIDDLE,

		KEYBOARD_BUTTON_UP,
		KEYBOARD_BUTTON_DOWN,
		KEYBOARD_BUTTON_LEFT,
		KEYBOARD_BUTTON_RIGHT,
		KEYBOARD_BUTTON_SPACE,
		KEYBOARD_BUTTON_RSHIFT,
		KEYBOARD_BUTTON_LSHIFT,
		KEYBOARD_BUTTON_F1,
		KEYBOARD_BUTTON_F2,
		KEYBOARD_BUTTON_F3,
		KEYBOARD_BUTTON_F4,
		KEYBOARD_BUTTON_F5,
		KEYBOARD_BUTTON_F6,
		KEYBOARD_BUTTON_F7,
		KEYBOARD_BUTTON_F8,
		KEYBOARD_BUTTON_F9,
		KEYBOARD_BUTTON_F10,
		KEYBOARD_BUTTON_F11,
		KEYBOARD_BUTTON_F12,
		KEYBOARD_BUTTON_ENTER,
		KEYBOARD_BUTTON_ESCAPE,
		KEYBOARD_BUTTON_HOME,
		KEYBOARD_BUTTON_RCONTROL,
		KEYBOARD_BUTTON_LCONTROL,
		KEYBOARD_BUTTON_DELETE,
		KEYBOARD_BUTTON_BACKSPACE,
		KEYBOARD_BUTTON_PAGEDOWN,
		KEYBOARD_BUTTON_PAGEUP,
		KEYBOARD_BUTTON_NUMPAD0,
		KEYBOARD_BUTTON_NUMPAD1,
		KEYBOARD_BUTTON_NUMPAD2,
		KEYBOARD_BUTTON_NUMPAD3,
		KEYBOARD_BUTTON_NUMPAD4,
		KEYBOARD_BUTTON_NUMPAD5,
		KEYBOARD_BUTTON_NUMPAD6,
		KEYBOARD_BUTTON_NUMPAD7,
		KEYBOARD_BUTTON_NUMPAD8,
		KEYBOARD_BUTTON_NUMPAD9,
		KEYBOARD_BUTTON_MULTIPLY,
		KEYBOARD_BUTTON_ADD,
		KEYBOARD_BUTTON_SEPARATOR,
		KEYBOARD_BUTTON_SUBTRACT,
		KEYBOARD_BUTTON_DECIMAL,
		KEYBOARD_BUTTON_DIVIDE,

		CHARACTER_RANGE_START = 65, // letter A

		GAMEPAD_RANGE_START = 256, // do not use!

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

		// Xbox mapping of generic codes:
		GAMEPAD_BUTTON_XBOX_X = GAMEPAD_BUTTON_1,
		GAMEPAD_BUTTON_XBOX_A = GAMEPAD_BUTTON_2,
		GAMEPAD_BUTTON_XBOX_B = GAMEPAD_BUTTON_3,
		GAMEPAD_BUTTON_XBOX_Y = GAMEPAD_BUTTON_4,
		GAMEPAD_BUTTON_XBOX_L1 = GAMEPAD_BUTTON_5,
		GAMEPAD_BUTTON_XBOX_R1 = GAMEPAD_BUTTON_6,
		GAMEPAD_BUTTON_XBOX_L3 = GAMEPAD_BUTTON_7,
		GAMEPAD_BUTTON_XBOX_R3 = GAMEPAD_BUTTON_8,
		GAMEPAD_BUTTON_XBOX_BACK = GAMEPAD_BUTTON_9,
		GAMEPAD_BUTTON_XBOX_START = GAMEPAD_BUTTON_10,

		// Playstation mapping of generic codes:
		GAMEPAD_BUTTON_PLAYSTATION_SQUARE = GAMEPAD_BUTTON_1,
		GAMEPAD_BUTTON_PLAYSTATION_CROSS = GAMEPAD_BUTTON_2,
		GAMEPAD_BUTTON_PLAYSTATION_CIRCLE = GAMEPAD_BUTTON_3,
		GAMEPAD_BUTTON_PLAYSTATION_TRIANGLE = GAMEPAD_BUTTON_4,
		GAMEPAD_BUTTON_PLAYSTATION_L1 = GAMEPAD_BUTTON_5,
		GAMEPAD_BUTTON_PLAYSTATION_R1 = GAMEPAD_BUTTON_6,
		GAMEPAD_BUTTON_PLAYSTATION_L3 = GAMEPAD_BUTTON_7,
		GAMEPAD_BUTTON_PLAYSTATION_R3 = GAMEPAD_BUTTON_8,
		GAMEPAD_BUTTON_PLAYSTATION_SHARE = GAMEPAD_BUTTON_9,
		GAMEPAD_BUTTON_PLAYSTATION_OPTION = GAMEPAD_BUTTON_10,
		GAMEPAD_BUTTON_PLAYSTATION_SELECT = GAMEPAD_BUTTON_PLAYSTATION_SHARE,
		GAMEPAD_BUTTON_PLAYSTATION_START = GAMEPAD_BUTTON_PLAYSTATION_OPTION,

		GAMEPAD_RANGE_END = 275, // do not use!
	};
	enum GAMEPAD_ANALOG
	{
		GAMEPAD_ANALOG_THUMBSTICK_L,
		GAMEPAD_ANALOG_THUMBSTICK_R,
		GAMEPAD_ANALOG_TRIGGER_L,
		GAMEPAD_ANALOG_TRIGGER_R,
	};

	struct KeyboardState
	{
		bool buttons[256] = {}; // it contains pressed buttons as "keyboard/typewriter" like, so no continuous presses
	};
	struct MouseState
	{
		XMFLOAT2 position = XMFLOAT2(0, 0);
		XMFLOAT2 delta_position = XMFLOAT2(0, 0);
		float delta_wheel = 0;
		float pressure = 1.0f;
		bool left_button_press = false;
		bool middle_button_press = false;
		bool right_button_press = false;
	};
	struct ControllerState
	{
		uint32_t buttons = 0;
		XMFLOAT2 thumbstick_L = XMFLOAT2(0, 0);
		XMFLOAT2 thumbstick_R = XMFLOAT2(0, 0);
		float trigger_L = 0;
		float trigger_R = 0;
	};
	struct ControllerFeedback
	{
		float vibration_left = 0;	// left vibration motor (0: no vibration, 1: maximum vibration)
		float vibration_right = 0;	// right vibration motor (0: no vibration, 1: maximum vibration)
		wi::Color led_color;			// led color
	};

	// call once at app start
	void Initialize();

	// call once per frame
	void Update(wi::platform::window_type window, wi::Canvas canvas);

	// Some things need to be cleared for next frame, like touches and delta states
	//	These things can occasionally be updated outside engine's loop, depending on operating system messages
	void ClearForNextFrame();

	const KeyboardState& GetKeyboardState();
	const MouseState& GetMouseState();
	
	// check if a button is down
	bool Down(BUTTON button, int playerindex = 0);
	// check if a button is pressed once
	bool Press(BUTTON button, int playerindex = 0);
	// check if a button has been just released
	bool Release(BUTTON button, int playerindex = 0);
	// check if a button is held down
	bool Hold(BUTTON button, uint32_t frames = 30, bool continuous = false, int playerIndex = 0);
	// get pointer position (eg. mouse pointer) (.xy) + scroll delta (.z) + pressure (.w)
	XMFLOAT4 GetPointer();
	// set pointer position (eg. mouse pointer)
	void SetPointer(const XMFLOAT4& props);
	// hide pointer
	void HidePointer(bool value);
	// read analog input from controllers, like thumbsticks or triggers
	XMFLOAT4 GetAnalog(GAMEPAD_ANALOG analog, int playerIndex = 0);
	// send various feedback to the controller
	void SetControllerFeedback(const ControllerFeedback& data, int playerindex = 0);

	struct Pen
	{
		XMFLOAT2 position = {};
		float pressure = 0;
	};
	// Set pen params, this will override mouse params
	void SetPen(const Pen& pen);

	struct Touch
	{
		enum TOUCHSTATE
		{
			TOUCHSTATE_PRESSED,
			TOUCHSTATE_RELEASED,
			TOUCHSTATE_MOVED,
			TOUCHSTATE_COUNT,
		} state;
		// current position of touch
		XMFLOAT2 pos;
	};
	const wi::vector<Touch>& GetTouches();

};

