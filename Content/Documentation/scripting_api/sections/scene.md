# Scene System (using entity-component system)

[← Back to Scripting API index](../index.md)

Manipulate the 3D scene with these components.

## Entity

An entity is just an int value (int in LUA and uint32 in C++) and works as a
handle to retrieve associated components.

```lua
    --- An entity is an integer handle. `Entity` is an alias for `integer`,
    --- used in signatures for readability.
    ---
    ---@alias Entity integer

    --- The invalid/no-entity sentinel value (0).
    ---
    ---@type integer
    INVALID_ENTITY = 0
```

## Scene

```lua
    --- Creates a custom scene.
    ---
    ---@return Scene
    function Scene() end

    --- The scene holds components. Entity handles can be used to retrieve
    --- associated components through the scene.
    ---
    ---@class Scene
    ---
    ---@field Weather WeatherComponent
    local Scene = {}

    --- Returns the global scene.
    ---
    ---@return Scene
    function GetScene() end

    --- Returns the global camera.
    ---
    ---@return CameraComponent
    function GetCamera() end

    --- Load Model from file. returns a root entity that everything in this
    --- model is attached to.
    ---
    ---@param fileName string
    ---@param transform? Matrix
    ---
    ---@return Entity
    function LoadModel(fileName, transform) end

    --- Load Model from file into specified scene. returns a root entity that
    --- everything in this model is attached to.
    ---
    ---@param scene Scene
    ---@param fileName string
    ---@param transform? Matrix
    ---
    ---@return Entity
    function LoadModel(scene, fileName, transform) end

    --- Deprecated, you can use `FILTER_` enums instead.
    ---
    ---@deprecated
    ---
    ---@type integer
    PICK_OPAQUE = 1

    --- Deprecated, you can use `FILTER_` enums instead.
    ---
    ---@deprecated
    ---
    ---@type integer
    PICK_TRANSPARENT = 2

    --- Deprecated, you can use `FILTER_` enums instead.
    ---
    ---@deprecated
    ---
    ---@type integer
    PICK_WATER = 4

    --- Deprecated, you can use `FILTER_` enums instead.
    ---
    ---@deprecated
    ---
    ---@type integer
    PICK_VOID = 0

    --- Perform ray-picking in the scene. pickType is a bitmask specifying
    --- object types to check against. layerMask is a bitmask specifying which
    --- layers to check against. Scene parameter is optional and will use the
    --- global scene if not specified. (deprecated, you can use the scene's
    --- Intersects() function instead).
    ---
    ---@deprecated
    ---
    ---@param ray Ray
    ---@param filterMask? integer
    ---@param layerMask? integer
    ---@param scene? Scene
    ---@param lod? integer
    ---
    ---@return Entity entity
    ---@return Vector position
    ---@return Vector normal
    ---@return number distance
    function Pick(ray, filterMask, layerMask, scene, lod) end

    --- Perform ray-picking in the scene. pickType is a bitmask specifying
    --- object types to check against. layerMask is a bitmask specifying which
    --- layers to check against. Scene parameter is optional and will use the
    --- global scene if not specified. (deprecated, you can use the scene's
    --- Intersects() function instead)
    ---
    ---@deprecated
    ---
    ---@param sphere Sphere
    ---@param filterMask? integer
    ---@param layerMask? integer
    ---@param scene? Scene
    ---@param lod? integer
    ---
    ---@return Entity entity
    ---@return Vector position
    ---@return Vector normal
    ---@return number distance
    function SceneIntersectSphere(sphere, filterMask, layerMask, scene, lod) end

    --- Perform ray-picking in the scene. pickType is a bitmask specifying
    --- object types to check against. layerMask is a bitmask specifying which
    --- layers to check against. Scene parameter is optional and will use the
    --- global scene if not specified. (deprecated, you can use the scene's
    --- Intersects() function instead)
    ---
    ---@deprecated
    ---
    ---@param capsule Capsule
    ---@param filterMask? integer
    ---@param layerMask? integer
    ---@param scene? Scene
    ---@param lod? integer
    ---
    ---@return Entity entity
    ---@return Vector position
    ---@return Vector normal
    ---@return number distance
    function SceneIntersectCapsule(
        capsule,
        filterMask,
        layerMask,
        scene,
        lod
    ) end

    --- Include nothing.
    ---
    ---@type integer
    FILTER_NONE = 0

    --- Include opaque meshes.
    ---
    ---@type integer
    FILTER_OPAQUE = 1

    --- Include transparent meshes.
    ---
    ---@type integer
    FILTER_TRANSPARENT = 2

    --- Include water meshes.
    ---
    ---@type integer
    FILTER_WATER = 4

    --- include navigation meshes.
    ---
    ---@type integer
    FILTER_NAVIGATION_MESH = 8

    --- Include terrain.
    ---@type integer
    FILTER_TERRAIN = 16

    --- Include all objects, meshes.
    ---
    ---@type integer
    FILTER_OBJECT_ALL = 31

    --- Include colliders.
    ---
    ---@type integer
    FILTER_COLLIDER = 32

    --- Include ragdoll body parts.
    ---
    ---@type integer
    FILTER_RAGDOLL = 64

    --- Include everything.
    ---
    ---@type integer
    FILTER_ALL = -1

    --- Intersects a primitive with the scene and returns collision parameters.
    --- If humanoid_bone is not `HumanoidBone.Count` then the intersection is a
    --- ragdoll, and entity refers to the humanoid entity.
    ---
    ---@param primitive Ray|Sphere|Capsule
    ---@param filterMask? integer
    ---@param layerMask? integer
    ---@param lod? integer
    ---
    ---@return Entity entity
    ---@return Vector position
    ---@return Vector normal
    ---@return number distance
    ---@return Vector velocity
    ---@return integer subsetIndex
    ---@return Matrix orientation
    ---@return Vector uv
    ---@return HumanoidBone humanoid_bone
    function Scene.Intersects(primitive, filterMask, layerMask, lod) end

    --- Intersects a primitive with the scene and returns true immediately on
    --- intersection, false if there was no intersection. This can be faster for
    --- occlusion check than regular `Intersects` that searches for closest
    --- intersection.
    ---
    ---@param primitive Ray
    ---@param filterMask? integer
    ---@param layerMask? integer
    ---@param lod? integer
    ---
    ---@return boolean
    function Scene.IntersectsFirst(primitive, filterMask, layerMask, lod) end

    --- Intersects the scene with a primitive and returns array of results. In
    --- case of Ray, RayIntersectionResult will be returned, for Sphere and
    --- Capsule SphereIntersectionResult will be returned.
    ---
    ---@param primitive Ray|Sphere|Capsule
    ---@param filterMask? integer
    ---@param layerMask? integer
    ---@param lod? integer
    ---
    ---@return RayIntersectionResult[]|SphereIntersectionResult[]
    function Scene.IntersectsAll(primitive, filterMask, layerMask, lod) end

    --- Updates the scene and every entity and component inside the scene.
    function Scene.Update() end

    --- Deletes every entity and component inside the scene.
    function Scene.Clear() end

    --- Moves contents from an other scene into this one. The other scene will
    --- be empty after this operation (contents are moved, not copied).
    ---
    ---@param other Scene
    function Scene.Merge(other) end

    --- Updates the full scene hierarchy system. Useful if you modified for
    --- example a parent transform and children immediately need up to date
    --- result in the script.
    function Scene.UpdateHierarchy() end

    --- Duplicates everything in the prefab scene into the current scene. If
    --- attached parameter is set to `true` then everything in prefab scene will
    --- be attached to a common root entity (with TransformComponent and
    --- LayerComponent) and the function will return that root entity.
    ---
    ---@param prefab Scene
    ---@param attached? boolean
    ---
    ---@return Entity
    function Scene.Instantiate(prefab, attached) end

    --- Creates an empty entity and returns it.
    ---
    ---@return Entity
    function CreateEntity() end

    --- Returns a table with all the entities present in the given scene.
    ---
    ---@return any
    function Scene.FindAllEntities() end

    --- Returns an entity ID if it exists, and INVALID_ENTITY otherwise. You can
    --- specify an ancestor entity if you only want to find entities that are
    --- descendants of ancestor entity.
    ---
    ---@param value string
    ---@param ancestor? integer
    ---
    ---@return Entity
    function Scene.Entity_FindByName(value, ancestor) end

    --- Removes an entity and deletes all its components if it exists. If
    --- recursive is specified, then all children will be removed as well
    --- (enabled by default). If keep_sorted is specified, then component order
    --- will be kept (disabled by default, slower).
    ---
    ---@param entity Entity
    ---@param recursive? boolean
    ---@param keep_sorted? boolean
    function Scene.Entity_Remove(entity, recursive, keep_sorted) end

    --- Same as Entity_Remove, but it runs on a background thread, status can be
    --- tracked by the [Async](#async) object that you provide.
    ---
    ---@param async Async
    ---@param entity Entity
    ---@param recursive? boolean
    ---@param keep_sorted? boolean
    function Scene.Entity_Remove_Async(
        async,
        entity,
        recursive,
        keep_sorted
    ) end

    --- Duplicates all of an entity's components and creates a new entity with
    --- them. Returns the clone entity handle.
    ---
    ---@param entity Entity
    ---
    ---@return Entity
    function Scene.Entity_Duplicate(entity) end

    --- Check whether entity is a descendant of ancestor. Returns `true` if
    --- entity is in the hierarchy tree of ancestor, `false` otherwise.
    ---
    ---@param entity Entity
    ---@param ancestor Entity
    ---
    ---@return boolean
    function Scene.Entity_IsDescendant(entity, ancestor) end

    --- Attach a name component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return NameComponent
    function Scene.Component_CreateName(entity) end

    --- Attach a layer component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return LayerComponent
    function Scene.Component_CreateLayer(entity) end

    --- Attach a transform component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return TransformComponent
    function Scene.Component_CreateTransform(entity) end

    --- Attach a camera component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return CameraComponent
    function Scene.Component_CreateCamera(entity) end

    --- Attach a light component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return LightComponent
    function Scene.Component_CreateLight(entity) end

    --- Attach an object component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return ObjectComponent
    function Scene.Component_CreateObject(entity) end

    --- Attach an IK component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return InverseKinematicsComponent
    function Scene.Component_CreateInverseKinematics(entity) end

    --- Attach a spring component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return SpringComponent
    function Scene.Component_CreateSpring(entity) end

    --- Attach a script component to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return ScriptComponent
    function Scene.Component_CreateScript(entity) end

    --- Attach a RigidBodyPhysicsComponent to an entity. The returned component
    --- is associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return RigidBodyPhysicsComponent
    function Scene.Component_CreateRigidBodyPhysics(entity) end

    --- Attach a SoftBodyPhysicsComponent to an entity. The returned component
    --- is associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return SoftBodyPhysicsComponent
    function Scene.Component_CreateSoftBodyPhysics(entity) end

    --- Attach a ForceFieldComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return ForceFieldComponent
    function Scene.Component_CreateForceField(entity) end

    --- Attach a WeatherComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return WeatherComponent
    function Scene.Component_CreateWeather(entity) end

    --- Attach a SoundComponent to an entity. The returned component is
    --- associated ith the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return SoundComponent
    function Scene.Component_CreateSound(entity) end

    --- Attach a VideoComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return VideoComponent
    function Scene.Component_CreateVideo(entity) end

    --- Attach a ColliderComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return ColliderComponent
    function Scene.Component_CreateCollider(entity) end

    --- Attach a ExpressionComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return ExpressionComponent
    function Scene.Component_CreateExpression(entity) end

    --- Attach a HumanoidComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return HumanoidComponent
    function Scene.Component_CreateHumanoid(entity) end

    --- Attach a DecalComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return DecalComponent
    function Scene.Component_CreateDecal(entity) end

    --- Attach a Sprite to an entity. The returned component is associated with
    --- the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return Sprite
    function Scene.Component_CreateSprite(entity) end

    --- Attach a SpriteFont to an entity. The returned component is associated
    --- with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return SpriteFont
    function Scene.Component_CreateFont(entity) end

    --- Attach a VoxelGrid to an entity. The returned component is associated
    --- with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return VoxelGrid
    function Scene.Component_CreateVoxelGrid(entity) end

    --- Attach a MetadataComponent to an entity. The returned component is
    --- associated with the entity and can be manipulated.
    ---
    ---@param entity Entity
    ---
    ---@return MetadataComponent
    function Scene.Component_CreateMetadata(entity) end

    --- Query the name component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return NameComponent?
    function Scene.Component_GetName(entity) end

    --- query the layer component of the entity (if exists).
    ---
    ---@param entity Entity
    ---
    ---@return LayerComponent?
    function Scene.Component_GetLayer(entity) end

    --- Query the transform component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return TransformComponent?
    function Scene.Component_GetTransform(entity) end

    --- Query the camera component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return CameraComponent?
    function Scene.Component_GetCamera(entity) end

    --- Query the animation component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return AnimationComponent?
    function Scene.Component_GetAnimation(entity) end

    --- Query the material component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return MaterialComponent?
    function Scene.Component_GetMaterial(entity) end

    --- Query the emitter component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return EmitterComponent?
    function Scene.Component_GetEmitter(entity) end

    --- Query the light component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return LightComponent?
    function Scene.Component_GetLight(entity) end

    --- Query the object component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return ObjectComponent?
    function Scene.Component_GetObject(entity) end

    --- Query the IK component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return InverseKinematicsComponent?
    function Scene.Component_GetInverseKinematics(entity) end

    --- Query the spring component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return SpringComponent?
    function Scene.Component_GetSpring(entity) end

    --- Query the script component of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return ScriptComponent?
    function Scene.Component_GetScript(entity) end

    --- Query the RigidBodyPhysicsComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return RigidBodyPhysicsComponent?
    function Scene.Component_GetRigidBodyPhysics(entity) end

    --- Query the SoftBodyPhysicsComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return SoftBodyPhysicsComponent?
    function Scene.Component_GetSoftBodyPhysics(entity) end

    --- Query the ForceFieldComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return ForceFieldComponent?
    function Scene.Component_GetForceField(entity) end

    --- Query the WeatherComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return WeatherComponent?
    function Scene.Component_GetWeather(entity) end

    --- Query the SoundComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return SoundComponent?
    function Scene.Component_GetSound(entity) end

    --- Query the VideoComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return VideoComponent?
    function Scene.Component_GetVideo(entity) end

    --- Query the ColliderComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return ColliderComponent?
    function Scene.Component_GetCollider(entity) end

    --- Query the ExpressionComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return ExpressionComponent?
    function Scene.Component_GetExpression(entity) end

    --- Query the HumanoidComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return HumanoidComponent?
    function Scene.Component_GetHumanoid(entity) end

    --- Query the DecalComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return DecalComponent?
    function Scene.Component_GetDecal(entity) end

    --- Query the Sprite of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return Sprite?
    function Scene.Component_GetSprite(entity) end

    --- Query the VoxelGrid of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return VoxelGrid?
    function Scene.Component_GetVoxelGrid(entity) end

    --- Query the MetadataComponent of the entity (if exists)..
    ---
    ---@param entity Entity
    ---
    ---@return MetadataComponent?
    function Scene.Component_GetMetadata(entity) end

    --- Returns an array of every Name component in the scene.
    ---
    ---@return NameComponent[]
    function Scene.Component_GetNameArray() end

    --- Returns an array of every Layer component in the scene.
    ---
    ---@return LayerComponent[]
    function Scene.Component_GetLayerArray() end

    --- Returns an array of every Transform component in the scene.
    ---
    ---@return TransformComponent[]
    function Scene.Component_GetTransformArray() end

    --- Returns an array of every Camera component in the scene.
    ---
    ---@return CameraComponent[]
    function Scene.Component_GetCameraArray() end

    --- Returns an array of every Animation component in the scene.
    ---
    ---@return AnimationComponent[]
    function Scene.Component_GetAnimationArray() end

    --- Returns an array of every Material component in the scene.
    ---
    ---@return MaterialComponent[]
    function Scene.Component_GetMaterialArray() end

    --- Returns an array of every Emitter component in the scene.
    ---
    ---@return EmitterComponent[]
    function Scene.Component_GetEmitterArray() end

    --- Returns an array of every Light component in the scene.
    ---
    ---@return LightComponent[]
    function Scene.Component_GetLightArray() end

    --- Returns an array of every Object component in the scene.
    ---
    ---@return ObjectComponent[]
    function Scene.Component_GetObjectArray() end

    --- Returns an array of every Mesh component in the scene.
    ---
    ---@return MeshComponent[]
    function Scene.Component_GetMeshArray() end

    --- Returns an array of every InverseKinematics component in the scene.
    ---
    ---@return InverseKinematicsComponent[]
    function Scene.Component_GetInverseKinematicsArray() end

    --- Returns an array of every Spring component in the scene.
    ---
    ---@return SpringComponent[]
    function Scene.Component_GetSpringArray() end

    --- Returns an array of every Script component in the scene.
    ---
    ---@return ScriptComponent[]
    function Scene.Component_GetScriptArray() end

    --- Returns an array of every RigidBodyPhysics component in the scene.
    ---
    ---@return RigidBodyPhysicsComponent[]
    function Scene.Component_GetRigidBodyPhysicsArray() end

    --- Returns an array of every SoftBodyPhysics component in the scene.
    ---
    ---@return SoftBodyPhysicsComponent[]
    function Scene.Component_GetSoftBodyPhysicsArray() end

    --- Returns an array of every ForceField component in the scene.
    ---
    ---@return ForceFieldComponent[]
    function Scene.Component_GetForceFieldArray() end

    --- Returns an array of every Weather component in the scene.
    ---
    ---@return WeatherComponent[]
    function Scene.Component_GetWeatherArray() end

    --- Returns an array of every Sound component in the scene.
    ---
    ---@return SoundComponent[]
    function Scene.Component_GetSoundArray() end

    --- Returns an array of every Video component in the scene.
    ---
    ---@return VideoComponent[]
    function Scene.Component_GetVideoArray() end

    --- Returns an array of every Collider component in the scene.
    ---
    ---@return ColliderComponent[]
    function Scene.Component_GetColliderArray() end

    --- Returns an array of every Expression component in the scene.
    ---
    ---@return ExpressionComponent[]
    function Scene.Component_GetExpressionArray() end

    --- Returns an array of every Humanoid component in the scene.
    ---
    ---@return HumanoidComponent[]
    function Scene.Component_GetHumanoidArray() end

    --- Returns an array of every Decal component in the scene.
    ---
    ---@return DecalComponent[]
    function Scene.Component_GetDecalArray() end

    --- Returns an array of every Sprite component in the scene.
    ---
    ---@return Sprite[]
    function Scene.Component_GetSpriteArray() end

    --- Returns an array of every Font component in the scene.
    ---
    ---@return SpriteFont[]
    function Scene.Component_GetFontArray() end

    --- Returns an array of every VoxelGrid component in the scene.
    ---
    ---@return VoxelGrid[]
    function Scene.Component_GetVoxelGridArray() end

    --- Returns an array of every Metadata component in the scene.
    ---
    ---@return MetadataComponent[]
    function Scene.Component_GetMetadataArray() end

    --- Returns an array of every entity that has a Name component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetNameArray() end

    --- Returns an array of every entity that has a Layer component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetLayerArray() end

    --- Returns an array of every entity that has a Transform component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetTransformArray() end

    --- Returns an array of every entity that has a Camera component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetCameraArray() end

    --- Returns an array of every entity that has an Animation component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetAnimationArray() end

    --- Returns an array of every entity that has an AnimationData component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetAnimationDataArray() end

    --- Returns an array of every entity that has a Material component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetMaterialArray() end

    --- Returns an array of every entity that has an Emitter component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetEmitterArray() end

    --- Returns an array of every entity that has a Light component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetLightArray() end

    --- Returns an array of every entity that has an Object component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetObjectArray() end

    --- Returns an array of every entity that has a Mesh component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetMeshArray() end

    --- Returns an array of every entity that has an InverseKinematics
    --- component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetInverseKinematicsArray() end

    --- Returns an array of every entity that has a Spring component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetSpringArray() end

    --- Returns an array of every entity that has a Script component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetScriptArray() end

    --- Returns an array of every entity that has a RigidBodyPhysics component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetRigidBodyPhysicsArray() end

    --- Returns an array of every entity that has a SoftBodyPhysics component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetSoftBodyPhysicsArray() end

    --- Returns an array of every entity that has a ForceField component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetForceFieldArray() end

    --- Returns an array of every entity that has a Weather component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetWeatherArray() end

    --- Returns an array of every entity that has a Sound component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetSoundArray() end

    --- Returns an array of every entity that has a Video component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetVideoArray() end

    --- Returns an array of every entity that has a Collider component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetColliderArray() end

    --- Returns an array of every entity that has an Expression component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetExpressionArray() end

    --- Returns an array of every entity that has a Humanoid component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetHumanoidArray() end

    --- Returns an array of every entity that has a Decal component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetDecalArray() end

    --- Returns an array of every entity that has a Sprite component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetSpriteArray() end

    --- Returns an array of every entity that has a VoxelGrid component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetVoxelGridArray() end

    --- Returns an array of every entity that has a Metadata component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetMetadataArray() end

    --- Remove the name component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveName(entity) end

    --- Remove the layer component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveLayer(entity) end

    --- Remove the transform component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveTransform(entity) end

    --- Remove the camera component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveCamera(entity) end

    --- Remove the animation component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveAnimation(entity) end

    --- Remove component Animation Data.
    ---
    ---@param entity Entity
    function Scene.Component_RemoveAnimationData(entity) end

    --- Remove the material component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveMaterial(entity) end

    --- Remove the emitter component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveEmitter(entity) end

    --- Remove the light component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveLight(entity) end

    --- Remove the object component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveObject(entity) end

    --- Remove the IK component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveInverseKinematics(entity) end

    --- Remove the spring component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveSpring(entity) end

    --- Remove the script component of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveScript(entity) end

    --- Remove the RigidBodyPhysicsComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveRigidBodyPhysics(entity) end

    --- Remove the SoftBodyPhysicsComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveSoftBodyPhysics(entity) end

    --- Remove the ForceFieldComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveForceField(entity) end

    --- Remove the WeatherComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveWeather(entity) end

    --- Remove the SoundComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveSound(entity) end

    --- Remove the VideoComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveVideo(entity) end

    --- Remove the ColliderComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveCollider(entity) end

    --- Remove the ExpressionComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveExpression(entity) end

    --- Remove the HumanoidComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveHumanoid(entity) end

    --- Remove the DecalComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveDecal(entity) end

    --- Remove the Sprite of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveSprite(entity) end

    --- Remove the SpriteFont of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveFont(entity) end

    --- Remove the VoxelGrid of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveVoxelGrid(entity) end

    --- Remove the MetadataComponent of the entity (if exists).
    ---
    ---@param entity Entity
    function Scene.Component_RemoveMetadata(entity) end

    --- Attaches entity to parent (adds a hierarchy component to entity). From
    --- now on, entity will inherit certain properties from parent, such as
    --- transform (entity will move with parent) or layer (entity's layer will
    --- be a sublayer of parent's layer). If child_already_in_local_space is
    --- false, then child will be transformed into parent's local space, if
    --- true, it will be used as-is.
    ---
    ---@param entity Entity
    ---@param parent Entity
    ---@param child_already_in_local_space? boolean
    function Scene.Component_Attach(
        entity,
        parent,
        child_already_in_local_space
    ) end

    --- Detaches entity from parent (if hierarchycomponent exists for it).
    --- Restores entity's original layer, and applies current transformation to
    --- entity.
    ---
    ---@param entity Entity
    function Scene.Component_Detach(entity) end

    --- Detaches all children from parent, as if calling Component_Detach for
    --- all of its children.
    ---
    ---@param parent Entity
    function Scene.Component_DetachChildren(parent) end

    --- Returns an AABB fully containing objects in the scene. Only valid after
    --- scene has been updated.
    ---
    ---@return AABB
    function Scene.GetBounds() end

    --- Returns the weather.
    ---
    ---@return WeatherComponent
    function Scene.GetWeather() end

    --- Sets the weather.
    ---
    ---@param weather WeatherComponent
    function Scene.SetWeather(weather) end

    --- Retargets an animation from a Humanoid to an other Humanoid such that
    --- the new animation will play back on the destination humanoid. dst :
    --- destination humanoid that the animation will be fit onto src : the
    --- animation to copy, it should already target humanoid bones. bake_data :
    --- if true, the retargeted data will be baked into a new animation data. If
    --- false, it will reuse the source animation data without creating a new
    --- one and retargeting will be applied at runtime on every Update. Returns
    --- entity ID of the new animation or INVALID_ENTITY if retargeting was not
    --- successful
    ---
    ---@param dst Entity
    ---@param src Entity
    ---@param bake_data boolean
    ---@param src_scene? Scene  Scene containing src (defaults to this scene).
    ---
    ---@return Entity
    function Scene.RetargetAnimation(dst, src, bake_data, src_scene) end

    --- Resets the pose of the specified entity to bind pose. The bind pose is
    --- taken from the bind matrices of bones of an ArmatureComponent. If the
    --- entity does not have an armature, then it will find the child armatures
    --- of the entity.
    ---
    ---@param entity Entity
    function Scene.ResetPose(entity) end

    --- Returns the approximate position on the ocean surface seen from a
    --- position in world space. If current weather doesn't have ocean enabled,
    --- Returns the world position itself. The result position is approximate
    --- because it involves reading back from GPU to the CPU, so the result can
    --- be delayed compared to the current GPU simulation. Note that the input
    --- position to this function will be taken on the XZ plane and modified by
    --- the displacement map's XZ value, and the Y (vertical) position will be
    --- taken from the ocean water height and displacement map only.
    ---
    ---@param worldPosition Vector
    function Scene.GetOceanPosAt(worldPosition) end

    --- Voxelizes a single object into the voxel grid. Subtract parameter
    --- controls whether the voxels are added (true) or removed (false). Lod
    --- argument selects object's level of detail.
    ---
    ---@param objectIndex integer
    ---@param voxelgrid VoxelGrid
    ---@param subtract? boolean
    ---@param lod? integer
    function Scene.VoxelizeObject(objectIndex, voxelgrid, subtract, lod) end

    --- Voxelizes all entities in the scene which intersect the voxel grid
    --- volume and match the filterMask and layerMask. Subtract parameter
    --- controls whether the voxels are added (true) or removed (false). Lod
    --- argument selects object's level of detail.
    ---
    ---@param voxelgrid VoxelGrid
    ---@param subtract? boolean
    ---@param filterMask? integer
    ---@param layerMask? integer
    ---@param lod? integer
    function Scene.VoxelizeScene(
        voxelgrid,
        subtract,
        filterMask,
        layerMask,
        lod
    ) end

    --- Maintenance utility to help fix Nan issues in TransformComponents.
    --- Transforms containing nans will be cleared and renamed with `_nanfix`
    --- postfix.
    function Scene.FixupNans() end

    --- Attaches a EmitterComponent to the entity.
    ---
    ---@param entity Entity
    ---
    ---@return EmitterComponent
    function Scene.Component_CreateEmitter(entity) end

    --- Attaches a HairParticleSystem to the entity.
    ---
    ---@param entity Entity
    ---
    ---@return HairParticleSystem
    function Scene.Component_CreateHairParticleSystem(entity) end

    --- Attaches a MaterialComponent to the entity.
    ---
    ---@param entity Entity
    ---
    ---@return MaterialComponent
    function Scene.Component_CreateMaterial(entity) end

    --- Attaches a CharacterComponent to the entity.
    ---
    ---@param entity Entity
    ---
    ---@return CharacterComponent
    function Scene.Component_CreateCharacter(entity) end

    --- Returns the MeshComponent of the entity, if it exists.
    ---
    ---@param entity Entity
    ---
    ---@return MeshComponent?
    function Scene.Component_GetMesh(entity) end

    --- Returns the HairParticleSystem of the entity, if it exists.
    ---
    ---@param entity Entity
    ---
    ---@return HairParticleSystem?
    function Scene.Component_GetHairParticleSystem(entity) end

    --- Returns the SpriteFont of the entity, if it exists.
    ---
    ---@param entity Entity
    ---
    ---@return SpriteFont?
    function Scene.Component_GetFont(entity) end

    --- Returns the CharacterComponent of the entity, if it exists.
    ---
    ---@param entity Entity
    ---
    ---@return CharacterComponent?
    function Scene.Component_GetCharacter(entity) end

    --- Returns an array of every HairParticleSystem component in the scene.
    ---
    ---@return HairParticleSystem[]
    function Scene.Component_GetHairParticleSystemArray() end

    --- Returns an array of every Character component in the scene.
    ---
    ---@return CharacterComponent[]
    function Scene.Component_GetCharacterArray() end

    --- Returns an array of every entity that has a HairParticleSystem
    --- component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetHairParticleSystemArray() end

    --- Returns an array of every entity that has a Character component.
    ---
    ---@return Entity[]
    function Scene.Entity_GetCharacterArray() end

    --- Removes the MeshComponent of the entity, if it exists.
    ---
    ---@param entity Entity
    function Scene.Component_RemoveMesh(entity) end

    --- Removes the HairParticleSystem of the entity, if it exists.
    ---
    ---@param entity Entity
    function Scene.Component_RemoveHairParticleSystem(entity) end

    --- Removes the CharacterComponent of the entity, if it exists.
    ---
    ---@param entity Entity
    function Scene.Component_RemoveCharacter(entity) end
```

## RayIntersectionResult

```lua
    --- Creates an empty RayIntersectionResult. Normally these are returned by
    --- `scene.IntersectsAll`, not constructed directly.
    ---
    ---@return RayIntersectionResult
    function RayIntersectionResult() end

    --- Result of one hit in scene.IntersectsAll.
    ---
    ---@class RayIntersectionResult
    local RayIntersectionResult = {}

    --- Returns the entity.
    ---
    ---@return Entity
    function RayIntersectionResult.GetEntity() end

    --- Returns the position.
    ---
    ---@return Vector
    function RayIntersectionResult.GetPosition() end

    --- Returns the normal.
    ---
    ---@return Vector
    function RayIntersectionResult.GetNormal() end

    --- Returns the uv.
    ---
    ---@return Vector
    function RayIntersectionResult.GetUV() end

    --- Returns the velocity.
    ---
    ---@return Vector
    function RayIntersectionResult.GetVelocity() end

    --- Returns the distance.
    ---
    ---@return number
    function RayIntersectionResult.GetDistance() end

    --- Returns the subset index.
    ---
    ---@return integer
    function RayIntersectionResult.GetSubsetIndex() end

    --- Returns the vertex id0.
    ---
    ---@return integer
    function RayIntersectionResult.GetVertexID0() end

    --- Returns the vertex id1.
    ---
    ---@return integer
    function RayIntersectionResult.GetVertexID1() end

    --- Returns the vertex id2.
    ---
    ---@return integer
    function RayIntersectionResult.GetVertexID2() end

    --- Returns the barycentrics.
    ---
    ---@return Vector
    function RayIntersectionResult.GetBarycentrics() end

    --- Returns the orientation.
    ---
    ---@return Vector
    function RayIntersectionResult.GetOrientation() end

    --- Returns the humanoid bone.
    ---
    ---@return integer
    function RayIntersectionResult.GetHumanoidBone() end
```

## SphereIntersectionResult

```lua
    --- Creates an empty SphereIntersectionResult. Normally these are returned
    --- by `scene.IntersectsAll`, not constructed directly.
    ---
    ---@return SphereIntersectionResult
    function SphereIntersectionResult() end

    --- Result of one hit in scene.IntersectsAll.
    ---
    ---@class SphereIntersectionResult
    local SphereIntersectionResult = {}

    --- Returns the entity.
    ---
    ---@return Entity
    function SphereIntersectionResult.GetEntity() end

    --- Returns the position.
    ---
    ---@return Vector
    function SphereIntersectionResult.GetPosition() end

    --- Returns the normal.
    ---
    ---@return Vector
    function SphereIntersectionResult.GetNormal() end

    --- Returns the velocity.
    ---
    ---@return Vector
    function SphereIntersectionResult.GetVelocity() end

    --- Returns the depth.
    ---
    ---@return number
    function SphereIntersectionResult.GetDepth() end

    --- Returns the subset index.
    ---
    ---@return integer
    function SphereIntersectionResult.GetSubsetIndex() end

    --- Returns the orientation.
    ---
    ---@return Vector
    function SphereIntersectionResult.GetOrientation() end

    --- Returns the humanoid bone.
    ---
    ---@return integer
    function SphereIntersectionResult.GetHumanoidBone() end
```

## NameComponent

```lua
    --- Creates a new, standalone NameComponent that owns its own data.
    ---
    ---@return NameComponent
    function NameComponent() end

    --- Holds a string that can more easily identify an entity to humans than an
    --- entity ID.
    ---
    ---@class NameComponent
    ---
    ---@field Name string
    local NameComponent = {}

    --- Sets the name.
    ---
    ---@param value string
    function NameComponent.SetName(value) end

    --- Query the name string.
    ---
    ---@return any
    function NameComponent.GetName() end
```

## LayerComponent

```lua
    --- Creates a new, standalone LayerComponent that owns its own data.
    ---
    ---@return LayerComponent
    function LayerComponent() end

    --- An integer mask that can be used to group entities together for certain
    --- operations such as: picking, rendering, etc.
    ---
    ---@class LayerComponent
    ---
    ---@field LayerMask number
    local LayerComponent = {}

    --- Sets layer mask.
    ---
    ---@param value integer
    function LayerComponent.SetLayerMask(value) end

    --- Query layer mask.
    ---
    ---@return integer
    function LayerComponent.GetLayerMask() end
```

## TransformComponent

```lua
    --- Creates a new, standalone TransformComponent that owns its own data.
    ---
    ---@return TransformComponent
    function TransformComponent() end

    --- Describes an orientation in 3D space.
    ---
    ---@class TransformComponent
    ---
    ---@field Translation_local Vector query the position in world space
    ---@field Rotation_local Vector
    ---@field Scale_local Vector query the scaling in world space
    local TransformComponent = {}

    --- Applies scaling.
    ---
    ---@param vectorXYZ Vector
    function TransformComponent.Scale(vectorXYZ) end

    --- Applies uniform scaling.
    ---
    ---@param value number
    function TransformComponent.Scale(value) end

    --- Applies rotation as roll,pitch,yaw.
    ---
    ---@param vectorRollPitchYaw Vector
    function TransformComponent.Rotate(vectorRollPitchYaw) end

    --- Applies rotation as quaternion.
    ---
    ---@param quaternion Vector
    function TransformComponent.RotateQuaternion(quaternion) end

    --- Applies translation (position offset).
    ---
    ---@param vectorXYZ Vector
    function TransformComponent.Translate(vectorXYZ) end

    --- Interpolates linearly between two transform components.
    ---
    ---@param a TransformComponent
    ---@param b TransformComponent
    ---@param t number
    function TransformComponent.Lerp(a, b, t) end

    --- Interpolates between four transform components on a spline.
    ---
    ---@param a TransformComponent
    ---@param b TransformComponent
    ---@param c TransformComponent
    ---@param d TransformComponent
    ---@param t number
    function TransformComponent.CatmullRom(a, b, c, d, t) end

    --- Applies a transformation matrix.
    ---
    ---@param matrix Matrix
    function TransformComponent.MatrixTransform(matrix) end

    --- Retrieve a 4x4 transformation matrix representing the transform
    --- component's current orientation.
    ---
    ---@return Matrix
    function TransformComponent.GetMatrix() end

    --- Reset to the world origin, as in position becomes Vector(0,0,0),
    --- rotation quaternion becomes Vector(0,0,0,1), scaling becomes
    --- Vector(1,1,1).
    function TransformComponent.ClearTransform() end

    --- Updates the underlying transformation matrix.
    function TransformComponent.UpdateTransform() end

    --- Query the position in world space.
    ---
    ---@return Vector
    function TransformComponent.GetPosition() end

    --- Query the rotation as a quaternion in world space.
    ---
    ---@return Vector
    function TransformComponent.GetRotation() end

    --- Query the scaling in world space.
    ---
    ---@return Vector
    function TransformComponent.GetScale() end

    --- Set scale in local space.
    ---
    ---@param value Vector
    function TransformComponent.SetScale(value) end

    --- Set rotation quaternion in local space.
    ---
    ---@param quaternnion Vector
    function TransformComponent.SetRotation(quaternnion) end

    --- Set position in local space.
    ---
    ---@param value Vector
    function TransformComponent.SetPosition(value) end

    --- Invalidate, this will cause transformer to be updated in next scene
    --- update.
    ---
    ---@param value boolean
    function TransformComponent.SetDirty(value) end

    --- Check if transform was invalidated since last update.
    ---
    ---@return boolean
    function TransformComponent.IsDirty() end

    --- rRturns forward direction.
    ---
    ---@return Vector
    function TransformComponent.GetForward() end

    --- Returns upwards direction.
    ---
    ---@return Vector
    function TransformComponent.GetUp() end

    --- Returns right direction.
    ---
    ---@return Vector
    function TransformComponent.GetRight() end
```

## CameraComponent

```lua
    --- Creates a new, standalone CameraComponent that owns its own data.
    ---
    ---@return CameraComponent
    function CameraComponent() end

    --- Represents a camera: its position, orientation and projection, used to
    --- render the scene from a viewpoint.
    ---
    ---@class CameraComponent
    ---
    ---@field FOV number
    ---@field NearPlane number
    ---@field FarPlane number
    ---@field FocalLength number
    ---@field ApertureSize number
    ---@field ApertureShape number
    local CameraComponent = {}

    --- Update the camera matrices.
    function CameraComponent.UpdateCamera() end

    --- Copies the transform's orientation to the camera, and sets the camera
    --- position, look direction and up direction. Camera matrices are not
    --- updated immediately. They will be updated by the Scene::Update() (if the
    --- camera is part of the scene), or by manually calling UpdateCamera().
    ---
    ---@param transform TransformComponent
    function CameraComponent.TransformCamera(transform) end

    --- Transform Camera.
    ---
    ---@param matrix Matrix
    function CameraComponent.TransformCamera(matrix) end

    --- Returns the fov.
    ---
    ---@return number
    function CameraComponent.GetFOV() end

    --- Sets the vertical field of view for the camera (value is an angle in
    --- radians).
    ---
    ---@param value number
    function CameraComponent.SetFOV(value) end

    --- Returns the near plane.
    ---
    ---@return number
    function CameraComponent.GetNearPlane() end

    --- Sets the near plane of the camera, which specifies the rendering cut off
    --- near the viewer. Must be a value greater than zero.
    ---
    ---@param value number
    function CameraComponent.SetNearPlane(value) end

    --- Returns the far plane.
    ---
    ---@return number
    function CameraComponent.GetFarPlane() end

    --- Sets the far plane (view distance) of the camera.
    ---
    ---@param value number
    function CameraComponent.SetFarPlane(value) end

    --- Returns the focal length.
    ---
    ---@return number
    function CameraComponent.GetFocalLength() end

    --- Sets the focal distance (focus distance) of the camera. This is used by
    --- depth of field.
    ---
    ---@param value number
    function CameraComponent.SetFocalLength(value) end

    --- Returns the aperture size.
    ---
    ---@return number
    function CameraComponent.GetApertureSize() end

    --- Sets the aperture size of the camera. Larger values will make the depth
    --- of field effect stronger.
    ---
    ---@param value number
    function CameraComponent.SetApertureSize(value) end

    --- Returns the aperture shape.
    ---
    ---@return number
    function CameraComponent.GetApertureShape() end

    --- Sets the aperture shape of camera, used for depth of field effect. The
    --- value's `.X` element specifies the horizontal, the `.Y` element
    --- specifies the vertical shape.
    ---
    ---@param value Vector
    function CameraComponent.SetApertureShape(value) end

    --- Returns the view.
    ---
    ---@return Matrix
    function CameraComponent.GetView() end

    --- Returns the projection.
    ---
    ---@return Matrix
    function CameraComponent.GetProjection() end

    --- Returns the view projection.
    ---
    ---@return Matrix
    function CameraComponent.GetViewProjection() end

    --- Returns the inv view.
    ---
    ---@return Matrix
    function CameraComponent.GetInvView() end

    --- Returns the inv projection.
    ---
    ---@return Matrix
    function CameraComponent.GetInvProjection() end

    --- Returns the inv view projection.
    ---
    ---@return Matrix
    function CameraComponent.GetInvViewProjection() end

    --- Returns the position.
    ---
    ---@return Vector
    function CameraComponent.GetPosition() end

    --- Returns the look direction.
    ---
    ---@return Vector
    function CameraComponent.GetLookDirection() end

    --- Returns the up direction.
    ---
    ---@return Vector
    function CameraComponent.GetUpDirection() end

    --- Returns the right direction.
    ---
    ---@return Vector
    function CameraComponent.GetRightDirection() end

    --- Sets the position of the camera. `UpdateCamera()` should be used after
    --- this to apply the value.
    ---
    ---@param value Vector
    function CameraComponent.SetPosition(value) end

    --- Sets the look direction of the camera. The value must be a normalized
    --- direction `Vector`, relative to the camera position, and also
    --- perpendicular to the up direction. `UpdateCamera()` should be used after
    --- this to apply the value. This value will also be set if using the
    --- `TransformCamera()` function, from the transform's rotation.
    ---
    ---@param value Vector
    function CameraComponent.SetLookDirection(value) end

    --- Sets the up direction of the camera. This must be a normalized direction
    --- `Vector`, relative to the camera position, and also perpendicular to the
    --- look direction. `UpdateCamera()` should be used after this to apply the
    --- value. This value will also be set if using the `TransformCamera()`
    --- function, from the transform's rotation.
    ---
    ---@param value Vector
    function CameraComponent.SetUpDirection(value) end

    --- Enable orthographic projection for the camera.
    ---
    ---@param value boolean
    function CameraComponent.SetOrtho(value) end

    --- Returns true if the camera is using orthographic projection, false
    --- otherwise.
    ---
    ---@return boolean
    function CameraComponent.IsOrtho() end

    --- Returns the ortho vertical size.
    ---
    ---@return number
    function CameraComponent.GetOrthoVerticalSize() end

    --- Sets the vertical size of the camera in world space, used only in
    --- orthographic projection mode.
    ---
    ---@param value number
    function CameraComponent.SetOrthoVerticalSize(value) end

    --- Projects the world-space point to screen space within the canvas logical
    --- width and height units (sceen width and height). If the Z coordinate is
    --- positive that means that it is in front of the camera, otherwise it is
    --- behind (can be considered to be clipped).
    ---
    ---@param point Vector
    ---@param canvas Canvas
    function CameraComponent.ProjectToScreen(point, canvas) end
```

## AnimationComponent

```lua
    --- Creates a new, standalone AnimationComponent that owns its own data.
    ---
    ---@return AnimationComponent
    function AnimationComponent() end

    --- Plays back and controls an animation (skeletal or property animation) on
    --- an entity.
    ---
    ---@class AnimationComponent
    ---
    ---@field Timer number
    ---@field Amount number
    local AnimationComponent = {}

    --- Play.
    function AnimationComponent.Play() end

    --- Stop.
    function AnimationComponent.Stop() end

    --- Pause.
    function AnimationComponent.Pause() end

    --- Sets the animation to repeat continuously.
    ---
    ---@param value boolean
    function AnimationComponent.SetLooped(value) end

    --- Returns true if the animation is set to repeat continuously.
    ---
    ---@return boolean
    function AnimationComponent.IsLooped() end

    --- Returns whether playing.
    ---
    ---@return boolean
    function AnimationComponent.IsPlaying() end

    --- Sets the timer.
    ---
    ---@param value number
    function AnimationComponent.SetTimer(value) end

    --- Returns the timer.
    ---
    ---@return number
    function AnimationComponent.GetTimer() end

    --- Sets the amount.
    ---
    ---@param value number
    function AnimationComponent.SetAmount(value) end

    --- Returns the amount.
    ---
    ---@return number
    function AnimationComponent.GetAmount() end

    --- Returns the start.
    ---
    ---@return number
    function AnimationComponent.GetStart() end

    --- Sets the start.
    ---
    ---@param value number
    function AnimationComponent.SetStart(value) end

    --- Returns the end.
    ---
    ---@return number
    function AnimationComponent.GetEnd() end

    --- Sets the end.
    ---
    ---@param value number
    function AnimationComponent.SetEnd(value) end

    --- Sets the animation to play forward and then backwards repeatedly.
    ---
    ---@param value boolean
    function AnimationComponent.SetPingPong(value) end

    --- Returns true if the animation is set to play forward and then backwards
    --- repeatedly.
    ---
    ---@return boolean
    function AnimationComponent.IsPingPong() end

    --- Sets the animation to play once.
    function AnimationComponent.SetPlayOnce() end

    --- Returns true if the animation is set to play once.
    ---
    ---@return boolean
    function AnimationComponent.IsPlayingOnce() end

    --- Returns whether the animation has reached its end.
    ---
    ---@return boolean
    function AnimationComponent.IsEnded() end

    --- Returns whether root motion is enabled.
    ---
    ---@return boolean
    function AnimationComponent.IsRootMotion() end

    --- Enables root motion for this animation.
    function AnimationComponent.RootMotionOn() end

    --- Disables root motion for this animation.
    function AnimationComponent.RootMotionOff() end

    --- Returns the root-motion translation.
    ---
    ---@return Vector
    function AnimationComponent.GetRootTranslation() end

    --- Returns the root-motion rotation as a quaternion.
    ---
    ---@return Vector
    function AnimationComponent.GetRootRotation() end
```

## MaterialComponent

```lua
    --- Creates a new, standalone MaterialComponent that owns its own data.
    ---
    ---@return MaterialComponent
    function MaterialComponent() end

    --- Surface material of an object: colors, textures and PBR shading
    --- parameters.
    ---
    ---@class MaterialComponent
    ---
    ---@field Saturation number
    ---@field _flags integer
    ---@field BaseColor Vector
    ---@field EmissiveColor Vector
    ---@field EngineStencilRef integer
    ---@field UserStencilRef integer
    ---@field ShaderType integer
    ---@field UserBlendMode integer
    ---@field SpecularColor Vector
    ---@field SubsurfaceScattering Vector
    ---@field TexMulAdd Vector
    ---@field Roughness number
    ---@field Reflectance number
    ---@field Metalness number
    ---@field NormalMapStrength number
    ---@field ParallaxOcclusionMapping number
    ---@field DisplacementMapping number
    ---@field Refraction number
    ---@field Transmission number
    ---@field Cloak number
    ---@field ChromaticAberration number
    ---@field AlphaRef number
    ---@field SheenColor Vector
    ---@field SheenRoughness number
    ---@field Clearcoat number
    ---@field ClearcoatRoughness number
    ---@field ShadingRate integer
    ---@field TexAnimDirection Vector
    ---@field TexAnimFrameRate number
    ---@field texAnimElapsedTime number
    ---@field customShaderID integer
    local MaterialComponent = {}

    --- Sets the base color.
    ---
    ---@param value Vector
    function MaterialComponent.SetBaseColor(value) end

    --- Sets the emissive color.
    ---
    ---@param value Vector
    function MaterialComponent.SetEmissiveColor(value) end

    --- Sets the engine stencil ref.
    ---
    ---@param value integer
    function MaterialComponent.SetEngineStencilRef(value) end

    --- Sets the user stencil ref.
    ---
    ---@param value integer
    function MaterialComponent.SetUserStencilRef(value) end

    --- Returns the stencil ref.
    ---
    ---@return integer
    function MaterialComponent.GetStencilRef() end

    --- Sets the tex mul add.
    ---
    ---@param vector Vector
    function MaterialComponent.SetTexMulAdd(vector) end

    --- Returns the tex mul add.
    ---
    ---@return Vector
    function MaterialComponent.GetTexMulAdd() end

    --- Sets the texture.
    ---
    ---@param slot TextureSlot
    ---@param texturefile string
    function MaterialComponent.SetTexture(slot, texturefile) end

    --- Sets the texture.
    ---
    ---@param slot TextureSlot
    ---@param texture Texture
    function MaterialComponent.SetTexture(slot, texture) end

    --- Sets the texture uvset.
    ---
    ---@param slot TextureSlot
    ---@param uvset integer
    function MaterialComponent.SetTextureUVSet(slot, uvset) end

    --- Returns the texture.
    ---
    ---@param slot TextureSlot
    ---
    ---@return Texture
    function MaterialComponent.GetTexture(slot) end

    --- Returns the texture name.
    ---
    ---@param slot TextureSlot
    ---
    ---@return any
    function MaterialComponent.GetTextureName(slot) end

    --- Returns the texture uvset.
    ---
    ---@param slot TextureSlot
    ---
    ---@return integer
    function MaterialComponent.GetTextureUVSet(slot) end

    --- Sets the cast shadow.
    ---
    ---@param value boolean
    function MaterialComponent.SetCastShadow(value) end

    --- Returns whether casting shadow.
    ---
    ---@return boolean
    function MaterialComponent.IsCastingShadow() end

    --- force transparent material draw in opaque pass (useful for coplanar
    --- polygons)
    ---
    ---@param value boolean
    function MaterialComponent.SetCoplanarBlending(value) end

    --- Returns whether coplanar blending.
    ---
    ---@return boolean
    function MaterialComponent.IsCoplanarBlending() end

    --- Identifies a texture slot on a MaterialComponent (base color, normal
    --- map, surface map, etc.).
    ---
    ---@enum TextureSlot
    TextureSlot = {
        BASECOLORMAP = 0,
        NORMALMAP = 1,
        SURFACEMAP = 2,
        EMISSIVEMAP = 3,
        DISPLACEMENTMAP = 4,
        OCCLUSIONMAP = 5,
        TRANSMISSIONMAP = 6,
        SHEENCOLORMAP = 7,
        SHEENROUGHNESSMAP = 8,
        CLEARCOATMAP = 9,
        CLEARCOATROUGHNESSMAP = 10,
        CLEARCOATNORMALMAP = 11,
        SPECULARMAP = 12,
        ANISOTROPYMAP = 13,
        TRANSPARENCYMAP = 14,
    }

    -- Shader types, for use with `MaterialComponent.SetShaderType`.

    --- Standard physically based rendering shader.
    ---
    ---@type integer
    SHADERTYPE_PBR = 0

    --- PBR with planar reflections.
    ---
    ---@type integer
    SHADERTYPE_PBR_PLANARREFLECTION = 1

    --- PBR with parallax occlusion mapping.
    ---
    ---@type integer
    SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING = 2

    --- PBR with anisotropic specular.
    ---
    ---@type integer
    SHADERTYPE_PBR_ANISOTROPIC = 3

    --- Water surface shader.
    ---
    ---@type integer
    SHADERTYPE_WATER = 4

    --- Cartoon (toon) shader.
    ---
    ---@type integer
    SHADERTYPE_CARTOON = 5

    --- Unlit shader (no lighting).
    ---
    ---@type integer
    SHADERTYPE_UNLIT = 6

    --- PBR cloth shader.
    ---
    ---@type integer
    SHADERTYPE_PBR_CLOTH = 7

    --- PBR with clearcoat.
    ---
    ---@type integer
    SHADERTYPE_PBR_CLEARCOAT = 8

    --- PBR cloth with clearcoat.
    ---
    ---@type integer
    SHADERTYPE_PBR_CLOTH_CLEARCOAT = 9

    --- Engine stencil reference: empty.
    ---
    ---@type integer
    STENCILREF_EMPTY = 0

    --- Engine stencil reference: default.
    ---
    ---@type integer
    STENCILREF_DEFAULT = 1

    --- Engine stencil reference: custom shader.
    ---
    ---@type integer
    STENCILREF_CUSTOMSHADER = 2

    --- Engine stencil reference: outline.
    ---
    ---@type integer
    STENCILREF_OUTLINE = 3

    --- Engine stencil reference: custom shader and outline.
    ---
    ---@type integer
    STENCILREF_CUSTOMSHADER_OUTLINE = 4

    --- Engine stencil reference: skin.
    ---
    ---@type integer
    STENCILREF_SKIN = 3

    --- Engine stencil reference: snow.
    ---
    ---@type integer
    STENCILREF_SNOW = 4
```

## MeshComponent

```lua
    --- Creates a new, standalone MeshComponent that owns its own data.
    ---
    ---@return MeshComponent
    function MeshComponent() end

    --- Geometry data (vertices, indices and subsets) that objects reference and
    --- render.
    ---
    ---@class MeshComponent
    ---
    ---@field _flags integer
    ---@field TessellationFactor number
    ---@field ArmatureID integer
    ---@field SubsetsPerLOD integer
    local MeshComponent = {}

    --- Sets the mesh subset material id.
    ---
    ---@param subsetindex integer
    ---@param materialID Entity
    function MeshComponent.SetMeshSubsetMaterialID(subsetindex, materialID) end

    --- Returns the mesh subset material id.
    ---
    ---@param subsetindex integer
    ---
    ---@return Entity
    function MeshComponent.GetMeshSubsetMaterialID(subsetindex) end

    --- creates subset containing all faces, returns subset index
    ---
    ---@return integer
    function MeshComponent.CreateSubset() end
```

## EmitterComponent

```lua
    --- Creates a new, standalone EmitterComponent that owns its own data.
    ---
    ---@return EmitterComponent
    function EmitterComponent() end

    --- A GPU particle emitter attached to an entity.
    ---
    ---@class EmitterComponent
    ---
    ---@field _flags integer
    ---@field ShaderType integer
    ---@field Mass number
    ---@field Velocity Vector
    ---@field Gravity Vector
    ---@field Drag number
    ---@field Restitution number
    ---@field EmitCount number emitted particle count per second
    ---@field Size number particle starting size
    ---@field Life number particle lifetime
    ---@field NormalFactor number normal factor that modulates emit velocities
    ---@field Randomness number general randomness factor
    ---@field LifeRandomness number lifetime randomness factor
    ---@field ScaleX number scaling along lifetime in X axis
    ---@field ScaleY number scaling along lifetime in Y axis
    ---@field Rotation number rotation speed
    ---@field MotionBlurAmount number set the motion elongation factor
    ---@field SPH_h number
    ---@field SPH_K number
    ---@field SPH_p0 number
    ---@field SPH_e number
    ---@field SpriteSheet_Frames_X integer
    ---@field SpriteSheet_Frames_Y integer
    ---@field SpriteSheet_Frame_Count integer
    ---@field SpriteSheet_Frame_Start integer
    ---@field SpriteSheet_Framerate number
    local EmitterComponent = {}

    --- Spawns a specific amount of particles immediately.
    ---
    ---@param value integer
    function EmitterComponent.Burst(value) end

    --- Spawns a specific amount of particles immediately at specified location
    --- and color multiplier.
    ---
    ---@param value integer
    ---@param position Vector
    ---@param color? Vector
    function EmitterComponent.Burst(value, position, color) end

    --- Spawns a specific amount of particles immediately at specified location
    --- and color multiplier.
    ---
    ---@param value integer
    ---@param transform Matrix
    ---@param color? Vector
    function EmitterComponent.Burst(value, transform, color) end

    --- Set the emitted particle count per second.
    ---
    ---@param value number
    function EmitterComponent.SetEmitCount(value) end

    --- Set particle starting size.
    ---
    ---@param value number
    function EmitterComponent.SetSize(value) end

    --- Set particle lifetime.
    ---
    ---@param value number
    function EmitterComponent.SetLife(value) end

    --- Set normal factor that modulates emit velocities.
    ---
    ---@param value number
    function EmitterComponent.SetNormalFactor(value) end

    --- Set general randomness factor.
    ---
    ---@param value number
    function EmitterComponent.SetRandomness(value) end

    --- Set lifetime randomness factor.
    ---
    ---@param value number
    function EmitterComponent.SetLifeRandomness(value) end

    --- Set scaling along lifetime in X axis.
    ---
    ---@param value number
    function EmitterComponent.SetScaleX(value) end

    --- Set scaling along lifetime in Y axis.
    ---
    ---@param value number
    function EmitterComponent.SetScaleY(value) end

    --- Set rotation speed.
    ---
    ---@param value number
    function EmitterComponent.SetRotation(value) end

    --- Set the motion elongation factor.
    ---
    ---@param value number
    function EmitterComponent.SetMotionBlurAmount(value) end

    --- Disable GPU colliders.
    ---
    ---@param value boolean
    function EmitterComponent.SetCollidersDisabled(value) end

    --- Returns whether colliders disabled.
    function EmitterComponent.IsCollidersDisabled() end

    --- Returns last known alive particle count (not that particles are tracked
    --- on GPU so this value might be out of date).
    ---
    ---@return integer
    function EmitterComponent.GetCurrentParticleCount() end
```

## HairParticleSystem

```lua
    --- Creates a new, standalone HairParticleSystem that owns its own data.
    ---
    ---@return HairParticleSystem
    function HairParticleSystem() end

    --- A GPU hair or grass particle system attached to an entity.
    ---
    ---@class HairParticleSystem
    ---
    ---@field _flags integer
    ---@field StrandCount integer
    ---@field SegmentCount integer
    ---@field RandomSeed integer
    ---@field Length number
    ---@field Stiffness number
    ---@field Randomness number
    ---@field ViewDistance number
    ---@field SpriteSheet_Frames_X integer
    ---@field SpriteSheet_Frames_Y integer
    ---@field SpriteSheet_Frame_Count integer
    ---@field SpriteSheet_Frame_Start integer
    local HairParticleSystem = {}
```

## LightComponent

```lua
    --- Creates a new, standalone LightComponent that owns its own data.
    ---
    ---@return LightComponent
    function LightComponent() end

    --- A light source in the scene (directional, point or spot).
    ---
    ---@class LightComponent
    ---
    ---@field Type integer
    ---@field Range number
    ---@field Intensity number
    ---@field Color Vector
    ---@field CastShadow boolean
    ---@field VolumetricsEnabled boolean
    ---@field OuterConeAngle number outer cone angle for spotlight in radians
    ---@field InnerConeAngle number inner cone angle for spotlight in radians
    local LightComponent = {}

    --- DIRECTIONAL.
    ---
    ---@type integer
    DIRECTIONAL = 0

    --- POINT.
    ---
    ---@type integer
    POINT = 1

    --- SPOT.
    ---
    ---@type integer
    SPOT = 2

    --- Sphere area light type.
    ---
    ---@type integer
    SPHERE = 3

    --- Disc area light type.
    ---
    ---@type integer
    DISC = 4

    --- Rectangle area light type.
    ---
    ---@type integer
    RECTANGLE = 5

    --- Tube area light type.
    ---
    ---@type integer
    TUBE = 6

    --- Set light type, see accepted values below (by default it is a point
    --- light).
    ---
    ---@param type integer
    function LightComponent.SetType(type) end

    --- Sets the range.
    ---
    ---@param value number
    function LightComponent.SetRange(value) end

    --- Brightness of the light. The unit depends on the light type: point and
    --- spot lights use luminous intensity in candela (lm/sr), while directional
    --- lights use illuminance in lux (lm/m2). See the glTF KHR_lights_punctual
    --- spec:
    --- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
    ---
    ---@param value number
    function LightComponent.SetIntensity(value) end

    --- Sets the color.
    ---
    ---@param value Vector
    function LightComponent.SetColor(value) end

    --- Sets the cast shadow.
    ---
    ---@param value boolean
    function LightComponent.SetCastShadow(value) end

    --- Enables or disables volumetrics.
    ---
    ---@param value boolean
    function LightComponent.SetVolumetricsEnabled(value) end

    --- Outer cone angle for spotlight in radians.
    ---
    ---@param value number
    function LightComponent.SetOuterConeAngle(value) end

    --- Inner cone angle for spotlight in radians (0 <= innerConeAngle <=
    --- outerConeAngle). Value of 0 disables inner cone angle.
    ---
    ---@param value number
    function LightComponent.SetInnerConeAngle(value) end

    --- Returns the type.
    ---
    ---@return integer
    function LightComponent.GetType() end

    --- Kept for backwards compatibility with non physical light units (before
    --- v0.70.0).
    ---
    ---@param value number
    function LightComponent.SetEnergy(value) end

    --- Kept for backwards compatibility with FOV angle (before v0.70.0).
    ---
    ---@param value number
    function LightComponent.SetFOV(value) end

    --- Returns whether the light casts shadows.
    ---
    ---@return boolean
    function LightComponent.IsCastShadow() end

    --- Returns whether volumetric light is enabled.
    ---
    ---@return boolean
    function LightComponent.IsVolumetricsEnabled() end
```

## ObjectComponent

```lua
    --- Creates a new, standalone ObjectComponent that owns its own data.
    ---
    ---@return ObjectComponent
    function ObjectComponent() end

    --- Places a mesh into the scene as a renderable instance, with per-instance
    --- rendering options.
    ---
    ---@class ObjectComponent
    ---
    ---@field MeshID integer
    ---@field CascadeMask integer
    ---@field RendertypeMask integer
    ---@field Color Vector
    ---@field EmissiveColor Vector
    ---@field UserStencilRef integer
    ---@field LodDistanceMultiplier number
    ---@field DrawDistance number
    local ObjectComponent = {}

    --- Returns the mesh id.
    ---
    ---@return Entity
    function ObjectComponent.GetMeshID() end

    --- Returns the cascade mask.
    ---
    ---@return integer
    function ObjectComponent.GetCascadeMask() end

    --- Returns the rendertype mask.
    ---
    ---@return integer
    function ObjectComponent.GetRendertypeMask() end

    --- Returns the color.
    ---
    ---@return Vector
    function ObjectComponent.GetColor() end

    --- Returns the emissive color.
    ---
    ---@return Vector
    function ObjectComponent.GetEmissiveColor() end

    --- Returns the user stencil ref.
    ---
    ---@return integer
    function ObjectComponent.GetUserStencilRef() end

    --- Returns the draw distance.
    ---
    ---@return number
    function ObjectComponent.GetDrawDistance() end

    --- Sets the mesh id.
    ---
    ---@param entity Entity
    function ObjectComponent.SetMeshID(entity) end

    --- Sets the cascade mask.
    ---
    ---@param value integer
    function ObjectComponent.SetCascadeMask(value) end

    --- Sets the rendertype mask.
    ---
    ---@param value integer
    function ObjectComponent.SetRendertypeMask(value) end

    --- Sets the color.
    ---
    ---@param value Vector
    function ObjectComponent.SetColor(value) end

    --- Set the RGB color for rim highlight
    ---
    ---@param value Vector
    function ObjectComponent.SetRimHighlightColor(value) end

    --- Set the intensity (multiplier) of rim highlight color
    ---
    ---@param value number
    function ObjectComponent.SetRimHighlightIntensity(value) end

    --- Set the falloff power of rim highlight
    ---
    ---@param value number
    function ObjectComponent.SetRimHighlightFalloff(value) end

    --- Sets the emissive color.
    ---
    ---@param value Vector
    function ObjectComponent.SetEmissiveColor(value) end

    --- Sets the user stencil ref.
    ---
    ---@param value integer
    function ObjectComponent.SetUserStencilRef(value) end

    --- Sets the draw distance.
    ---
    ---@param value number
    function ObjectComponent.SetDrawDistance(value) end

    --- Enable/disable foreground object rendering. Foreground objects will be
    --- always rendered on top of regular objects, useful for FPS weapon or
    --- hands.
    ---
    ---@param value boolean
    function ObjectComponent.SetForeground(value) end

    --- Returns whether foreground.
    ---
    ---@return boolean
    function ObjectComponent.IsForeground() end

    --- You can set the object to not be visible in main camera, but it will
    --- remain visible in reflections and shadows, useful for FPS character
    --- model.
    ---
    ---@param value boolean
    function ObjectComponent.SetNotVisibleInMainCamera(value) end

    --- Returns whether not visible in main camera.
    ---
    ---@return boolean
    function ObjectComponent.IsNotVisibleInMainCamera() end

    --- You can set the object to not be visible in main camera, but it will
    --- remain visible in reflections and shadows, useful for vampires.
    ---
    ---@param value boolean
    function ObjectComponent.SetNotVisibleInReflections(value) end

    --- Returns whether not visible in reflections.
    ---
    ---@return boolean
    function ObjectComponent.IsNotVisibleInReflections() end

    --- Enable wet map for the object, this will automatically track the
    --- wetness.
    ---
    ---@param value boolean
    function ObjectComponent.SetWetmapEnabled(value) end

    --- Returns whether wetmap enabled.
    ---
    ---@return boolean
    function ObjectComponent.IsWetmapEnabled() end

    --- Can turn off rendering of an object.
    ---
    ---@param value boolean
    function ObjectComponent.SetRenderable(value) end

    --- Returns whether renderable.
    ---
    ---@return boolean
    function ObjectComponent.IsRenderable() end

    --- Returns the alpha test reference value.
    ---
    ---@return number
    function ObjectComponent.GetAlphaRef() end

    --- Sets the alpha test reference value.
    ---
    ---@param value number
    function ObjectComponent.SetAlphaRef(value) end
```

## InverseKinematicsComponent

Describes an Inverse Kinematics effector.

```lua
    --- Creates a new, standalone InverseKinematicsComponent that owns its
    --- own data.
    ---
    ---@return InverseKinematicsComponent
    function InverseKinematicsComponent() end

    --- Drives a chain of bones toward a target entity using inverse kinematics.
    ---
    ---@class InverseKinematicsComponent
    ---
    ---@field Target integer
    ---@field ChainLength integer
    ---@field IterationCount integer
    local InverseKinematicsComponent = {}

    --- Sets the target entity (The IK entity and its parent hierarchy chain
    --- will try to reach the target).
    ---
    ---@param entity Entity
    function InverseKinematicsComponent.SetTarget(entity) end

    --- Sets the chain length, in other words, how many parents will be computed
    --- by the IK system.
    ---
    ---@param value integer
    function InverseKinematicsComponent.SetChainLength(value) end

    --- Sets the accuracy of the IK system simulation.
    ---
    ---@param value integer
    function InverseKinematicsComponent.SetIterationCount(value) end

    --- Disable/Enable the IK simulation.
    ---
    ---@param value boolean
    function InverseKinematicsComponent.SetDisabled(value) end

    --- Returns the target.
    ---
    ---@return Entity
    function InverseKinematicsComponent.GetTarget() end

    --- Returns the chain length.
    ---
    ---@return integer
    function InverseKinematicsComponent.GetChainLength() end

    --- Returns the iteration count.
    ---
    ---@return integer
    function InverseKinematicsComponent.GetIterationCount() end

    --- Returns whether disabled.
    ---
    ---@return boolean
    function InverseKinematicsComponent.IsDisabled() end
```

## SpringComponent

```lua
    --- Creates a new, standalone SpringComponent that owns its own data.
    ---
    ---@return SpringComponent
    function SpringComponent() end

    --- Adds spring/jiggle physics to a transform, for soft secondary motion
    --- such as bones.
    ---
    ---@class SpringComponent
    ---
    ---@field Stiffness number
    ---@field Damping number
    ---@field WindAffection number
    ---@field DragForce number
    ---@field HitRadius number
    ---@field GravityPower number
    ---@field GravityDirection Vector
    local SpringComponent = {}

    --- Sets the stiffness.
    ---
    ---@param value number
    function SpringComponent.SetStiffness(value) end

    --- Sets the damping.
    ---
    ---@param value number
    function SpringComponent.SetDamping(value) end

    --- Sets the wind affection.
    ---
    ---@param value number
    function SpringComponent.SetWindAffection(value) end

```

## ScriptComponent

```lua
    --- Creates a new, standalone ScriptComponent that owns its own data.
    ---
    ---@return ScriptComponent
    function ScriptComponent() end

    --- A lua script bound to an entity.
    ---
    ---@class ScriptComponent
    local ScriptComponent = {}

    --- Creates from file.
    ---
    ---@param filename string
    function ScriptComponent.CreateFromFile(filename) end

    --- Play.
    function ScriptComponent.Play() end

    --- Returns whether playing.
    ---
    ---@return boolean
    function ScriptComponent.IsPlaying() end

    --- Sets the play once.
    ---
    ---@param once boolean
    function ScriptComponent.SetPlayOnce(once) end

    --- Stop.
    function ScriptComponent.Stop() end
```

## RigidBodyPhysicsComponent

```lua
    --- Creates a new, standalone RigidBodyPhysicsComponent that owns its
    --- own data.
    ---
    ---@return RigidBodyPhysicsComponent
    function RigidBodyPhysicsComponent() end

    --- Describes a Rigid Body Physics object.
    ---
    ---@class RigidBodyPhysicsComponent
    ---
    ---@field Shape integer
    ---@field Mass number
    ---@field Friction number
    ---@field Restitution number
    ---@field LinearDamping number
    ---@field AngularDamping number
    ---@field Buoyancy number
    ---@field BoxParams_HalfExtents Vector
    ---@field SphereParams_Radius number
    ---@field CapsuleParams_Radius number
    ---@field CapsuleParams_Height number
    ---@field TargetMeshLOD integer
    ---@field MaxSlopeAngle number character physics max slope angle in radians
    ---@field GravityFactor number character physics gravity factor
    local RigidBodyPhysicsComponent = {}

    --- Returns true if this is a vehicle, false otherwise.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsVehicle() end

    --- Returns true if this is a car vehicle, false otherwise.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsCar() end

    --- Returns true if this is a motorcycle vehicle, false otherwise.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsMotorcycle() end

    --- Check if the rigidbody is able to deactivate after inactivity.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsDisableDeactivation() end

    --- Check if the rigidbody is movable or just static.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsKinematic() end

    --- Checks whether rigid body is set to be deactivated when added to
    --- simulation.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsStartDeactivated() end

    --- Sets if the rigidbody is able to deactivate after inactivity.
    ---
    ---@param value boolean
    function RigidBodyPhysicsComponent.SetDisableDeactivation(value) end

    --- Set the rigid body to be kinematic (which means it is optimized for
    --- being moved by the system or user logic, not the physics engine).
    ---
    ---@param value boolean
    function RigidBodyPhysicsComponent.SetKinematic(value) end

    --- If true, rigid body will be deactivated when added to the simulation (if
    --- it's dynamic, it won't fall).
    ---
    ---@param value boolean
    function RigidBodyPhysicsComponent.SetStartDeactivated(value) end

    --- Enable character physics that is driven by the physics engine.
    ---
    ---@param value boolean
    function RigidBodyPhysicsComponent.SetCharacterPhysics(value) end

    --- Returns true if this rigid body has character physics enabled.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsCharacterPhysics() end

    --- Locks the physics to the 2D plane (XY translation, Z rotation).
    ---
    ---@param value boolean
    function RigidBodyPhysicsComponent.SetLocked2D(value) end

    --- Returns true if the physics is locked to the 2D plane.
    ---
    ---@return boolean
    function RigidBodyPhysicsComponent.IsLocked2D() end

    --- Collision shape of a RigidBodyPhysicsComponent.
    ---
    ---@enum RigidBodyShape
    RigidBodyShape = {
        Box = 0,
        Sphere = 1,
        Capsule = 2,
        ConvexHull = 3,
        TriangleMesh = 4,
    }
```

## SoftBodyPhysicsComponent

```lua
    --- Creates a new, standalone SoftBodyPhysicsComponent that owns its
    --- own data.
    ---
    ---@return SoftBodyPhysicsComponent
    function SoftBodyPhysicsComponent() end

    --- Describes a Soft Body Physics object.
    ---
    ---@class SoftBodyPhysicsComponent
    ---
    ---@field Mass number
    ---@field Friction number
    ---@field Restitution number
    ---@field VertexRadius number
    local SoftBodyPhysicsComponent = {}

    --- Set how much detail the soft body simulation has compared to the
    --- graphics mesh. Setting this will rebuild the soft body, so individual
    --- physics vertex settings will be lost.
    ---
    ---@param value number
    function SoftBodyPhysicsComponent.SetDetail(value) end

    --- Returns the detail.
    ---
    ---@return number
    function SoftBodyPhysicsComponent.GetDetail() end

    --- Sets the disable deactivation.
    ---
    ---@param value boolean
    function SoftBodyPhysicsComponent.SetDisableDeactivation(value) end

    --- Returns whether disable deactivation.
    ---
    ---@return boolean
    function SoftBodyPhysicsComponent.IsDisableDeactivation() end

    --- Enables or disables wind.
    ---
    ---@param value boolean
    function SoftBodyPhysicsComponent.SetWindEnabled(value) end

    --- Returns whether wind enabled.
    ---
    ---@return boolean
    function SoftBodyPhysicsComponent.IsWindEnabled() end

    --- Creates from mesh.
    ---
    ---@param mesh MeshComponent
    function SoftBodyPhysicsComponent.CreateFromMesh(mesh) end
```

## ForceFieldComponent

```lua
    --- Creates a new, standalone ForceFieldComponent that owns its own data.
    ---
    ---@return ForceFieldComponent
    function ForceFieldComponent() end

    --- A force field that attracts or repels particles and physics bodies
    --- (point or plane type).
    ---
    ---@class ForceFieldComponent
    ---
    ---@field Type integer
    ---@field Gravity number
    ---@field Range number
    local ForceFieldComponent = {}
```

## WeatherComponent

```lua
    --- Creates a new, standalone WeatherComponent that owns its own data.
    ---
    ---@return WeatherComponent
    function WeatherComponent() end

    --- Global environment settings: sky, fog, wind, stars, rain, ocean, clouds
    --- and ambient lighting.
    ---
    ---@class WeatherComponent
    ---
    ---@field fogHeightSky number
    ---@field cloudiness number
    ---@field cloudScale number
    ---@field cloudSpeed number
    ---@field cloud_shadow_amount number
    ---@field cloud_shadow_scale number
    ---@field cloud_shadow_speed number
    ---@field rainLength number
    ---@field skyMapName string
    ---@field colorGradingMapName string
    ---@field volumetricCloudsWeatherMapFirstName string
    ---@field volumetricCloudsWeatherMapSecondName string
    ---@field OceanParameters OceanParameters
    ---@field AtmosphereParameters AtmosphereParameters
    ---@field VolumetricCloudParameters VolumetricCloudParameters
    ---@field SkyMapName string Resource name for sky texture
    ---@field ColorGradingMapName string Resource name for color grading map
    ---@field sunColor Vector
    ---@field sunDirection Vector
    ---@field skyExposure number
    ---@field horizon Vector
    ---@field zenith Vector
    ---@field ambient Vector
    ---@field fogStart number
    ---@field fogDensity number
    ---@field fogHeightStart number
    ---@field fogHeightEnd number
    ---@field windDirection Vector
    ---@field windRandomness number
    ---@field windWaveSize number
    ---@field windSpeed number
    ---@field stars number
    ---@field rainAmount number
    ---@field rainLenght number
    ---@field rainSpeed number
    ---@field rainScale number
    ---@field rainColor Vector
    ---@field gravity Vector
    local WeatherComponent = {}

    --- Check if weather's ocean simulation is enabled.
    ---
    ---@return boolean
    function WeatherComponent.IsOceanEnabled() end

    --- Check if weather's sky is rendered in a simple, unrealistic way.
    ---
    ---@return boolean
    function WeatherComponent.IsSimpleSky() end

    --- Check if weather's sky is rendered in a physically correct, realistic
    --- way.
    ---
    ---@return boolean
    function WeatherComponent.IsRealisticSky() end

    --- Check if weather is rendering volumetric clouds.
    ---
    ---@return boolean
    function WeatherComponent.IsVolumetricClouds() end

    --- Check if weather is rendering height fog visual effect.
    ---
    ---@return boolean
    function WeatherComponent.IsHeightFog() end

    --- Sets if weather's ocean simulation is enabled or not.
    ---
    ---@param value boolean
    function WeatherComponent.SetOceanEnabled(value) end

    --- Sets if weather's sky is rendered in a simple, unrealistic way or not.
    ---
    ---@param value boolean
    function WeatherComponent.SetSimpleSky(value) end

    --- Sets if weather's sky is rendered in a physically correct, realistic way
    --- or not.
    ---
    ---@param value boolean
    function WeatherComponent.SetRealisticSky(value) end

    --- Sets if weather is rendering volumetric clouds or not.
    ---
    ---@param value boolean
    function WeatherComponent.SetVolumetricClouds(value) end

    --- Sets if weather is rendering height fog visual effect or not.
    ---
    ---@param value boolean
    function WeatherComponent.SetHeightFog(value) end

    --- Returns whether volumetric clouds cast shadow is enabled.
    ---
    ---@return boolean
    function WeatherComponent.IsVolumetricCloudsCastShadow() end

    --- Returns whether override fog color is enabled.
    ---
    ---@return boolean
    function WeatherComponent.IsOverrideFogColor() end

    --- Returns whether realistic sky aerial perspective is enabled.
    ---
    ---@return boolean
    function WeatherComponent.IsRealisticSkyAerialPerspective() end

    --- Returns whether realistic sky high quality is enabled.
    ---
    ---@return boolean
    function WeatherComponent.IsRealisticSkyHighQuality() end

    --- Returns whether realistic sky receive shadow is enabled.
    ---
    ---@return boolean
    function WeatherComponent.IsRealisticSkyReceiveShadow() end

    --- Returns whether volumetric clouds receive shadow is enabled.
    ---
    ---@return boolean
    function WeatherComponent.IsVolumetricCloudsReceiveShadow() end

    --- Sets whether volumetric clouds cast shadow is enabled.
    ---
    ---@param value boolean
    function WeatherComponent.SetVolumetricCloudsCastShadow(value) end

    --- Sets whether override fog color is enabled.
    ---
    ---@param value boolean
    function WeatherComponent.SetOverrideFogColor(value) end

    --- Sets whether realistic sky aerial perspective is enabled.
    ---
    ---@param value boolean
    function WeatherComponent.SetRealisticSkyAerialPerspective(value) end

    --- Sets whether realistic sky high quality is enabled.
    ---
    ---@param value boolean
    function WeatherComponent.SetRealisticSkyHighQuality(value) end

    --- Sets whether realistic sky receive shadow is enabled.
    ---
    ---@param value boolean
    function WeatherComponent.SetRealisticSkyReceiveShadow(value) end

    --- Sets whether volumetric clouds receive shadow is enabled.
    ---
    ---@param value boolean
    function WeatherComponent.SetVolumetricCloudsReceiveShadow(value) end
```

### OceanParameters

```lua
    --- Creates a new, standalone OceanParameters that owns its own data.
    ---
    ---@return OceanParameters
    function OceanParameters() end

    --- Parameters of the ocean simulation, accessed through
    --- weather.OceanParameters.
    ---
    ---@class OceanParameters
    ---
    ---@field dmap_dim integer
    ---@field patch_length number
    ---@field time_scale number
    ---@field wave_amplitude number
    ---@field wind_dir Vector
    ---@field wind_speed number
    ---@field wind_dependency number
    ---@field choppy_scale number
    ---@field waterColor Vector
    ---@field waterHeight number
    ---@field surfaceDetail integer
    ---@field surfaceDisplacementTolerance number
    local OceanParameters = {}
```

### AtmosphereParameters

Physically based atmosphere and sky parameters, accessed through
`weather.AtmosphereParameters`.

```lua
    --- Creates a new, standalone AtmosphereParameters that owns its own data.
    ---
    ---@return AtmosphereParameters
    function AtmosphereParameters() end

    --- Physically based atmosphere and sky parameters, accessed through
    --- weather.AtmosphereParameters.
    ---
    ---@class AtmosphereParameters
    ---
    ---@field bottomRadius number
    ---@field topRadius number
    ---@field planetCenter Vector
    ---@field rayleighDensityExpScale number
    ---@field rayleighScattering Vector
    ---@field mieDensityExpScale number
    ---@field mieScattering Vector
    ---@field mieExtinction Vector
    ---@field mieAbsorption Vector
    ---@field miePhaseG number
    ---@field absorptionDensity0LayerWidth number
    ---@field absorptionDensity0ConstantTerm number
    ---@field absorptionDensity0LinearTerm number
    ---@field absorptionDensity1ConstantTerm number
    ---@field absorptionDensity1LinearTerm number
    ---@field absorptionExtinction Vector
    ---@field groundAlbedo Vector
    local AtmosphereParameters = {}
```

### VolumetricCloudParameters

```lua
    --- Creates a new, standalone VolumetricCloudParameters that owns its
    --- own data.
    ---
    ---@return VolumetricCloudParameters
    function VolumetricCloudParameters() end

    --- Volumetric cloud rendering parameters, accessed through
    --- `weather.VolumetricCloudParameters`.
    ---
    ---@class VolumetricCloudParameters
    ---
    ---@field cloudAmbientGroundMultiplier number
    ---@field horizonBlendAmount number
    ---@field horizonBlendPower number
    ---@field cloudStartHeight number
    ---@field cloudThickness number
    ---@field animationMultiplier number
    ---@field albedoFirst Vector
    ---@field extinctionCoefficientFirst Vector
    ---@field skewAlongWindDirectionFirst number
    ---@field totalNoiseScaleFirst number
    ---@field curlScaleFirst number
    ---@field curlNoiseModifierFirst number
    ---@field detailScaleFirst number
    ---@field detailNoiseModifierFirst number
    ---@field skewAlongCoverageWindDirectionFirst number
    ---@field weatherScaleFirst number
    ---@field coverageAmountFirst number
    ---@field coverageMinimumFirst number
    ---@field typeAmountFirst number
    ---@field typeMinimumFirst number
    ---@field rainAmountFirst number
    ---@field rainMinimumFirst number
    ---@field gradientSmallFirst number
    ---@field gradientMediumFirst number
    ---@field gradientLargeFirst number
    ---@field anvilDeformationSmallFirst number
    ---@field anvilDeformationMediumFirst number
    ---@field anvilDeformationLargeFirst number
    ---@field windSpeedFirst number
    ---@field windAngleFirst number
    ---@field windUpAmountFirst number
    ---@field coverageWindSpeedFirst number
    ---@field coverageWindAngleFirst number
    ---@field albedoSecond Vector
    ---@field extinctionCoefficientSecond Vector
    ---@field skewAlongWindDirectionSecond number
    ---@field totalNoiseScaleSecond number
    ---@field curlScaleSecond number
    ---@field curlNoiseModifierSecond number
    ---@field detailScaleSecond number
    ---@field detailNoiseModifierSecond number
    ---@field skewAlongCoverageWindDirectionSecond number
    ---@field weatherScaleSecond number
    ---@field coverageAmountSecond number
    ---@field coverageMinimumSecond number
    ---@field typeAmountSecond number
    ---@field typeMinimumSecond number
    ---@field rainAmountSecond number
    ---@field rainMinimumSecond number
    ---@field gradientSmallSecond number
    ---@field gradientMediumSecond number
    ---@field gradientLargeSecond number
    ---@field anvilDeformationSmallSecond number
    ---@field anvilDeformationMediumSecond number
    ---@field anvilDeformationLargeSecond number
    ---@field windSpeedSecond number
    ---@field windAngleSecond number
    ---@field windUpAmountSecond number
    ---@field coverageWindSpeedSecond number
    ---@field coverageWindAngleSecond number
    local VolumetricCloudParameters = {}
```

## SoundComponent

```lua
    --- Creates a new, standalone SoundComponent that owns its own data.
    ---
    ---@return SoundComponent
    function SoundComponent() end

    --- Describes a Sound object.
    ---
    ---@class SoundComponent
    ---
    ---@field Filename string
    ---@field Volume number
    local SoundComponent = {}

    --- Plays the sound.
    function SoundComponent.Play() end

    --- Stop the sound.
    function SoundComponent.Stop() end

    --- Sets if the sound is looping when playing.
    ---
    ---@param value boolean
    function SoundComponent.SetLooped(value) end

    --- Disable/Enable 3D sounds.
    ---
    ---@param value boolean
    function SoundComponent.SetDisable3D(value) end

    --- Check if sound is playing.
    ---
    ---@return boolean
    function SoundComponent.IsPlaying() end

    --- Check if sound is looping.
    ---
    ---@return boolean
    function SoundComponent.IsLooped() end

    --- Sets the sound.
    ---
    ---@param sound Sound
    function SoundComponent.SetSound(sound) end

    --- Sets the sound instance.
    ---
    ---@param inst SoundInstance
    function SoundComponent.SetSoundInstance(inst) end

    --- Returns the sound.
    ---
    ---@return Sound
    function SoundComponent.GetSound() end

    --- Returns the sound instance.
    ---
    ---@return SoundInstance
    function SoundComponent.GetSoundInstance() end

    --- Sets the sound file path.
    ---
    ---@param filename string
    function SoundComponent.SetFilename(filename) end

    --- Sets the playback volume.
    ---
    ---@param volume number
    function SoundComponent.SetVolume(volume) end

    --- Returns the sound file path.
    ---
    ---@return string
    function SoundComponent.GetFilename() end

    --- Returns the playback volume.
    ---
    ---@return number
    function SoundComponent.GetVolume() end

    --- Returns whether 3D spatialization is disabled.
    ---
    ---@return boolean
    function SoundComponent.IsDisable3D() end
```

## VideoComponent

Describes a video object

```lua
    --- Creates a new, standalone VideoComponent that owns its own data.
    ---
    ---@return VideoComponent
    function VideoComponent() end

    --- Plays a video file within the scene.
    ---
    ---@class VideoComponent
    ---
    ---@field Filename string
    local VideoComponent = {}

    --- Play.
    function VideoComponent.Play() end

    --- Stop.
    function VideoComponent.Stop() end

    --- Sets the looped.
    ---
    ---@param value boolean
    function VideoComponent.SetLooped(value) end

    --- Returns whether playing.
    ---
    ---@return boolean
    function VideoComponent.IsPlaying() end

    --- Returns whether looped.
    ---
    ---@return boolean
    function VideoComponent.IsLooped() end

    --- Returns video length in seconds.
    ---
    ---@return number
    function VideoComponent.GetLength() end

    --- Returns the current timer in seconds.
    ---
    ---@return number
    function VideoComponent.GetCurrentTimer() end

    --- Sets the decoder state to be decoding from specific time in seconds
    --- (approximately).
    ---
    ---@param timerSeconds number
    function VideoComponent.Seek(timerSeconds) end

    --- Sets the video.
    ---
    ---@param video Video
    function VideoComponent.SetVideo(video) end

    --- Sets the video instance.
    ---
    ---@param instance VideoInstance
    function VideoComponent.SetVideoInstance(instance) end

    --- Returns the video.
    ---
    ---@return Video
    function VideoComponent.GetVideo() end

    --- Returns the video instance.
    ---
    ---@return VideoInstance
    function VideoComponent.GetVideoInstance() end

    --- Sets the video file path.
    ---
    ---@param filename string
    function VideoComponent.SetFilename(filename) end

    --- Returns the video file path.
    ---
    ---@return string
    function VideoComponent.GetFilename() end
```

## ColliderComponent

Describes a Collider object.

```lua
    --- Creates a new, standalone ColliderComponent that owns its own data.
    ---
    ---@return ColliderComponent
    function ColliderComponent() end

    --- A simple collision primitive (sphere, capsule or plane) used for
    --- character and soft-body collision.
    ---
    ---@class ColliderComponent
    ---
    ---@field Shape integer Shape of the collider
    ---@field Radius number
    ---@field Offset Vector
    ---@field Tail Vector
    local ColliderComponent = {}

    --- Enables or disables CPU.
    ---
    ---@param value boolean
    function ColliderComponent.SetCPUEnabled(value) end

    --- Enables or disables GPU.
    ---
    ---@param value boolean
    function ColliderComponent.SetGPUEnabled(value) end

    --- Returns the capsule.
    ---
    ---@return Capsule
    function ColliderComponent.GetCapsule() end

    --- Returns the sphere.
    ---
    ---@return Sphere
    function ColliderComponent.GetSphere() end

    --- The shape of a ColliderComponent.
    ---
    ---@enum ColliderShape
    ColliderShape = {
        Sphere = 0,
        Capsule = 1,
        Plane = 2,
    }
```

## ExpressionComponent

```lua
    --- Creates a new, standalone ExpressionComponent that owns its own data.
    ---
    ---@return ExpressionComponent
    function ExpressionComponent() end

    --- Controls facial expressions and blend-shape weights of a character.
    ---
    ---@class ExpressionComponent
    local ExpressionComponent = {}

    --- Find an expression within the ExpressionComponent by name.
    ---
    ---@param name string
    ---
    ---@return integer
    function ExpressionComponent.FindExpressionID(name) end

    --- Set expression weight by ID. The ID can be a non-preset expression. Use
    --- FindExpressionID() to retrieve non-preset expression IDs.
    ---
    ---@param id integer
    ---
    ---@param weight number
    function ExpressionComponent.SetWeight(id, weight) end

    --- Returns current weight of expression.
    ---
    ---@param id integer
    ---
    ---@return number
    function ExpressionComponent.GetWeight(id) end

    --- Set a preset expression's weight. You can get access to preset values
    --- from ExpressionPreset table.
    ---
    ---@param preset ExpressionPreset
    ---@param weight number
    function ExpressionComponent.SetPresetWeight(preset, weight) end

    --- Returns current weight of preset expression.
    ---
    ---@param preset ExpressionPreset
    ---
    ---@return number
    function ExpressionComponent.GetPresetWeight(preset) end

    --- Force continuous talking animation, even if no voice is playing.
    ---
    ---@param value boolean
    function ExpressionComponent.SetForceTalkingEnabled(value) end

    --- Returns whether force talking enabled.
    ---
    ---@return boolean
    function ExpressionComponent.IsForceTalkingEnabled() end

    --- Sets the preset override mouth.
    ---
    ---@param preset ExpressionPreset
    ---@param override ExpressionOverride
    function ExpressionComponent.SetPresetOverrideMouth(preset, override) end

    --- Sets the preset override blink.
    ---
    ---@param preset ExpressionPreset
    ---@param override ExpressionOverride
    function ExpressionComponent.SetPresetOverrideBlink(preset, override) end

    --- Sets the preset override look.
    ---
    ---@param preset ExpressionPreset
    ---@param override ExpressionOverride
    function ExpressionComponent.SetPresetOverrideLook(preset, override) end

    --- Sets the override mouth.
    ---
    ---@param id integer
    ---@param override ExpressionOverride
    function ExpressionComponent.SetOverrideMouth(id, override) end

    --- Sets the override blink.
    ---
    ---@param id integer
    ---@param override ExpressionOverride
    function ExpressionComponent.SetOverrideBlink(id, override) end

    --- Sets the override look.
    ---
    ---@param id integer
    ---@param override ExpressionOverride
    function ExpressionComponent.SetOverrideLook(id, override) end

    --- Standard facial expression presets (VRM-style) for an
    --- ExpressionComponent.
    ---
    ---@enum ExpressionPreset
    ExpressionPreset = {
        Happy = 0,
        Angry = 1,
        Sad = 2,
        Relaxed = 3,
        Surprised = 4,
        Aa = 5,
        Ih = 6,
        Ou = 7,
        Ee = 8,
        Oh = 9,
        Blink = 10,
        BlinkLeft = 11,
        BlinkRight = 12,
        LookUp = 13,
        LookDown = 14,
        LookLeft = 15,
        LookRight = 16,
        Neutral = 17,
        None = 0,
        Block = 1,
        Blend = 2,
    }

    --- How an ExpressionComponent override blends with procedural animation.
    ---@enum ExpressionOverride
    ExpressionOverride = {
        None = 0,
        Block = 1,
        Blend = 2,
    }
```

## HumanoidComponent

```lua
    --- Creates a new, standalone HumanoidComponent that owns its own data.
    ---
    ---@return HumanoidComponent
    function HumanoidComponent() end

    --- Maps an entity's skeleton onto standard humanoid bones, enabling
    --- retargeting and ragdoll.
    ---
    ---@class HumanoidComponent
    local HumanoidComponent = {}

    --- Get the entity that is mapped to the specified humanoid bone. Use
    --- HumanoidBone table to get access to humanoid bone values.
    ---
    ---@param bone HumanoidBone
    ---
    ---@return Entity
    function HumanoidComponent.GetBoneEntity(bone) end

    --- Enable/disable automatic lookAt (for head and eyes movement).
    ---
    ---@param value boolean
    function HumanoidComponent.SetLookAtEnabled(value) end

    --- Set a target lookAt position (for head an eyes movement).
    ---
    ---@param value Vector
    function HumanoidComponent.SetLookAt(value) end

    --- Activate dynamic ragdoll physics. Note that kinematic ragdoll physics is
    --- always active (ragdoll is animation-driven/kinematic by default).
    ---
    ---@param value boolean
    function HumanoidComponent.SetRagdollPhysicsEnabled(value) end

    --- Returns whether ragdoll physics enabled.
    ---
    ---@return boolean
    function HumanoidComponent.IsRagdollPhysicsEnabled() end

    --- Completely disables ragdoll physics object creation for this humanoid.
    ---
    ---@param value boolean
    function HumanoidComponent.SetRagdollDisabled(value) end

    --- Returns whether ragdoll disabled.
    ---
    ---@return boolean
    function HumanoidComponent.IsRagdollDisabled() end

    --- Lock ragdoll to the 2D plane (XY translation, Z rotation).
    ---
    ---@param value boolean
    function HumanoidComponent.SetRagdoll2D(value) end

    --- Returns whether it's ragdoll2d.
    ---
    ---@return boolean
    function HumanoidComponent.IsRagdoll2D() end

    --- Turn off intersection test for this ragdoll. This only affects direct
    --- intersection check with Scene::Intersects().
    ---
    ---@param value boolean
    function HumanoidComponent.SetIntersectionDisabled(value) end

    --- Returns whether intersection disabled.
    ---
    ---@return boolean
    function HumanoidComponent.IsIntersectionDisabled() end

    --- Control the overall fatness of the ragdoll body parts except head
    --- (default: 1).
    ---
    ---@param value number
    function HumanoidComponent.SetRagdollFatness(value) end

    --- Control the overall size of the ragdoll head (default: 1).
    ---
    ---@param value number
    function HumanoidComponent.SetRagdollHeadSize(value) end

    --- Returns the ragdoll fatness.
    ---
    ---@return number
    function HumanoidComponent.GetRagdollFatness() end

    --- Returns the ragdoll head size.
    ---
    ---@return number
    function HumanoidComponent.GetRagdollHeadSize() end

    --- Dynamically modify arm spacing after animation (negative: pull together,
    --- positive: push apart).
    ---
    ---@param value number
    function HumanoidComponent.SetArmSpacing(value) end

    --- Returns the arm spacing.
    ---
    ---@return number
    function HumanoidComponent.GetArmSpacing() end

    --- Dynamically modify leg spacing after animation (negative: pull together,
    --- positive: push apart).
    ---
    ---@param value number
    function HumanoidComponent.SetLegSpacing(value) end

    --- Returns the leg spacing.
    ---
    ---@return number
    function HumanoidComponent.GetLegSpacing() end

    --- Standard humanoid bone identifiers used by HumanoidComponent.
    ---
    ---@enum HumanoidBone
    HumanoidBone = {
        Hips = 0,  -- included in ragdoll,
        Spine = 1,  -- included in ragdoll,
        Chest = 2,
        UpperChest = 3,
        Neck = 4,  -- included in ragdoll,
        Head = 5,  -- included in ragdoll if Neck is not available,
        LeftEye = 6,
        RightEye = 7,
        Jaw = 8,
        LeftUpperLeg = 9,  -- included in ragdoll,
        LeftLowerLeg = 10,  -- included in ragdoll,
        LeftFoot = 11,  -- included in ragdoll,
        LeftToes = 12,
        RightUpperLeg = 13,  -- included in ragdoll,
        RightLowerLeg = 14,  -- included in ragdoll,
        RightFoot = 15,  -- included in ragdoll,
        RightToes = 16,
        LeftShoulder = 17,
        LeftUpperArm = 18,  -- included in ragdoll,
        LeftLowerArm = 19,  -- included in ragdoll,
        LeftHand = 20,
        RightShoulder = 21,
        RightUpperArm = 22,  -- included in ragdoll,
        RightLowerArm = 23,  -- included in ragdoll,
        RightHand = 24,
        LeftThumbMetacarpal = 25,
        LeftThumbProximal = 26,
        LeftThumbDistal = 27,
        LeftIndexProximal = 28,
        LeftIndexIntermediate = 29,
        LeftIndexDistal = 30,
        LeftMiddleProximal = 31,
        LeftMiddleIntermediate = 32,
        LeftMiddleDistal = 33,
        LeftRingProximal = 34,
        LeftRingIntermediate = 35,
        LeftRingDistal = 36,
        LeftLittleProximal = 37,
        LeftLittleIntermediate = 38,
        LeftLittleDistal = 39,
        RightThumbMetacarpal = 40,
        RightThumbProximal = 41,
        RightThumbDistal = 42,
        RightIndexIntermediate = 43,
        RightIndexDistal = 44,
        RightIndexProximal = 45,
        RightMiddleProximal = 46,
        RightMiddleIntermediate = 47,
        RightMiddleDistal = 48,
        RightRingProximal = 49,
        RightRingIntermediate = 50,
        RightRingDistal = 51,
        RightLittleProximal = 52,
        RightLittleIntermediate = 53,
        RightLittleDistal = 54,
        Count = 55,
    }
```

## DecalComponent

```lua
    --- Creates a new, standalone DecalComponent that owns its own data.
    ---
    ---@return DecalComponent
    function DecalComponent() end

    --- The decal component is a textured sticker that can be put down onto
    --- meshes. Most of the properties can be controlled through an attached
    --- `TransformComponent` and `MaterialComponent`.
    ---
    ---@class DecalComponent
    local DecalComponent = {}

    --- Set decal to only use alpha from base color texture. Useful for blending
    --- normalmap-only decals.
    ---
    ---@param value boolean
    function DecalComponent.SetBaseColorOnlyAlpha(value) end

    --- Returns whether base color only alpha.
    ---
    ---@return boolean
    function DecalComponent.IsBaseColorOnlyAlpha() end

    --- Sets the slope blend power.
    ---
    ---@param value number
    function DecalComponent.SetSlopeBlendPower(value) end

    --- Returns the slope blend power.
    ---
    ---@return number
    function DecalComponent.GetSlopeBlendPower() end
```

## MetadataComponent

```lua
    --- Creates a new, standalone MetadataComponent that owns its own data.
    ---
    ---@return MetadataComponent
    function MetadataComponent() end

    --- The metadata component can store and retrieve an arbitrary amount of
    --- named user values for an entity. It is possible to use the same name for
    --- multiple of different value types, but one value can not have multiple
    --- entries with the same name.
    ---
    ---@class MetadataComponent
    local MetadataComponent = {}

    --- Returns whether it has bool.
    ---
    ---@param name string
    ---
    ---@return boolean
    function MetadataComponent.HasBool(name) end

    --- Returns whether it has int.
    ---
    ---@param name string
    ---
    ---@return boolean
    function MetadataComponent.HasInt(name) end

    --- Returns whether it has float.
    ---
    ---@param name string
    ---
    ---@return boolean
    function MetadataComponent.HasFloat(name) end

    --- Returns whether it has string.
    ---
    ---@param name string
    ---
    ---@return boolean
    function MetadataComponent.HasString(name) end

    --- Returns the preset.
    ---
    ---@return integer
    function MetadataComponent.GetPreset() end

    --- Returns the bool.
    ---
    ---@param name string
    ---
    ---@return boolean
    function MetadataComponent.GetBool(name) end

    --- Returns the int.
    ---
    ---@param name string
    ---
    ---@return integer
    function MetadataComponent.GetInt(name) end

    --- Returns the float.
    ---
    ---@param name string
    ---
    ---@return number
    function MetadataComponent.GetFloat(name) end

    --- Returns the string.
    ---
    ---@param name string
    ---
    ---@return any
    function MetadataComponent.GetString(name) end

    --- Sets the preset.
    ---
    ---@param preset integer
    function MetadataComponent.SetPreset(preset) end

    --- Sets the bool.
    ---
    ---@param name string
    ---@param value boolean
    function MetadataComponent.SetBool(name, value) end

    --- Sets the int.
    ---
    ---@param name string
    ---@param value integer
    function MetadataComponent.SetInt(name, value) end

    --- Sets the float.
    ---
    ---@param name string
    ---@param value number
    function MetadataComponent.SetFloat(name, value) end

    --- Sets the string.
    ---
    ---@param name string
    ---@param value string
    function MetadataComponent.SetString(name, value) end

    --- Built-in metadata categories used to tag entities (player, enemy,
    --- pickup, etc.).
    ---
    ---@enum MetadataPreset
    MetadataPreset = {
        Custom = 0,
        Waypoint = 1,
        Player = 2,
        Enemy = 3,
        NPC = 4,
        Pickup = 5,
        Vehicle = 6,
        PointOfInterest = 7,
    }
```

## CharacterComponent

```lua
    --- Creates a new, standalone CharacterComponent that owns its own data.
    ---
    ---@return CharacterComponent
    function CharacterComponent() end

    --- Implementation of basic character controller features such as movement
    --- in the scene, inverse kinematics for legs, swimming, water ripples, etc.
    --- Note that CharacterComponent is NOT using physics, but a custom
    --- character logic. The character will collide with other characters,
    --- objects that are tagged as Navmesh, and colliders that are tagged with
    --- CPU enabled.
    ---
    ---@class CharacterComponent
    local CharacterComponent = {}

    --- Enable/disable character processing (enabled by default).
    ---
    ---@param value boolean
    function CharacterComponent.SetActive(value) end

    --- Returns whether the character processing is active or not.
    ---
    ---@return boolean
    function CharacterComponent.IsActive() end

    --- Move the character in a direction continuously. The given vector doesn't
    --- need to be normalized, the length of it corresponds to the movement
    --- amount. The character will be moved the next time the scene is updated.
    --- The movement will be blocked by objects tagged as navigation mesh and
    --- CPU colliders. If this entity has a layer component, the layer will be
    --- used to ensure that the character doesn't collide with that layer.
    ---
    ---@param value Vector
    function CharacterComponent.Move(value) end

    --- Similar to Move, but relative to the facing direction.
    ---
    ---@param value Vector
    function CharacterComponent.Strafe(value) end

    --- Jump upwards by an amount. The jump will be executed in the next scene
    --- update, with collisions.
    ---
    ---@param amount number
    function CharacterComponent.Jump(amount) end

    --- Turn towards a direction continuously.
    ---
    ---@param value Vector
    function CharacterComponent.Turn(value) end

    --- Lean sideways, negative values mean left, positive values mean right.
    ---
    ---@param value number
    function CharacterComponent.Lean(value) end

    --- Apply shaking to the character. horizontal, vertical: movement amount in
    --- directions; frequency: speed of movement; decay: speed of slowing down.
    ---
    ---@param horizontal number
    ---@param vertical? number
    ---@param frequency? number
    ---@param decay? number
    function CharacterComponent.Shake(
        horizontal,
        vertical,
        frequency,
        decay
    ) end

    --- Adds animation for tracking blending state. The simple animation
    --- blending will perform blend-out for each animation except the currenttly
    --- active one.
    ---
    ---@param entity Entity
    function CharacterComponent.AddAnimation(entity) end

    --- Play the animation. This will be blended in as primary animation, others
    --- will be belnded out.
    ---
    ---@param entity Entity
    function CharacterComponent.PlayAnimation(entity) end

    --- Stops current animation.
    function CharacterComponent.StopAnimation() end

    --- Set target blend amount of current animation.
    ---
    ---@param value number
    function CharacterComponent.SetAnimationAmount(value) end

    --- Returns target blend amount of current animation.
    ---
    ---@return number
    function CharacterComponent.GetAnimationAmount() end

    --- Returns the timer of current animation.
    ---
    ---@return number
    function CharacterComponent.GetAnimationTimer() end

    --- Returns true if the current animation is ended, false otherwise.
    ---
    ---@return boolean
    function CharacterComponent.IsAnimationEnded() end

    --- Velocity multiplier when moving on ground, default: 0.92.
    ---
    ---@param value number
    function CharacterComponent.SetGroundFriction(value) end

    --- Velocity multiplier when swimming in water, default: 0.9.
    ---
    ---@param value number
    function CharacterComponent.SetWaterFriction(value) end

    --- Slope detection threshold, default: 0.2.
    ---
    ---@param value number
    function CharacterComponent.SetSlopeThreshold(value) end

    --- Leaning min/max clamping, default: 0.12.
    ---
    ---@param value number
    function CharacterComponent.SetLeaningLimit(value) end

    --- Turning smoothing speed when using Turn(), default: 10.0.
    ---
    ---@param value number
    function CharacterComponent.SetTurningSpeed(value) end

    --- Frame rate of simulation, default: 120.
    ---
    ---@param value number
    function CharacterComponent.SetFixedUpdateFPS(value) end

    --- Gravity value, default: -30.
    ---
    ---@param value number
    function CharacterComponent.SetGravity(value) end

    --- Vertical offset to keep from water. Useful if character is too submerged
    --- in the swimming state.
    ---
    ---@param value number
    function CharacterComponent.SetWaterVerticalOffset(value) end

    --- Set health of the character.
    ---
    ---@param value integer
    function CharacterComponent.SetHealth(value) end

    --- Set the horizontal size of the character capsule (same as capsule
    --- radius).
    ---
    ---@param value number
    function CharacterComponent.SetWidth(value) end

    --- Set the vertical size of the character capsule (same as capsule height).
    ---
    ---@param value number
    function CharacterComponent.SetHeight(value) end

    --- Apply an overall scale on the character.
    ---
    ---@param value number
    function CharacterComponent.SetScale(value) end

    --- Set current position immediately (teleport).
    ---
    ---@param value Vector
    function CharacterComponent.SetPosition(value) end

    --- Set current velocity immediately.
    ---
    ---@param value Vector
    function CharacterComponent.SetVelocity(value) end

    --- Set the facing direction of the character.
    ---
    ---@param value Vector
    function CharacterComponent.SetFacing(value) end

    --- Apply a relative offset (relative to facing direction).
    ---
    ---@param value Vector
    function CharacterComponent.SetRelativeOffset(value) end

    --- Enable/disable foot placement with inverse kinematics.
    ---
    ---@param value boolean
    function CharacterComponent.SetFootPlacementEnabled(value) end

    --- Set whether character collision with other characters is disabled or not
    --- for this character (default: false).
    ---
    ---@param value boolean
    function CharacterComponent.SetCharacterToCharacterCollisionDisabled(
        value
    ) end

    --- locks the character position to the 2D plane (XY translation, rotation
    --- is unlocked but it can only move sideways).
    ---
    ---@param value boolean
    function CharacterComponent.SetLocked2D(value) end

    --- Returns true if the position is locked to the 2D plane.
    ---
    ---@return boolean
    function CharacterComponent.IsLocked2D() end

    --- Get the current health.
    ---
    ---@return integer
    function CharacterComponent.GetHealth() end

    --- Get the horizontal size of the character capsule (same as capsule
    --- radius).
    ---
    ---@return number
    function CharacterComponent.GetWidth() end

    --- Get the vertical size of the character capsule (same as capsule height).
    ---
    ---@return number
    function CharacterComponent.GetHeight() end

    --- Get the overall scale of the character.
    ---
    ---@return number
    function CharacterComponent.GetScale() end

    --- Retrieve the current position without interpolation (this is the raw
    --- value from fixed timestep update)
    ---
    ---@return Vector
    function CharacterComponent.GetPosition() end

    --- Retrieve the current position with interpolation (this is the position
    --- that is rendered).
    ---
    ---@return Vector
    function CharacterComponent.GetPositionInterpolated() end

    --- Get current velocity.
    ---
    ---@return Vector
    function CharacterComponent.GetVelocity() end

    --- Get current movement direction.
    ---
    ---@return Vector
    function CharacterComponent.GetMovement() end

    --- Returns whether the character is currently standing on ground or not.
    ---
    ---@return boolean
    function CharacterComponent.IsGrounded() end

    --- Returns whether the character is currently intersecting a wall or not.
    ---
    ---@return boolean
    function CharacterComponent.IsWallIntersect() end

    --- Returns whether the character is currently swimming or not.
    ---
    ---@return boolean
    function CharacterComponent.IsSwimming() end

    --- Returns whether foot placement with inverse kinematics is currently
    --- enabled or not.
    ---
    ---@return boolean
    function CharacterComponent.IsFootPlacementEnabled() end

    --- Returns whether character collision with other characters is disabled or
    --- not for this character (default: false).
    function CharacterComponent.IsCharacterToCharacterCollisionDisabled() end

    --- Returns the capsule representing the character.
    ---
    ---@return Capsule
    function CharacterComponent.GetCapsule() end

    --- Returns the immediate facing of the character.
    ---
    ---@return Vector
    function CharacterComponent.GetFacing() end

    --- Returns the smoothed facing of the character.
    ---
    ---@return Vector
    function CharacterComponent.GetFacingSmoothed() end

    --- Returns the relative offset (relative to facing direction).
    ---
    ---@return Vector
    function CharacterComponent.GetRelativeOffset() end

    --- Returns immediate leaning amount.
    ---
    ---@return number
    function CharacterComponent.GetLeaning() end

    --- Returns smoothed leaning amount.
    ---
    ---@return number
    function CharacterComponent.GetLeaningSmoothed() end

    --- Returns vertical offset that accounts for character's position after
    --- foot placements.
    ---
    ---@return number
    function CharacterComponent.GetFootOffset() end

    --- Set the goal for path finding, it will be processed the next time the
    --- scene is updated. You can get the results by accessing the pathquery
    --- object of the character with GetPathQuery().
    ---
    ---@param goal Vector
    ---@param voxelgrid VoxelGrid
    function CharacterComponent.SetPathGoal(goal, voxelgrid) end

    --- Returns the PathQuery object of this character.
    ---
    ---@return PathQuery
    function CharacterComponent.GetPathQuery() end

    --- Sets whether a dedicated shadow is used.
    ---
    ---@param value boolean
    function CharacterComponent.SetDedicatedShadow(value) end

    --- Returns whether a dedicated shadow is used.
    ---
    ---@return boolean
    function CharacterComponent.IsDedicatedShadow() end
```
