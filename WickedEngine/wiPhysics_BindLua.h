#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiPhysics.h"

namespace wi::lua
{
	class Physics_BindLua
	{
	public:
		static const char className[];
		static Luna<Physics_BindLua>::FunctionType methods[];
		static Luna<Physics_BindLua>::PropertyType properties[];

		Physics_BindLua(lua_State* L) {}

		int SetEnabled(lua_State* L);
		int IsEnabled(lua_State* L);
		int SetSimulationEnabled(lua_State* L);
		int IsSimulationEnabled(lua_State* L);
		int SetDebugDrawEnabled(lua_State* L);
		int IsDebugDrawEnabled(lua_State* L);
		int SetAccuracy(lua_State* L);
		int GetAccuracy(lua_State* L);

		int SetLinearVelocity(lua_State* L);
		int SetAngularVelocity(lua_State* L);
		int ApplyForce(lua_State* L);
		int ApplyForceAt(lua_State* L);
		int ApplyImpulse(lua_State* L);
		int ApplyImpulseAt(lua_State* L);
		int ApplyTorque(lua_State* L);
		int ApplyTorqueImpulse(lua_State* L);
		int SetActivationState(lua_State* L);

		static void Bind();
	};
}
