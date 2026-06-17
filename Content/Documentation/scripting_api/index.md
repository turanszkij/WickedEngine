# Wicked Engine Scripting API Documentation

![Wicked Engine logo](../../logo_small.png)

This is a reference and explanation of Lua scripting features in Wicked Engine.

- [Editor Manual](../WickedEditor-Manual.pdf)
- [C++ Documentation](../WickedEngine-Documentation.md)

## Contents

- [Introduction and usage](#introduction-and-usage)
- [Utility Tools](sections/utility.md)
- Engine Bindings
  - [BackLog](sections/backlog.md)
  - [Renderer](sections/renderer.md)
    - [PaintTextureParams](sections/renderer.md#painttextureparams)
    - [PaintDecalParams](sections/renderer.md#paintdecalparams)
  - [Sprite](sections/sprite.md)
    - [ImageParams](sections/sprite.md#imageparams)
    - [SpriteAnim](sections/sprite.md#spriteanim)
    - [MovingTexAnim](sections/sprite.md#movingtexanim)
    - [DrawRectAnim](sections/sprite.md#drawrectanim)
  - [SpriteFont](sections/sprite.md#spritefont)
  - [Texture](sections/texture.md)
    - [texturehelper](sections/texture.md#texturehelper)
  - [Audio](sections/audio.md)
    - [Sound](sections/audio.md#sound)
    - [SoundInstance](sections/audio.md#soundinstance)
    - [SoundInstance3D](sections/audio.md#soundinstance3d)
    - [Submix Types](sections/audio.md#submix-types)
    - [Reverb Types](sections/audio.md#reverb-types)
  - [Video](sections/video.md)
    - [Video](sections/video.md#video-1)
    - [VideoInstance](sections/video.md#videoinstance)
  - [Vector](sections/math.md)
  - [Matrix](sections/math.md#matrix)
  - [Async](sections/async.md)
  - [Scene System (using entity-component system)](sections/scene.md)
    - [Entity](sections/scene.md#entity)
    - [Scene](sections/scene.md#scene)
    - [RayIntersectionResult](sections/scene.md#rayintersectionresult)
    - [SphereIntersectionResult](sections/scene.md#sphereintersectionresult)
    - [NameComponent](sections/scene.md#namecomponent)
    - [LayerComponent](sections/scene.md#layercomponent)
    - [TransformComponent](sections/scene.md#transformcomponent)
    - [CameraComponent](sections/scene.md#cameracomponent)
    - [AnimationComponent](sections/scene.md#animationcomponent)
    - [MaterialComponent](sections/scene.md#materialcomponent)
    - [MeshComponent](sections/scene.md#meshcomponent)
    - [EmitterComponent](sections/scene.md#emittercomponent)
    - [HairParticleSystem](sections/scene.md#hairparticlesystem)
    - [LightComponent](sections/scene.md#lightcomponent)
    - [ObjectComponent](sections/scene.md#objectcomponent)
    - [InverseKinematicsComponent](sections/scene.md#inversekinematicscomponent)
    - [SpringComponent](sections/scene.md#springcomponent)
    - [ScriptComponent](sections/scene.md#scriptcomponent)
    - [RigidBodyPhysicsComponent](sections/scene.md#rigidbodyphysicscomponent)
    - [SoftBodyPhysicsComponent](sections/scene.md#softbodyphysicscomponent)
    - [ForceFieldComponent](sections/scene.md#forcefieldcomponent)
    - [WeatherComponent](sections/scene.md#weathercomponent)
      - [OceanParameters](sections/scene.md#oceanparameters)
      - [AtmosphereParameters](sections/scene.md#atmosphereparameters)
      - [VolumetricCloudParameters](sections/scene.md#volumetriccloudparameters)
    - [SoundComponent](sections/scene.md#soundcomponent)
    - [VideoComponent](sections/scene.md#videocomponent)
    - [ColliderComponent](sections/scene.md#collidercomponent)
    - [ExpressionComponent](sections/scene.md#expressioncomponent)
    - [HumanoidComponent](sections/scene.md#humanoidcomponent)
    - [DecalComponent](sections/scene.md#decalcomponent)
    - [MetadataComponent](sections/scene.md#metadatacomponent)
    - [CharacterComponent](sections/scene.md#charactercomponent)
  - [Canvas](sections/canvas.md)
  - [High Level Interface](sections/high_level.md)
    - [Application](sections/high_level.md#application)
    - [RenderPath](sections/high_level.md#renderpath)
      - [RenderPath2D : RenderPath](sections/high_level.md#renderpath2d--renderpath)
      - [RenderPath3D : RenderPath2D](sections/high_level.md#renderpath3d--renderpath2d)
      - [LoadingScreen : RenderPath2D](sections/high_level.md#loadingscreen--renderpath2d)
  - [Primitives](sections/primitives.md)
    - [Ray](sections/primitives.md#ray)
    - [AABB](sections/primitives.md#aabb)
    - [Sphere](sections/primitives.md#sphere)
    - [Capsule](sections/primitives.md#capsule)
  - [Input](sections/input.md)
    - [ControllerFeedback](sections/input.md#controllerfeedback)
    - [Touch](sections/input.md#touch)
    - [TOUCHSTATE](sections/input.md#touchstate)
    - [Keyboard Key codes](sections/input.md#keyboard-key-codes)
    - [Mouse Key Codes](sections/input.md#mouse-key-codes)
    - [Gamepad Key Codes](sections/input.md#gamepad-key-codes)
      - [Generic button codes](sections/input.md#generic-button-codes)
      - [Xbox button codes](sections/input.md#xbox-button-codes)
      - [PlayStation button codes](sections/input.md#playstation-button-codes)
    - [Gamepad Analog Codes](sections/input.md#gamepad-analog-codes)
    - [Controller preference](sections/input.md#controller-preference)
    - [Cursor codes](sections/input.md#cursor-codes)
  - [Physics](sections/physics.md)
    - [PickDragOperation](sections/physics.md#pickdragoperation)
  - [Path finding](sections/pathfinding.md)
    - [VoxelGrid](sections/pathfinding.md#voxelgrid)
    - [PathQuery](sections/pathfinding.md#pathquery)
  - [TrailRenderer](sections/trailrenderer.md)
  - [Network](sections/network.md)

## Introduction and usage

Scripting in Wicked Engine is powered by Lua, meaning that the user can make use
of the syntax and features of the flexible and powerful Lua language. Apart from
the common features, certain engine features are also available to use. You can
load lua script files and execute them, or the engine scripting console (also
known as the BackLog) can also be used to execute single line scripts (or you
can execute file scripts by the dofile command here). Upon startup, the engine
will try to load a startup script file named startup.lua in the root directory
of the application. If not found, an error message will be thrown followed by
the normal execution by the program. In the startup file, you can specify any
startup logic, for example loading content or anything.

The Backlog is mapped to the HOME button on the keyboard. This will bring down
an interface where lua commands can be entered with the keyboard. The ENTER
button will execute the command that was entered. Pressing the HOME button again
will exit the Backlog.

- Tip: You can inspect any object's functionality by calling
  getprops(YourObject) on them (where YourObject is the object which is to be
  inspected). The result will be displayed on the BackLog.

### Reading this documentation

The API in each section is described as annotated Lua inside ` ```lua ` code
blocks, using [LuaLS](https://luals.github.io/) (EmmyLua) annotations so editors
can surface types and signatures:

- A class and its methods:

```lua
    --- Vector class description.
    ---
    ---@class Vector
    ---
    ---@field X number
    local Vector = {}

    --- Returns the dot product of two vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return number
    function Vector.Dot(v1, v2) end
```

- A constructor is a global function returning the type, e.g.
  `function Vector(x, y, z, w) end`.
- Global functions and values have no class prefix (e.g. `function GetScene()`,
  `application`).
- Optional parameters are marked with `?` (e.g. `---@param layer? string`).
- Enumerations use `---@enum`; alternate call signatures use `---@overload`.
