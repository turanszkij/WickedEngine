#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiRenderer.h"

class Texture_BindLua
{
public:
	wiGraphics::Texture texture;

	static const char className[];
	static Luna<Texture_BindLua>::FunctionType methods[];
	static Luna<Texture_BindLua>::PropertyType properties[];

	Texture_BindLua() = default;
	Texture_BindLua(wiGraphics::Texture texture);
	Texture_BindLua(lua_State *L);

	static void Bind();
};

