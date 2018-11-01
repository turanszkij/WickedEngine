#include "wiInputManager_BindLua.h"
#include "Vector_BindLua.h"

const char wiInputManager_BindLua::className[] = "InputManager";

Luna<wiInputManager_BindLua>::FunctionType wiInputManager_BindLua::methods[] = {
	lunamethod(wiInputManager_BindLua, Down),
	lunamethod(wiInputManager_BindLua, Press),
	lunamethod(wiInputManager_BindLua, Hold),
	lunamethod(wiInputManager_BindLua, GetPointer),
	lunamethod(wiInputManager_BindLua, SetPointer),
	lunamethod(wiInputManager_BindLua, HidePointer),
	lunamethod(wiInputManager_BindLua, GetTouches),
	{ NULL, NULL }
};
Luna<wiInputManager_BindLua>::PropertyType wiInputManager_BindLua::properties[] = {
	{ NULL, NULL }
};

int wiInputManager_BindLua::Down(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int code = wiLua::SGetInt(L, 1);
		wiInputManager::InputType type = wiInputManager::KEYBOARD;
		if (argc > 1)
		{
			type = (wiInputManager::InputType)wiLua::SGetInt(L, 2);
		}
		wiLua::SSetBool(L, wiInputManager::down((DWORD)code, type));
		return 1;
	}
	else
		wiLua::SError(L, "Down(int code, opt int type = KEYBOARD) not enough arguments!");
	return 0;
}
int wiInputManager_BindLua::Press(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int code = wiLua::SGetInt(L, 1);
		wiInputManager::InputType type = wiInputManager::KEYBOARD;
		if (argc > 1)
		{
			type = (wiInputManager::InputType)wiLua::SGetInt(L, 2);
		}
		wiLua::SSetBool(L, wiInputManager::press((DWORD)code, type));
		return 1;
	}
	else
		wiLua::SError(L, "Press(int code, opt int type = KEYBOARD) not enough arguments!");
	return 0;
}
int wiInputManager_BindLua::Hold(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int code = wiLua::SGetInt(L, 1);
		int duration = 30;
		if (argc > 1)
		{
			duration = wiLua::SGetInt(L, 2);
		}
		bool continuous = false;
		if (argc > 2)
		{
			continuous = wiLua::SGetBool(L, 3);
		}
		wiInputManager::InputType type = wiInputManager::KEYBOARD;
		if (argc > 3)
		{
			type = (wiInputManager::InputType)wiLua::SGetInt(L, 4);
		}
		wiLua::SSetBool(L, wiInputManager::hold((DWORD)code, (DWORD)duration, continuous, type));
		return 1;
	}
	else
		wiLua::SError(L, "Hold(int code, opt int duration = 30, opt boolean continuous = false, opt int type = KEYBOARD) not enough arguments!");
	return 0;
}
int wiInputManager_BindLua::GetPointer(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&wiInputManager::getpointer())));
	return 1;
}
int wiInputManager_BindLua::SetPointer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			XMFLOAT4 props;
			XMStoreFloat4(&props, vec->vector);
			wiInputManager::setpointer(props);
		}
		else
			wiLua::SError(L, "SetPointer(Vector props) argument is not a Vector!");
	}
	else
		wiLua::SError(L, "SetPointer(Vector props) not enough arguments!");
	return 0;
}
int wiInputManager_BindLua::HidePointer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiInputManager::hidepointer(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "HidePointer(bool value) not enough arguments!");
	return 0;
}
int wiInputManager_BindLua::GetTouches(lua_State* L)
{
	auto& touches = wiInputManager::getTouches();
	for (auto& touch : touches)
	{
		Luna<Touch_BindLua>::push(L, new Touch_BindLua(touch));
	}
	return (int)touches.size();
}

void wiInputManager_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiInputManager_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
		wiLua::GetGlobal()->RunText("input = InputManager()");

		//Input types
		wiLua::GetGlobal()->RunText("DIRECTINPUT_JOYPAD		= 0");
		wiLua::GetGlobal()->RunText("XINPUT_JOYPAD			= 1");
		wiLua::GetGlobal()->RunText("KEYBOARD				= 2");
		wiLua::GetGlobal()->RunText("RAWINPUT_JOYPAD		= 3");

		//Keyboard
		wiLua::GetGlobal()->RunText("VK_UP			= 0x26");
		wiLua::GetGlobal()->RunText("VK_DOWN		= 0x28");
		wiLua::GetGlobal()->RunText("VK_LEFT		= 0x25");
		wiLua::GetGlobal()->RunText("VK_RIGHT		= 0x27");
		wiLua::GetGlobal()->RunText("VK_SPACE		= 0x20");
		wiLua::GetGlobal()->RunText("VK_RETURN		= 0x0D");
		wiLua::GetGlobal()->RunText("VK_RSHIFT		= 0xA1");
		wiLua::GetGlobal()->RunText("VK_LSHIFT		= 0xA0");
		wiLua::GetGlobal()->RunText("VK_F1			= 0x70");
		wiLua::GetGlobal()->RunText("VK_F2			= 0x71");
		wiLua::GetGlobal()->RunText("VK_F3			= 0x72");
		wiLua::GetGlobal()->RunText("VK_F4			= 0x73");
		wiLua::GetGlobal()->RunText("VK_F5			= 0x74");
		wiLua::GetGlobal()->RunText("VK_F6			= 0x75");
		wiLua::GetGlobal()->RunText("VK_F7			= 0x76");
		wiLua::GetGlobal()->RunText("VK_F8			= 0x77");
		wiLua::GetGlobal()->RunText("VK_F9			= 0x78");
		wiLua::GetGlobal()->RunText("VK_F10			= 0x79");
		wiLua::GetGlobal()->RunText("VK_F11			= 0x7A");
		wiLua::GetGlobal()->RunText("VK_F12			= 0x7B");

		//Mouse
		wiLua::GetGlobal()->RunText("VK_LBUTTON		= 0x01");
		wiLua::GetGlobal()->RunText("VK_MBUTTON		= 0x04");
		wiLua::GetGlobal()->RunText("VK_RBUTTON		= 0x02");

		//Touch
		wiLua::GetGlobal()->RunText("TOUCHSTATE_PRESSED		= 0");
		wiLua::GetGlobal()->RunText("TOUCHSTATE_RELEASED	= 1");
		wiLua::GetGlobal()->RunText("TOUCHSTATE_MOVED		= 2");
	}

	Touch_BindLua::Bind();
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
		Luna<Touch_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
