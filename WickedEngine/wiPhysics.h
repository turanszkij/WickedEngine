#pragma once
#include "wiECS.h"
#include "wiScene.h"
#include "wiJobSystem.h"
#include "wiPrimitive.h"

#include <memory>

namespace wi::physics
{
	// Initializes the physics engine
	void Initialize();

	// Enable/disable the physics engine all together
	void SetEnabled(bool value);
	bool IsEnabled();

	// Enable/disable the physics simulation.
	//	Physics engine state will be updated but not simulated
	void SetSimulationEnabled(bool value);
	bool IsSimulationEnabled();

	// Enable/disable the physics interpolation.
	//	When enabled, physics simulation's fixed frame rate will be
	//	interpolated to match the variable frame rate of rendering
	void SetInterpolationEnabled(bool value);
	bool IsInterpolationEnabled();

	// Enable/disable debug drawing of physics objects
	void SetDebugDrawEnabled(bool value);
	bool IsDebugDrawEnabled();

	// Adjust constraint debugging sizes
	void SetConstraintDebugSize(float value);
	float GetConstraintDebugSize();

	// Set the accuracy of the simulation
	//	This value corresponds to maximum simulation step count
	//	Higher values will be slower but more accurate
	void SetAccuracy(int value);
	int GetAccuracy();

	// Set frames per second value of physics simulation (default = 120 FPS)
	void SetFrameRate(float value);
	float GetFrameRate();

	// Update the physics state, run simulation, etc.
	void RunPhysicsUpdateSystem(
		wi::jobsystem::context& ctx,
		wi::scene::Scene& scene,
		float dt
	);

	// Teleport a dynamic rigid body:
	void SetPosition(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& position
	);
	void SetPositionAndRotation(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& position,
		const XMFLOAT4& rotation
	);

	// Set linear velocity to rigid body
	void SetLinearVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	);
	// Set angular velocity to rigid body
	void SetAngularVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	);

	XMFLOAT3 GetVelocity(wi::scene::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT3 GetPosition(wi::scene::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT4 GetRotation(wi::scene::RigidBodyPhysicsComponent& physicscomponent);

	XMFLOAT3 GetCharacterGroundPosition(wi::scene::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT3 GetCharacterGroundNormal(wi::scene::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT3 GetCharacterGroundVelocity(wi::scene::RigidBodyPhysicsComponent& physicscomponent);

	// Returns true if character is standing on the ground or steep ground, false otherwise
	bool IsCharacterGroundSupported(wi::scene::RigidBodyPhysicsComponent& physicscomponent);

	enum class CharacterGroundStates
	{
		OnGround,		// Character is on the ground and can move freely.
		OnSteepGround,	// Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
		NotSupported,	// Character is touching an object, but is not supported by it and should fall. The GetGroundXXX functions will return information about the touched object.
		InAir,			// Character is in the air and is not touching anything.
	};
	CharacterGroundStates GetCharacterGroundState(wi::scene::RigidBodyPhysicsComponent& physicscomponent);

	// Tries to change the physics character's shape, returns tru if successful, false if character would be blocked with the new shape
	bool ChangeCharacterShape(wi::scene::RigidBodyPhysicsComponent& physicscomponent, const wi::scene::RigidBodyPhysicsComponent::CapsuleParams& capsule);

	// Apply movement logic to a physics character
	//	movement_direction : a normalized or zero direction
	//	movement_speed	: horizontal movement speed of the character
	//	jump	: vertical jump speed (positive)
	//	controlMovementDuringJump	: whether movement can be applied when character is in the air
	void MoveCharacter(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& movement_direction,
		float movement_speed = 6,
		float jump = 0,
		bool controlMovementDuringJump = false
	);

	// Apply force at body center
	void ApplyForce(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	);
	// Apply force at position
	void ApplyForceAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force,
		const XMFLOAT3& at,
		bool at_local = true // whether at is in local space of the body or not (global)
	);

	// Apply impulse at body center
	void ApplyImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	);
	void ApplyImpulse(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse
	);
	// Apply impulse at position
	void ApplyImpulseAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at,
		bool at_local = true // whether at is in local space of the body or not (global)
	);
	void ApplyImpulseAt(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at,
		bool at_local = true // whether at is in local space of the body or not (global)
	);

	void ApplyTorque(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	);

	// Set input from driver
	//	forward:	Value between -1 and 1 for auto transmission and value between 0 and 1 indicating desired driving direction and amount the gas pedal is pressed
	//	right:		Value between -1 and 1 indicating desired steering angle (1 = right)
	//	brake:		Value between 0 and 1 indicating how strong the brake pedal is pressed
	//	handbrake:	Value between 0 and 1 indicating how strong the hand brake is pulled (back brake for motorcycles)
	void DriveVehicle(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		float forward,
		float right = 0,
		float brake = 0,
		float handbrake = 0
	);

	// Signed velocity amount in forward direction
	float GetVehicleForwardVelocity(wi::scene::RigidBodyPhysicsComponent& physicscomponent);

	// Override all vehicle wheel transforms in world space
	void OverrideWehicleWheelTransforms(wi::scene::Scene& scene);

	bool IsConstraintBroken(const wi::scene::PhysicsConstraintComponent& physicscomponent);
	void SetConstraintBroken(wi::scene::PhysicsConstraintComponent& physicscomponent, bool broken = true);

	enum class ActivationState
	{
		Active,
		Inactive,
	};
	void SetActivationState(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		ActivationState state
	);
	void SetActivationState(
		wi::scene::SoftBodyPhysicsComponent& physicscomponent,
		ActivationState state
	);
	void ActivateAllRigidBodies(wi::scene::Scene& scene);

	void ResetPhysicsObjects(wi::scene::Scene& scene);

	XMFLOAT3 GetSoftBodyNodePosition(
		wi::scene::SoftBodyPhysicsComponent& physicscomponent,
		uint32_t physicsIndex
	);

	void SetGhostMode(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		bool value
	);
	void SetGhostMode(
		wi::scene::HumanoidComponent& humanoid,
		bool value
	);
	void SetRagdollGhostMode(wi::scene::HumanoidComponent& humanoid, bool value);

	struct RayIntersectionResult
	{
		wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 position_local = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		wi::ecs::Entity humanoid_ragdoll_entity = wi::ecs::INVALID_ENTITY;
		wi::scene::HumanoidComponent::HumanoidBone humanoid_bone = wi::scene::HumanoidComponent::HumanoidBone::Count;
		int softbody_triangleID = -1;
		const void* physicsobject = nullptr;
		constexpr bool IsValid() const { return entity != wi::ecs::INVALID_ENTITY; }
	};
	RayIntersectionResult Intersects(
		const wi::scene::Scene& scene,
		wi::primitive::Ray ray
	);

	struct PickDragOperation
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state != nullptr; }
	};
	enum class ConstraintType
	{
		Fixed,	// Fixes the whole object with the current orientation
		Point,	// Fixes the object to rotate around a point
	};
	void PickDrag(
		const wi::scene::Scene& scene,
		wi::primitive::Ray ray,
		PickDragOperation& op,
		ConstraintType constraint_type = ConstraintType::Fixed,
		float break_distance = FLT_MAX
	);

	// Create the shape immediately, useful when you want to do this from a thread
	//	Otherwise this will be created on demand before it's used for the first time
	void CreateRigidBodyShape(
		wi::scene::Scene& scene,
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& scale_local = XMFLOAT3(1, 1, 1),
		const wi::scene::MeshComponent* mesh = nullptr,
		bool disable_caching = false
	);
}
