#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiImageEffects.h"

class wiImageEffects_BindLua
{
public:
	wiImageEffects effects;

	static const char className[];
	static Luna<wiImageEffects_BindLua>::FunctionType methods[];
	static Luna<wiImageEffects_BindLua>::PropertyType properties[];

	wiImageEffects_BindLua(const wiImageEffects& effects);
	wiImageEffects_BindLua(lua_State *L);
	~wiImageEffects_BindLua();

	int GetPos(lua_State* L);
	int GetSize(lua_State* L);
	int GetOpacity(lua_State* L);
	int GetFade(lua_State* L);
	int GetRotation(lua_State* L);
	int GetMipLevel(lua_State* L);

	int SetPos(lua_State* L);
	int SetSize(lua_State* L);
	int SetOpacity(lua_State* L);
	int SetFade(lua_State* L);
	int SetRotation(lua_State* L);
	int SetMipLevel(lua_State* L);

	static void Bind();
};

