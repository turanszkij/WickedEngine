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
		int SearchCover(lua_State* L);
		int IsSuccessful(lua_State* L);
		int GetNextWaypoint(lua_State* L);
		int SetDebugDrawWaypointsEnabled(lua_State* L);
		int SetFlying(lua_State* L);
		int IsFlying(lua_State* L);
		int SetAgentWidth(lua_State* L);
		int GetAgentWidth(lua_State* L);
		int SetAgentHeight(lua_State* L);
		int GetAgentHeight(lua_State* L);
		int GetWaypointCount(lua_State* L);
		int GetWaypoint(lua_State* L);
		int GetGoal(lua_State* L);

		static void Bind();
	};
}
