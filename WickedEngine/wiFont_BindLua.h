#pragma once
#include "wiLua.h"
#include "wiLuna.h"

class wiFont;

class wiFont_BindLua
{
public:
	wiFont* font;

	static const char className[];
	static Luna<wiFont_BindLua>::FunctionType methods[];
	static Luna<wiFont_BindLua>::PropertyType properties[];

	wiFont_BindLua(wiFont* font);
	wiFont_BindLua(lua_State* L);
	~wiFont_BindLua();

	int GetProperties(lua_State* L);
	int SetProperties(lua_State* L);
	int SetText(lua_State* L);
	int GetText(lua_State* L);


	static void Bind();
};

