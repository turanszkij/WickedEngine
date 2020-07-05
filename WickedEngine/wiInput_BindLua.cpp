#include "wiInput_BindLua.h"
#include "Vector_BindLua.h"

const char wiInput_BindLua::className[] = "Input";

Luna<wiInput_BindLua>::FunctionType wiInput_BindLua::methods[] = {
	lunamethod(wiInput_BindLua, Down),
	lunamethod(wiInput_BindLua, Press),
	lunamethod(wiInput_BindLua, Hold),
	lunamethod(wiInput_BindLua, GetPointer),
	lunamethod(wiInput_BindLua, SetPointer),
	lunamethod(wiInput_BindLua, GetPointerDelta),
	lunamethod(wiInput_BindLua, HidePointer),
	lunamethod(wiInput_BindLua, GetAnalog),
	lunamethod(wiInput_BindLua, GetTouches),
	lunamethod(wiInput_BindLua, SetControllerFeedback),
	{ NULL, NULL }
};
Luna<wiInput_BindLua>::PropertyType wiInput_BindLua::properties[] = {
	{ NULL, NULL }
};

int wiInput_BindLua::Down(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::BUTTON button = (wiInput::BUTTON)wiLua::SGetInt(L, 1);
		int playerindex = 0;
		if (argc > 1)
		{
			playerindex = wiLua::SGetInt(L, 2);
		}
		wiLua::SSetBool(L, wiInput::Down(button, playerindex));
		return 1;
	}
	else
		wiLua::SError(L, "Down(int code, opt int playerindex = 0) not enough arguments!");
	return 0;
}
int wiInput_BindLua::Press(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::BUTTON code = (wiInput::BUTTON)wiLua::SGetInt(L, 1);
		int playerindex = 0;
		if (argc > 1)
		{
			playerindex = wiLua::SGetInt(L, 2);
		}
		wiLua::SSetBool(L, wiInput::Press(code, playerindex));
		return 1;
	}
	else
		wiLua::SError(L, "Press(int code, opt int playerindex = 0) not enough arguments!");
	return 0;
}
int wiInput_BindLua::Hold(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::BUTTON button = (wiInput::BUTTON)wiLua::SGetInt(L, 1);
		uint32_t duration = 30;
		if (argc > 1)
		{
			duration = wiLua::SGetInt(L, 2);
		}
		bool continuous = false;
		if (argc > 2)
		{
			continuous = wiLua::SGetBool(L, 3);
		}
		int playerindex = 0;
		if (argc > 3)
		{
			playerindex = wiLua::SGetInt(L, 4);
		}
		wiLua::SSetBool(L, wiInput::Hold(button, duration, continuous, playerindex));
		return 1;
	}
	else
		wiLua::SError(L, "Hold(int code, opt int duration = 30, opt boolean continuous = false, opt int playerindex = 0) not enough arguments!");
	return 0;
}
int wiInput_BindLua::GetPointer(lua_State* L)
{
	XMFLOAT4 P = wiInput::GetPointer();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&P)));
	return 1;
}
int wiInput_BindLua::SetPointer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			XMFLOAT4 props;
			XMStoreFloat4(&props, vec->vector);
			wiInput::SetPointer(props);
		}
		else
			wiLua::SError(L, "SetPointer(Vector props) argument is not a Vector!");
	}
	else
		wiLua::SError(L, "SetPointer(Vector props) not enough arguments!");
	return 0;
}
int wiInput_BindLua::GetPointerDelta(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&wiInput::GetMouseState().delta_position)));
	return 1;
}
int wiInput_BindLua::HidePointer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::HidePointer(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "HidePointer(bool value) not enough arguments!");
	return 0;
}
int wiInput_BindLua::GetAnalog(lua_State* L)
{
	XMFLOAT4 result = XMFLOAT4(0, 0, 0, 0);

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInput::GAMEPAD_ANALOG type = (wiInput::GAMEPAD_ANALOG)wiLua::SGetInt(L, 1);
		int playerindex = 0;
		if (argc > 1)
		{
			playerindex = wiLua::SGetInt(L, 2);
		}
		result = wiInput::GetAnalog(type, playerindex);
	}
	else
		wiLua::SError(L, "GetAnalog(int type, opt int playerindex = 0) not enough arguments!");

	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&result)));
	return 1;
}
int wiInput_BindLua::GetTouches(lua_State* L)
{
	auto& touches = wiInput::GetTouches();
	for (auto& touch : touches)
	{
		Luna<Touch_BindLua>::push(L, new Touch_BindLua(touch));
	}
	return (int)touches.size();
}
int wiInput_BindLua::SetControllerFeedback(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		ControllerFeedback_BindLua* fb = Luna<ControllerFeedback_BindLua>::lightcheck(L, 1); // Ferdinand Braun
		if (fb != nullptr)
		{
			int playerindex = 0;
			if (argc > 1)
			{
				playerindex = wiLua::SGetInt(L, 2);
			}
			wiInput::SetControllerFeedback(fb->feedback, playerindex);
		}
		else
		{
			wiLua::SError(L, "SetControllerFeedback(ControllerFeedback feedback, opt int playerindex = 0) first argument is not a ControllerFeedback!");
		}
	}
	else
		wiLua::SError(L, "SetControllerFeedback(ControllerFeedback feedback, opt int playerindex = 0) not enough arguments!");
	return 0;
}

void wiInput_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiInput_BindLua>::Register(wiLua::GetLuaState());
		wiLua::RunText("input = Input()");

		wiLua::RunText("MOUSE_BUTTON_LEFT			= 1");
		wiLua::RunText("MOUSE_BUTTON_RIGHT			= 2");
		wiLua::RunText("MOUSE_BUTTON_MIDDLE		= 3");

		wiLua::RunText("KEYBOARD_BUTTON_UP			= 4");
		wiLua::RunText("KEYBOARD_BUTTON_DOWN		= 5");
		wiLua::RunText("KEYBOARD_BUTTON_LEFT		= 6");
		wiLua::RunText("KEYBOARD_BUTTON_RIGHT		= 7");
		wiLua::RunText("KEYBOARD_BUTTON_SPACE		= 8");
		wiLua::RunText("KEYBOARD_BUTTON_RSHIFT		= 9");
		wiLua::RunText("KEYBOARD_BUTTON_LSHIFT		= 10");
		wiLua::RunText("KEYBOARD_BUTTON_F1			= 11");
		wiLua::RunText("KEYBOARD_BUTTON_F2			= 12");
		wiLua::RunText("KEYBOARD_BUTTON_F3			= 13");
		wiLua::RunText("KEYBOARD_BUTTON_F4			= 14");
		wiLua::RunText("KEYBOARD_BUTTON_F5			= 15");
		wiLua::RunText("KEYBOARD_BUTTON_F6			= 16");
		wiLua::RunText("KEYBOARD_BUTTON_F7			= 17");
		wiLua::RunText("KEYBOARD_BUTTON_F8			= 18");
		wiLua::RunText("KEYBOARD_BUTTON_F9			= 19");
		wiLua::RunText("KEYBOARD_BUTTON_F10		= 20");
		wiLua::RunText("KEYBOARD_BUTTON_F11		= 21");
		wiLua::RunText("KEYBOARD_BUTTON_F12		= 22");
		wiLua::RunText("KEYBOARD_BUTTON_ENTER		= 23");
		wiLua::RunText("KEYBOARD_BUTTON_ESCAPE		= 24");
		wiLua::RunText("KEYBOARD_BUTTON_HOME		= 25");
		wiLua::RunText("KEYBOARD_BUTTON_RCONTROL	= 26");
		wiLua::RunText("KEYBOARD_BUTTON_LCONTROL	= 27");
		wiLua::RunText("KEYBOARD_BUTTON_DELETE		= 28");
		wiLua::RunText("KEYBOARD_BUTTON_BACK		= 29");
		wiLua::RunText("KEYBOARD_BUTTON_PAGEDOWN	= 30");
		wiLua::RunText("KEYBOARD_BUTTON_PAGEUP		= 31");

		wiLua::RunText("GAMEPAD_BUTTON_UP			= 257");
		wiLua::RunText("GAMEPAD_BUTTON_LEFT		= 258");
		wiLua::RunText("GAMEPAD_BUTTON_DOWN		= 259");
		wiLua::RunText("GAMEPAD_BUTTON_RIGHT		= 260");
		wiLua::RunText("GAMEPAD_BUTTON_1			= 261");
		wiLua::RunText("GAMEPAD_BUTTON_2			= 262");
		wiLua::RunText("GAMEPAD_BUTTON_3			= 263");
		wiLua::RunText("GAMEPAD_BUTTON_4			= 264");
		wiLua::RunText("GAMEPAD_BUTTON_5			= 265");
		wiLua::RunText("GAMEPAD_BUTTON_6			= 266");
		wiLua::RunText("GAMEPAD_BUTTON_7			= 267");
		wiLua::RunText("GAMEPAD_BUTTON_8			= 268");
		wiLua::RunText("GAMEPAD_BUTTON_9			= 269");
		wiLua::RunText("GAMEPAD_BUTTON_10			= 270");
		wiLua::RunText("GAMEPAD_BUTTON_11			= 271");
		wiLua::RunText("GAMEPAD_BUTTON_12			= 272");
		wiLua::RunText("GAMEPAD_BUTTON_13			= 273");
		wiLua::RunText("GAMEPAD_BUTTON_14			= 274");

		//Analog
		wiLua::RunText("GAMEPAD_ANALOG_THUMBSTICK_L	= 0");
		wiLua::RunText("GAMEPAD_ANALOG_THUMBSTICK_R	= 1");
		wiLua::RunText("GAMEPAD_ANALOG_TRIGGER_L		= 2");
		wiLua::RunText("GAMEPAD_ANALOG_TRIGGER_R		= 3");

		//Touch
		wiLua::RunText("TOUCHSTATE_PRESSED		= 0");
		wiLua::RunText("TOUCHSTATE_RELEASED	= 1");
		wiLua::RunText("TOUCHSTATE_MOVED		= 2");
	}

	Touch_BindLua::Bind();
	ControllerFeedback_BindLua::Bind();
}







const char Touch_BindLua::className[] = "Touch";

Luna<Touch_BindLua>::FunctionType Touch_BindLua::methods[] = {
	lunamethod(Touch_BindLua, GetState),
	lunamethod(Touch_BindLua, GetPos),
	{ NULL, NULL }
};
Luna<Touch_BindLua>::PropertyType Touch_BindLua::properties[] = {
	{ NULL, NULL }
};

int Touch_BindLua::GetState(lua_State* L)
{
	wiLua::SSetInt(L, (int)touch.state);
	return 1;
}
int Touch_BindLua::GetPos(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&touch.pos)));
	return 1;
}

void Touch_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Touch_BindLua>::Register(wiLua::GetLuaState());
	}
}







const char ControllerFeedback_BindLua::className[] = "ControllerFeedback";

Luna<ControllerFeedback_BindLua>::FunctionType ControllerFeedback_BindLua::methods[] = {
	lunamethod(ControllerFeedback_BindLua, SetVibrationRight),
	lunamethod(ControllerFeedback_BindLua, SetVibrationLeft),
	lunamethod(ControllerFeedback_BindLua, SetLEDColor),
	{ NULL, NULL }
};
Luna<ControllerFeedback_BindLua>::PropertyType ControllerFeedback_BindLua::properties[] = {
	{ NULL, NULL }
};

int ControllerFeedback_BindLua::SetVibrationLeft(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		feedback.vibration_left = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetVibrationLeft(float value) not enough arguments!");
	}
	return 0;
}
int ControllerFeedback_BindLua::SetVibrationRight(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		feedback.vibration_right = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetVibrationRight(float value) not enough arguments!");
	}
	return 0;
}
int ControllerFeedback_BindLua::SetLEDColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			XMFLOAT4 col;
			XMStoreFloat4(&col, vec->vector);
			feedback.led_color = wiColor::fromFloat4(col);
		}
		else
		{
			feedback.led_color.rgba = wiLua::SGetInt(L, 1);
		}
	}
	else
	{
		wiLua::SError(L, "SetLEDColor(int hexcolor) not enough arguments!");
	}
	return 0;
}

void ControllerFeedback_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<ControllerFeedback_BindLua>::Register(wiLua::GetLuaState());
	}
}
