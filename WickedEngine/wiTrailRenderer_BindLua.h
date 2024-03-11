#pragma once
#include "CommonInclude.h"
#include "wiLua.h"
#include "wiLuna.h"
#include "wiTrailRenderer.h"

namespace wi::lua
{
	class TrailRenderer_BindLua
	{
	public:
		wi::TrailRenderer trail;

		inline static constexpr char className[] = "TrailRenderer";
		static Luna<TrailRenderer_BindLua>::FunctionType methods[];
		static Luna<TrailRenderer_BindLua>::PropertyType properties[];

		TrailRenderer_BindLua() = default;
		TrailRenderer_BindLua(lua_State* L) {}
		TrailRenderer_BindLua(wi::TrailRenderer& ref) : trail(ref) {}
		TrailRenderer_BindLua(wi::TrailRenderer* ref) : trail(*ref) {}

		int AddPoint(lua_State* L);
		int Cut(lua_State* L);
		int Clear(lua_State* L);
		int GetPointCount(lua_State* L);
		int GetPoint(lua_State* L);
		int SetPoint(lua_State* L);
		int SetBlendMode(lua_State* L);
		int GetBlendMode(lua_State* L);
		int SetSubdivision(lua_State* L);
		int GetSubdivision(lua_State* L);
		int SetWidth(lua_State* L);
		int GetWidth(lua_State* L);
		int SetColor(lua_State* L);
		int GetColor(lua_State* L);
		int SetTexture(lua_State* L);
		int GetTexture(lua_State* L);
		int SetTexture2(lua_State* L);
		int GetTexture2(lua_State* L);
		int SetTexMulAdd(lua_State* L);
		int GetTexMulAdd(lua_State* L);
		int SetTexMulAdd2(lua_State* L);
		int GetTexMulAdd2(lua_State* L);

		static void Bind();
	};
}
