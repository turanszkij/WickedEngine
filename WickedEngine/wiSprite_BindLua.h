#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSprite.h"

class wiSprite_BindLua
{
public:
	wiSprite sprite;

	static const char className[];
	static Luna<wiSprite_BindLua>::FunctionType methods[];
	static Luna<wiSprite_BindLua>::PropertyType properties[];

	wiSprite_BindLua(const wiSprite& sprite);
	wiSprite_BindLua(lua_State *L);

	int SetParams(lua_State *L);
	int GetParams(lua_State *L);
	int SetAnim(lua_State *L);
	int GetAnim(lua_State *L);

	static void Bind();
};

