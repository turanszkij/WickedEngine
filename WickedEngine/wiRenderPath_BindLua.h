#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiRenderPath.h"

namespace wi::lua
{
	class RenderPath_BindLua
	{
	public:
		RenderPath* component = nullptr;
		inline static constexpr char className[] = "RenderPath";
		static Luna<RenderPath_BindLua>::FunctionType methods[];
		static Luna<RenderPath_BindLua>::PropertyType properties[];

		RenderPath_BindLua() = default;
		RenderPath_BindLua(RenderPath* component) :component(component) {}
		RenderPath_BindLua(lua_State* L) {}
		virtual ~RenderPath_BindLua() = default;

		int GetLayerMask(lua_State* L);
		int SetLayerMask(lua_State* L);

		static void Bind();
	};
}
