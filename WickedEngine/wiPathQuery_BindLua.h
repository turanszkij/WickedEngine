#pragma once
#include "CommonInclude.h"
#include "wiLua.h"
#include "wiLuna.h"
#include "wiPathQuery.h"

namespace wi::lua
{
	class PathQuery_BindLua
	{
	public:
		wi::PathQuery pathquery;
		inline static constexpr char className[] = "PathQuery";
		static Luna<PathQuery_BindLua>::FunctionType methods[];
		static Luna<PathQuery_BindLua>::PropertyType properties[];

		PathQuery_BindLua() = default;
		PathQuery_BindLua(lua_State* L) {}

		int Process(lua_State* L);
		int GetNextWaypoint(lua_State* L);
		int SetDebugDrawWaypointsEnabled(lua_State* L);

		static void Bind();
	};
}
