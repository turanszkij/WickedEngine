#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiRenderer.h"

class Texture_BindLua
{
public:
	TextureView texture;

	static const char className[];
	static Luna<Texture_BindLua>::FunctionType methods[];
	static Luna<Texture_BindLua>::PropertyType properties[];

	Texture_BindLua(TextureView texture = nullptr);
	Texture_BindLua(lua_State *L);
	~Texture_BindLua();

	static void Bind();
};

