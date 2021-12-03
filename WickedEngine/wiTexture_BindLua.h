#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiGraphics.h"

namespace wi::lua
{

	class Texture_BindLua
	{
	public:
		wi::graphics::Texture texture;

		static const char className[];
		static Luna<Texture_BindLua>::FunctionType methods[];
		static Luna<Texture_BindLua>::PropertyType properties[];

		Texture_BindLua() = default;
		Texture_BindLua(wi::graphics::Texture texture);
		Texture_BindLua(lua_State* L);

		static void Bind();
	};

}
