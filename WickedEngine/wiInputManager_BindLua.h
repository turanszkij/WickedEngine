#pragma once
#include "wiLua.h"
#include "wiLuna.h"

class wiInputManager_BindLua
{
public:
	static const char className[];
	static Luna<wiInputManager_BindLua>::FunctionType methods[];
	static Luna<wiInputManager_BindLua>::PropertyType properties[];

	wiInputManager_BindLua(lua_State* L){}
	~wiInputManager_BindLua(){}

	int Down(lua_State* L);
	int Press(lua_State* L);
	int Hold(lua_State* L);
	int Pointer(lua_State* L);
	int SetPointer(lua_State* L);


	static void Bind();
};
