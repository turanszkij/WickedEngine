#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSpriteFont.h"

class wiSpriteFont_BindLua
{
public:
	wiSpriteFont font;

	static const char className[];
	static Luna<wiSpriteFont_BindLua>::FunctionType methods[];
	static Luna<wiSpriteFont_BindLua>::PropertyType properties[];

	wiSpriteFont_BindLua(const wiSpriteFont& font);
	wiSpriteFont_BindLua(lua_State* L);

	int SetStyle(lua_State* L);
	int SetText(lua_State* L);
	int SetSize(lua_State* L);
	int SetPos(lua_State* L);
	int SetSpacing(lua_State* L);
	int SetAlign(lua_State* L);
	int SetColor(lua_State* L);
	int SetShadowColor(lua_State* L);

	int GetText(lua_State* L);
	int GetSize(lua_State* L);
	int GetPos(lua_State* L);
	int GetSpacing(lua_State* L);
	int GetAlign(lua_State* L);
	int GetColor(lua_State* L);
	int GetShadowColor(lua_State* L);

	static void Bind();
};

