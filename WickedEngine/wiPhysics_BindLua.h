#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiPhysics.h"

namespace wi::lua
{
	class Physics_BindLua
	{
	public:
		inline static constexpr char className[] = "Physics";
		static Luna<Physics_BindLua>::FunctionType methods[];
		static Luna<Physics_BindLua>::PropertyType properties[];

		Physics_BindLua() = default;
		Physics_BindLua(lua_State* L) {}

		int SetEnabled(lua_State* L);
		int IsEnabled(lua_State* L);
		int SetSimulationEnabled(lua_State* L);
		int IsSimulationEnabled(lua_State* L);
		int SetInterpolationEnabled(lua_State* L);
		int IsInterpolationEnabled(lua_State* L);
		int SetDebugDrawEnabled(lua_State* L);
		int IsDebugDrawEnabled(lua_State* L);
		int SetAccuracy(lua_State* L);
		int GetAccuracy(lua_State* L);
		int SetFrameRate(lua_State* L);
		int GetFrameRate(lua_State* L);

		int SetPosition(lua_State* L);
		int SetPositionAndRotation(lua_State* L);
		int SetLinearVelocity(lua_State* L);
		int SetAngularVelocity(lua_State* L);
		int ApplyForce(lua_State* L);
		int ApplyForceAt(lua_State* L);
		int ApplyImpulse(lua_State* L);
		int ApplyImpulseAt(lua_State* L);
		int ApplyTorque(lua_State* L);
		int SetActivationState(lua_State* L);
		int ActivateAllRigidBodies(lua_State* L);
		int ResetPhysicsObjects(lua_State* L);
		int GetVelocity(lua_State* L);
		int GetPosition(lua_State* L);
		int GetRotation(lua_State* L);
		int GetCharacterGroundPosition(lua_State* L);
		int GetCharacterGroundNormal(lua_State* L);
		int GetCharacterGroundVelocity(lua_State* L);
		int IsCharacterGroundSupported(lua_State* L);
		int GetCharacterGroundState(lua_State* L);
		int ChangeCharacterShape(lua_State* L);
		int MoveCharacter(lua_State* L);
		int SetGhostMode(lua_State* L);
		int SetRagdollGhostMode(lua_State* L);

		int Intersects(lua_State* L);
		int PickDrag(lua_State* L);

		int DriveVehicle(lua_State* L);
		int GetVehicleForwardVelocity(lua_State* L);

		static void Bind();
	};

	class PickDragOperation_BindLua
	{
	public:
		wi::physics::PickDragOperation op;
		inline static constexpr char className[] = "PickDragOperation";
		static Luna<PickDragOperation_BindLua>::FunctionType methods[];
		static Luna<PickDragOperation_BindLua>::PropertyType properties[];

		PickDragOperation_BindLua() = default;
		PickDragOperation_BindLua(lua_State* L) {}

		int Finish(lua_State* L);
	};
}
