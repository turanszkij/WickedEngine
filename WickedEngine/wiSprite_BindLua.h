#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSprite.h"

namespace wi::lua
{
	class Sprite_BindLua
	{
	public:
		wi::Sprite sprite;

		static const char className[];
		static Luna<Sprite_BindLua>::FunctionType methods[];
		static Luna<Sprite_BindLua>::PropertyType properties[];

		Sprite_BindLua(const wi::Sprite& sprite);
		Sprite_BindLua(lua_State* L);

		int SetParams(lua_State* L);
		int GetParams(lua_State* L);
		int SetAnim(lua_State* L);
		int GetAnim(lua_State* L);

		static void Bind();
	};
}
