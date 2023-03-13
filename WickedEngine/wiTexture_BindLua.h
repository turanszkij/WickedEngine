#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiGraphics.h"
#include "wiResourceManager.h"

namespace wi::lua
{

	class Texture_BindLua
	{
	public:
		wi::Resource resource;

		inline static constexpr char className[] = "Texture";
		static Luna<Texture_BindLua>::FunctionType methods[];
		static Luna<Texture_BindLua>::PropertyType properties[];

		Texture_BindLua(wi::Resource resource) :resource(resource) {}
		Texture_BindLua(wi::graphics::Texture texture) { resource.SetTexture(texture); }
		Texture_BindLua(lua_State* L);

		static void Bind();
	};

}
