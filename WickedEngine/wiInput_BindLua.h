#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiInput.h"

class wiInput_BindLua
{
public:
	static const char className[];
	static Luna<wiInput_BindLua>::FunctionType methods[];
	static Luna<wiInput_BindLua>::PropertyType properties[];

	wiInput_BindLua(lua_State* L){}

	int Down(lua_State* L);
	int Press(lua_State* L);
	int Hold(lua_State* L);
	int GetPointer(lua_State* L);
	int SetPointer(lua_State* L);
	int GetPointerDelta(lua_State* L);
	int HidePointer(lua_State* L);
	int GetAnalog(lua_State* L);
	int GetTouches(lua_State* L);
	int SetControllerFeedback(lua_State* L);

	static void Bind();
};

class Touch_BindLua
{
public:
	wiInput::Touch touch;
	static const char className[];
	static Luna<Touch_BindLua>::FunctionType methods[];
	static Luna<Touch_BindLua>::PropertyType properties[];

	Touch_BindLua(lua_State* L) {}
	Touch_BindLua(const wiInput::Touch& touch) :touch(touch) {}

	int GetState(lua_State* L);
	int GetPos(lua_State* L);

	static void Bind();
};

class ControllerFeedback_BindLua
{
public:
	wiInput::ControllerFeedback feedback;
	static const char className[];
	static Luna<ControllerFeedback_BindLua>::FunctionType methods[];
	static Luna<ControllerFeedback_BindLua>::PropertyType properties[];

	ControllerFeedback_BindLua(lua_State* L) {}
	ControllerFeedback_BindLua(const wiInput::ControllerFeedback& feedback) :feedback(feedback) {}

	int SetVibrationLeft(lua_State* L);
	int SetVibrationRight(lua_State* L);
	int SetLEDColor(lua_State* L);

	static void Bind();
};
