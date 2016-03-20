#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiRenderer.h"

class Texture_BindLua
{
public:
	wiGraphicsTypes::Texture2D* texture;

	static const char className[];
	static Luna<Texture_BindLua>::FunctionType methods[];
	static Luna<Texture_BindLua>::PropertyType properties[];

	Texture_BindLua(wiGraphicsTypes::Texture2D* texture = nullptr);
	Texture_BindLua(lua_State *L);
	~Texture_BindLua();

	static void Bind();
};

