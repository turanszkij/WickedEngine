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
	~wiInput_BindLua(){}

	int Down(lua_State* L);
	int Press(lua_State* L);
	int Hold(lua_State* L);
	int GetPointer(lua_State* L);
	int SetPointer(lua_State* L);
	int HidePointer(lua_State* L);
	int GetAnalog(lua_State* L);
	int GetTouches(lua_State* L);

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
	~Touch_BindLua() {}

	int GetState(lua_State* L);
	int GetPos(lua_State* L);

	static void Bind();
};
