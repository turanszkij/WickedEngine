# Wicked Engine Scripting API Documentation

![Wicked Engine logo](../logo_small.png)

This is a reference and explanation of Lua scripting features in Wicked Engine.

- [Editor Manual](WickedEditor-Manual.pdf)
- [C++ Documentation](WickedEngine-Documentation.md)

The API reference itself is organized by topic under
[`scripting_api/sections/`](scripting_api/sections); this page is the entry
point, with the table of contents below and instructions for using the docs as
editor IntelliSense.

## Contents

- [Introduction and usage](#introduction-and-usage)
  - [Reading this documentation](#reading-this-documentation)
- [Editor support (autocomplete & type checking)](#editor-support-autocomplete--type-checking)
- [Editing this documentation](#editing-this-documentation)
- [Utility Tools](scripting_api/sections/utility.md)
- Engine Bindings
  - [BackLog](scripting_api/sections/backlog.md)
  - [Renderer](scripting_api/sections/renderer.md)
    - [PaintTextureParams](scripting_api/sections/renderer.md#painttextureparams)
    - [PaintDecalParams](scripting_api/sections/renderer.md#paintdecalparams)
  - [Sprite](scripting_api/sections/sprite.md)
    - [ImageParams](scripting_api/sections/sprite.md#imageparams)
    - [SpriteAnim](scripting_api/sections/sprite.md#spriteanim)
    - [MovingTexAnim](scripting_api/sections/sprite.md#movingtexanim)
    - [DrawRectAnim](scripting_api/sections/sprite.md#drawrectanim)
  - [SpriteFont](scripting_api/sections/sprite.md#spritefont)
  - [Texture](scripting_api/sections/texture.md)
    - [texturehelper](scripting_api/sections/texture.md#texturehelper)
  - [Audio](scripting_api/sections/audio.md)
    - [Sound](scripting_api/sections/audio.md#sound)
    - [SoundInstance](scripting_api/sections/audio.md#soundinstance)
    - [SoundInstance3D](scripting_api/sections/audio.md#soundinstance3d)
    - [Submix Types](scripting_api/sections/audio.md#submix-types)
    - [Reverb Types](scripting_api/sections/audio.md#reverb-types)
  - [Video](scripting_api/sections/video.md)
    - [Video](scripting_api/sections/video.md#video-1)
    - [VideoInstance](scripting_api/sections/video.md#videoinstance)
  - [Vector](scripting_api/sections/math.md)
  - [Matrix](scripting_api/sections/math.md#matrix)
  - [Async](scripting_api/sections/async.md)
  - [Scene System (using entity-component system)](scripting_api/sections/scene.md)
    - [Entity](scripting_api/sections/scene.md#entity)
    - [Scene](scripting_api/sections/scene.md#scene)
    - [RayIntersectionResult](scripting_api/sections/scene.md#rayintersectionresult)
    - [SphereIntersectionResult](scripting_api/sections/scene.md#sphereintersectionresult)
    - [NameComponent](scripting_api/sections/scene.md#namecomponent)
    - [LayerComponent](scripting_api/sections/scene.md#layercomponent)
    - [TransformComponent](scripting_api/sections/scene.md#transformcomponent)
    - [CameraComponent](scripting_api/sections/scene.md#cameracomponent)
    - [AnimationComponent](scripting_api/sections/scene.md#animationcomponent)
    - [MaterialComponent](scripting_api/sections/scene.md#materialcomponent)
    - [MeshComponent](scripting_api/sections/scene.md#meshcomponent)
    - [EmitterComponent](scripting_api/sections/scene.md#emittercomponent)
    - [HairParticleSystem](scripting_api/sections/scene.md#hairparticlesystem)
    - [LightComponent](scripting_api/sections/scene.md#lightcomponent)
    - [ObjectComponent](scripting_api/sections/scene.md#objectcomponent)
    - [InverseKinematicsComponent](scripting_api/sections/scene.md#inversekinematicscomponent)
    - [SpringComponent](scripting_api/sections/scene.md#springcomponent)
    - [ScriptComponent](scripting_api/sections/scene.md#scriptcomponent)
    - [RigidBodyPhysicsComponent](scripting_api/sections/scene.md#rigidbodyphysicscomponent)
    - [SoftBodyPhysicsComponent](scripting_api/sections/scene.md#softbodyphysicscomponent)
    - [ForceFieldComponent](scripting_api/sections/scene.md#forcefieldcomponent)
    - [WeatherComponent](scripting_api/sections/scene.md#weathercomponent)
      - [OceanParameters](scripting_api/sections/scene.md#oceanparameters)
      - [AtmosphereParameters](scripting_api/sections/scene.md#atmosphereparameters)
      - [VolumetricCloudParameters](scripting_api/sections/scene.md#volumetriccloudparameters)
    - [SoundComponent](scripting_api/sections/scene.md#soundcomponent)
    - [VideoComponent](scripting_api/sections/scene.md#videocomponent)
    - [ColliderComponent](scripting_api/sections/scene.md#collidercomponent)
    - [ExpressionComponent](scripting_api/sections/scene.md#expressioncomponent)
    - [HumanoidComponent](scripting_api/sections/scene.md#humanoidcomponent)
    - [DecalComponent](scripting_api/sections/scene.md#decalcomponent)
    - [MetadataComponent](scripting_api/sections/scene.md#metadatacomponent)
    - [CharacterComponent](scripting_api/sections/scene.md#charactercomponent)
  - [Canvas](scripting_api/sections/canvas.md)
  - [High Level Interface](scripting_api/sections/high_level.md)
    - [Application](scripting_api/sections/high_level.md#application)
    - [RenderPath](scripting_api/sections/high_level.md#renderpath)
      - [RenderPath2D : RenderPath](scripting_api/sections/high_level.md#renderpath2d--renderpath)
      - [RenderPath3D : RenderPath2D](scripting_api/sections/high_level.md#renderpath3d--renderpath2d)
      - [LoadingScreen : RenderPath2D](scripting_api/sections/high_level.md#loadingscreen--renderpath2d)
  - [Primitives](scripting_api/sections/primitives.md)
    - [Ray](scripting_api/sections/primitives.md#ray)
    - [AABB](scripting_api/sections/primitives.md#aabb)
    - [Sphere](scripting_api/sections/primitives.md#sphere)
    - [Capsule](scripting_api/sections/primitives.md#capsule)
  - [Input](scripting_api/sections/input.md)
    - [ControllerFeedback](scripting_api/sections/input.md#controllerfeedback)
    - [Touch](scripting_api/sections/input.md#touch)
    - [TOUCHSTATE](scripting_api/sections/input.md#touchstate)
    - [Keyboard Key codes](scripting_api/sections/input.md#keyboard-key-codes)
    - [Mouse Key Codes](scripting_api/sections/input.md#mouse-key-codes)
    - [Gamepad Key Codes](scripting_api/sections/input.md#gamepad-key-codes)
      - [Generic button codes](scripting_api/sections/input.md#generic-button-codes)
      - [Xbox button codes](scripting_api/sections/input.md#xbox-button-codes)
      - [PlayStation button codes](scripting_api/sections/input.md#playstation-button-codes)
    - [Gamepad Analog Codes](scripting_api/sections/input.md#gamepad-analog-codes)
    - [Controller preference](scripting_api/sections/input.md#controller-preference)
    - [Cursor codes](scripting_api/sections/input.md#cursor-codes)
  - [Physics](scripting_api/sections/physics.md)
    - [PickDragOperation](scripting_api/sections/physics.md#pickdragoperation)
  - [Path finding](scripting_api/sections/pathfinding.md)
    - [VoxelGrid](scripting_api/sections/pathfinding.md#voxelgrid)
    - [PathQuery](scripting_api/sections/pathfinding.md#pathquery)
  - [TrailRenderer](scripting_api/sections/trailrenderer.md)
  - [Network](scripting_api/sections/network.md)

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

## Editor support (autocomplete & type checking)

Because every binding in this documentation is written as annotated Lua, it can
be turned into [Lua Language Server](https://luals.github.io/) definitions so you
get autocomplete, signatures and type checking for the whole scripting API in
your own Lua project (for example with the *sumneko.lua* VS Code extension).

### 1. Generate the definitions

Requires only Python 3 (no dependencies):

```sh
cd Content/Documentation/scripting_api
python3 generate_stubs.py
```

This produces a single `wicked_engine_bindings.lua` containing the whole API.
Prefer one file per topic? Run `python3 generate_stubs.py --split`, which writes
`library/wicked_engine/<topic>.lua` instead.

### 2. Install it into your project

Copy `wicked_engine_bindings.lua` into your project, for example into a
`library/` directory:

```text
your_project/
  .luarc.json
  library/
    wicked_engine_bindings.lua
  your_scripts.lua
```

Re-run the generator and copy again whenever the documentation changes.

### 3. Configure `.luarc.json`

Create a `.luarc.json` at the root of your project pointing the language server
at the definitions:

```json
{
  "$schema": "https://raw.githubusercontent.com/LuaLS/vscode-lua/master/setting/schema.json",
  "workspace": {
    "library": ["library/wicked_engine_bindings.lua"],
    "checkThirdParty": false
  },
  "runtime": {
    "version": "Lua 5.4"
  }
}
```

`workspace.library` is the only required entry — it makes the language server
load the definitions. With it, the whole engine scripting API is recognized: the
definitions declare the engine global objects (`vector`, `matrix`, `application`,
`main`, `audio`, `input`, `physics`, `texturehelper`) and every engine global
function, constant and enum, so you don't need to list any of those.

If your own project uses globals that aren't defined in any of your `.lua`
files, add them under `diagnostics.globals` so they aren't reported as
undefined, for example:

```json
  "diagnostics": {
    "globals": [ "my_custom_global", "MY_CONSTANT" ]
  }
```

(A global your code *assigns* in a workspace file — e.g. `scene = GetScene()` —
is already known and does not need to be listed.)

## Editing this documentation

The Markdown under `scripting_api/sections/` is the source of truth; the
generated definitions are derived from it.

- Document each binding as annotated Lua in a fenced `lua` block (see
  [Reading this documentation](#reading-this-documentation) for the
  conventions).
- A new `scripting_api/sections/<name>.md` file is picked up automatically by
  the generator.
- Mark illustrative *usage* snippets (not API definitions) with a
  `---@diagnostic disable: ...` line as the first line of the block; the
  generator skips those so they don't end up in the definitions.
- After editing, re-run `generate_stubs.py` and, ideally, validate with
  `lua-language-server --check wicked_engine_bindings.lua`.
