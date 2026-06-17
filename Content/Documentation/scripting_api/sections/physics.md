# Physics

[← Back to Scripting API index](../index.md)

```lua
    --- Creates a Physics object. Use the global `physics` instead of
    --- constructing one.
    ---
    ---@return Physics
    function Physics() end

    --- Global physics system. Use the global `physics` object to control and
    --- query the physics simulation.
    ---
    ---@class Physics
    local Physics = {}

    --- Use this global object to access physics functions.
    ---
    ---@type Physics
    physics = nil

    --- Enable/disable the physics engine all together.
    ---
    ---@param value boolean
    function Physics.SetEnabled(value) end

    --- Returns whether physics is enabled.
    ---
    ---@return boolean
    function Physics.IsEnabled() end

    --- Enable/disable the physics simulation. Physics engine state will be
    --- updated but not simulated
    ---
    ---@param value boolean
    function Physics.SetSimulationEnabled(value) end

    --- Returns whether simulation enabled.
    ---
    ---@return boolean
    function Physics.IsSimulationEnabled() end

    --- Enable/disable the physics interpolation. When enabled, simulation's
    --- fixed frame rate will be interpolated to match the variable frame rate
    --- of rendering
    ---
    ---@param value boolean
    function Physics.SetInterpolationEnabled(value) end

    --- Returns whether interpolation enabled.
    ---
    ---@return boolean
    function Physics.IsInterpolationEnabled() end

    --- Enable/disable debug drawing of physics objects.
    ---
    ---@param value boolean
    function Physics.SetDebugDrawEnabled(value) end

    --- Returns whether debug draw enabled.
    ---
    ---@return boolean
    function Physics.IsDebugDrawEnabled() end

    --- Set the accuracy of the simulation. This value corresponds to maximum
    --- simulation step count. Higher values will be slower but more accurate.
    ---
    ---@param value integer
    function Physics.SetAccuracy(value) end

    --- Returns the accuracy.
    ---
    ---@return integer
    function Physics.GetAccuracy() end

    --- Set the frames per second resolution of physics simulation
    --- (default = 120 FPS).
    ---
    ---@param value number
    function Physics.SetFrameRate(value) end

    --- Returns the frame rate.
    ---
    ---@return number
    function Physics.GetFrameRate() end

    --- returns linear velocity of the body in the latest simulation step.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return Vector
    function Physics.GetVelocity(component) end

    --- Returns current position of the body in the latest simulation step.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return Vector
    function Physics.GetPosition(component) end

    --- Returns current rotation of the body in the latest simulation step.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return Vector
    function Physics.GetRotation(component) end

    --- Returns the ground position of the rigidbody if it has character physics
    --- enabled.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return Vector
    function Physics.GetCharacterGroundPosition(component) end

    --- Returns the ground normal of the rigidbody if it has character physics
    --- enabled.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return Vector
    function Physics.GetCharacterGroundNormal(component) end

    --- Returns the ground velocity of the rigidbody if it has character physics
    --- enabled.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return Vector
    function Physics.GetCharacterGroundVelocity(component) end

    --- Returns true if the character physics is supported by normal or steep
    --- ground.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return boolean
    function Physics.IsCharacterGroundSupported(component) end

    --- Returns the `CharacterGroundStates` of the character physics.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---
    ---@return CharacterGroundStates
    function Physics.GetCharacterGroundState(component) end

    --- Changes the physics character's shape into a capsule with specified
    --- height and radius. Returns true if successful, false otherwise. Failure
    --- means that something is blocking the character.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param height number
    ---@param radius number
    ---
    ---@return boolean
    function Physics.ChangeCharacterShape(component, height, radius) end

    --- Applies movement logic to physics character.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param movement_direction Vector
    ---@param movement_speed? number
    ---@param jump? number
    ---@param controlMovementDuringJump? boolean
    function Physics.MoveCharacter(
        component,
        movement_direction,
        movement_speed,
        jump,
        controlMovementDuringJump
    ) end

    --- Enable/disable ghost mode for rigid body or ragdoll (all collision
    --- disabled).
    ---
    ---@param component RigidBodyPhysicsComponent|HumanoidComponent
    ---@param value boolean
    function Physics.SetGhostMode(component, value) end

    --- Enable/disable ghost mode for a ragdoll. In ghost mode, the ragdoll will
    --- not collide with anything. Enable this if the humanoid sits inside a
    --- vehicle for example.
    ---
    ---@param humanoid HumanoidComponent
    ---@param value boolean
    function Physics.SetRagdollGhostMode(humanoid, value) end

    --- Teleport a dynamic body.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param position Vector
    function Physics.SetPosition(component, position) end

    --- Teleport a dynamic body.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param position Vector
    ---@param rotationQuaternion Vector
    function Physics.SetPositionAndRotation(
        component,
        position,
        rotationQuaternion
    ) end

    --- Set the linear velocity manually.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param velocity Vector
    function Physics.SetLinearVelocity(component, velocity) end

    --- Set the angular velocity manually.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param velocity Vector
    function Physics.SetAngularVelocity(component, velocity) end

    --- Apply force at body center.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param force Vector
    function Physics.ApplyForce(component, force) end

    --- Apply force at body local position (at_local controls whether the at is
    --- in body's local space or not).
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param force Vector
    ---@param at Vector
    ---@param at_local boolean
    function Physics.ApplyForceAt(component, force, at, at_local) end

    --- Apply impulse at body center.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param impulse Vector
    function Physics.ApplyImpulse(component, impulse) end

    --- Apply impulse at body center of ragdoll bone.
    ---
    ---@param humanoid HumanoidComponent
    ---@param bone HumanoidBone
    ---@param impulse Vector
    function Physics.ApplyImpulse(humanoid, bone, impulse) end

    --- Apply impulse at body local position (at_local controls whether the `at`
    --- is in body's local space or not).
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param impulse Vector
    ---@param at Vector
    ---@param at_local boolean
    function Physics.ApplyImpulseAt(component, impulse, at, at_local) end

    --- Apply impulse at body local position of ragdoll bone (at_local controls
    --- whether the `at` is in body's local space or not).
    ---
    ---@param humanoid HumanoidComponent
    ---@param bone HumanoidBone
    ---@param impulse Vector
    ---@param at Vector
    ---@param at_local boolean
    function Physics.ApplyImpulseAt(humanoid, bone, impulse, at, at_local) end

    --- Apply torque at body center.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param torque Vector
    function Physics.ApplyTorque(component, torque) end

    --- Activate all rigid bodies in the scene
    ---
    ---@param scene Scene
    function Physics.ActivateAllRigidBodies(scene) end

    --- Reset all rigid bodies to their initial orientations.
    ---
    ---@param scene Scene
    function Physics.ResetPhysicsObjects(scene) end

    --- Force set activation state to rigid body. Use a value
    --- `ACTIVATION_STATE_ACTIVE` or `ACTIVATION_STATE_INACTIVE`.
    ---
    ---@param component RigidBodyPhysicsComponent
    ---@param state integer
    function Physics.SetActivationState(component, state) end

    --- Force set activation state to soft body. Use a value
    --- `ACTIVATION_STATE_ACTIVE` or `ACTIVATION_STATE_INACTIVE`.
    ---
    ---@param component SoftBodyPhysicsComponent
    ---@param state integer
    function Physics.SetActivationState(component, state) end

    --- ACTIVATION STATE ACTIVE.
    ---
    ---@type integer
    ACTIVATION_STATE_ACTIVE = 0

    --- ACTIVATION STATE INACTIVE.
    ---
    ---@type integer
    ACTIVATION_STATE_INACTIVE = 1

    --- Describes how a character controller is currently supported by the
    --- ground.
    ---
    ---@enum CharacterGroundStates
    CharacterGroundStates = {
        OnGround = 0, -- Character is on the ground and can move freely.
        -- Character is on a slope that is too steep and can't climb up any
        -- further. The caller should start applying downward velocity if
        -- sliding from the slope is desired.
        OnSteepGround = 1,
        -- Character is touching an object, but is not supported by it and
        -- should fall. The GetGroundXXX functions will return information about
        -- the touched object.
        NotSupported = 2,
        InAir = 3, -- Character is in the air and is not touching anything.
    }

    --- Performs physics scene intersection for closest hit with a ray.
    ---
    ---@param scene Scene
    ---@param ray Ray
    ---
    ---@return Entity entity
    ---@return Vector position
    ---@return Vector normal
    ---@return Entity humanoid_ragdoll_entity
    ---@return HumanoidBone humanoid_bone
    ---@return Vector position_local
    function Physics.Intersects(scene, ray) end

    --- Set input from driver: forward and right values are values between -1
    --- and 1 to indicate reverse/forward or left/right. brake and handbrake
    --- (handbrake = back brake for motorcycles) are values between 0 and 1.
    ---
    ---@param rigidbody RigidBodyPhysicsComponent
    ---@param forward? number
    ---@param right? number
    ---@param brake? number
    ---@param handbrake? number
    function Physics.DriveVehicle(
        rigidbody,
        forward,
        right,
        brake,
        handbrake
    ) end

    --- Signed velocity amount in forward direction.
    ---
    ---@param rigidbody RigidBodyPhysicsComponent
    ---
    ---@return number
    function Physics.GetVehicleForwardVelocity(rigidbody) end

    --- Pick and drag physics objects such as ragdolls and rigid bodies.
    ---
    ---@param scene Scene
    ---@param ray Ray
    ---@param op PickDragOperation
    ---@param constraint? ConstraintType
    ---@param break_distance? number
    function Physics.PickDrag(scene, ray, op, constraint, break_distance) end

    --- Types of physics constraint that can link two rigid bodies.
    ---
    ---@enum ConstraintType
    ConstraintType = {
        Fixed = 0,
        Point = 1,
    }

    --- Returns the character collision tolerance distance.
    ---
    ---@return number
    function Physics.GetCharacterCollisionTolerance() end

    --- Sets the character collision tolerance distance.
    ---
    ---@param value number
    function Physics.SetCharacterCollisionTolerance(value) end

    --- Optimizes the physics broad phase for the scene. Pass force = true to
    --- rebuild even when not flagged dirty.
    ---
    ---@param scene Scene
    ---@param force? boolean
    function Physics.OptimizeBroadPhase(scene, force) end
```

## PickDragOperation

```lua
    --- Creates a PickDragOperation, used to grab and drag physics bodies with
    --- the cursor.
    ---
    ---@return PickDragOperation
    function PickDragOperation() end

    --- Tracks a physics pick drag operation. Use it with `phyiscs.PickDrag()`
    --- function. When using this object first time to PickDrag, the operation
    --- will be started and the operation will end when you call Finish() or
    --- when the object is destroyed.
    ---
    ---@class PickDragOperation
    local PickDragOperation = {}

    --- Finish the operation, puts down the physics object.
    function PickDragOperation.Finish() end
```
