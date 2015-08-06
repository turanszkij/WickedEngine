#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSprite.h"

class wiSprite_BindLua
{
private:
	wiSprite* sprite;
public:
	static const char className[];
	static Luna<wiSprite_BindLua>::FunctionType methods[];
	static Luna<wiSprite_BindLua>::PropertyType properties[];

	wiSprite_BindLua(wiSprite* sprite = nullptr);
	wiSprite_BindLua(lua_State *L);
	~wiSprite_BindLua();

	static void Bind();
};

