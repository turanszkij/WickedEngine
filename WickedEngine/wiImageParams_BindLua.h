#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiImage.h"

class wiImageParams_BindLua
{
public:
	wiImageParams params;

	static const char className[];
	static Luna<wiImageParams_BindLua>::FunctionType methods[];
	static Luna<wiImageParams_BindLua>::PropertyType properties[];

	wiImageParams_BindLua(const wiImageParams& params);
	wiImageParams_BindLua(lua_State *L);
	~wiImageParams_BindLua();

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

