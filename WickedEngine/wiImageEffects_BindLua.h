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

	static void Bind();
};

