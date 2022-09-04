#pragma once
#include "wiECS.h"
#include "wiScene.h"
#include "wiJobSystem.h"

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

	// Enable/disable debug drawing of physics objects
	void SetDebugDrawEnabled(bool value);
	bool IsDebugDrawEnabled();

	// Set the accuracy of the simulation
	//	This value corresponds to maximum simulation step count
	//	Higher values will be slower but more accurate
	void SetAccuracy(int value);
	int GetAccuracy();

	// Update the physics state, run simulation, etc.
	void RunPhysicsUpdateSystem(
		wi::jobsystem::context& ctx,
		wi::scene::Scene& scene,
		float dt
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

	// Apply force at body center
	void ApplyForce(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	);
	// Apply force at body local position
	void ApplyForceAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force,
		const XMFLOAT3& at
	);

	// Apply impulse at body center
	void ApplyImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	);
	// Apply impulse at body local position
	void ApplyImpulseAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at
	);

	void ApplyTorque(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	);
	void ApplyTorqueImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	);

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
}
