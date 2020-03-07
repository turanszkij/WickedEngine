#pragma once
#include "wiLua.h"
#include "wiLuna.h"

class wiNetwork_BindLua
{
public:
	static const char className[];
	static Luna<wiNetwork_BindLua>::FunctionType methods[];
	static Luna<wiNetwork_BindLua>::PropertyType properties[];

	wiNetwork_BindLua(lua_State* L) {}

	static void Bind();
};
