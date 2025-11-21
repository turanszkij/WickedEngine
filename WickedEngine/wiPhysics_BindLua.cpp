#include "wiPhysics_BindLua.h"
#include "wiPhysics.h"
#include "wiScene_BindLua.h"
#include "wiMath_BindLua.h"
#include "wiPrimitive_BindLua.h"

namespace wi::lua
{
	Luna<Physics_BindLua>::FunctionType Physics_BindLua::methods[] = {
		lunamethod(Physics_BindLua, SetEnabled),
		lunamethod(Physics_BindLua, IsEnabled),
		lunamethod(Physics_BindLua, SetSimulationEnabled),
		lunamethod(Physics_BindLua, IsSimulationEnabled),
		lunamethod(Physics_BindLua, SetInterpolationEnabled),
		lunamethod(Physics_BindLua, IsInterpolationEnabled),
		lunamethod(Physics_BindLua, SetDebugDrawEnabled),
		lunamethod(Physics_BindLua, IsDebugDrawEnabled),
		lunamethod(Physics_BindLua, SetAccuracy),
		lunamethod(Physics_BindLua, GetAccuracy),
		lunamethod(Physics_BindLua, SetFrameRate),
		lunamethod(Physics_BindLua, GetFrameRate),
		lunamethod(Physics_BindLua, SetPosition),
		lunamethod(Physics_BindLua, SetPositionAndRotation),
		lunamethod(Physics_BindLua, SetLinearVelocity),
		lunamethod(Physics_BindLua, SetAngularVelocity),
		lunamethod(Physics_BindLua, ApplyForceAt),
		lunamethod(Physics_BindLua, ApplyForce),
		lunamethod(Physics_BindLua, ApplyForceAt),
		lunamethod(Physics_BindLua, ApplyImpulse),
		lunamethod(Physics_BindLua, ApplyImpulseAt),
		lunamethod(Physics_BindLua, ApplyTorque),
		lunamethod(Physics_BindLua, SetActivationState),
		lunamethod(Physics_BindLua, ActivateAllRigidBodies),
		lunamethod(Physics_BindLua, ResetPhysicsObjects),
		lunamethod(Physics_BindLua, GetVelocity),
		lunamethod(Physics_BindLua, GetPosition),
		lunamethod(Physics_BindLua, GetRotation),
		lunamethod(Physics_BindLua, GetCharacterGroundPosition),
		lunamethod(Physics_BindLua, GetCharacterGroundNormal),
		lunamethod(Physics_BindLua, GetCharacterGroundVelocity),
		lunamethod(Physics_BindLua, IsCharacterGroundSupported),
		lunamethod(Physics_BindLua, GetCharacterGroundState),
		lunamethod(Physics_BindLua, ChangeCharacterShape),
		lunamethod(Physics_BindLua, MoveCharacter),
		lunamethod(Physics_BindLua, SetGhostMode),
		lunamethod(Physics_BindLua, SetRagdollGhostMode),
		lunamethod(Physics_BindLua, Intersects),
		lunamethod(Physics_BindLua, PickDrag),
		lunamethod(Physics_BindLua, DriveVehicle),
		lunamethod(Physics_BindLua, GetVehicleForwardVelocity),
		lunamethod(Physics_BindLua, SetCharacterCollisionTolerance),
		lunamethod(Physics_BindLua, GetCharacterCollisionTolerance),
		{ NULL, NULL }
	};
	Luna<Physics_BindLua>::PropertyType Physics_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int Physics_BindLua::SetEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::physics::SetEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, wi::physics::IsEnabled());
		return 1;
	}
	int Physics_BindLua::SetSimulationEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::physics::SetSimulationEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetSimulationEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsSimulationEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, wi::physics::IsSimulationEnabled());
		return 1;
	}
	int Physics_BindLua::SetInterpolationEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::physics::SetInterpolationEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetInterpolationEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsInterpolationEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, wi::physics::IsInterpolationEnabled());
		return 1;
	}
	int Physics_BindLua::SetDebugDrawEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::physics::SetDebugDrawEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetDebugDrawEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsDebugDrawEnabled(lua_State* L)
	{
		wi::lua::SSetBool(L, wi::physics::IsDebugDrawEnabled());
		return 1;
	}
	int Physics_BindLua::SetAccuracy(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::physics::SetAccuracy(wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "SetAccuracy(int value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetAccuracy(lua_State* L)
	{
		wi::lua::SSetInt(L, wi::physics::GetAccuracy());
		return 1;
	}
	int Physics_BindLua::SetFrameRate(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::physics::SetFrameRate(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetFrameRate(float value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetFrameRate(lua_State* L)
	{
		wi::lua::SSetFloat(L, wi::physics::GetFrameRate());
		return 1;
	}

	int Physics_BindLua::SetPosition(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) second argument is not a Vector!");
				return 0;
			}
			wi::physics::SetPosition(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			wi::lua::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetPositionAndRotation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec2 == nullptr)
			{
				wi::lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) third argument is not a Vector!");
				return 0;
			}
			wi::physics::SetPositionAndRotation(
				*component->component,
				vec->GetFloat3(),
				vec2->data
			);
		}
		else
			wi::lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetLinearVelocity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) second argument is not a Vector!");
				return 0;
			}
			wi::physics::SetLinearVelocity(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			wi::lua::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetAngularVelocity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) second argument is not a Vector!");
				return 0;
			}
			wi::physics::SetAngularVelocity(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			wi::lua::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyForce(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) second argument is not a Vector!");
				return 0;
			}
			wi::physics::ApplyForce(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			wi::lua::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyForceAt(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) third argument is not a Vector!");
				return 0;
			}
			bool at_local = true;
			if (argc > 3)
				at_local = wi::lua::SGetBool(L, 4);

			wi::physics::ApplyForceAt(
				*component->component,
				*(XMFLOAT3*)vec,
				*(XMFLOAT3*)vec2,
				at_local
			);
		}
		else
			wi::lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyImpulse(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				scene::HumanoidComponent_BindLua* humanoid = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
				if (humanoid == nullptr)
				{
					wi::lua::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) first argument is not a RigidBodyPhysicsComponent!");
					wi::lua::SError(L, "ApplyImpulse(HumanoidComponent component, HumanoidBone bone, Vector impulse) first argument is not a HumanoidComponent!");
					return 0;
				}
				wi::scene::HumanoidComponent::HumanoidBone bone = (wi::scene::HumanoidComponent::HumanoidBone)wi::lua::SGetInt(L, 2);
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
				if (vec == nullptr)
				{
					wi::lua::SError(L, "ApplyImpulse(HumanoidComponent component, HumanoidBone bone, Vector impulse) third argument is not a Vector!");
					return 0;
				}
				wi::physics::ApplyImpulse(
					*humanoid->component,
					bone,
					*(XMFLOAT3*)vec
				);
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) second argument is not a Vector!");
				return 0;
			}
			wi::physics::ApplyImpulse(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			wi::lua::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyImpulseAt(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				scene::HumanoidComponent_BindLua* humanoid = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
				if (humanoid == nullptr)
				{
					wi::lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) first argument is not a RigidBodyPhysicsComponent!");
					wi::lua::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) first argument is not a HumanoidComponent!");
					return 0;
				}
				wi::scene::HumanoidComponent::HumanoidBone bone = (wi::scene::HumanoidComponent::HumanoidBone)wi::lua::SGetInt(L, 2);
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
				if (vec == nullptr)
				{
					wi::lua::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) third argument is not a Vector!");
					return 0;
				}
				Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 4);
				if (vec2 == nullptr)
				{
					wi::lua::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) fourth argument is not a Vector!");
					return 0;
				}
				bool at_local = true;
				if (argc > 4)
					at_local = wi::lua::SGetBool(L, 5);

				wi::physics::ApplyImpulseAt(
					*humanoid->component,
					bone,
					*(XMFLOAT3*)vec,
					*(XMFLOAT3*)vec2,
					at_local
				);
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) third argument is not a Vector!");
				return 0;
			}
			bool at_local = true;
			if (argc > 3)
				at_local = wi::lua::SGetBool(L, 4);

			wi::physics::ApplyImpulseAt(
				*component->component,
				*(XMFLOAT3*)vec,
				*(XMFLOAT3*)vec2,
				at_local
			);
		}
		else
			wi::lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyTorque(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) second argument is not a Vector!");
				return 0;
			}
			wi::physics::ApplyTorque(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			wi::lua::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetActivationState(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* rigid = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rigid == nullptr)
			{
				scene::SoftBodyPhysicsComponent_BindLua* soft = Luna<scene::SoftBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
				if (soft == nullptr)
				{
					wi::lua::SError(L, "SetActivationState(RigidBodyPhysicsComponent | SoftBodyPhysicsComponent component, int state) first argument is not a RigidBodyPhysicsComponent or SoftBodyPhysicsComponent!");
					return 0;
				}
				wi::physics::SetActivationState(
					*soft->component,
					(wi::physics::ActivationState)wi::lua::SGetInt(L, 2)
				);
				return 0;
			}
			wi::physics::SetActivationState(
				*rigid->component,
				(wi::physics::ActivationState)wi::lua::SGetInt(L, 2)
			);
		}
		else
			wi::lua::SError(L, "SetActivationState(RigidBodyPhysicsComponent | SoftBodyPhysicsComponent component, int state) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ActivateAllRigidBodies(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				wi::lua::SError(L, "ActivateAllRigidBodies(Scene scene) first argument is not a Scene!");
				return 0;
			}
			wi::physics::ActivateAllRigidBodies(*scene->scene);
		}
		else
			wi::lua::SError(L, "ActivateAllRigidBodies(Scene scene) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ResetPhysicsObjects(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				wi::lua::SError(L, "ResetPhysicsObjects(Scene scene) first argument is not a Scene!");
				return 0;
			}
			wi::physics::ResetPhysicsObjects(*scene->scene);
		}
		else
			wi::lua::SError(L, "ResetPhysicsObjects(Scene scene) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetVelocity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "GetVelocity(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, wi::physics::GetVelocity(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "GetVelocity(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetPosition(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "GetPosition(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, wi::physics::GetPosition(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "GetPosition(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetRotation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "GetRotation(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, wi::physics::GetRotation(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "GetRotation(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundPosition(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "GetCharacterGroundPosition(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, wi::physics::GetCharacterGroundPosition(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "GetCharacterGroundPosition(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundNormal(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "GetCharacterGroundNormal(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, wi::physics::GetCharacterGroundNormal(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "GetCharacterGroundNormal(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundVelocity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "GetCharacterGroundVelocity(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, wi::physics::GetCharacterGroundVelocity(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "GetCharacterGroundVelocity(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsCharacterGroundSupported(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "IsCharacterGroundSupported(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			wi::lua::SSetBool(L, wi::physics::IsCharacterGroundSupported(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "IsCharacterGroundSupported(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundState(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "GetCharacterGroundState(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			wi::lua::SSetInt(L, (int)wi::physics::GetCharacterGroundState(*component->component));
			return 1;
		}
		else
			wi::lua::SError(L, "GetCharacterGroundState(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ChangeCharacterShape(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "ChangeCharacterShape(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			wi::scene::RigidBodyPhysicsComponent::CapsuleParams capsule;
			if (argc > 1)
			{
				capsule.height = wi::lua::SGetFloat(L, 2);
				if (argc > 2)
				{
					capsule.radius = wi::lua::SGetFloat(L, 3);
				}
			}
			wi::lua::SSetBool(L, wi::physics::ChangeCharacterShape(*component->component, capsule));
			return 1;
		}
		else
			wi::lua::SError(L, "ChangeCharacterShape(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::MoveCharacter(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) second argument is not a Vector!");
				return 0;
			}

			float movement_speed = 6.0f;
			float jump = 0;
			bool controlMovementDuringJump = false;

			if (argc > 2)
			{
				movement_speed = wi::lua::SGetFloat(L, 3);
				if (argc > 3)
				{
					jump = wi::lua::SGetFloat(L, 4);
					if (argc > 4)
					{
						controlMovementDuringJump = wi::lua::SGetBool(L, 5);
					}
				}
			}

			wi::physics::MoveCharacter(
				*component->component,
				vec->GetFloat3(),
				movement_speed,
				jump,
				controlMovementDuringJump
			);
		}
		else
			wi::lua::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetGhostMode(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::RigidBodyPhysicsComponent_BindLua* component = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component != nullptr)
			{
				bool value = wi::lua::SGetBool(L, 2);
				wi::physics::SetGhostMode(*component->component, value);
				return 0;
			}
			scene::HumanoidComponent_BindLua* humanoidcomponent = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
			if (humanoidcomponent != nullptr)
			{
				bool value = wi::lua::SGetBool(L, 2);
				wi::physics::SetGhostMode(*humanoidcomponent->component, value);
				return 0;
			}
		}
		else
			wi::lua::SError(L, "SetGhostMode(RigidBodyPhysicsComponent|HumanoidComponent component, bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetRagdollGhostMode(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::HumanoidComponent_BindLua* component = Luna<scene::HumanoidComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				wi::lua::SError(L, "SetRagdollGhostMode(HumanoidComponent component, bool value) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			bool value = wi::lua::SGetBool(L, 2);

			wi::physics::SetRagdollGhostMode(*component->component, value);
			return 0;
		}
		else
			wi::lua::SError(L, "SetRagdollGhostMode(HumanoidComponent component, bool value) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::SetCharacterCollisionTolerance(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::physics::SetCharacterCollisionTolerance(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetCharacterCollisionTolerance(float value) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::GetCharacterCollisionTolerance(lua_State* L)
	{
		wi::lua::SSetFloat(L, wi::physics::GetCharacterCollisionTolerance());
		return 1;
	}

	int Physics_BindLua::Intersects(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				wi::lua::SError(L, "Intersects(Scene, Ray) first argument is not a Scene!");
				return 0;
			}
			primitive::Ray_BindLua* ray = Luna<primitive::Ray_BindLua>::lightcheck(L, 2);
			if (ray == nullptr)
			{
				wi::lua::SError(L, "Intersects(Scene, Ray) second argument is not a Ray!");
				return 0;
			}

			wi::physics::RayIntersectionResult result = wi::physics::Intersects(*scene->scene, ray->ray);
			wi::lua::SSetLongLong(L, result.entity);
			Luna<Vector_BindLua>::push(L, result.position);
			Luna<Vector_BindLua>::push(L, result.normal);
			wi::lua::SSetLongLong(L, result.humanoid_ragdoll_entity);
			wi::lua::SSetInt(L, (int)result.humanoid_bone);
			Luna<Vector_BindLua>::push(L, result.position_local);
			return 6;
		}
		wi::lua::SError(L, "Intersects(Scene, Ray) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::PickDrag(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			scene::Scene_BindLua* scene = Luna<scene::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				wi::lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) first argument is not a Scene!");
				return 0;
			}
			primitive::Ray_BindLua* ray = Luna<primitive::Ray_BindLua>::lightcheck(L, 2);
			if (ray == nullptr)
			{
				wi::lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) second argument is not a Ray!");
				return 0;
			}
			PickDragOperation_BindLua* op = Luna<PickDragOperation_BindLua>::lightcheck(L, 3);
			if (op == nullptr)
			{
				wi::lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) third argument is not a PickDragOperation!");
				return 0;
			}

			wi::physics::ConstraintType type = wi::physics::ConstraintType::Fixed;
			float break_distance = FLT_MAX;

			if (argc > 3)
			{
				type = (wi::physics::ConstraintType)wi::lua::SGetInt(L, 4);

				if (argc > 4)
				{
					break_distance = wi::lua::SGetFloat(L, 5);
				}
			}

			wi::physics::PickDrag(*scene->scene, ray->ray, op->op, type, break_distance);
			return 0;
		}
		wi::lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::DriveVehicle(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* rb = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rb == nullptr)
			{
				wi::lua::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			if (!rb->component->IsVehicle())
			{
				wi::lua::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) argument is a RigidBodyPhysicsComponent, but not a vehicle!");
				return 0;
			}

			float forward = 0;
			float right = 0;
			float brake = 0;
			float handbrake = 0;

			if (argc > 1)
			{
				forward = wi::lua::SGetFloat(L, 2);

				if (argc > 2)
				{
					right = wi::lua::SGetFloat(L, 3);

					if (argc > 3)
					{
						brake = wi::lua::SGetFloat(L, 4);

						if (argc > 4)
						{
							handbrake = wi::lua::SGetFloat(L, 5);
						}
					}
				}
			}

			wi::physics::DriveVehicle(*rb->component, forward, right, brake, handbrake);
			return 0;
		}
		wi::lua::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetVehicleForwardVelocity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			scene::RigidBodyPhysicsComponent_BindLua* rb = Luna<scene::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rb == nullptr)
			{
				wi::lua::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			if (!rb->component->IsVehicle())
			{
				wi::lua::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) argument is a RigidBodyPhysicsComponent, but not a vehicle!");
				return 0;
			}

			wi::lua::SSetFloat(L, wi::physics::GetVehicleForwardVelocity(*rb->component));
			return 1;
		}
		wi::lua::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) not enough arguments!");
		return 0;
	}

	void Physics_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<PickDragOperation_BindLua>::Register(wi::lua::GetLuaState());
			Luna<Physics_BindLua>::Register(wi::lua::GetLuaState());
			Luna<Physics_BindLua>::push_global(wi::lua::GetLuaState(), "physics");

			wi::lua::RunText(R"(
ACTIVATION_STATE_ACTIVE = 0
ACTIVATION_STATE_INACTIVE = 1

ConstraintType = {
	Fixed = 0,
	Point = 1
}

CharacterGroundStates = {
	OnGround = 0,
	OnSteepGround = 1,
	NotSupported = 2,
	InAir = 3,
}
)");
		}
	}




	Luna<PickDragOperation_BindLua>::FunctionType PickDragOperation_BindLua::methods[] = {
		lunamethod(PickDragOperation_BindLua, Finish),
		{ NULL, NULL }
	};
	Luna<PickDragOperation_BindLua>::PropertyType PickDragOperation_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int PickDragOperation_BindLua::Finish(lua_State* L)
	{
		op = {};
		return 0;
	}
}
