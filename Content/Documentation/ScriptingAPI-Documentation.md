# Wicked Engine Scripting API Documentation

![Wicked Engine logo](../logo_small.png)

This is a reference and explanation of Lua scripting features in Wicked Engine.

- [Editor Manual](WickedEditor-Manual.pdf)
- [C++ Documentation](WickedEngine-Documentation.md)

The full API reference is below, organized by topic, followed by instructions
for using this documentation as editor IntelliSense.

## Contents

- [Introduction and usage](#introduction-and-usage)
  - [Reading this documentation](#reading-this-documentation)
- [Editor support (autocomplete & type checking)](#editor-support-autocomplete--type-checking)
- [Editing this documentation](#editing-this-documentation)
- [Utility Tools](#utility-tools)
- [Engine Bindings](#engine-bindings)
  - [BackLog](#backlog)
  - [Renderer](#renderer)
    - [PaintTextureParams](#painttextureparams)
    - [PaintDecalParams](#paintdecalparams)
  - [Sprite](#sprites-and-fonts)
    - [ImageParams](#imageparams)
    - [SpriteAnim](#spriteanim)
    - [MovingTexAnim](#movingtexanim)
    - [DrawRectAnim](#drawrectanim)
  - [SpriteFont](#spritefont)
  - [Texture](#texture)
    - [texturehelper](#texturehelper)
  - [Audio](#audio)
    - [Sound](#sound)
    - [SoundInstance](#soundinstance)
    - [SoundInstance3D](#soundinstance3d)
    - [Submix Types](#submix-types)
    - [Reverb Types](#reverb-types)
  - [Video](#video)
    - [Video](#video-1)
    - [VideoInstance](#videoinstance)
  - [Vector](#math-types)
  - [Matrix](#matrix)
  - [Async](#async)
  - [Scene System (using entity-component system)](#scene-system-using-entity-component-system)
    - [Entity](#entity)
    - [Scene](#scene)
    - [RayIntersectionResult](#rayintersectionresult)
    - [SphereIntersectionResult](#sphereintersectionresult)
    - [NameComponent](#namecomponent)
    - [LayerComponent](#layercomponent)
    - [TransformComponent](#transformcomponent)
    - [CameraComponent](#cameracomponent)
    - [AnimationComponent](#animationcomponent)
    - [MaterialComponent](#materialcomponent)
    - [MeshComponent](#meshcomponent)
    - [EmitterComponent](#emittercomponent)
    - [HairParticleSystem](#hairparticlesystem)
    - [LightComponent](#lightcomponent)
    - [ObjectComponent](#objectcomponent)
    - [InverseKinematicsComponent](#inversekinematicscomponent)
    - [SpringComponent](#springcomponent)
    - [ScriptComponent](#scriptcomponent)
    - [RigidBodyPhysicsComponent](#rigidbodyphysicscomponent)
    - [SoftBodyPhysicsComponent](#softbodyphysicscomponent)
    - [ForceFieldComponent](#forcefieldcomponent)
    - [WeatherComponent](#weathercomponent)
      - [OceanParameters](#oceanparameters)
      - [AtmosphereParameters](#atmosphereparameters)
      - [VolumetricCloudParameters](#volumetriccloudparameters)
    - [SoundComponent](#soundcomponent)
    - [VideoComponent](#videocomponent)
    - [ColliderComponent](#collidercomponent)
    - [ExpressionComponent](#expressioncomponent)
    - [HumanoidComponent](#humanoidcomponent)
    - [DecalComponent](#decalcomponent)
    - [MetadataComponent](#metadatacomponent)
    - [CharacterComponent](#charactercomponent)
  - [Canvas](#canvas)
  - [High Level Interface](#high-level-interface)
    - [Application](#application)
    - [RenderPath](#renderpath)
      - [RenderPath2D : RenderPath](#renderpath2d--renderpath)
      - [RenderPath3D : RenderPath2D](#renderpath3d--renderpath2d)
      - [LoadingScreen : RenderPath2D](#loadingscreen--renderpath2d)
  - [Primitives](#primitives)
    - [Ray](#ray)
    - [AABB](#aabb)
    - [Sphere](#sphere)
    - [Capsule](#capsule)
  - [Input](#input)
    - [ControllerFeedback](#controllerfeedback)
    - [Touch](#touch)
    - [TOUCHSTATE](#touchstate)
    - [Keyboard Key codes](#keyboard-key-codes)
    - [Mouse Key Codes](#mouse-key-codes)
    - [Gamepad Key Codes](#gamepad-key-codes)
      - [Generic button codes](#generic-button-codes)
      - [Xbox button codes](#xbox-button-codes)
      - [PlayStation button codes](#playstation-button-codes)
    - [Gamepad Analog Codes](#gamepad-analog-codes)
    - [Controller preference](#controller-preference)
    - [Cursor codes](#cursor-codes)
  - [Physics](#physics)
    - [PickDragOperation](#pickdragoperation)
  - [Path finding](#path-finding)
    - [VoxelGrid](#voxelgrid)
    - [PathQuery](#pathquery)
  - [TrailRenderer](#trailrenderer)
  - [Network](#network)

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

This Markdown documentation is the source of truth; the generated definitions
are derived from it.

- Document each binding as annotated Lua in a fenced `lua` block (see
  [Reading this documentation](#reading-this-documentation) for the
  conventions).
- Place each binding under the appropriate heading; the generator extracts
  every `lua` block in document order.
- Mark illustrative *usage* snippets (not API definitions) with a
  `---@diagnostic disable: ...` line as the first line of the block; the
  generator skips those so they don't end up in the definitions.
- After editing, re-run `generate_stubs.py` and, ideally, validate with
  `lua-language-server --check wicked_engine_bindings.lua`.

## Utility Tools

This section describes the common tools for scripting which are not necessarily engine features.

These are some helpers to aid in debugging:

Helpers to determine the current platform, if need to implement specific
functionality:

```lua
    --- Send a signal globally. This can wake up processes if there are any
    --- waiting on the same signal name.
    ---
    ---@param name string
    function signal(name) end

    --- Wait until a specified signal arrives.
    ---
    ---@param name string
    function waitSignal(name) end

    --- Start a new process.
    ---
    ---@param func fun()
    ---
    ---@return boolean success
    ---@return thread co
    function runProcess(func) end

    --- Stop and remove all processes.
    function killProcesses() end

    --- Stop and remove a single process, identified by its coroutine.
    ---
    ---@param co thread
    function killProcess(co) end

    --- Stop and remove all processes started from the given script PID.
    ---
    ---@param pid integer
    function killProcessPID(pid) end

    --- Stop and remove all processes started from the given script file.
    ---
    ---@param file string
    function killProcessFile(file) end

    --- Wait until some time has passed (to be used from inside a process).
    ---
    ---@param seconds number
    function waitSeconds(seconds) end

    --- Get reflection data from object.
    ---
    ---@param object table
    function getprops(object) end

    --- Get the length of a table.
    ---
    ---@param object table
    ---
    ---@return integer
    function len(object) end

    --- Post table contents to the backlog.
    ---
    ---@param list table
    function backlog_post_list(list) end

    --- Modifies the logging level.
    ---
    ---@param level LogLevel
    function backlog_setlevel(level) end

    --- Severity levels for backlog/console log messages.
    ---
    ---@enum LogLevel
    LogLevel = {
        None = 0,
        Default = 1,
        Warning = 2,
        Error = 3,
        Success = 4,
    }

    --- Wait for a fixed update step to be called (to be used from inside a
    --- process).
    function fixedupdate() end

    --- Wait for variable update step to be called (to be used from inside a
    --- process).
    function update() end

    --- Wait for a render step to be called (to be used from inside a process).
    function render() end

    --- Returns the delta time in seconds (time passed since previous update()).
    ---
    ---@return number
    function getDeltaTime() end

    --- Linear interpolation.
    ---
    ---@param a number
    ---@param b number
    ---@param t number
    ---
    ---@return number
    function math.lerp(a, b, t) end

    --- Inverse linear interpolation.
    ---
    ---@param a number
    ---@param b number
    ---@param t number
    ---
    ---@return number
    function math.inverse_lerp(a, b, t) end

    --- Inverse linear interpolation.
    ---
    ---@param a number
    ---@param b number
    ---@param t number
    ---
    ---@return number
    function math.inverselerp(a, b, t) end

    --- Clamp x between min and max.
    ---
    ---@param x number
    ---@param min number
    ---@param max number
    ---
    ---@return number
    function math.clamp(x, min, max) end

    --- Clamp x between 0 and 1.
    ---
    ---@param x number
    ---
    ---@return number
    function math.saturate(x) end

    --- Round x to nearest integer.
    ---
    ---@param x number
    ---
    ---@return number
    function math.round(x) end

    --- Executes a binary LUA file.
    ---
    ---@param filename string
    function dobinaryfile(filename) end

    --- Compiles a text LUA file (filename_src) into a binary LUA file
    --- (filename_dst).
    ---
    ---@param filename_src string
    ---@param dilename_dst string
    function compilebinaryfile(filename_src, dilename_dst) end

    --- Returns true if current application is the editor, false otherwise.
    ---
    ---@return boolean
    function IsThisEditor() end

    --- Returns control to the editor and kills running scripts.
    function ReturnToEditor() end

    --- Returns true if this is a debug build, false otherwise.
    ---
    ---@return boolean
    function IsThisDebugBuild() end

    --- Returns whether platform windows.
    ---
    ---@return boolean
    function IsPlatformWindows() end

    --- Returns whether platform linux.
    ---
    ---@return boolean
    function IsPlatformLinux() end

    --- Returns whether platform macos.
    ---
    ---@return boolean
    function IsPlatformMACOS() end

    --- Returns whether platform ios.
    ---
    ---@return boolean
    function IsPlatformIOS() end

    --- Returns whether platform ps5.
    ---
    ---@return boolean
    function IsPlatformPS5() end

    --- Returns whether platform xbox.
    ---
    ---@return boolean
    function IsPlatformXBOX() end

    --- Returns whether a file exists at the given path.
    ---
    ---@param name string
    ---
    ---@return boolean
    function FileExists(name) end

    --- Returns whether a directory exists at the given path.
    ---
    ---@param name string
    ---
    ---@return boolean
    function DirectoryExists(name) end

    --- Creates a directory at the given path (including parent directories).
    ---
    ---@param name string
    function DirectoryCreate(name) end

    --- Returns a path that files can be safely saved to on every platform. This
    --- is the base directory for such a path; files should be saved into an
    --- application-specific named folder within it.
    ---
    ---@return string
    function GetSaveDataPath() end

    --- Returns the engine major version number.
    ---
    ---@return integer
    function GetVersionMajor() end

    --- Returns the engine minor version number.
    ---
    ---@return integer
    function GetVersionMinor() end

    --- Returns the engine revision version number.
    ---
    ---@return integer
    function GetVersionRevision() end

    --- Returns the full engine version string.
    ---
    ---@return string
    function GetVersionString() end

    --- Returns the engine credits text.
    ---
    ---@return string
    function GetCreditsString() end

    --- Returns the engine supporters text.
    ---
    ---@return string
    function GetSupportersString() end

    --- Returns the file path of the currently running script
    --- (empty string if not available).
    ---
    ---@return string
    function script_file() end

    --- Returns the directory of the currently running script
    --- (empty string if not available).
    ---
    ---@return string
    function script_dir() end

    --- Returns the process id (PID) of the currently running script
    --- (0 if not available).
    ---
    ---@return integer
    function script_pid() end
```

## Engine Bindings

The scripting API provides functions for the developer to manipulate engine behaviour or query it for information.

### BackLog

The BackLog is the engine's scripting console: type text with the keyboard and
run it with the RETURN key; script errors are also displayed here. These
functions are in the global scope and manipulate the BackLog.

```lua
    --- Removes all entries from the backlog.
    function backlog_clear() end

    --- Posts a string to the backlog.
    ---
    ---@param params string
    function backlog_post(params) end

    --- Sets the font size of the backlog.
    ---
    ---@param size integer
    function backlog_fontsize(size) end

    --- Returns whether the backlog is currently active (visible).
    ---
    ---@return boolean
    function backlog_isactive() end

    --- Sets the row spacing of the backlog text.
    ---
    ---@param spacing integer
    function backlog_fontrowspacing(spacing) end

    --- Opens (shows) the backlog console.
    function backlog_open() end

    --- Disables and locks the backlog so the HOME key won't bring it up.
    function backlog_lock() end

    --- Unlocks the backlog so it can be toggled with the HOME key.
    function backlog_unlock() end

    --- Disables Lua code execution from the backlog.
    function backlog_blocklua() end

    --- Re-enables Lua code execution from the backlog.
    function backlog_unblocklua() end

    --- Flushes pending backlog messages immediately.
    function backlog_flush() end

    --- Sets the interval (in milliseconds) at which the backlog auto-flushes
    --- pending messages.
    ---
    ---@param milliseconds integer
    function backlog_setautoflushinterval(milliseconds) end
```

### Renderer

The renderer manages rendering, the scene graph and debug drawing. All of the
functions below are in the global scope (there is no `Renderer` object).

```lua
    --- Returns the current game speed multiplier.
    ---
    ---@return number
    function GetGameSpeed() end

    --- Sets the game speed multiplier (1 = normal speed).
    ---
    ---@param speed number
    function SetGameSpeed(speed) end

    --- Deprecated: gamma is no longer supported.
    ---
    ---@deprecated
    ---
    ---@param value number
    function SetGamma(value) end

    --- Deprecated: resolution is now handled by window events.
    ---
    ---@deprecated
    function SetResolution() end

    --- Enables or disables vertical synchronization.
    ---
    ---@param enabled boolean
    function SetVSyncEnabled(enabled) end

    --- Returns whether the graphics device supports hardware ray tracing.
    ---
    ---@return boolean
    function IsRaytracingSupported() end

    --- Reloads all shaders.
    function ReloadShaders() end

    --- Clears the scene and the associated renderer resources. Clears the
    --- global scene if no scene is given.
    ---
    ---@param scene? Scene
    function ClearWorld(scene) end

    --- Deprecated: use application.GetCanvas().GetLogicalWidth() instead.
    ---
    ---@deprecated
    ---
    ---@return number
    function GetScreenWidth() end

    --- Deprecated: use application.GetCanvas().GetLogicalHeight() instead.
    ---
    ---@deprecated
    ---
    ---@return number
    function GetScreenHeight() end

    --- Sets the resolution of the 2D (spot and point light) shadow maps.
    ---
    ---@param resolution integer
    function SetShadowProps2D(resolution) end

    --- Sets the resolution of the cubemap (point light) shadow maps.
    ---
    ---@param resolution integer
    function SetShadowPropsCube(resolution) end

    --- Enables or disables the per-object shadow level-of-detail override.
    ---
    ---@param value boolean
    function SetShadowLODOverrideEnabled(value) end

    --- Enables or disables occlusion culling.
    ---
    ---@param enabled boolean
    function SetOcclusionCullingEnabled(enabled) end

    --- Enables or disables meshlet occlusion culling.
    ---
    ---@param value boolean
    function SetMeshletOcclusionCullingEnabled(value) end

    --- Allows or disallows the use of mesh shaders (if supported).
    ---
    ---@param enabled boolean
    function SetMeshShaderAllowed(enabled) end

    --- Enables or disables temporal anti-aliasing (TAA).
    ---
    ---@param value boolean
    function SetTemporalAAEnabled(value) end

    --- Enables or disables ray-traced shadows (requires hardware ray tracing).
    ---
    ---@param value boolean
    function SetRaytracedShadowsEnabled(value) end

    --- Enables or disables DDGI (dynamic diffuse global illumination).
    ---
    ---@param value boolean
    function SetDDGIEnabled(value) end

    --- Enables or disables capsule shadows.
    ---
    ---@param enabled boolean
    function SetCapsuleShadowEnabled(enabled) end

    --- Sets the capsule shadow fade-out distance.
    ---
    ---@param value number
    function SetCapsuleShadowFade(value) end

    --- Sets the capsule shadow cone angle, in radians.
    ---
    ---@param value number
    function SetCapsuleShadowAngle(value) end

    --- Enables or disables all debug drawing by the renderer.
    ---
    ---@param value boolean
    function SetDebugDrawEnabled(value) end

    --- Enables or disables drawing of object bounding boxes.
    ---
    ---@param enabled boolean
    function SetDebugBoxesEnabled(enabled) end

    --- Enables or disables drawing of the spatial partition tree.
    ---
    ---@param enabled boolean
    function SetDebugPartitionTreeEnabled(enabled) end

    --- Enables or disables drawing of armature bones.
    ---
    ---@param enabled boolean
    function SetDebugBonesEnabled(enabled) end

    --- Enables or disables drawing of particle emitters.
    ---
    ---@param enabled boolean
    function SetDebugEmittersEnabled(enabled) end

    --- Enables or disables drawing of environment probes.
    ---
    ---@param enabled boolean
    function SetDebugEnvProbesEnabled(enabled) end

    --- Enables or disables drawing of force fields.
    ---
    ---@param enabled boolean
    function SetDebugForceFieldsEnabled(enabled) end

    --- Enables or disables drawing of cameras.
    ---
    ---@param value boolean
    function SetDebugCamerasEnabled(value) end

    --- Enables or disables drawing of colliders.
    ---
    ---@param value boolean
    function SetDebugCollidersEnabled(value) end

    --- Enables or disables the light-culling debug visualization.
    ---
    ---@param enabled boolean
    function SetDebugLightCulling(enabled) end

    --- Enables or disables the grid helper overlay.
    ---
    ---@param value boolean
    function SetGridHelperEnabled(value) end

    --- Enables or disables the DDGI debug visualization.
    ---
    ---@param value boolean
    function SetDDGIDebugEnabled(value) end

    --- Queues a debug line from origin to end_, drawn for the current frame.
    ---
    ---@param origin Vector
    ---@param end_ Vector
    ---@param color? Vector
    ---@param depth? boolean
    function DrawLine(origin, end_, color, depth) end

    --- Queues a debug point, drawn for the current frame.
    ---
    ---@param origin Vector
    ---@param size? number
    ---@param color? Vector
    ---@param depth? boolean
    function DrawPoint(origin, size, color, depth) end

    --- Queues a debug wireframe box transformed by boxMatrix, drawn for the
    --- current frame.
    ---
    ---@param boxMatrix Matrix
    ---@param color? Vector
    ---@param depth? boolean
    function DrawBox(boxMatrix, color, depth) end

    --- Queues a debug wireframe sphere, drawn for the current frame.
    ---
    ---@param sphere Sphere
    ---@param color? Vector
    ---@param depth? boolean
    function DrawSphere(sphere, color, depth) end

    --- Queues a debug wireframe capsule, drawn for the current frame.
    ---
    ---@param capsule Capsule
    ---@param color? Vector
    ---@param depth? boolean
    function DrawCapsule(capsule, color, depth) end

    --- Queues debug text at a world position, drawn for the current frame.
    --- flags is a combination of DEBUG_TEXT_* values.
    ---
    ---@param text string
    ---@param position? Vector
    ---@param color? Vector
    ---@param scaling? number
    ---@param flags? integer
    function DrawDebugText(text, position, color, scaling, flags) end

    --- Debug text can be occluded by scene geometry (depth tested).
    ---
    ---@type integer
    DEBUG_TEXT_DEPTH_TEST = 1

    --- Debug text is rotated to always face the camera.
    ---
    ---@type integer
    DEBUG_TEXT_CAMERA_FACING = 2

    --- Debug text keeps a constant screen size regardless of distance.
    ---
    ---@type integer
    DEBUG_TEXT_CAMERA_SCALING = 4

    --- Draws the voxel grid in the debug rendering phase. The VoxelGrid object
    --- must not be destroyed until then.
    ---
    ---@param voxelgrid VoxelGrid
    function DrawVoxelGrid(voxelgrid) end

    --- Draws the path query in the debug rendering phase. The PathQuery object
    --- must not be destroyed until then.
    ---
    ---@param pathquery PathQuery
    function DrawPathQuery(pathquery) end

    --- Draws the trail in the debug rendering phase. The TrailRenderer object
    --- must not be destroyed until then.
    ---
    ---@param trail TrailRenderer
    function DrawTrail(trail) end

    --- Paints a brush stroke into a texture using the given PaintTextureParams.
    ---
    ---@param params PaintTextureParams
    function PaintIntoTexture(params) end

    --- Creates a texture that can be used as the destination of
    --- PaintIntoTexture.
    ---
    ---@param width integer
    ---@param height integer
    ---@param mips? integer
    ---@param initialColor? Vector
    ---
    ---@return Texture
    function CreatePaintableTexture(width, height, mips, initialColor) end

    --- Paints into a texture using an object's UV mapping, projecting a texture
    --- by a decal matrix (a way to bake skinned decals at runtime).
    ---
    ---@param params PaintDecalParams
    function PaintDecalIntoObjectSpaceTexture(params) end

    --- Puts down a water ripple using the default embedded asset.
    ---
    ---@param position Vector
    ---
    ---@overload fun(imagename: string, position: Vector)
    function PutWaterRipple(position) end
```

#### PaintTextureParams

```lua
    --- Creates a PaintTextureParams object.
    ---
    ---@return PaintTextureParams
    function PaintTextureParams() end

    --- Parameters describing a brush stroke for PaintIntoTexture.
    ---
    ---@class PaintTextureParams
    local PaintTextureParams = {}

    --- Sets the texture that is edited (painted into).
    ---
    ---@param tex Texture
    function PaintTextureParams.SetEditTexture(tex) end

    --- Sets the brush texture (the stamp shape/pattern).
    ---
    ---@param tex Texture
    function PaintTextureParams.SetBrushTexture(tex) end

    --- Sets the reveal texture (used for reveal-style painting).
    ---
    ---@param tex Texture
    function PaintTextureParams.SetRevealTexture(tex) end

    --- Sets the brush color.
    ---
    ---@param value Vector
    function PaintTextureParams.SetBrushColor(value) end

    --- Sets the brush center, in pixels.
    ---
    ---@param value Vector
    function PaintTextureParams.SetCenterPixel(value) end

    --- Sets the brush radius, in pixels.
    ---
    ---@param value integer
    function PaintTextureParams.SetBrushRadius(value) end

    --- Sets the brush strength (how much each stroke applies).
    ---
    ---@param value number
    function PaintTextureParams.SetBrushAmount(value) end

    --- Sets the brush edge smoothness.
    ---
    ---@param value number
    function PaintTextureParams.SetBrushSmoothness(value) end

    --- Sets the brush rotation, in radians.
    ---
    ---@param value number
    function PaintTextureParams.SetBrushRotation(value) end

    --- Sets the brush shape.
    ---
    ---@param value integer
    function PaintTextureParams.SetBrushShape(value) end
```

#### PaintDecalParams

```lua
    --- Creates a PaintDecalParams object.
    ---
    ---@return PaintDecalParams
    function PaintDecalParams() end

    --- Parameters describing a decal to bake into an object's texture with
    --- PaintDecalIntoObjectSpaceTexture.
    ---
    ---@class PaintDecalParams
    local PaintDecalParams = {}

    --- Sets the source texture (the decal image).
    ---
    ---@param tex Texture
    function PaintDecalParams.SetInTexture(tex) end

    --- Sets the destination texture (the object's texture).
    ---
    ---@param tex Texture
    function PaintDecalParams.SetOutTexture(tex) end

    --- Sets the decal matrix in world space.
    ---
    ---@param mat Matrix
    function PaintDecalParams.SetMatrix(mat) end

    --- Sets the target object entity; positions and UVs are taken from it.
    ---
    ---@param entity Entity
    function PaintDecalParams.SetObject(entity) end

    --- Adjusts decal fading based on surface slope relative to the decal
    --- projection (default 0: no slope blend).
    ---
    ---@param power number
    function PaintDecalParams.SetSlopeBlendPower(power) end
```

### Sprites and Fonts

#### Sprite

```lua
    --- Constructs a sprite, optionally from a texture and a mask texture file.
    ---
    ---@param texture? string
    ---@param maskTexture? string
    ---
    ---@return Sprite
    function Sprite(texture, maskTexture) end

    --- Render images on the screen.
    ---
    ---@class Sprite
    ---
    ---@field Params ImageParams
    ---@field Anim SpriteAnim
    local Sprite = {}

    --- Sets the rendering parameters (position, size, color, …) for the sprite.
    ---
    ---@param effects ImageParams
    function Sprite.SetParams(effects) end

    --- Returns the sprite's rendering parameters.
    ---
    ---@return ImageParams
    function Sprite.GetParams() end

    --- Sets the animation helper that drives this sprite.
    ---
    ---@param anim SpriteAnim
    function Sprite.SetAnim(anim) end

    --- Returns the sprite's animation helper.
    ---
    ---@return SpriteAnim
    function Sprite.GetAnim() end

    --- Sets the base color texture.
    ---
    ---@param texture Texture
    function Sprite.SetTexture(texture) end

    --- Returns the base color texture.
    ---
    ---@return Texture
    function Sprite.GetTexture() end

    --- Sets the mask texture (multiplied with the base color).
    ---
    ---@param texture Texture
    function Sprite.SetMaskTexture(texture) end

    --- Returns the mask texture.
    ---
    ---@return Texture
    function Sprite.GetMaskTexture() end

    --- Sets the background texture (used by the background effect).
    ---
    ---@param texture Texture
    function Sprite.SetBackgroundTexture(texture) end

    --- Returns the background texture.
    ---
    ---@return Texture
    function Sprite.GetBackgroundTexture() end

    --- Shows or hides the sprite.
    ---
    ---@param value boolean
    function Sprite.SetHidden(value) end

    --- Returns whether the sprite is hidden.
    ---
    ---@return boolean
    function Sprite.IsHidden() end
```

##### ImageParams

Specify Sprite properties, like position, size, color and effects.

```lua
    --- Constructs image parameters. Called with two numbers they are the width
    --- and height (at position 0, 0); called with three or four they are posX,
    --- posY, width and height. Defaults: pos (0, 0), size (100, 100).
    ---
    ---@param posX number
    ---@param posY number
    ---@param width number
    ---@param height? number
    ---
    ---@return ImageParams
    ---
    ---@overload fun(width: number, height?: number): ImageParams
    function ImageParams(posX, posY, width, height) end

    ---@class ImageParams
    ---@field Pos Vector
    ---@field Size Vector
    ---@field Pivot Vector
    ---@field Color Vector
    ---@field Opacity number
    ---@field Fade number
    ---@field Rotation number
    ---@field TexOffset Vector
    ---@field TexOffset2 Vector
    local ImageParams = {}

    --- Returns the position.
    ---
    ---@return Vector
    function ImageParams.GetPos() end

    --- Returns the size.
    ---
    ---@return Vector
    function ImageParams.GetSize() end

    --- Returns the pivot point (rotation/scaling origin, in [0, 1] of the
    --- size).
    ---
    ---@return Vector
    function ImageParams.GetPivot() end

    --- Returns the color/tint (RGBA).
    ---
    ---@return Vector
    function ImageParams.GetColor() end

    --- Returns the opacity (alpha multiplier).
    ---
    ---@return number
    function ImageParams.GetOpacity() end

    --- Returns the saturation.
    ---
    ---@return number
    function ImageParams.GetSaturation() end

    --- Returns the fade amount (0: visible, 1: faded out).
    ---
    ---@return number
    function ImageParams.GetFade() end

    --- Returns the rotation in radians.
    ---
    ---@return number
    function ImageParams.GetRotation() end

    --- Returns the texture UV offset.
    ---
    ---@return Vector
    function ImageParams.GetTexOffset() end

    --- Returns the secondary texture UV offset.
    ---
    ---@return Vector
    function ImageParams.GetTexOffset2() end

    --- Returns the border soften amount.
    ---
    ---@return number
    function ImageParams.GetBorderSoften() end

    --- Returns the draw rectangle (x, y, width, height) in texture pixels.
    ---
    ---@return Vector
    function ImageParams.GetDrawRect() end

    --- Returns the secondary draw rectangle (x, y, width, height).
    ---
    ---@return Vector
    function ImageParams.GetDrawRect2() end

    --- Returns whether the draw rectangle is enabled.
    ---
    ---@return boolean
    function ImageParams.IsDrawRectEnabled() end

    --- Returns whether the secondary draw rectangle is enabled.
    ---
    ---@return boolean
    function ImageParams.IsDrawRect2Enabled() end

    --- Returns whether horizontal mirroring is enabled.
    ---
    ---@return boolean
    function ImageParams.IsMirrorEnabled() end

    --- Returns whether the background blur is enabled.
    ---
    ---@return boolean
    function ImageParams.IsBackgroundBlurEnabled() end

    --- Returns whether the background is enabled.
    ---
    ---@return boolean
    function ImageParams.IsBackgroundEnabled() end

    --- Returns whether the distortion mask is enabled.
    ---
    ---@return boolean
    function ImageParams.IsDistortionMaskEnabled() end

    --- Sets the position.
    ---
    ---@param pos Vector
    function ImageParams.SetPos(pos) end

    --- Sets the size.
    ---
    ---@param size Vector
    function ImageParams.SetSize(size) end

    --- Sets the pivot point (rotation/scaling origin, in [0, 1] of the size).
    ---
    ---@param value Vector
    function ImageParams.SetPivot(value) end

    --- Sets the color/tint (RGBA).
    ---
    ---@param value Vector
    function ImageParams.SetColor(value) end

    --- Sets the opacity (alpha multiplier).
    ---
    ---@param opacity number
    function ImageParams.SetOpacity(opacity) end

    --- Sets the saturation.
    ---
    ---@param saturation number
    function ImageParams.SetSaturation(saturation) end

    --- Sets the fade amount (0: visible, 1: faded out).
    ---
    ---@param fade number
    function ImageParams.SetFade(fade) end

    --- Sets the stencil test mode and reference value.
    ---
    ---@param stencilMode integer one of the STENCILMODE_* constants
    ---@param stencilRef integer
    function ImageParams.SetStencil(stencilMode, stencilRef) end

    --- Sets how the stencil reference value is interpreted.
    ---
    ---@param stencilRefMode integer one of the STENCILREFMODE_* constants
    function ImageParams.SetStencilRefMode(stencilRefMode) end

    --- Sets the blend mode (one of the BLENDMODE_* constants).
    ---
    ---@param blendMode integer
    function ImageParams.SetBlendMode(blendMode) end

    --- Sets the sampling quality (one of the QUALITY_* constants).
    ---
    ---@param quality integer
    function ImageParams.SetQuality(quality) end

    --- Sets the texture addressing mode (one of the SAMPLEMODE_* constants).
    ---
    ---@param sampleMode integer
    function ImageParams.SetSampleMode(sampleMode) end

    --- Sets the rotation in radians.
    ---
    ---@param rotation number
    function ImageParams.SetRotation(rotation) end

    --- Sets the texture UV offset.
    ---
    ---@param value Vector
    function ImageParams.SetTexOffset(value) end

    --- Sets the secondary texture UV offset.
    ---
    ---@param value Vector
    function ImageParams.SetTexOffset2(value) end

    --- Sets the border soften amount.
    ---
    ---@param alpha number
    function ImageParams.SetBorderSoften(alpha) end

    --- Enables the draw rectangle (x, y, width, height) in texture pixels.
    ---
    ---@param value Vector
    function ImageParams.EnableDrawRect(value) end

    --- Enables the secondary draw rectangle (x, y, width, height).
    ---
    ---@param value Vector
    function ImageParams.EnableDrawRect2(value) end

    --- Disables the draw rectangle.
    function ImageParams.DisableDrawRect() end

    --- Disables the secondary draw rectangle.
    function ImageParams.DisableDrawRect2() end

    --- Enables horizontal mirroring.
    function ImageParams.EnableMirror() end

    --- Disables horizontal mirroring.
    function ImageParams.DisableMirror() end

    --- Enables the background blur effect.
    function ImageParams.EnableBackgroundBlur() end

    --- Disables the background blur effect.
    function ImageParams.DisableBackgroundBlur() end

    --- Enables drawing the background.
    function ImageParams.EnableBackground() end

    --- Disables drawing the background.
    function ImageParams.DisableBackground() end

    --- Enables using the mask texture as a distortion mask.
    function ImageParams.EnableDistortionMask() end

    --- Disables the distortion mask.
    function ImageParams.DisableDistortionMask() end

    --- Sets the alpha range used when masking (start and end thresholds).
    ---
    ---@param start number
    ---@param end_ number
    function ImageParams.SetMaskAlphaRange(start, end_) end

    --- Returns the mask alpha range as start, end.
    ---
    ---@return number range_start
    ---@return number range_end
    function ImageParams.GetMaskAlphaRange() end

    --- Sets the direction of the angular softness effect.
    ---
    ---@param value Vector
    function ImageParams.SetAngularSoftnessDirection(value) end

    --- Sets the inner angle of the angular softness effect.
    ---
    ---@param value number
    function ImageParams.SetAngularSoftnessInnerAngle(value) end

    --- Sets the outer angle of the angular softness effect.
    ---
    ---@param value number
    function ImageParams.SetAngularSoftnessOuterAngle(value) end

    --- Enables double-sided angular softness.
    function ImageParams.EnableAngularSoftnessDoubleSided() end

    --- Inverts the angular softness effect.
    function ImageParams.EnableAngularSoftnessInverse() end

    --- Disables double-sided angular softness.
    function ImageParams.DisableAngularSoftnessDoubleSided() end

    --- Disables inverted angular softness.
    function ImageParams.DisableAngularSoftnessInverse() end

    --- Enables rounded corners.
    function ImageParams.EnableCornerRounding() end

    --- Disables rounded corners.
    function ImageParams.DisableCornerRounding() end

    --- Sets the rounding of one corner (0-3) with a radius and segment count.
    ---
    ---@param corner integer corner index in [0, 3]
    ---@param rounding number corner radius
    ---@param segments? integer subdivision count (default 18)
    function ImageParams.SetCornerRounding(corner, rounding, segments) end

    --- Fills the image with a gradient between two UV coordinates.
    ---
    ---@param type ImageGradientType
    ---@param uv_start Vector
    ---@param uv_end Vector
    ---@param color Vector
    function ImageParams.SetGradient(type, uv_start, uv_end, color) end
```

The following constants and enum are used by the ImageParams setters:

```lua
    -- Stencil comparison modes (use with SetStencil).
    STENCILMODE_DISABLED = 0
    STENCILMODE_EQUAL = 1
    STENCILMODE_LESS = 2
    STENCILMODE_LESSEQUAL = 3
    STENCILMODE_GREATER = 4
    STENCILMODE_GREATEREQUAL = 5
    STENCILMODE_NOT = 6
    STENCILMODE_ALWAYS = 7

    -- How the stencil reference value is interpreted (use with
    -- SetStencilRefMode).
    STENCILREFMODE_ENGINE = 0
    STENCILREFMODE_USER = 1
    STENCILREFMODE_ALL = 2

    -- Texture addressing modes (use with SetSampleMode).
    SAMPLEMODE_CLAMP = 0
    SAMPLEMODE_WRAP = 1
    SAMPLEMODE_MIRROR = 2

    -- Sampling quality (use with SetQuality).
    QUALITY_NEAREST = 0
    QUALITY_LINEAR = 1
    QUALITY_ANISOTROPIC = 2
    QUALITY_BICUBIC = 3

    -- Blend modes (use with SetBlendMode).
    BLENDMODE_OPAQUE = 0
    BLENDMODE_ALPHA = 1
    BLENDMODE_PREMULTIPLIED = 2
    BLENDMODE_ADDITIVE = 3

    ---@enum ImageGradientType
    ImageGradientType = {
        None = 0,
        Linear = 1,
        LinearReflected = 2,
        Circular = 3,
    }
```

##### SpriteAnim

```lua
    --- Constructs a sprite animation helper.
    ---
    ---@return SpriteAnim
    function SpriteAnim() end

    --- Animate Sprites easily with this helper.
    ---
    ---@class SpriteAnim
    ---
    ---@field Rot number
    ---@field Rotation number
    ---@field Opacity number
    ---@field Fade number
    ---@field Repeatable boolean
    ---@field Velocity Vector
    ---@field ScaleX number
    ---@field ScaleY number
    ---@field MovingTexAnim MovingTexAnim
    ---@field DrawRecAnim DrawRectAnim
    local SpriteAnim = {}

    --- Sets the per-second rotation offset used by the wobble effect.
    ---
    ---@param val number
    function SpriteAnim.SetRot(val) end

    --- Sets the per-second rotation speed.
    ---
    ---@param val number
    function SpriteAnim.SetRotation(val) end

    --- Sets the per-second opacity change.
    ---
    ---@param val number
    function SpriteAnim.SetOpacity(val) end

    --- Sets the per-second fade change.
    ---
    ---@param val number
    function SpriteAnim.SetFade(val) end

    --- Sets the wobble animation amount.
    ---
    ---@param val number
    function SpriteAnim.SetWobbleAnimAmount(val) end

    --- Sets the wobble animation speed.
    ---
    ---@param val number
    function SpriteAnim.SetWobbleAnimSpeed(val) end

    --- Sets whether the animation repeats.
    ---
    ---@param val boolean
    function SpriteAnim.SetRepeatable(val) end

    --- Sets the per-second position velocity.
    ---
    ---@param val Vector
    function SpriteAnim.SetVelocity(val) end

    --- Sets the per-second horizontal scale change.
    ---
    ---@param val number
    function SpriteAnim.SetScaleX(val) end

    --- Sets the per-second vertical scale change.
    ---
    ---@param val number
    function SpriteAnim.SetScaleY(val) end

    --- Sets the moving-texture sub-animation.
    ---
    ---@param val MovingTexAnim
    function SpriteAnim.SetMovingTexAnim(val) end

    --- Sets the frame-by-frame draw-rectangle sub-animation.
    ---
    ---@param val DrawRectAnim
    function SpriteAnim.SetDrawRecAnim(val) end

    --- Returns the per-second rotation offset.
    ---
    ---@return number
    function SpriteAnim.GetRot() end

    --- Returns the per-second rotation speed.
    ---
    ---@return number
    function SpriteAnim.GetRotation() end

    --- Returns the per-second opacity change.
    ---
    ---@return number
    function SpriteAnim.GetOpacity() end

    --- Returns the per-second fade change.
    ---
    ---@return number
    function SpriteAnim.GetFade() end

    --- Returns whether the animation repeats.
    ---
    ---@return boolean
    function SpriteAnim.GetRepeatable() end

    --- Returns the per-second position velocity.
    ---
    ---@return Vector
    function SpriteAnim.GetVelocity() end

    --- Returns the per-second horizontal scale change.
    ---
    ---@return number
    function SpriteAnim.GetScaleX() end

    --- Returns the per-second vertical scale change.
    ---
    ---@return number
    function SpriteAnim.GetScaleY() end

    --- Returns the moving-texture sub-animation.
    ---
    ---@return MovingTexAnim
    function SpriteAnim.GetMovingTexAnim() end

    --- Returns the frame-by-frame draw-rectangle sub-animation.
    ---
    ---@return DrawRectAnim
    function SpriteAnim.GetDrawRecAnim() end
```

##### MovingTexAnim

```lua
    --- Constructs a moving-texture animation, optionally with X and Y speeds.
    ---
    ---@param speedX? number
    ---@param speedY? number
    ---
    ---@return MovingTexAnim
    function MovingTexAnim(speedX, speedY) end

    --- Scroll a sprite's texture continuously.
    ---
    ---@class MovingTexAnim
    ---
    ---@field SpeedX number
    ---@field SpeedY number
    local MovingTexAnim = {}

    --- Sets the horizontal scroll speed.
    ---
    ---@param val number
    function MovingTexAnim.SetSpeedX(val) end

    --- Sets the vertical scroll speed.
    ---
    ---@param val number
    function MovingTexAnim.SetSpeedY(val) end

    --- Returns the horizontal scroll speed.
    ---
    ---@return number
    function MovingTexAnim.GetSpeedX() end

    --- Returns the vertical scroll speed.
    ---
    ---@return number
    function MovingTexAnim.GetSpeedY() end
```

##### DrawRectAnim

```lua
    --- Constructs a frame-by-frame draw-rectangle animation.
    ---
    ---@return DrawRectAnim
    function DrawRectAnim() end

    --- Animate a sprite frame by frame from a sprite sheet.
    ---
    ---@class DrawRectAnim
    ---
    ---@field FrameRate number
    ---@field FrameCount integer
    ---@field HorizontalFrameCount integer
    local DrawRectAnim = {}

    --- Sets the playback frame rate (frames per second).
    ---
    ---@param val number
    function DrawRectAnim.SetFrameRate(val) end

    --- Sets the total number of frames.
    ---
    ---@param val integer
    function DrawRectAnim.SetFrameCount(val) end

    --- Sets the number of frames per row in the sprite sheet.
    ---
    ---@param val integer
    function DrawRectAnim.SetHorizontalFrameCount(val) end

    --- Returns the playback frame rate.
    ---
    ---@return number
    function DrawRectAnim.GetFrameRate() end

    --- Returns the total number of frames.
    ---
    ---@return integer
    function DrawRectAnim.GetFrameCount() end

    --- Returns the number of frames per row.
    ---
    ---@return integer
    function DrawRectAnim.GetHorizontalFrameCount() end
```

#### SpriteFont

```lua
    --- Constructs a sprite font, optionally with initial text.
    ---
    ---@param text? string
    ---
    ---@return SpriteFont
    function SpriteFont(text) end

    --- Render text with a custom font.
    ---
    ---@class SpriteFont
    ---
    ---@field Text string
    ---@field Size integer
    ---@field Pos Vector
    ---@field Spacing Vector
    ---@field Align integer
    ---@field Color Vector
    ---@field ShadowColor Vector
    ---@field Bolden number
    ---@field Softness number
    ---@field ShadowBolden number
    ---@field ShadowSoftness number
    ---@field ShadowOffset Vector
    local SpriteFont = {}

    --- Sets the font style (typeface) by name, with an optional pixel size.
    ---
    ---@param fontstyle string
    ---@param size? integer font size in pixels (default 16)
    function SpriteFont.SetStyle(fontstyle, size) end

    --- Sets the displayed text (empty string if omitted).
    ---
    ---@param text? string
    function SpriteFont.SetText(text) end

    --- Sets the font size in pixels.
    ---
    ---@param size integer
    function SpriteFont.SetSize(size) end

    --- Sets an additional uniform scale applied on top of the size.
    ---
    ---@param scale number
    function SpriteFont.SetScale(scale) end

    --- Sets the screen position.
    ---
    ---@param pos Vector
    function SpriteFont.SetPos(pos) end

    --- Sets the horizontal and vertical spacing between characters and lines.
    ---
    ---@param spacing Vector
    function SpriteFont.SetSpacing(spacing) end

    --- Sets the horizontal and (optional) vertical alignment.
    ---
    ---@param halign integer one of the WIFALIGN_* constants
    ---@param valign? integer one of the WIFALIGN_* constants
    function SpriteFont.SetAlign(halign, valign) end

    --- Sets the text color, as a Vector (RGBA) or a packed hex color code.
    ---
    ---@param color Vector
    ---
    ---@overload fun(colorHexCode: integer)
    function SpriteFont.SetColor(color) end

    --- Sets the shadow color, as a Vector (RGBA) or a packed hex color code.
    ---
    ---@param shadowcolor Vector
    ---
    ---@overload fun(colorHexCode: integer)
    function SpriteFont.SetShadowColor(shadowcolor) end

    --- Sets how much the glyphs are bold.
    ---
    ---@param value number
    function SpriteFont.SetBolden(value) end

    --- Sets the glyph edge softness.
    ---
    ---@param value number
    function SpriteFont.SetSoftness(value) end

    --- Sets how much the shadow glyphs are bold.
    ---
    ---@param value number
    function SpriteFont.SetShadowBolden(value) end

    --- Sets the shadow edge softness.
    ---
    ---@param value number
    function SpriteFont.SetShadowSoftness(value) end

    --- Sets the shadow offset.
    ---
    ---@param value Vector
    function SpriteFont.SetShadowOffset(value) end

    --- Sets the width at which text wraps (<= 0 disables wrapping).
    ---
    ---@param value number
    function SpriteFont.SetHorizontalWrapping(value) end

    --- Shows or hides the text.
    ---
    ---@param value boolean
    function SpriteFont.SetHidden(value) end

    --- Enables flipping the letters horizontally.
    ---
    ---@param value boolean
    function SpriteFont.SetFlippedHorizontally(value) end

    --- Enables flipping the letters vertically.
    ---
    ---@param value boolean
    function SpriteFont.SetFlippedVertically(value) end

    --- Returns the displayed text.
    ---
    ---@return string
    function SpriteFont.GetText() end

    --- Returns the font size in pixels.
    ---
    ---@return integer
    function SpriteFont.GetSize() end

    --- Returns the additional uniform scale.
    ---
    ---@return number
    function SpriteFont.GetScale() end

    --- Returns the screen position.
    ---
    ---@return Vector
    function SpriteFont.GetPos() end

    --- Returns the character and line spacing.
    ---
    ---@return Vector
    function SpriteFont.GetSpacing() end

    --- Returns the horizontal and vertical alignment.
    ---
    ---@return integer halign
    ---@return integer valign
    function SpriteFont.GetAlign() end

    --- Returns the text color.
    ---
    ---@return Vector
    function SpriteFont.GetColor() end

    --- Returns the shadow color.
    ---
    ---@return Vector
    function SpriteFont.GetShadowColor() end

    --- Returns how much the glyphs are bold.
    ---
    ---@return number
    function SpriteFont.GetBolden() end

    --- Returns the glyph edge softness.
    ---
    ---@return number
    function SpriteFont.GetSoftness() end

    --- Returns how much the shadow glyphs are bold.
    ---
    ---@return number
    function SpriteFont.GetShadowBolden() end

    --- Returns the shadow edge softness.
    ---
    ---@return number
    function SpriteFont.GetShadowSoftness() end

    --- Returns the shadow offset.
    ---
    ---@return Vector
    function SpriteFont.GetShadowOffset() end

    --- Returns the wrapping width.
    ---
    ---@return number
    function SpriteFont.GetHorizontalWrapping() end

    --- Returns whether the text is hidden.
    ---
    ---@return boolean
    function SpriteFont.IsHidden() end

    --- Returns whether the letters are flipped horizontally.
    ---
    ---@return boolean
    function SpriteFont.IsFlippedHorizontally() end

    --- Returns whether the letters are flipped vertically.
    ---
    ---@return boolean
    function SpriteFont.IsFlippedVertically() end

    --- Returns the rendered text width and height in a Vector's X and Y.
    ---
    ---@return Vector
    function SpriteFont.TextSize() end

    --- Sets the time to fully type the text, in seconds (0 disables
    --- typewriting).
    ---
    ---@param value number
    function SpriteFont.SetTypewriterTime(value) end

    --- Sets whether the typewriter animation restarts when finished.
    ---
    ---@param value boolean
    function SpriteFont.SetTypewriterLooped(value) end

    --- Sets the character index the typewriter animation starts from.
    ---
    ---@param value integer
    function SpriteFont.SetTypewriterCharacterStart(value) end

    --- Sets a sound effect played as each new letter appears.
    ---
    ---@param sound Sound
    ---@param soundinstance SoundInstance
    function SpriteFont.SetTypewriterSound(sound, soundinstance) end

    --- Resets the typewriter animation to the first character.
    ---
    function SpriteFont.ResetTypewriter() end

    --- Finishes the typewriter animation immediately.
    ---
    function SpriteFont.TypewriterFinish() end

    --- Returns whether the typewriter animation has finished.
    ---
    ---@return boolean
    function SpriteFont.IsTypewriterFinished() end
```

The text alignment constants used by SetAlign and GetAlign:

```lua
    WIFALIGN_LEFT = 0
    WIFALIGN_CENTER = 1
    WIFALIGN_RIGHT = 2
    WIFALIGN_TOP = 3
    WIFALIGN_BOTTOM = 4
```

### Texture

```lua
    --- Creates a texture. With a filename, loads the image from file; with no
    --- argument, returns an empty (invalid) texture handle.
    ---
    ---@param filename? string
    ---
    ---@return Texture
    function Texture(filename) end

    --- A 2D texture image, loaded from file or created procedurally.
    ---
    ---@class Texture
    local Texture = {}

    --- Returns whether the texture contains valid data (was created or loaded
    --- successfully).
    ---
    ---@return boolean
    function Texture.IsValid() end

    --- Returns the width of the texture in pixels.
    ---
    ---@return integer
    function Texture.GetWidth() end

    --- Returns the height of the texture in pixels.
    ---
    ---@return integer
    function Texture.GetHeight() end

    --- Returns the depth of the texture (for 3D/volume textures).
    ---
    ---@return integer
    function Texture.GetDepth() end

    --- Returns the number of slices in the texture array.
    ---
    ---@return integer
    function Texture.GetArraySize() end

    --- Saves the texture to a file. The extension in the filename selects the
    --- format and must be one of: .JPG, .PNG, .TGA, .BMP, .DDS.
    ---
    ---@param filename string
    function Texture.Save(filename) end

    --- Sets the highest allowed texture asset resolution (only affects DDS
    --- textures that contain mipmaps).
    ---
    ---@param resolution integer
    function SetTextureResolutionLimit(resolution) end

    --- Returns the current texture resolution limit.
    ---
    ---@return integer
    function GetTextureResolutionLimit() end
```

> The engine also defines streaming-memory-threshold variants of these two
> functions, but they are not currently exposed to Lua, so only the
> resolution-limit form above is callable.

#### texturehelper

```lua
    --- A global utility object for creating textures programmatically. It
    --- shares the `Texture` type, so the handles it returns expose the query
    --- methods above.
    ---
    ---@class TextureHelper
    local TextureHelper = {}

    --- Global helper for creating textures programmatically.
    ---
    ---@type TextureHelper
    texturehelper = nil

    --- Returns the built-in Wicked Engine logo texture.
    ---
    ---@return Texture
    function TextureHelper.GetLogo() end

    --- Creates a gradient texture from the given parameters.
    ---
    ---@param type GradientType kind of gradient to generate
    ---@param width? integer texture width in pixels (default 256)
    ---@param height? integer texture height in pixels (default 256)
    ---@param uv_start? Vector gradient start UV (default Vector(0, 0))
    ---@param uv_end? Vector gradient end UV (default Vector(0, 0))
    ---@param flags? GradientFlags modifier flags (default GradientFlags.None)
    ---@param swizzle? string per-channel swizzle (default "rgba")
    ---@param perlin_scale? number perlin noise scale (default 1)
    ---@param perlin_seed? integer perlin noise seed (default 1234)
    ---@param perlin_octaves? integer perlin noise octaves (default 8)
    ---@param perlin_persistence? number perlin noise persistence (default 0.5)
    ---
    ---@return Texture
    function TextureHelper.CreateGradientTexture(
        type,
        width,
        height,
        uv_start,
        uv_end,
        flags,
        swizzle,
        perlin_scale,
        perlin_seed,
        perlin_octaves,
        perlin_persistence
    ) end

    --- Creates a lens distortion normal map (16-bit precision).
    ---
    ---@param width? integer texture width in pixels (default 256)
    ---@param height? integer texture height in pixels (default 256)
    ---@param uv_start? Vector distortion center UV (default Vector(0.5, 0.5))
    ---@param radius? number distortion radius (default 0.5)
    ---@param squish? number vertical squish factor (default 1)
    ---@param blend? number blend amount (default 1)
    ---@param edge_smoothness? number edge smoothness (default 0.04)
    ---
    ---@return Texture
    function TextureHelper.CreateLensDistortionNormalMap(
        width,
        height,
        uv_start,
        radius,
        squish,
        blend,
        edge_smoothness
    ) end
```

The gradient functions above use these enums:

```lua
    --- Gradient shapes for texturehelper.CreateGradientTexture.
    ---
    ---@enum GradientType
    GradientType = {
        Linear = 0,
        Circular = 1,
        Angular = 2,
    }

    --- Modifier flags for texturehelper.CreateGradientTexture (combine with
    --- bitwise or).
    ---
    ---@enum GradientFlags
    GradientFlags = {
        None = 0,
        Inverse = 1 << 0,
        Smoothstep = 1 << 1,
        PerlinNoise = 1 << 2,
        R16Unorm = 1 << 3,
    }
```

Example texture creation:

```lua
    -- Example usage (sprite/material are placeholders for your own objects).
    ---@diagnostic disable: undefined-global, param-type-mismatch
    texture = texturehelper.CreateGradientTexture(
        GradientType.Circular, -- gradient type
        256, 256, -- resolution of the texture
        -- start and end UV coordinates set the gradient direction and extent:
        Vector(0.5, 0.5), Vector(0.5, 0),
        -- modifier flags (bitwise combination):
        GradientFlags.Inverse | GradientFlags.Smoothstep
            | GradientFlags.PerlinNoise,
        -- per channel, one of: 0, 1, r, g, b, a, x, y, z, w (lower or upper
        -- case):
        "rrr1",
        2, -- perlin noise scale
        123, -- perlin noise seed
        6, -- perlin noise octaves
        0.8 -- perlin noise persistence
    )
    texture.Save("gradient.png") -- save it to a file
    sprite.SetTexture(texture) -- assign it to a sprite
    material.SetTexture(TextureSlot.BASECOLORMAP, texture) -- to a material
```

### Audio

```lua
    --- Creates an Audio object. Use the global `audio` instead of constructing
    --- one.
    ---
    ---@return Audio
    function Audio() end

    --- Loads and plays an audio files.
    ---
    ---@class Audio
    local Audio = {}

    --- The audio device.
    ---
    ---@type Audio
    audio = nil

    --- Creates a sound file, returns true if successful, false otherwise.
    ---
    ---@param filename string
    ---@param sound Sound
    ---
    ---@return boolean
    function Audio.CreateSound(filename, sound) end

    --- Creates a sound instance that can be replayed, returns true if
    --- successful, false otherwise.
    ---
    ---@param sound Sound
    ---@param soundinstance SoundInstance
    ---
    ---@return boolean
    function Audio.CreateSoundInstance(sound, soundinstance) end

    --- Plays the audio.
    ---
    ---@param soundinstance SoundInstance
    function Audio.Play(soundinstance) end

    --- Pauses the audio.
    ---
    ---@param soundinstance SoundInstance
    function Audio.Pause(soundinstance) end

    --- Stops the audio.
    ---
    ---@param soundinstance SoundInstance
    function Audio.Stop(soundinstance) end

    --- Returns the volume of a `soundinstance`. If `soundinstance` is not
    --- provided, returns the master volume.
    ---
    ---@param soundinstance? SoundInstance
    ---
    ---@return number
    function Audio.GetVolume(soundinstance) end

    --- Sets the volume of a `soundinstance`. If `soundinstance` is not
    --- provided, sets the master volume.
    ---
    ---@param volume number
    ---@param soundinstance? SoundInstance
    function Audio.SetVolume(volume, soundinstance) end

    --- Disable looping. By default, sound instances are looped when created.
    ---
    ---@param soundinstance SoundInstance
    function Audio.ExitLoop(soundinstance) end

    --- Returns the volume of the submix group.
    ---
    ---@param submixtype integer
    ---
    ---@return number
    function Audio.GetSubmixVolume(submixtype) end

    --- Sets the volume for a submix group.
    ---
    ---@param submixtype integer
    ---@param volume number
    function Audio.SetSubmixVolume(submixtype, volume) end

    --- Adds 3D effect to the sound instance.
    ---
    ---@param soundinstance SoundInstance
    ---@param instance3D SoundInstance3D
    function Audio.Update3D(soundinstance, instance3D) end

    --- Sets an environment effect for reverb globally. Refer to Reverb Types
    --- section for acceptable input values.
    ---
    ---@param reverbtype integer
    function Audio.SetReverb(reverbtype) end
```

#### Sound

```lua
    --- Creates a sound. With a filename it loads the sound from file; with no
    --- argument it creates an empty sound.
    ---
    ---@param name? string
    ---
    ---@return Sound
    function Sound(name) end

    --- An audio file. Can be instanced several times via SoundInstance.
    ---
    ---@class Sound
    local Sound = {}

    --- Returns whether the sound was created successfully.
    ---
    ---@return boolean
    function Sound.IsValid() end
```

#### SoundInstance

```lua
    --- Creates a sound instance. With a sound it is created from that sound
    --- (with optional begin offset and length in seconds); with no argument it
    --- creates an empty instance.
    ---
    ---@param sound? Sound
    ---@param begin? number
    ---@param length? number
    ---
    ---@return SoundInstance
    function SoundInstance(sound, begin, length) end

    --- An audio file instance that can be played. Note: after modifying
    --- parameters of the SoundInstance, the SoundInstance will need to be
    --- recreated from a specified sound.
    ---
    ---@class SoundInstance
    local SoundInstance = {}

    --- Set a submix type group (default is SUBMIX_TYPE_SOUNDEFFECT).
    ---
    ---@param submixtype integer
    function SoundInstance.SetSubmixType(submixtype) end

    --- Beginning of the playback in seconds, relative to the Sound it will be
    --- created from (0 = from beginning).
    ---
    ---@param seconds number
    function SoundInstance.SetBegin(seconds) end

    --- Length in seconds (0 = until end).
    ---
    ---@param seconds number
    function SoundInstance.SetLength(seconds) end

    --- Loop region begin in seconds, relative to the instance begin time (0 =
    --- from beginning).
    ---
    ---@param seconds number
    function SoundInstance.SetLoopBegin(seconds) end

    --- Loop region length in seconds (0 = until the end).
    ---
    ---@param seconds number
    function SoundInstance.SetLoopLength(seconds) end

    --- Enable/disable reverb for the sound instance.
    ---
    ---@param value boolean
    function SoundInstance.SetEnableReverb(value) end

    --- Enable/disable looping for the sound instance.
    ---
    ---@param value boolean
    function SoundInstance.SetLooped(value) end

    --- Returns the submix type.
    ---
    ---@return integer
    function SoundInstance.GetSubmixType() end

    --- Returns the begin.
    ---
    ---@return number
    function SoundInstance.GetBegin() end

    --- Returns the length.
    ---
    ---@return number
    function SoundInstance.GetLength() end

    --- Returns the loop begin.
    ---
    ---@return number
    function SoundInstance.GetLoopBegin() end

    --- Returns the loop length.
    ---
    ---@return number
    function SoundInstance.GetLoopLength() end

    --- Returns whether reverb is enabled.
    ---
    ---@return boolean
    function SoundInstance.IsEnableReverb() end

    --- Returns whether looped is enabled.
    ---
    ---@return boolean
    function SoundInstance.IsLooped() end

    --- Returns whether the sound instance was created successfully.
    ---
    ---@return boolean
    function SoundInstance.IsValid() end
```

#### SoundInstance3D

```lua
    --- Creates the 3D relation object. By default, the listener and emitter are
    --- on the same position, and that disables the 3D effect.
    ---
    ---@return SoundInstance3D
    function SoundInstance3D() end

    --- Describes the relation between a sound instance and a listener in a 3D
    --- world.
    ---
    ---@class SoundInstance3D
    local SoundInstance3D = {}

    --- Sets the listener position.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerPos(value) end

    --- Sets the listener up.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerUp(value) end

    --- Sets the listener front.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerFront(value) end

    --- Sets the listener velocity.
    ---
    ---@param value Vector
    function SoundInstance3D.SetListenerVelocity(value) end

    --- Sets the emitter position.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterPos(value) end

    --- Sets the emitter up.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterUp(value) end

    --- Sets the emitter front.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterFront(value) end

    --- Sets the emitter velocity.
    ---
    ---@param value Vector
    function SoundInstance3D.SetEmitterVelocity(value) end

    --- Sets the emitter radius.
    ---
    ---@param radius number
    function SoundInstance3D.SetEmitterRadius(radius) end
```

#### Submix Types

The submix types group sound instances together to be controlled together.

```lua
    --- Sound effect group.
    ---
    ---@type integer
    SUBMIX_TYPE_SOUNDEFFECT = 0

    --- Music group.
    ---
    ---@type integer
    SUBMIX_TYPE_MUSIC = 1

    --- User submix group.
    ---
    ---@type integer
    SUBMIX_TYPE_USER0 = 2

    --- uUer submix group.
    ---
    ---@type integer
    SUBMIX_TYPE_USER1 = 3
```

#### Reverb Types

The reverb types are built in presets that can mimic a specific kind of
environment.

```lua
    --- REVERB PRESET DEFAULT.
    ---
    ---@type integer
    REVERB_PRESET_DEFAULT = 0

    --- REVERB PRESET GENERIC.
    ---
    ---@type integer
    REVERB_PRESET_GENERIC = 1

    --- REVERB PRESET FOREST.
    ---
    ---@type integer
    REVERB_PRESET_FOREST = 2

    --- REVERB PRESET PADDEDCELL.
    ---
    ---@type integer
    REVERB_PRESET_PADDEDCELL = 3

    --- REVERB PRESET ROOM.
    ---
    ---@type integer
    REVERB_PRESET_ROOM = 4

    --- REVERB PRESET BATHROOM.
    ---
    ---@type integer
    REVERB_PRESET_BATHROOM = 5

    --- REVERB PRESET LIVINGROOM.
    ---
    ---@type integer
    REVERB_PRESET_LIVINGROOM = 6

    --- REVERB PRESET STONEROOM.
    ---
    ---@type integer
    REVERB_PRESET_STONEROOM = 7

    --- REVERB PRESET AUDITORIUM.
    ---
    ---@type integer
    REVERB_PRESET_AUDITORIUM = 8

    --- REVERB PRESET CONCERTHALL.
    ---
    ---@type integer
    REVERB_PRESET_CONCERTHALL = 9

    --- REVERB PRESET CAVE.
    ---
    ---@type integer
    REVERB_PRESET_CAVE = 10

    --- REVERB PRESET ARENA.
    ---
    ---@type integer
    REVERB_PRESET_ARENA = 11

    --- REVERB PRESET HANGAR.
    ---
    ---@type integer
    REVERB_PRESET_HANGAR = 12

    --- REVERB PRESET CARPETEDHALLWAY.
    ---
    ---@type integer
    REVERB_PRESET_CARPETEDHALLWAY = 13

    --- REVERB PRESET HALLWAY.
    ---
    ---@type integer
    REVERB_PRESET_HALLWAY = 14

    --- REVERB PRESET STONECORRIDOR.
    ---
    ---@type integer
    REVERB_PRESET_STONECORRIDOR = 15

    --- REVERB PRESET ALLEY.
    ---
    ---@type integer
    REVERB_PRESET_ALLEY = 16

    --- REVERB PRESET CITY.
    ---
    ---@type integer
    REVERB_PRESET_CITY = 17

    --- REVERB PRESET MOUNTAINS.
    ---
    ---@type integer
    REVERB_PRESET_MOUNTAINS = 18

    --- REVERB PRESET QUARRY.
    ---
    ---@type integer
    REVERB_PRESET_QUARRY = 19

    --- REVERB PRESET PLAIN.
    ---
    ---@type integer
    REVERB_PRESET_PLAIN = 20

    --- REVERB PRESET PARKINGLOT.
    ---
    ---@type integer
    REVERB_PRESET_PARKINGLOT = 21

    --- REVERB PRESET SEWERPIPE.
    ---
    ---@type integer
    REVERB_PRESET_SEWERPIPE = 22

    --- REVERB PRESET UNDERWATER.
    ---
    ---@type integer
    REVERB_PRESET_UNDERWATER = 23

    --- REVERB PRESET SMALLROOM.
    ---
    ---@type integer
    REVERB_PRESET_SMALLROOM = 24

    --- REVERB PRESET MEDIUMROOM.
    ---
    ---@type integer
    REVERB_PRESET_MEDIUMROOM = 25

    --- REVERB PRESET LARGEROOM.
    ---
    ---@type integer
    REVERB_PRESET_LARGEROOM = 26

    --- REVERB PRESET MEDIUMHALL.
    ---
    ---@type integer
    REVERB_PRESET_MEDIUMHALL = 27

    --- REVERB PRESET LARGEHALL.
    ---
    ---@type integer
    REVERB_PRESET_LARGEHALL = 28

    --- REVERB PRESET PLATE.
    ---
    ---@type integer
    REVERB_PRESET_PLATE = 29
```

### Video

The video interface consists of two types of objects: Video and VideoInstance.
Note: these are the underlying objects that VideoComponents use in the scene, to
easily set videos to materials or lights, look at the
[VideoComponent](#videocomponent) object that you can use with the scene's
entity component system.

#### Video

```lua
    --- Loads an MP4 video file (currently only the H264 internal compression
    --- format is supported).
    ---
    ---@param filename string
    ---
    ---@return Video
    function Video(filename) end

    --- A video resource loaded from file.
    ---
    ---@class Video
    local Video = {}

    --- Returns true if the video was successfully created.
    ---
    ---@return boolean
    function Video.IsValid() end

    --- Returns the duration seconds.
    ---
    ---@return number
    function Video.GetDurationSeconds() end
```

#### VideoInstance

The VideoInstance object is responsible to decode the video frames and output
them to textures. One Video can be decoded with multiple VideoInstances to
display frames of the video at different timings. Normally you would use one
VideoInstance for a video, unless you want to show the video multiple times at
once at different locations.

```lua
    --- Creates a decoder instance for the video data.
    ---
    ---@param video Video
    ---
    ---@return VideoInstance
    function VideoInstance(video) end

    --- A playing instance of a Video, with playback controls.
    ---
    ---@class VideoInstance
    local VideoInstance = {}


    --- Returns true if the video instance was successfully created.
    ---
    ---@return boolean
    function VideoInstance.IsValid() end

    --- Play.
    function VideoInstance.Play() end

    --- Pause.
    function VideoInstance.Pause() end

    --- Stop.
    function VideoInstance.Stop() end

    --- Sets whether playback loops when the video reaches the end.
    ---
    ---@param looped boolean
    function VideoInstance.SetLooped(looped) end

    --- Seek.
    ---
    ---@param timerSeconds number
    function VideoInstance.Seek(timerSeconds) end

    --- Returns current video playback timer for this decoder instance in
    --- seconds.
    ---
    ---@return number
    function VideoInstance.GetCurrentTimer() end

    --- Returns true if the video playback has ended.
    ---
    ---@return boolean
    function VideoInstance.IsEnded() end
```

### Math Types

#### Vector

```lua
    --- Constructs a 4-component vector. Any missing component defaults to 0.
    ---
    ---@param x? number
    ---@param y? number
    ---@param z? number
    ---@param w? number
    ---
    ---@return Vector
    function Vector(x, y, z, w) end

    --- A four component floating point vector. Provides efficient calculations
    --- with SIMD support.
    ---
    --- The global `vector` object exposes the same functions, so they can be
    --- called in a static style, e.g. `vector.Dot(a, b)`.
    ---
    ---@class Vector
    ---
    ---@field X number
    ---@field Y number
    ---@field Z number
    ---@field W number
    local Vector = {}

    --- Global helper exposing every Vector function for static-style calls,
    --- e.g. vector.Lerp(a, b, t).
    ---
    ---@type Vector
    vector = nil

    --- Returns the X component of the vector.
    ---
    ---@return number
    function Vector.GetX() end

    --- Returns the Y component of the vector.
    ---
    ---@return number
    function Vector.GetY() end

    --- Returns the Z component of the vector.
    ---
    ---@return number
    function Vector.GetZ() end

    --- Returns the W component of the vector.
    ---
    ---@return number
    function Vector.GetW() end

    --- Sets the X component of the vector.
    ---
    ---@param value number
    function Vector.SetX(value) end

    --- Sets the Y component of the vector.
    ---
    ---@param value number
    function Vector.SetY(value) end

    --- Sets the Z component of the vector.
    ---
    ---@param value number
    function Vector.SetZ(value) end

    --- Sets the W component of the vector.
    ---
    ---@param value number
    function Vector.SetW(value) end

    --- Returns the 3D length (magnitude) of a vector. Called with no argument
    --- it operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return number
    ---
    ---@overload fun(): number
    function Vector.Length(v) end

    --- Returns the squared 3D length of a vector (cheaper than Length). Called
    --- with no argument it operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return number
    ---
    ---@overload fun(): number
    function Vector.LengthSquared(v) end

    --- Returns the distance between two points.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return number
    function Vector.Distance(v1, v2) end

    --- Returns the squared distance between two points (cheaper than Distance).
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return number
    function Vector.DistanceSquared(v1, v2) end

    --- Returns a normalized (unit-length) copy of a vector. Called with no
    --- argument it operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Vector.Normalize(v) end

    --- Returns a normalized copy of a quaternion. Called with no argument it
    --- operates on the vector itself.
    ---
    ---@param v Vector
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Vector.QuaternionNormalize(v) end

    --- Clamps every component of a vector between min and max. Called with two
    --- arguments it operates on the vector itself.
    ---
    ---@param v Vector
    ---@param min number
    ---@param max number
    ---
    ---@return Vector
    ---
    ---@overload fun(min: number, max: number): Vector
    function Vector.Clamp(v, min, max) end

    --- Transforms a vector by a matrix (4D, uses the w component).
    ---
    ---@param vec Vector
    ---@param matrix Matrix
    ---
    ---@return Vector
    function Vector.Transform(vec, matrix) end

    --- Transforms a 3D normal by a matrix (ignores translation).
    ---
    ---@param vec Vector
    ---@param matrix Matrix
    ---
    ---@return Vector
    function Vector.TransformNormal(vec, matrix) end

    --- Transforms a 3D coordinate by a matrix (applies the perspective divide).
    ---
    ---@param vec Vector
    ---@param matrix Matrix
    ---
    ---@return Vector
    function Vector.TransformCoord(vec, matrix) end

    --- Returns the component-wise sum of two vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    function Vector.Add(v1, v2) end

    --- Returns the component-wise difference of two vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    function Vector.Subtract(v1, v2) end

    --- Multiplies two vectors component-wise, or scales a vector by a scalar.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    ---
    ---@overload fun(v: Vector, f: number): Vector
    ---@overload fun(f: number, v: Vector): Vector
    function Vector.Multiply(v1, v2) end

    --- Returns the dot product of two vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return number
    function Vector.Dot(v1, v2) end

    --- Returns the cross product of two 3D vectors.
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---
    ---@return Vector
    function Vector.Cross(v1, v2) end

    --- Linearly interpolates between two vectors by factor t in [0, 1].
    ---
    ---@param v1 Vector
    ---@param v2 Vector
    ---@param t number
    ---
    ---@return Vector
    function Vector.Lerp(v1, v2, t) end

    --- Rotates a 3D vector by a quaternion.
    ---
    ---@param v Vector          the vector to rotate
    ---@param quaternion Vector the rotation quaternion
    ---
    ---@return Vector
    function Vector.Rotate(v, quaternion) end

    --- Returns a quaternion representing the identity (no) rotation.
    ---
    ---@return Vector
    function Vector.QuaternionIdentity() end

    --- Returns the inverse of a quaternion.
    ---
    ---@param quaternion Vector
    ---
    ---@return Vector
    function Vector.QuaternionInverse(quaternion) end

    --- Multiplies (concatenates) two quaternions.
    ---
    ---@param quaternion1 Vector
    ---@param quaternion2 Vector
    ---
    ---@return Vector
    function Vector.QuaternionMultiply(quaternion1, quaternion2) end

    --- Builds a quaternion from Euler angles packed as (roll, pitch, yaw).
    ---
    ---@param rotXYZ Vector
    ---
    ---@return Vector
    function Vector.QuaternionFromRollPitchYaw(rotXYZ) end

    --- Extracts Euler angles (roll, pitch, yaw) from a quaternion.
    ---
    ---@param quaternion Vector
    ---
    ---@return Vector
    function Vector.QuaternionToRollPitchYaw(quaternion) end

    --- Spherically interpolates between two quaternions by factor t.
    ---
    ---@param quaternion1 Vector
    ---@param quaternion2 Vector
    ---@param t number
    ---
    ---@return Vector
    function Vector.QuaternionSlerp(quaternion1, quaternion2, t) end

    --- Spherically interpolates between two quaternions by factor t. Same as
    --- QuaternionSlerp.
    ---
    ---@param quaternion1 Vector
    ---@param quaternion2 Vector
    ---@param t number
    ---
    ---@return Vector
    function Vector.Slerp(quaternion1, quaternion2, t) end

    --- Constructs a plane (as a Vector of coefficients) from a point and a
    --- normal.
    ---
    ---@param point Vector
    ---@param normal Vector
    ---
    ---@return Vector
    function Vector.PlaneFromPointNormal(point, normal) end

    --- Constructs a plane (as a Vector of coefficients) from three points.
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param c Vector
    ---
    ---@return Vector
    function Vector.PlaneFromPoints(a, b, c) end

    --- Computes the angle between two 3D vectors around the given axis, in the
    --- range [0, max_angle] (default 2*pi).
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param axis Vector
    ---@param max_angle? number
    ---
    ---@return number
    function Vector.GetAngle(a, b, axis, max_angle) end

    --- Computes the signed angle between two 3D vectors around the given axis.
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param axis Vector
    ---
    ---@return number
    function Vector.GetAngleSigned(a, b, axis) end
```

#### Matrix

```lua
    --- Constructs a 4x4 matrix from 16 row-major components. Any missing
    --- component defaults to 0; with no arguments the identity matrix is
    --- returned.
    ---
    ---@param m00? number
    ---@param m01? number
    ---@param m02? number
    ---@param m03? number
    ---@param m10? number
    ---@param m11? number
    ---@param m12? number
    ---@param m13? number
    ---@param m20? number
    ---@param m21? number
    ---@param m22? number
    ---@param m23? number
    ---@param m30? number
    ---@param m31? number
    ---@param m32? number
    ---@param m33? number
    ---
    ---@return Matrix
    function Matrix(
        m00, m01, m02, m03,
        m10, m11, m12, m13,
        m20, m21, m22, m23,
        m30, m31, m32, m33
    ) end

    --- A four by four matrix. Provides efficient calculations with SIMD
    --- support.
    ---
    --- The global `matrix` object exposes the same functions, so they can be
    --- called in a static style, e.g. `matrix.Multiply(a, b)`.
    ---
    ---@class Matrix
    local Matrix = {}

    --- Global helper exposing every Matrix function for static-style calls,
    --- e.g. matrix.Multiply(a, b).
    ---
    ---@type Matrix
    matrix = nil

    --- Returns the given row (0-3) of the matrix as a Vector.
    ---
    ---@param row? integer row index in [0, 3] (default 0)
    ---
    ---@return Vector
    function Matrix.GetRow(row) end

    --- Builds a translation matrix from a position vector (identity if
    --- omitted).
    ---
    ---@param vector? Vector
    ---
    ---@return Matrix
    function Matrix.Translation(vector) end

    --- Builds a rotation matrix from Euler angles packed as (roll, pitch, yaw).
    ---
    ---@param rollPitchYaw? Vector
    ---
    ---@return Matrix
    function Matrix.Rotation(rollPitchYaw) end

    --- Builds a rotation matrix around the X axis.
    ---
    ---@param angleInRadians? number
    ---
    ---@return Matrix
    function Matrix.RotationX(angleInRadians) end

    --- Builds a rotation matrix around the Y axis.
    ---
    ---@param angleInRadians? number
    ---
    ---@return Matrix
    function Matrix.RotationY(angleInRadians) end

    --- Builds a rotation matrix around the Z axis.
    ---
    ---@param angleInRadians? number
    ---
    ---@return Matrix
    function Matrix.RotationZ(angleInRadians) end

    --- Builds a rotation matrix from a quaternion.
    ---
    ---@param quaternion? Vector
    ---
    ---@return Matrix
    function Matrix.RotationQuaternion(quaternion) end

    --- Builds a scaling matrix from a per-axis Vector or a uniform scalar
    --- (identity if omitted).
    ---
    ---@param scaleXYZ? Vector
    ---
    ---@return Matrix
    ---
    ---@overload fun(scale: number): Matrix
    function Matrix.Scale(scaleXYZ) end

    --- Builds a left-handed view matrix looking from eye toward a direction.
    ---
    ---@param eye Vector
    ---@param direction Vector
    ---@param up? Vector up direction (default (0, 1, 0))
    ---
    ---@return Matrix
    function Matrix.LookTo(eye, direction, up) end

    --- Builds a left-handed view matrix looking from eye at a focus point.
    ---
    ---@param eye Vector
    ---@param focusPos Vector
    ---@param up? Vector up direction (default (0, 1, 0))
    ---
    ---@return Matrix
    function Matrix.LookAt(eye, focusPos, up) end

    --- Returns the product of two matrices.
    ---
    ---@param m1 Matrix
    ---@param m2 Matrix
    ---
    ---@return Matrix
    function Matrix.Multiply(m1, m2) end

    --- Returns the component-wise sum of two matrices.
    ---
    ---@param m1 Matrix
    ---@param m2 Matrix
    ---
    ---@return Matrix
    function Matrix.Add(m1, m2) end

    --- Returns the transpose of a matrix.
    ---
    ---@param m Matrix
    ---
    ---@return Matrix
    function Matrix.Transpose(m) end

    --- Returns the inverse of a matrix together with its determinant.
    ---
    ---@param m Matrix
    ---
    ---@return Matrix inverse
    ---@return number determinant
    function Matrix.Inverse(m) end

    --- Returns the forward direction of a matrix. Called with no argument it
    --- operates on the matrix itself.
    ---
    ---@param mat Matrix
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Matrix.GetForward(mat) end

    --- Returns the up direction of a matrix. Called with no argument it
    --- operates on the matrix itself.
    ---
    ---@param mat Matrix
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Matrix.GetUp(mat) end

    --- Returns the right direction of a matrix. Called with no argument it
    --- operates on the matrix itself.
    ---
    ---@param mat Matrix
    ---
    ---@return Vector
    ---
    ---@overload fun(): Vector
    function Matrix.GetRight(mat) end
```

### Async

```lua
    --- Constructs a new Async tracker object.
    ---
    ---@return Async
    function Async() end

    --- The Async object can be used for tracking or waiting for completion of
    --- functions running on background threads.
    ---
    ---@class Async
    local Async = {}

    --- Wait for completion of async tasks on this tracker.
    function Async.Wait() end

    --- Checks if all async tasks on this tracker have been completed.
    ---
    ---@return boolean
    function Async.IsCompleted() end
```

### Scene System (using entity-component system)

Manipulate the 3D scene with these components.

#### Entity

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

#### Scene

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

#### RayIntersectionResult

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

#### SphereIntersectionResult

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

#### NameComponent

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

#### LayerComponent

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

#### TransformComponent

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

#### CameraComponent

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

#### AnimationComponent

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

#### MaterialComponent

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

#### MeshComponent

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

#### EmitterComponent

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

#### HairParticleSystem

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

#### LightComponent

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

#### ObjectComponent

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

#### InverseKinematicsComponent

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

#### SpringComponent

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

#### ScriptComponent

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

#### RigidBodyPhysicsComponent

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

#### SoftBodyPhysicsComponent

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

#### ForceFieldComponent

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

#### WeatherComponent

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

##### OceanParameters

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

##### AtmosphereParameters

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

##### VolumetricCloudParameters

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

#### SoundComponent

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

#### VideoComponent

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

#### ColliderComponent

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

#### ExpressionComponent

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

#### HumanoidComponent

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

#### DecalComponent

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

#### MetadataComponent

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

#### CharacterComponent

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

### Canvas

```lua
    --- Creates a Canvas object. In practice you obtain the active canvas from
    --- the application (e.g. `application:GetCanvas()`) instead of constructing
    --- one.
    ---
    ---@return Canvas
    function Canvas() end

    --- Describes a drawable area.
    ---
    ---@class Canvas
    local Canvas = {}

    --- Get the canvas pixels per inch (DPI).
    ---
    ---@return number
    function Canvas.GetDPI() end

    --- Scaling factor between physical and logical size.
    ---
    ---@return number
    function Canvas.GetDPIScaling() end

    --- A custom scaling factor on top of the DPI scaling.
    ---
    ---@return number
    function Canvas.GetCustomScaling() end

    --- Sets a custom scaling factor that will be applied on top of DPI scaling.
    ---
    ---@param value number
    function Canvas.SetCustomScaling(value) end

    --- Get the canvas width in pixels.
    ---
    ---@return integer
    function Canvas.GetPhysicalWidth() end

    --- Get the canvas height in pixels.
    ---
    ---@return integer
    function Canvas.GetPhysicalHeight() end

    --- Get the canvas width in dpi scaled units.
    ---
    ---@return number
    function Canvas.GetLogicalWidth() end

    --- Get the canvas height in dpi scaled units.
    ---
    ---@return number
    function Canvas.GetLogicalHeight() end
```

### High Level Interface

**This section must only be used from standalone lua scripts, and must not be
used from a ScriptComponent.** This is because ScriptComponent is always running
inside scene.Update(), and paths can not be switched at that time safely. On the
other hand, a standalone lua script can define its own update logic and render
path and change application behavior.

#### Application

```lua
    --- Fade transition styles used when switching render paths (see
    --- Application.SetActivePath and Application.Fade).
    ---
    ---@enum FadeType
    FadeType = {
        FadeToColor = 0,
        CrossFade = 1,
    }
```

```lua
    --- Creates an Application object. In practice you use the global
    --- `application` rather than constructing one.
    ---
    ---@return Application
    function Application() end

    --- This is the main entry point and manages the lifetime of the
    --- application.
    ---
    ---@class Application
    local Application = {}

    --- Application.
    ---
    ---@type Application
    application = nil

    --- Main.
    ---
    ---@deprecated
    ---
    ---@type Application
    main = nil

    --- Returns the active render path. The concrete type depends on what is
    --- currently active (a 3D path, a loading screen, etc.).
    ---
    ---@return RenderPath3D|LoadingScreen|RenderPath2D|RenderPath|nil
    function Application.GetActivePath() end

    --- Sets the active render path.
    ---
    ---@param path RenderPath
    ---@param fadeSeconds? number
    ---@param fadeColorR? integer
    ---@param fadeColorG? integer
    ---@param fadeColorB? integer
    ---@param fadetype? FadeType
    function Application.SetActivePath(
        path,
        fadeSeconds,
        fadeColorR,
        fadeColorG,
        fadeColorB,
        fadetype
    ) end

    --- Enable/disable frame skipping in fixed update.
    ---
    ---@param enabled boolean
    function Application.SetFrameSkip(enabled) end

    --- Switch to fullscreen/windowed.
    ---
    ---@param value boolean
    function Application.SetFullScreen(value) end

    --- Set target frame rate for fixed update and variable rate update when
    --- frame rate is locked.
    ---
    ---@param fps number
    function Application.SetTargetFrameRate(fps) end

    --- If enabled, variable rate update will use a fixed delta time.
    ---
    ---@param enabled boolean
    function Application.SetFrameRateLock(enabled) end

    --- If enabled, information display will be visible in the top left corner
    --- of the application.
    ---
    ---@param active boolean
    function Application.SetInfoDisplay(active) end

    --- Toggle display of engine watermark, version number, etc. if info display
    --- is enabled.
    ---
    ---@param active boolean
    function Application.SetWatermarkDisplay(active) end

    --- Toggle display of frame rate if info display is enabled.
    ---
    ---@param active boolean
    function Application.SetFPSDisplay(active) end

    --- Toggle display of resolution if info display is enabled.
    ---
    ---@param active boolean
    function Application.SetResolutionDisplay(active) end

    --- Toggle display of logical size of canvas if info display is enabled.
    ---
    ---@param active boolean
    function Application.SetLogicalSizeDisplay(active) end

    --- Toggle display of output color space if info display is enabled.
    ---
    ---@param active boolean
    function Application.SetColorSpaceDisplay(active) end

    --- Toggle display of active graphics pipeline count if info display is
    --- enabled.
    ---
    ---@param active boolean
    function Application.SetPipelineCountDisplay(active) end

    --- Toggle display of heap allocation statistics if info display is enabled.
    ---
    ---@param active boolean
    function Application.SetHeapAllocationCountDisplay(active) end

    --- Toggle display of video memory usage if info display is enabled.
    ---
    ---@param active boolean
    function Application.SetVRAMUsageDisplay(active) end

    --- Toggle color grading helper display in the top left corner.
    ---
    ---@param value boolean
    function Application.SetColorGradingHelper(value) end

    --- Returns whether HDR display output is supported on the current monitor.
    ---
    ---@return boolean
    function Application.IsHDRSupported() end

    --- Sets HDR display mode (if monitor supports it).
    ---
    ---@param bool boolean
    function Application.SetHDR(bool) end

    --- Returns a copy of the application's current canvas.
    ---
    ---@return Canvas
    function Application.GetCanvas() end

    --- Applies the specified canvas to the application.
    ---
    ---@param canvas Canvas
    function Application.SetCanvas(canvas) end

    --- Closes the program.
    function Application.Exit() end

    --- Returns true when fadeout is full (fadeout can be set when switching
    --- paths with SetActivePath()).
    function Application.IsFaded() end

    --- Starts a fade transition over fadeSeconds to the given RGB color (each
    --- channel 0-255), using the given FadeType.
    ---
    ---@param fadeSeconds number
    ---@param fadeColorR? integer
    ---@param fadeColorG integer
    ---@param fadeColorB integer
    ---@param fadetype FadeType
    function Application.Fade(
        fadeSeconds,
        fadeColorR,
        fadeColorG,
        fadeColorB,
        fadetype
    ) end

    --- Starts a cross-fade transition over the given number of seconds,
    --- blending from the previous frame.
    ---
    ---@param fadeSeconds number
    function Application.CrossFade(fadeSeconds) end

    --- Enable/disable the on-screen profiler.
    ---
    ---@param enabled boolean
    function SetProfilerEnabled(enabled) end

    --- Toggle the on-screen profiler (this function is made for convenience to
    --- write faster).
    function prof() end

    --- Closes the application.
    function exit() end
```

#### RenderPath

```lua
    --- Creates a base RenderPath. A RenderPath represents a high-level part of
    --- the application (menu, loading screen, gameplay screen, etc.).
    ---
    ---@return RenderPath
    function RenderPath() end

    --- A RenderPath is a high level system that represents a part of the whole
    --- application. It is responsible to handle high level rendering and logic
    --- flow. A render path can be for example a loading screen, a menu screen,
    --- or primary game screen, etc.
    ---
    ---@class RenderPath
    local RenderPath = {}

    --- Returns the layer mask.
    ---
    ---@return integer
    function RenderPath.GetLayerMask() end

    --- Sets the layer mask.
    ---
    ---@param mask integer
    function RenderPath.SetLayerMask(mask) end
```

##### RenderPath2D : RenderPath

```lua
    --- Creates a 2D render path for drawing sprites and fonts.
    ---
    ---@return RenderPath2D
    function RenderPath2D() end

    --- A 2D render path that holds Sprites and SpriteFonts, sorts them by
    --- layer, and updates and renders them.
    ---
    ---@class RenderPath2D : RenderPath
    local RenderPath2D = {}

    --- Adds sprite.
    ---
    ---@param sprite Sprite
    ---@param layer? string
    function RenderPath2D.AddSprite(sprite, layer) end

    --- Adds video sprite.
    ---
    ---@param videoinstance VideoInstance
    ---@param sprite Sprite
    ---@param layer? string
    function RenderPath2D.AddVideoSprite(videoinstance, sprite, layer) end

    --- Adds font.
    ---
    ---@param font SpriteFont
    ---@param layer? string
    function RenderPath2D.AddFont(font, layer) end

    --- Removes font.
    ---
    ---@param font SpriteFont
    function RenderPath2D.RemoveFont(font) end

    --- Clear Sprites.
    function RenderPath2D.ClearSprites() end

    --- Clear Fonts.
    function RenderPath2D.ClearFonts() end

    --- Returns the sprite order.
    ---
    ---@param sprite Sprite
    ---
    ---@return integer?
    function RenderPath2D.GetSpriteOrder(sprite) end

    --- Returns the font order.
    ---
    ---@param font SpriteFont
    ---
    ---@return integer?
    function RenderPath2D.GetFontOrder(font) end

    --- Adds layer.
    ---
    ---@param name string
    function RenderPath2D.AddLayer(name) end

    --- Returns the layers.
    ---
    ---@return any
    function RenderPath2D.GetLayers() end

    --- Sets the layer order.
    ---
    ---@param name string
    ---@param order integer
    function RenderPath2D.SetLayerOrder(name, order) end

    --- Sets the sprite order.
    ---
    ---@param sprite Sprite
    ---@param order integer
    function RenderPath2D.SetSpriteOrder(sprite, order) end

    --- Sets the font order.
    ---
    ---@param font SpriteFont
    ---@param order integer
    function RenderPath2D.SetFontOrder(font, order) end

    --- Returns HDR scaling value used for SDR to HDR linear output mapping
    --- conversion (default: 9.0).
    ---
    ---@return number
    function RenderPath2D.GetHDRScaling() end

    --- Sets HDR scaling value used for SDR to HDR linear output mapping
    --- conversion (default: 9.0).
    ---
    ---@param value number
    function RenderPath2D.SetHDRScaling(value) end

    --- Copies everything from other renderpath into this.
    ---
    ---@param other RenderPath
    function RenderPath2D.CopyFrom(other) end

    --- Removes a sprite from the render path (or all sprites if none given).
    ---
    ---@param sprite? Sprite
    function RenderPath2D.RemoveSprite(sprite) end
```

##### RenderPath3D : RenderPath2D

```lua
    --- Creates a 3D scene render path. It inherits the 2D features, so it can
    --- also draw a 2D overlay.
    ---
    ---@return RenderPath3D
    function RenderPath3D() end

    --- The default scene render path.
    --- It inherits functions from RenderPath2D, so it can render a 2D overlay.
    ---
    ---@class RenderPath3D : RenderPath2D
    local RenderPath3D = {}

    --- Scale internal rendering resolution. This can provide major performance
    --- improvement when GPU rendering speed is the bottleneck.
    ---
    ---@param value number
    function RenderPath3D.SetResolutionScale(value) end

    --- Sets up the ambient occlusion effect (possible values below).
    ---
    ---@param value integer
    function RenderPath3D.SetAO(value) end

    --- Turn off AO computation (use in SetAO() function).
    ---
    ---@type integer
    AO_DISABLED = 0

    --- Enable simple brute force screen space ambient occlusion (use in SetAO()
    --- function).
    ---
    ---@type integer
    AO_SSAO = 1

    --- Enable horizon based screen space ambient occlusion (use in SetAO()
    --- function).
    ---
    ---@type integer
    AO_HBAO = 2

    --- Enable multi scale screen space ambient occlusion (use in SetAO()
    --- function).
    ---
    ---@type integer
    AO_MSAO = 3

    --- Use ray-traced ambient occlusion in SetAO() (requires hardware ray
    --- tracing).
    ---
    ---@type integer
    AO_RTAO = 4

    --- Applies AO power value if any AO is enabled.
    ---
    ---@param value number
    function RenderPath3D.SetAOPower(value) end

    --- Sets max range for ray traced AO.
    ---
    ---@param value number
    function RenderPath3D.SetAORange(value) end

    --- Enables or disables screen-space reflections (SSR).
    ---
    ---@param value boolean
    function RenderPath3D.SetSSREnabled(value) end

    --- Enables or disables screen-space global illumination (SSGI).
    ---
    ---@param value boolean
    function RenderPath3D.SetSSGIEnabled(value) end

    --- Enables or disables ray-traced diffuse global illumination (requires
    --- hardware ray tracing).
    ---
    ---@param value boolean
    function RenderPath3D.SetRaytracedDiffuseEnabled(value) end

    --- Enables or disables ray-traced reflections (requires hardware ray
    --- tracing).
    ---
    ---@param value boolean
    function RenderPath3D.SetRaytracedReflectionsEnabled(value) end

    --- Enables or disables shadow rendering for the scene.
    ---
    ---@param value boolean
    function RenderPath3D.SetShadowsEnabled(value) end

    --- Enables or disables planar and environment reflections.
    ---
    ---@param value boolean
    function RenderPath3D.SetReflectionsEnabled(value) end

    --- Controls the planar reflection render resolution and multisampling anti
    --- aliasing. msaaSampleCount must be a value from these: 1,2,4,8
    ---
    ---@param resolutionScale number
    ---@param msaaSampleCount integer
    function RenderPath3D.SetPlanarReflectionQuality(
        resolutionScale,
        msaaSampleCount
    ) end

    --- Enables or disables FXAA (fast approximate anti-aliasing).
    ---
    ---@param value boolean
    function RenderPath3D.SetFXAAEnabled(value) end

    --- Enables or disables the bloom post-process effect.
    ---
    ---@param value boolean
    function RenderPath3D.SetBloomEnabled(value) end

    --- Sets the bloom threshold.
    ---
    ---@param value number
    function RenderPath3D.SetBloomThreshold(value) end

    --- Enables or disables color grading.
    ---
    ---@param value boolean
    function RenderPath3D.SetColorGradingEnabled(value) end

    --- Enables or disables volume lights.
    ---
    ---@param value boolean
    function RenderPath3D.SetVolumeLightsEnabled(value) end

    --- Enables or disables light shafts.
    ---
    ---@param value boolean
    function RenderPath3D.SetLightShaftsEnabled(value) end

    --- Enables or disables lens flare.
    ---
    ---@param value boolean
    function RenderPath3D.SetLensFlareEnabled(value) end

    --- Enables or disables motion blur.
    ---
    ---@param value boolean
    function RenderPath3D.SetMotionBlurEnabled(value) end

    --- Sets the motion blur strength.
    ---
    ---@param value number
    function RenderPath3D.SetMotionBlurStrength(value) end

    --- Enables or disables dither.
    ---
    ---@param value boolean
    function RenderPath3D.SetDitherEnabled(value) end

    --- Enables or disables depth of field.
    ---
    ---@param value boolean
    function RenderPath3D.SetDepthOfFieldEnabled(value) end

    --- Sets the depth of field strength.
    ---
    ---@param value number
    function RenderPath3D.SetDepthOfFieldStrength(value) end

    --- Sets the light shafts strength.
    ---
    ---@param value number
    function RenderPath3D.SetLightShaftsStrength(value) end

    --- Sets the msaasample count.
    ---
    ---@param count integer
    function RenderPath3D.SetMSAASampleCount(count) end

    --- Enables or disables crtfilter.
    ---
    ---@param value boolean
    function RenderPath3D.SetCRTFilterEnabled(value) end

    --- Enables or disables sharpen filter.
    ---
    ---@param value boolean
    function RenderPath3D.SetSharpenFilterEnabled(value) end

    --- Sets the sharpen filter amount.
    ---
    ---@param value number
    function RenderPath3D.SetSharpenFilterAmount(value) end

    --- Enables or disables eye adaption.
    ---
    ---@param value boolean
    function RenderPath3D.SetEyeAdaptionEnabled(value) end

    --- Sets the exposure.
    ---
    ---@param value number
    function RenderPath3D.SetExposure(value) end

    --- Sets the hdrcalibration.
    ---
    ---@param value number
    function RenderPath3D.SetHDRCalibration(value) end

    --- Enables or disables outline.
    ---
    ---@param value boolean
    function RenderPath3D.SetOutlineEnabled(value) end

    --- Sets the outline threshold.
    ---
    ---@param value number
    function RenderPath3D.SetOutlineThreshold(value) end

    --- Sets the outline thickness.
    ---
    ---@param value number
    function RenderPath3D.SetOutlineThickness(value) end

    --- Sets the outline color.
    ---
    ---@param r number
    ---@param g number
    ---@param b number
    ---@param a number
    function RenderPath3D.SetOutlineColor(r, g, b, a) end

    --- FSR 1.0 on/off.
    ---
    ---@param value boolean
    function RenderPath3D.SetFSREnabled(value) end

    --- FSR 1.0 sharpness 0: sharpest, 2: least sharp.
    ---
    ---@param value number
    function RenderPath3D.SetFSRSharpness(value) end

    --- FSR 2.1 on/off.
    ---
    ---@param value boolean
    function RenderPath3D.SetFSR2Enabled(value) end

    --- FSR 2.1 sharpness 0: least sharp, 1: sharpest (this is different to FSR
    --- 1.0).
    ---
    ---@param value number
    function RenderPath3D.SetFSR2Sharpness(value) end

    --- FSR 2.1 preset will modify resolution scaling and sampler LOD bias.
    ---
    ---@param value FSR2_Preset
    function RenderPath3D.SetFSR2Preset(value) end

    --- Set a tonemap type.
    ---
    ---@param value Tonemap
    function RenderPath3D.SetTonemap(value) end

    --- Enable visibility rendering mode, this renders the scene in compute
    --- shader instead of forward rendering. This can have performance
    --- improvement when triangle density on screen is very high.
    ---
    ---@param value boolean
    function RenderPath3D.SetVisibilityComputeShadingEnabled(value) end

    --- Sets cropping from left of the screen in logical units.
    ---
    ---@param value number
    function RenderPath3D.SetCropLeft(value) end

    --- Sets cropping from top of the screen in logical units.
    ---
    ---@param value number
    function RenderPath3D.SetCropTop(value) end

    --- Sets cropping from right of the screen in logical units.
    ---
    ---@param value number
    function RenderPath3D.SetCropRight(value) end

    --- Sets cropping from bottom of the screen in logical units.
    ---
    ---@param value number
    function RenderPath3D.SetCropBottom(value) end

    --- returns the last post process render texture.
    ---
    ---@return Texture
    function RenderPath3D.GetLastPostProcessRT() end

    --- Set a normal map texture as full screen distortion mask.
    ---
    ---@param texture Texture
    function RenderPath3D.SetDistortionOverlay(texture) end

    --- Enables or disables chromatic aberration.
    ---
    ---@param value boolean
    function RenderPath3D.SetChromaticAberrationEnabled(value) end

    --- Sets the chromatic aberration amount.
    ---
    ---@param value number
    function RenderPath3D.SetChromaticAberrationAmount(value) end

    --- Sets the eye adaption rate.
    ---
    ---@param value number
    function RenderPath3D.SetEyeAdaptionRate(value) end

    --- Sets the eye adaption key.
    ---
    ---@param value number
    function RenderPath3D.SetEyeAdaptionKey(value) end

    --- Sets the contrast.
    ---
    ---@param value number
    function RenderPath3D.SetContrast(value) end

    --- Sets the saturation.
    ---
    ---@param value number
    function RenderPath3D.SetSaturation(value) end

    --- Sets the brightness.
    ---
    ---@param value number
    function RenderPath3D.SetBrightness(value) end

    --- Sets the light shafts fade speed.
    ---
    ---@param value number
    function RenderPath3D.SetLightShaftsFadeSpeed(value) end

    --- Enables or disables mesh blend.
    ---
    ---@param value boolean
    function RenderPath3D.SetMeshBlendEnabled(value) end

    --- Enables or disables occlusion culling.
    ---
    ---@param value boolean
    function RenderPath3D.SetOcclusionCullingEnabled(value) end

    --- Sets the ssgidepth rejection.
    ---
    ---@param value number
    function RenderPath3D.SetSSGIDepthRejection(value) end

    --- Quality presets for AMD FidelityFX Super Resolution 2 (FSR2) temporal
    --- upscaling.
    ---
    ---@enum FSR2_Preset
    FSR2_Preset = {
        Quality = 0, -- 1.5x scaling, -1.58 sampler LOD bias
        Balanced = 1, -- 1.7x scaling, -1.76 sampler LOD bias
        Performance = 2, -- 2.0x scaling, -2.0 sampler LOD bias
        Ultra_Performance = 3, -- 3.0x scaling, -2.58 sampler LOD bias
        Reinhard = 0,
        ACES = 1,
    }

    --- Tone mapping operator used for HDR to LDR conversion.
    ---
    ---@enum Tonemap
    Tonemap = {
        Reinhard = 0,
        ACES = 1,
    }
```

##### LoadingScreen : RenderPath2D

```lua
    --- Creates a LoadingScreen render path that loads resources in the
    --- background and can display loading progress.
    ---
    ---@return LoadingScreen
    function LoadingScreen() end

    --- A RenderPath2D but one that internally manages resource loading and can
    --- display information about the process. It inherits functions from
    --- RenderPath2D.
    ---
    ---@class LoadingScreen : RenderPath2D
    local LoadingScreen = {}

    --- Adds a scene loading task into the global scene and returns the root
    --- entity handle immediately. The loading task will be started
    --- asynchronously when the LoadingScreen is activated by the Application.
    ---
    ---@param fileName string
    ---@param matrix? Matrix
    ---
    ---@return integer
    function LoadingScreen.AddLoadModelTask(fileName, matrix) end

    --- Adds a scene loading task into the specified scene and returns the root
    --- entity handle immediately. The loading task will be started
    --- asynchronously when the LoadingScreen is activated by the Application.
    ---
    ---@param scene Scene
    ---@param fileName string
    ---@param matrix? Matrix
    ---
    ---@return integer
    function LoadingScreen.AddLoadModelTask(scene, fileName, matrix) end

    --- Loads resources of a RenderPath and activates it after all loading tasks
    --- have finished.
    ---
    ---@param path RenderPath
    ---@param app Application
    ---@param fadeSeconds? number
    ---@param fadeR? integer
    ---@param fadeG? integer
    ---@param fadeB? integer
    ---@param fadetype? FadeType
    function LoadingScreen.AddRenderPathActivationTask(
        path,
        app,
        fadeSeconds,
        fadeR,
        fadeG,
        fadeB,
        fadetype
    ) end

    --- Returns true when all loading tasks have finished and loading screen is
    --- stopped (eg. application swapped it out).
    ---
    ---@return boolean
    function LoadingScreen.IsFinished() end

    --- Returns percentage of loading complete (0% - 100%). When all loading
    --- tasks are finished or there are no tasks, it returns 100.
    ---
    ---@return integer
    function LoadingScreen.GetProgress() end

    --- Set a full screen background texture that wil be displayed when loading
    --- screen is active.
    ---
    ---@param tex Texture
    function LoadingScreen.SetBackgroundTexture(tex) end

    --- Returns the background texture.
    ---
    ---@return Texture
    function LoadingScreen.GetBackgroundTexture() end

    --- Sets the alignment of the background image.
    ---
    ---@param mode BackgroundMode
    function LoadingScreen.SetBackgroundMode(mode) end

    --- Returns the background mode.
    ---
    ---@return integer
    function LoadingScreen.GetBackgroundMode() end

    --- Controls how a fullscreen background image is fitted to the screen.
    ---
    ---@enum BackgroundMode
    BackgroundMode = {
        -- fill the whole screen, will cut off parts of the image if aspects
        -- don't match
        Fill = 0,
        -- fit the image completely inside the screen, will result in black bars
        -- on screen if aspects don't match
        Fit = 1,
        Stretch = 2, -- fill the whole screen, and stretch the image if needed
    }
```

### Primitives

#### Ray

```lua
    --- Creates a ray from an origin and a direction, with optional minimum and
    --- maximum distance along the ray.
    ---
    ---@param origin Vector
    ---@param direction Vector
    ---@param Tmin? number
    ---@param tmax? number
    ---
    ---@return Ray
    function Ray(origin, direction, Tmin, tmax) end

    --- A ray defined by an origin Vector and a normalized direction Vector. It
    --- can be used to intersect with other primitives or the scene.
    ---
    ---@class Ray
    ---
    ---@field Origin Vector
    ---@field Direction Vector
    local Ray = {}

    --- Returns whether the ray intersects the given AABB.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function Ray.Intersects(aabb) end

    --- Returns whether the ray intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean
    function Ray.Intersects(sphere) end

    --- Returns whether the ray intersects the given capsule.
    ---
    ---@param capsule Capsule
    ---
    ---@return boolean
    function Ray.Intersects(capsule) end

    --- Returns the point where the ray intersects the given plane.
    ---
    ---@param plane Vector
    ---
    ---@return Vector point
    function Ray.Intersects(plane) end

    --- Returns the origin.
    ---
    ---@return Vector
    function Ray.GetOrigin() end

    --- Returns the direction.
    ---
    ---@return Vector
    function Ray.GetDirection() end

    --- Sets the origin.
    ---
    ---@param vector Vector
    function Ray.SetOrigin(vector) end

    --- Sets the direction.
    ---
    ---@param vector Vector
    function Ray.SetDirection(vector) end

    --- Creates a ray from two points. Point a will be the ray origin, pointing
    --- towards point b.
    ---
    ---@param a Vector
    ---@param b Vector
    function Ray.CreateFromPoints(a, b) end

    --- Compute placement orientation matrix at intersection result. This matrix
    --- can be used to place entities in the scene oriented on the surface.
    ---
    ---@param position Vector
    ---@param normal Vector
    ---
    ---@return Matrix
    function Ray.GetPlacementOrientation(position, normal) end
```

#### AABB

```lua
    --- If no argument is given, it will be infinitely inverse that can't
    --- intersect.
    ---
    ---@param min? Vector
    ---@param max Vector
    ---
    ---@return AABB
    function AABB(min, max) end

    --- Axis Aligned Bounding Box. Can be intersected with other primitives.
    ---
    ---@class AABB
    ---
    ---@field Min Vector
    ---@field Max Vector
    local AABB = {}


    --- Omit the z component for intersection check for more precise 2D
    --- intersection.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function AABB.Intersects2D(aabb) end

    --- Returns whether this AABB intersects the given AABB.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function AABB.Intersects(aabb) end

    --- Returns whether this AABB intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean
    function AABB.Intersects(sphere) end

    --- Returns whether this AABB intersects the given ray.
    ---
    ---@param ray Ray
    ---
    ---@return boolean
    function AABB.Intersects(ray) end

    --- Returns whether the given point is inside this AABB.
    ---
    ---@param point Vector
    ---
    ---@return boolean
    function AABB.Intersects(point) end

    --- Returns the minimum corner of the box.
    ---
    ---@return Vector
    function AABB.GetMin() end

    --- Returns the maximum corner of the box.
    ---
    ---@return Vector
    function AABB.GetMax() end

    --- Sets the minimum corner of the box.
    ---
    ---@param vector Vector
    function AABB.SetMin(vector) end

    --- Sets the maximum corner of the box.
    ---
    ---@param vector Vector
    function AABB.SetMax(vector) end

    --- Returns the center point of the box.
    ---
    ---@return Vector
    function AABB.GetCenter() end

    --- Returns the half-extents (half the size along each axis) of the box.
    ---
    ---@return Vector
    function AABB.GetHalfExtents() end

    --- Transforms the AABB with a matrix and returns the resulting conservative
    --- AABB.
    ---
    ---@param matrix Matrix
    ---
    ---@return AABB
    function AABB.Transform(matrix) end

    --- Get a matrix that represents the AABB as OBB (oriented bounding box).
    ---
    ---@return Matrix
    function AABB.GetAsBoxMatrix() end

    --- Projects the AABB to the screen, returns a 2D rectangle in UV-space as
    --- Vector(topleftX, topleftY, bottomrightX, bottomrightY), each value is in
    --- range [0, 1].
    ---
    ---@param ViewProjection Matrix
    ---
    ---@return Vector
    function AABB.ProjectToScreen(ViewProjection) end
```

#### Sphere

```lua
    --- Creates a sphere from a center point and a radius.
    ---
    ---@param center Vector
    ---@param radius number
    ---
    ---@return Sphere
    function Sphere(center, radius) end

    --- Sphere defined by center Vector and radius. Can be intersected with
    --- other primitives.
    ---
    ---@class Sphere
    ---
    ---@field Center Vector
    ---@field Radius number
    local Sphere = {}

    --- Returns whether this sphere intersects the given AABB.
    ---
    ---@param aabb AABB
    ---
    ---@return boolean
    function Sphere.Intersects(aabb) end

    --- Returns whether this sphere intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean
    function Sphere.Intersects(sphere) end

    --- Returns whether this sphere intersects the given capsule.
    ---
    ---@param capsule Capsule
    ---
    ---@return boolean
    function Sphere.Intersects(capsule) end

    --- Returns whether this sphere intersects the given ray.
    ---
    ---@param ray Ray
    ---
    ---@return boolean
    function Sphere.Intersects(ray) end

    --- Returns whether the given point is inside this sphere.
    ---
    ---@param point Vector
    ---
    ---@return boolean
    function Sphere.Intersects(point) end

    --- Returns the center of the sphere.
    ---
    ---@return Vector
    function Sphere.GetCenter() end

    --- Returns the radius of the sphere.
    ---
    ---@return number
    function Sphere.GetRadius() end

    --- Sets the center of the sphere.
    ---
    ---@param value Vector
    function Sphere.SetCenter(value) end

    --- Sets the radius of the sphere.
    ---
    ---@param value number
    function Sphere.SetRadius(value) end

    --- Compute placement orientation matrix at intersection result. This matrix
    --- can be used to place entities in the scene oriented on the surface.
    ---
    ---@param position Vector
    ---@param normal Vector
    ---
    ---@return Matrix
    function Sphere.GetPlacementOrientation(position, normal) end
```

#### Capsule

```lua
    --- Creates a capsule from a base point, a tip point and a radius.
    ---
    ---@param base Vector
    ---@param tip Vector
    ---@param radius number
    ---
    ---@return Capsule
    function Capsule(base, tip, radius) end

    --- It's like two spheres connected by a cylinder. Base and Tip are the two
    --- endpoints, radius is the cylinder's radius.
    ---
    ---@class Capsule
    ---
    ---@field Base Vector
    ---@field Tip Vector
    ---@field Radius number
    local Capsule = {}

    --- Returns whether this capsule intersects the given capsule.
    ---
    ---@param other Capsule
    ---
    ---@return boolean intersects
    ---@return Vector position
    ---@return Vector normal
    ---@return number depth
    function Capsule.Intersects(other) end

    --- Returns whether this capsule intersects the given sphere.
    ---
    ---@param sphere Sphere
    ---
    ---@return boolean intersects
    ---@return number depth
    ---@return Vector normal
    function Capsule.Intersects(sphere) end

    --- Returns whether this capsule intersects the given ray.
    ---
    ---@param ray Ray
    ---
    ---@return boolean
    function Capsule.Intersects(ray) end

    --- Returns whether the given point is inside this capsule.
    ---
    ---@param point Vector
    ---
    ---@return boolean
    function Capsule.Intersects(point) end

    --- Returns the base point of the capsule.
    ---
    ---@return Vector
    function Capsule.GetBase() end

    --- Returns the tip point of the capsule.
    ---
    ---@return Vector
    function Capsule.GetTip() end

    --- Returns the radius of the capsule.
    ---
    ---@return number
    function Capsule.GetRadius() end

    --- Sets the base point of the capsule.
    ---
    ---@param value Vector
    function Capsule.SetBase(value) end

    --- Sets the tip point of the capsule.
    ---
    ---@param value Vector
    function Capsule.SetTip(value) end

    --- Sets the radius of the capsule.
    ---
    ---@param value number
    function Capsule.SetRadius(value) end

    --- Compute placement orientation matrix at intersection result. This matrix
    --- can be used to place entities in the scene oriented on the surface.
    ---
    ---@param position Vector
    ---@param normal Vector
    ---
    ---@return Matrix
    function Capsule.GetPlacementOrientation(position, normal) end

    --- Returns the axis-aligned bounding box of the capsule.
    ---
    ---@return AABB
    function Capsule.GetAABB() end
```

### Input

```lua
    --- Creates an Input object. Use the global `input` instead of constructing
    --- one.
    ---
    ---@return Input
    function Input() end

    --- Provides access to keyboard, mouse, touch and gamepad input. Use the
    --- global `input` object.
    ---
    ---@class Input
    local Input = {}

    --- Use this global object to access input functions.
    ---
    ---@type Input
    input = nil

    --- Check whether a button is currently being held down.
    ---
    ---@param code integer
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Down(code, playerindex) end

    --- Check whether a button has just been pushed that wasn't before.
    ---
    ---@param code integer
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Press(code, playerindex) end

    --- Check whether a button has just been released that was down before.
    ---
    ---@param code integer
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Release(code, playerindex) end

    --- Check whether a button was being held down for a specific duration
    --- (number of frames). If continuous == true, than it will also return true
    --- after the duration was reached.
    ---
    ---@param code integer
    ---@param duration? integer
    ---@param continuous? boolean
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Hold(code, duration, continuous, playerindex) end

    --- Get mouse pointer or primary touch position (x, y). Also returns mouse
    --- wheel delta movement (z), and pen pressure (w).
    ---
    ---@return Vector
    function Input.GetPointer() end

    --- Set mouse position.
    ---
    ---@param pos Vector
    function Input.SetPointer(pos) end

    --- Native delta mouse movement.
    ---
    ---@return Vector
    function Input.GetPointerDelta() end

    --- Hide Pointer.
    ---
    ---@param visible boolean
    function Input.HidePointer(visible) end

    --- Read analog data from gamepad. type parameter must be from
    --- GAMEPAD_ANALOG values.
    ---
    ---@param type integer
    ---@param playerindex? integer
    ---
    ---@return Vector
    function Input.GetAnalog(type, playerindex) end

    --- Returns the touches.
    ---
    ---@return Touch
    function Input.GetTouches() end

    --- Sets controller feedback such as vibration or LED color.
    ---
    ---@param feedback ControllerFeedback
    ---@param playerindex? integer
    function Input.SetControllerFeedback(feedback, playerindex) end

    --- Returns 0 (`BUTTON_NONE`) if nothing is pressed, or the first
    --- appropriate button code otherwise.
    ---
    ---@param playerindex? integer
    ---
    ---@return integer
    function Input.WhatIsPressed(playerindex) end

    --- Returns whether that button code is a gamepad button or not.
    ---
    ---@param button integer
    ---
    ---@return boolean
    function Input.IsGamepadButton(button) end

    --- Returns button code for a given string name.
    ---
    ---@param str string
    ---
    ---@return integer
    function Input.StringToButton(str) end

    --- Returns string name for the given button code. You can set a preference
    --- for controller type which can modify the string returned.
    ---
    ---@param button integer
    ---@param preference? integer
    ---
    ---@return any
    function Input.ButtonToString(button, preference) end

    --- Sets the current cursor type. Values can be of the cursor values, see
    --- below.
    ---
    ---@param cursor integer
    function Input.SetCursor(cursor) end

    --- Sets the specified cursor type to an image from a cursor file.
    ---
    ---@param cursor integer
    ---@param filename string
    function Input.SetCursorFromFile(cursor, filename) end

    --- Resets the specified cursor to the default one.
    ---
    ---@param cursor integer
    function Input.ResetCursor(cursor) end

    --- Resets all cursors to the defaults.
    function Input.ResetCursors() end

    --- Returns the parameters of the current touch pinch gesture: the pinch
    --- center position, the cumulative scale, and the scale change this frame.
    ---
    ---@return Vector position
    ---@return number scale
    ---@return number delta_scale
    function Input.GetTouchPinch() end

    --- Returns the touch pan gesture delta movement.
    ---
    ---@return Vector
    function Input.GetTouchPan() end

    --- Returns true if a pan touch gesture is currently active.
    ---
    ---@return boolean
    function Input.IsTouchPanning() end

    --- Returns true if a new pan touch gesture is starting this frame.
    ---
    ---@return boolean
    function Input.IsTouchPanStarting() end
```

#### ControllerFeedback

```lua
    --- Controller Feedback.
    ---
    ---@return ControllerFeedback
    function ControllerFeedback() end

    --- Describes controller feedback such as touch and LED color which can be
    --- replayed on a controller.
    ---
    ---@class ControllerFeedback
    local ControllerFeedback = {}

    --- Vibration amount of left motor (0: no vibration, 1: max vibration).
    ---
    ---@param value number
    function ControllerFeedback.SetVibrationLeft(value) end

    --- Vibration amount of right motor (0: no vibration, 1: max vibration).
    ---
    ---@param value number
    function ControllerFeedback.SetVibrationRight(value) end

    --- Sets the colored LED color if controller has one.
    ---
    ---@param color Vector
    function ControllerFeedback.SetLEDColor(color) end

    --- Sets the colored LED color if controller has one (ABGR hex color code).
    ---
    ---@param hexcolor integer
    function ControllerFeedback.SetLEDColor(hexcolor) end
```

#### Touch

```lua
    --- Creates a Touch contact descriptor.
    ---
    ---@return Touch
    function Touch() end

    --- Describes a touch contact point
    ---
    ---@class Touch
    local Touch = {}

    --- Returns the touch state (one of the TOUCHSTATE_* values).
    ---
    ---@return integer
    function Touch.GetState() end

    --- Returns the touch position.
    ---
    ---@return Vector
    function Touch.GetPos() end
```

#### TOUCHSTATE

```lua
    --- TOUCHSTATE PRESSED.
    ---
    ---@type integer
    TOUCHSTATE_PRESSED = 0

    --- TOUCHSTATE RELEASED.
    ---
    ---@type integer
    TOUCHSTATE_RELEASED = 1

    --- TOUCHSTATE MOVED.
    ---
    ---@type integer
    TOUCHSTATE_MOVED = 2
```

#### Keyboard Key codes

You can also generate a key code by calling string.byte(char uppercaseLetter)
where the parameter represents the desired key of the keyboard.

```lua
    --- No button (sentinel value for "no input").
    ---
    ---@type integer
    BUTTON_NONE = 0

    --- KEYBOARD BUTTON UP.
    ---
    ---@type integer
    KEYBOARD_BUTTON_UP = 518

    --- KEYBOARD BUTTON DOWN.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DOWN = 519

    --- KEYBOARD BUTTON LEFT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_LEFT = 520

    --- KEYBOARD BUTTON RIGHT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_RIGHT = 521

    --- KEYBOARD BUTTON SPACE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_SPACE = 522

    --- KEYBOARD BUTTON RSHIFT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_RSHIFT = 523

    --- KEYBOARD BUTTON LSHIFT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_LSHIFT = 524

    --- KEYBOARD BUTTON F1.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F1 = 525

    --- KEYBOARD BUTTON F2.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F2 = 526

    --- KEYBOARD BUTTON F3.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F3 = 527

    --- KEYBOARD BUTTON F4.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F4 = 528

    --- KEYBOARD BUTTON F5.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F5 = 529

    --- KEYBOARD BUTTON F6.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F6 = 530

    --- KEYBOARD BUTTON F7.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F7 = 531

    --- KEYBOARD BUTTON F8.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F8 = 532

    --- KEYBOARD BUTTON F9.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F9 = 533

    --- KEYBOARD BUTTON F10.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F10 = 534

    --- KEYBOARD BUTTON F11.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F11 = 535

    --- KEYBOARD BUTTON F12.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F12 = 536

    --- KEYBOARD BUTTON ENTER.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ENTER = 537

    --- KEYBOARD BUTTON ESCAPE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ESCAPE = 538

    --- KEYBOARD BUTTON BACK (backspace).
    ---
    ---@type integer
    KEYBOARD_BUTTON_BACK = 543

    --- KEYBOARD BUTTON HOME.
    ---
    ---@type integer
    KEYBOARD_BUTTON_HOME = 539

    --- KEYBOARD BUTTON RCONTROL.
    ---
    ---@type integer
    KEYBOARD_BUTTON_RCONTROL = 540

    --- KEYBOARD BUTTON LCONTROL.
    ---
    ---@type integer
    KEYBOARD_BUTTON_LCONTROL = 541

    --- KEYBOARD BUTTON DELETE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DELETE = 542

    --- KEYBOARD BUTTON BACKSPACE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_BACKSPACE = 0

    --- KEYBOARD BUTTON PAGEDOWN.
    ---
    ---@type integer
    KEYBOARD_BUTTON_PAGEDOWN = 544

    --- KEYBOARD BUTTON PAGEUP.
    ---
    ---@type integer
    KEYBOARD_BUTTON_PAGEUP = 545

    --- KEYBOARD BUTTON NUMPAD0.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD0 = 546

    --- KEYBOARD BUTTON NUMPAD1.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD1 = 547

    --- KEYBOARD BUTTON NUMPAD2.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD2 = 548

    --- KEYBOARD BUTTON NUMPAD3.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD3 = 549

    --- KEYBOARD BUTTON NUMPAD4.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD4 = 550

    --- KEYBOARD BUTTON NUMPAD5.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD5 = 551

    --- KEYBOARD BUTTON NUMPAD6.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD6 = 552

    --- KEYBOARD BUTTON NUMPAD7.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD7 = 553

    --- KEYBOARD BUTTON NUMPAD8.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD8 = 554

    --- KEYBOARD BUTTON NUMPAD9.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD9 = 555

    --- KEYBOARD BUTTON MULTIPLY.
    ---
    ---@type integer
    KEYBOARD_BUTTON_MULTIPLY = 556

    --- KEYBOARD BUTTON ADD.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ADD = 557

    --- KEYBOARD BUTTON SEPARATOR.
    ---
    ---@type integer
    KEYBOARD_BUTTON_SEPARATOR = 558

    --- KEYBOARD BUTTON SUBTRACT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_SUBTRACT = 559

    --- KEYBOARD BUTTON DECIMAL.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DECIMAL = 560

    --- KEYBOARD BUTTON DIVIDE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DIVIDE = 561

    --- KEYBOARD BUTTON TAB.
    ---
    ---@type integer
    KEYBOARD_BUTTON_TAB = 562

    --- KEYBOARD BUTTON TILDE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_TILDE = 563

    --- KEYBOARD BUTTON INSERT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_INSERT = 564

    --- KEYBOARD BUTTON ALT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ALT = 565

    --- KEYBOARD BUTTON ALTGR.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ALTGR = 566
```

#### Mouse Key Codes

Helpers to check mouse wheel scrolling like buttons.

```lua
    --- MOUSE BUTTON LEFT.
    ---
    ---@type integer
    MOUSE_BUTTON_LEFT = 513

    --- MOUSE BUTTON RIGHT.
    ---
    ---@type integer
    MOUSE_BUTTON_RIGHT = 514

    --- MOUSE BUTTON MIDDLE.
    ---
    ---@type integer
    MOUSE_BUTTON_MIDDLE = 515

    --- MOUSE SCROLL AS BUTTON UP.
    ---
    ---@type integer
    MOUSE_SCROLL_AS_BUTTON_UP = 516

    --- MOUSE SCROLL AS BUTTON DOWN.
    ---
    ---@type integer
    MOUSE_SCROLL_AS_BUTTON_DOWN = 517
```

#### Gamepad Key Codes

Generic codes work across controllers; the Xbox and PlayStation codes below
are aliases mapping to the generic ones.

##### Generic button codes

```lua
    --- Gamepad D-pad up.
    ---
    ---@type integer
    GAMEPAD_BUTTON_UP = 257

    --- Gamepad D-pad down.
    ---
    ---@type integer
    GAMEPAD_BUTTON_DOWN = 259

    --- Gamepad D-pad left.
    ---
    ---@type integer
    GAMEPAD_BUTTON_LEFT = 258

    --- Gamepad D-pad right.
    ---
    ---@type integer
    GAMEPAD_BUTTON_RIGHT = 260

    --- Generic gamepad button 1.
    ---
    ---@type integer
    GAMEPAD_BUTTON_1 = 261

    --- Generic gamepad button 2.
    ---
    ---@type integer
    GAMEPAD_BUTTON_2 = 262

    --- Generic gamepad button 3.
    ---
    ---@type integer
    GAMEPAD_BUTTON_3 = 263

    --- Generic gamepad button 4.
    ---
    ---@type integer
    GAMEPAD_BUTTON_4 = 264

    --- Generic gamepad button 5.
    ---
    ---@type integer
    GAMEPAD_BUTTON_5 = 265

    --- Generic gamepad button 6.
    ---
    ---@type integer
    GAMEPAD_BUTTON_6 = 266

    --- Generic gamepad button 7.
    ---
    ---@type integer
    GAMEPAD_BUTTON_7 = 267

    --- Generic gamepad button 8.
    ---
    ---@type integer
    GAMEPAD_BUTTON_8 = 268

    --- Generic gamepad button 9.
    ---
    ---@type integer
    GAMEPAD_BUTTON_9 = 269

    --- Generic gamepad button 10.
    ---
    ---@type integer
    GAMEPAD_BUTTON_10 = 270

    --- Generic gamepad button 11.
    ---
    ---@type integer
    GAMEPAD_BUTTON_11 = 271

    --- Generic gamepad button 12.
    ---
    ---@type integer
    GAMEPAD_BUTTON_12 = 272

    --- Generic gamepad button 13.
    ---
    ---@type integer
    GAMEPAD_BUTTON_13 = 273

    --- Generic gamepad button 14.
    ---
    ---@type integer
    GAMEPAD_BUTTON_14 = 274

    -- Analog sticks and triggers exposed as digital buttons:

    --- Left thumbstick pushed up, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_UP = 275

    --- Left thumbstick pushed down, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_DOWN = 277

    --- Left thumbstick pushed left, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_LEFT = 276

    --- Left thumbstick pushed right, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_RIGHT = 278

    --- Right thumbstick pushed up, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_UP = 279

    --- Right thumbstick pushed down, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_DOWN = 281

    --- Right thumbstick pushed left, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_LEFT = 280

    --- Right thumbstick pushed right, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_RIGHT = 282

    --- Left trigger, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON = 283

    --- Right trigger, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON = 284
```

##### Xbox button codes

```lua
    --- Xbox X button (alias of GAMEPAD_BUTTON_1).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_X = 261

    --- Xbox A button (alias of GAMEPAD_BUTTON_2).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_A = 262

    --- Xbox B button (alias of GAMEPAD_BUTTON_3).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_B = 263

    --- Xbox Y button (alias of GAMEPAD_BUTTON_4).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_Y = 264

    --- Xbox L1 button (alias of GAMEPAD_BUTTON_5).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_L1 = 265

    --- Xbox LT button (alias of GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_LT = 283

    --- Xbox R1 button (alias of GAMEPAD_BUTTON_6).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_R1 = 266

    --- Xbox RT button (alias of GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_RT = 284

    --- Xbox L3 button (alias of GAMEPAD_BUTTON_7).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_L3 = 267

    --- Xbox R3 button (alias of GAMEPAD_BUTTON_8).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_R3 = 268

    --- Xbox BACK button (alias of GAMEPAD_BUTTON_9).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_BACK = 269

    --- Xbox START button (alias of GAMEPAD_BUTTON_10).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_START = 270
```

##### PlayStation button codes

```lua
    --- PlayStation SQUARE button (alias of GAMEPAD_BUTTON_1).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_SQUARE = 261

    --- PlayStation CROSS button (alias of GAMEPAD_BUTTON_2).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_CROSS = 262

    --- PlayStation CIRCLE button (alias of GAMEPAD_BUTTON_3).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_CIRCLE = 263

    --- PlayStation TRIANGLE button (alias of GAMEPAD_BUTTON_4).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_TRIANGLE = 264

    --- PlayStation L1 button (alias of GAMEPAD_BUTTON_5).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_L1 = 265

    --- PlayStation L2 button (alias of GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_L2 = 283

    --- PlayStation R1 button (alias of GAMEPAD_BUTTON_6).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_R1 = 266

    --- PlayStation R2 button (alias of GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_R2 = 284

    --- PlayStation L3 button (alias of GAMEPAD_BUTTON_7).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_L3 = 267

    --- PlayStation R3 button (alias of GAMEPAD_BUTTON_8).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_R3 = 268

    --- PlayStation SHARE button (alias of GAMEPAD_BUTTON_9).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_SHARE = 269

    --- PlayStation OPTION button (alias of GAMEPAD_BUTTON_10).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_OPTION = 270

    --- PlayStation SELECT button (alias of GAMEPAD_BUTTON_PLAYSTATION_SHARE).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_SELECT = 269

    --- PlayStation START button (alias of GAMEPAD_BUTTON_PLAYSTATION_OPTION).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_START = 270
```

#### Gamepad Analog Codes

```lua
    --- GAMEPAD ANALOG THUMBSTICK L.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L = 0

    --- GAMEPAD ANALOG THUMBSTICK R.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R = 1

    --- GAMEPAD ANALOG TRIGGER L.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_L = 2

    --- GAMEPAD ANALOG TRIGGER R.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_R = 3
```

#### Controller preference

```lua
    --- CONTROLLER PREFERENCE GENERIC.
    ---
    ---@type integer
    CONTROLLER_PREFERENCE_GENERIC = 0

    --- CONTROLLER PREFERENCE PLAYSTATION.
    ---
    ---@type integer
    CONTROLLER_PREFERENCE_PLAYSTATION = 1

    --- CONTROLLER PREFERENCE XBOX.
    ---
    ---@type integer
    CONTROLLER_PREFERENCE_XBOX = 2
```

#### Cursor codes

```lua
    --- CURSOR DEFAULT.
    ---
    ---@type integer
    CURSOR_DEFAULT = 0

    --- CURSOR TEXTINPUT.
    ---
    ---@type integer
    CURSOR_TEXTINPUT = 1

    --- CURSOR RESIZEALL.
    ---
    ---@type integer
    CURSOR_RESIZEALL = 2

    --- CURSOR RESIZE NS.
    ---
    ---@type integer
    CURSOR_RESIZE_NS = 3

    --- CURSOR RESIZE EW.
    ---
    ---@type integer
    CURSOR_RESIZE_EW = 4

    --- CURSOR RESIZE NESW.
    ---
    ---@type integer
    CURSOR_RESIZE_NESW = 5

    --- CURSOR RESIZE NWSE.
    ---
    ---@type integer
    CURSOR_RESIZE_NWSE = 6

    --- CURSOR HAND.
    ---
    ---@type integer
    CURSOR_HAND = 7

    --- CURSOR NOTALLOWED.
    ---
    ---@type integer
    CURSOR_NOTALLOWED = 8

    --- CURSOR CROSS.
    ---
    ---@type integer
    CURSOR_CROSS = 9
```

### Physics

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

#### PickDragOperation

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

### Path finding

#### VoxelGrid

```lua
    --- Creates a voxel grid of the given dimensions, used for navigation and
    --- voxel ray queries.
    ---
    ---@param dimX? integer
    ---@param dimY integer
    ---@param dimZ integer
    ---
    ---@return VoxelGrid
    function VoxelGrid(dimX, dimY, dimZ) end

    --- Path finding operations can be made by using a voxel grid and path
    --- queries. The voxel grid can store spatial information about a scene, or
    --- a part of the scene, while the path query manages the path finding
    --- result from a point to a different point within the voxel grid.
    ---
    ---@class VoxelGrid
    local VoxelGrid = {}


    --- Allocates memory for dimX * dimY * dimZ number of voxels and initializes
    --- them to empty.
    ---
    ---@param dimX integer
    ---@param dimY integer
    ---@param dimZ integer
    function VoxelGrid.Init(dimX, dimY, dimZ) end

    --- Initializes all voxels to empty.
    function VoxelGrid.ClearData() end

    --- Places the voxel grid volume to fit to the given AABB. The number of
    --- voxels doesn't change, only the center and voxel size.
    ---
    ---@param aabb AABB
    function VoxelGrid.FromAABB(aabb) end

    --- Voxelizes triangle, and either adds it to the voxels (default), or
    --- removes voxels
    ---
    ---@param a Vector
    ---@param b Vector
    ---@param c Vector
    ---@param subtract? boolean
    function VoxelGrid.InjectTriangle(a, b, c, subtract) end

    --- Voxelizes axis aligned bounding box, and either adds it to the voxels
    --- (default), or removes voxels.
    ---
    ---@param aabb AABB
    ---@param subtract? boolean
    function VoxelGrid.InjectAABB(aabb, subtract) end

    --- Voxelizes sphere, and either adds it to the voxels (default), or removes
    --- voxels.
    ---
    ---@param sphere Sphere
    ---@param subtract? boolean
    function VoxelGrid.InjectSphere(sphere, subtract) end

    --- Voxelizes capsule, and either adds it to the voxels (default), or
    --- removes voxels.
    ---
    ---@param capsule Capsule
    ---@param subtract? boolean
    function VoxelGrid.InjectCapsule(capsule, subtract) end

    --- Converts a position in world space to voxel coordinate.
    ---
    ---@param pos Vector
    ---
    ---@return integer x
    ---@return integer y
    ---@return integer z
    function VoxelGrid.WorldToCoord(pos) end

    --- converts voxel coordinate to world space position
    ---
    ---@param x integer
    ---@param y integer
    ---@param z integer
    ---
    ---@return Vector
    function VoxelGrid.CoordToWorld(x, y, z) end

    --- Returns false if voxel is empty, true if it's valid.
    ---
    ---@param pos Vector
    ---
    ---@return boolean
    function VoxelGrid.CheckVoxel(pos) end

    --- Returns false if voxel is empty, true if it's valid.
    ---
    ---@param x integer
    ---@param y integer
    ---@param z integer
    ---
    ---@return boolean
    function VoxelGrid.CheckVoxel(x, y, z) end

    --- Sets a single voxel to the specified state.
    ---
    ---@param pos Vector
    ---@param value boolean
    function VoxelGrid.SetVoxel(pos, value) end

    --- Sets a single voxel to the specified state.
    ---
    ---@param x integer
    ---@param y integer
    ---@param z integer
    ---@param value boolean
    function VoxelGrid.SetVoxel(x, y, z, value) end

    --- Returns the center of the voxel grid volume.
    ---
    ---@return Vector
    function VoxelGrid.GetCenter() end

    --- Sets the center of the voxel grid volume.
    ---
    ---@param pos Vector
    function VoxelGrid.SetCenter(pos) end

    --- Get the half extent of one voxel in world space.
    ---
    ---@return Vector
    function VoxelGrid.GetVoxelSize() end

    --- Sets the half extent of one voxel in world space.
    ---
    ---@param voxelsize Vector
    function VoxelGrid.SetVoxelSize(voxelsize) end

    --- Sets the half extent of one voxel in world space uniformly in all
    --- dimension.
    ---
    ---@param voxelsize number
    function VoxelGrid.SetVoxelSize(voxelsize) end

    --- Returns color of debug visualization of voxels.
    ---
    ---@return Vector
    function VoxelGrid.GetDebugColor() end

    --- Set the color for debug visualization of voxels.
    ---
    ---@param color Vector
    function VoxelGrid.SetDebugColor(color) end

    --- Returns color of debug visualization of voxel grid extents.
    ---
    ---@return Vector
    function VoxelGrid.GetDebugColorExtent() end

    --- Set the color for debug visualization of voxel grid extents.
    ---
    ---@param color Vector
    function VoxelGrid.SetDebugColorExtent(color) end

    --- Returns the memory consumption of the voxel grid in bytes.
    ---
    ---@return integer
    function VoxelGrid.GetMemorySize() end

    --- Performs line of sight occlusion test from observer to subject voxel
    --- coordinates. Returns false if occlusion was found, true otherwise.
    ---
    ---@param observer_x integer
    ---@param observer_y integer
    ---@param observer_z integer
    ---@param subject_x integer
    ---@param subject_y integer
    ---@param subject_z integer
    ---
    ---@return boolean
    function VoxelGrid.IsVisible(
        observer_x,
        observer_y,
        observer_z,
        subject_x,
        subject_y,
        subject_z
    ) end

    --- Performs line of sight occlusion test from observer to subject world
    --- space points. Returns false if occlusion was found, true otherwise.
    ---
    ---@param observer AABB
    ---@param subject AABB
    ---
    ---@return boolean
    function VoxelGrid.IsVisible(observer, subject) end

    --- Performs line of sight occlusion test from observer world space point to
    --- subject AABB. Returns true if any of the AABB's touched voxels is
    --- visible, false otherwise.
    ---
    ---@param observer AABB
    ---@param subject AABB
    ---
    ---@return boolean
    function VoxelGrid.IsVisible(observer, subject) end

    --- Sets every empty voxel which is enclosed to solid.
    function VoxelGrid.FloodFill() end

    --- Adds the voxels of another grid into this one.
    ---
    ---@param other VoxelGrid
    function VoxelGrid.Add(other) end

    --- Removes the voxels of another grid from this one.
    ---
    ---@param other VoxelGrid
    function VoxelGrid.Subtract(other) end
```

#### PathQuery

```lua
    --- Creates a PathQuery for finding paths through a voxel grid.
    ---
    ---@return PathQuery
    function PathQuery() end

    --- Computes and stores a path through a VoxelGrid, for navigation and AI
    --- movement.
    ---
    ---@class PathQuery
    local PathQuery = {}


    --- Computes the path from start to goal on a voxel grid and stores the
    --- result.
    ---
    ---@param start Vector
    ---@param goal Vector
    ---@param voxelgrid VoxelGrid
    function PathQuery.Process(start, goal, voxelgrid) end

    --- Searches for a cover for subject position to hide from observer. The
    --- search will be in a specific direction, within the specified distance
    --- (approximately, within voxel precision).
    ---
    ---@param observer Vector
    ---@param subject Vector
    ---@param direction Vector
    ---@param max_distance number
    ---@param voxelgrid VoxelGrid
    function PathQuery.SearchCover(
        observer,
        subject,
        direction,
        max_distance,
        voxelgrid
    ) end

    --- Returns whether the last call to Process() was successfully able to find
    --- a path.
    ---
    ---@return boolean
    function PathQuery.IsSuccessful() end

    --- Get the next waypoint on the path from the starting location. This
    --- requires that Process() has been called beforehand.
    ---
    ---@return Vector
    function PathQuery.GetNextWaypoint() end

    --- Enable/disable waypoint debug rendering when using DrawPathQuery(). If
    --- enabled, voxel waypoints will be drawn in blue, simplified voxel
    --- waypoints will be drawn in pink.
    ---
    ---@param value boolean
    function PathQuery.SetDebugDrawWaypointsEnabled(value) end

    --- Enable/disable flying behavior. When flying is enabled, then the path
    --- will be on empty voxels (air), otherwise and by default the path will be
    --- on filled voxels (ground).
    ---
    ---@param value boolean
    function PathQuery.SetFlying(value) end

    --- Returns whether flying.
    ---
    ---@return boolean
    function PathQuery.IsFlying() end

    --- Set the navigation width requirement in voxels. This means how many
    --- voxels the query will keep away from obstacles horizontally.
    ---
    ---@param value integer
    function PathQuery.SetAgentWidth(value) end

    --- Returns the agent width.
    ---
    ---@param value integer
    ---
    ---@return integer
    function PathQuery.GetAgentWidth(value) end

    --- Set the navigation height requirement in voxels. This means how many
    --- voxels the query will keep away from obstacles vertically.
    ---
    ---@param value integer
    function PathQuery.SetAgentHeight(value) end

    --- Returns the agent height.
    ---
    ---@param value integer
    ---
    ---@return integer
    function PathQuery.GetAgentHeight(value) end

    --- Returns the number of waypoints that were computed in Process().
    ---
    ---@return integer
    function PathQuery.GetWaypointCount() end

    --- Returns the waypoint at specified index (direction: start -> goal).
    ---
    ---@param index integer
    ---
    ---@return Vector
    function PathQuery.GetWaypoint(index) end

    --- Returns the goal position.
    ---
    ---@return Vector
    function PathQuery.GetGoal() end
```

### TrailRenderer

```lua
    --- Trail Renderer.
    ---
    ---@return TrailRenderer
    function TrailRenderer() end

    --- Renders a smooth ribbon trail through a series of points, e.g. for
    --- projectiles or weapon swipes.
    ---
    ---@class TrailRenderer
    local TrailRenderer = {}

    --- Adds a new point to the trail. Note: if rotation is not specified, then
    --- point will be camera facing, otherwise UP direction will be rotated.
    ---
    ---@param pos Vector
    ---@param width? number
    ---@param color? Vector
    ---@param rotationQuaternion? Vector
    function TrailRenderer.AddPoint(pos, width, color, rotationQuaternion) end

    --- Cuts the trail at last point and starts a new trail. You can specify
    --- that this cut will create a loop of the previously added points.
    ---
    ---@param loop? boolean
    function TrailRenderer.Cut(loop) end

    --- Applies fade for the whole trail continuously, and removes segments that
    --- can be removed due to faded.
    ---
    ---@param amount number
    function TrailRenderer.Fade(amount) end

    --- Removes all points and cuts from the trail.
    ---
    function TrailRenderer.Clear() end

    --- Returns the number of points in the trail.
    ---
    ---@return integer
    function TrailRenderer.GetPointCount() end

    --- Returns the position, width and color of the trail point at the given
    --- index.
    ---
    ---@param index integer
    ---
    ---@return Vector position
    ---@return number width
    ---@return Vector color
    function TrailRenderer.GetPoint(index) end

    --- Sets the point parameters on the specified index.
    ---
    ---@param pos Vector
    ---@param width? number
    ---@param color? Vector
    function TrailRenderer.SetPoint(pos, width, color) end

    --- Set blend mode of the whole trail.
    ---
    ---@param blendmode integer
    function TrailRenderer.SetBlendMode(blendmode) end

    --- Returns the blend mode.
    ---
    ---@return integer
    function TrailRenderer.GetBlendMode() end

    --- Set the subdivision amount of the whole trail.
    ---
    ---@param subdiv integer
    function TrailRenderer.SetSubdivision(subdiv) end

    --- Returns the subdivision.
    ---
    ---@return integer
    function TrailRenderer.GetSubdivision() end

    --- Set the width of the whole trail.
    ---
    ---@param width number
    function TrailRenderer.SetWidth(width) end

    --- Returns the width.
    ---
    ---@return number
    function TrailRenderer.GetWidth() end

    --- Set the color of the whole trail.
    ---
    ---@param color Vector
    function TrailRenderer.SetColor(color) end

    --- Returns the color.
    ---
    ---@return Vector
    function TrailRenderer.GetColor() end

    --- Set the texture of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexture(tex) end

    --- Returns the texture.
    ---
    ---@return Texture
    function TrailRenderer.GetTexture() end

    --- Set the texture2 of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexture2(tex) end

    --- Returns the texture2.
    ---
    ---@return Texture
    function TrailRenderer.GetTexture2() end

    --- Set the texture UV tiling multiply-add value of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexMulAdd(tex) end

    --- Returns the tex mul add.
    ---
    ---@return Texture
    function TrailRenderer.GetTexMulAdd() end

    --- Set the texture2 UV tiling multiply-add value of the whole trail.
    ---
    ---@param tex Texture
    function TrailRenderer.SetTexMulAdd2(tex) end

    --- Returns the tex mul add2.
    ---
    ---@return Texture
    function TrailRenderer.GetTexMulAdd2() end

    --- Sets the depth soften amount (default = 10).
    ---
    ---@param value number
    function TrailRenderer.SetDepthSoften(value) end
```

### Network

Scripting handle for the engine's networking system. It is registered as the
`Network` class and exposed through the global `network` object.

> **Note:** the underlying `wi::network` C++ API (sockets, connections, send /
> receive) is **not** surfaced to Lua yet, so this object currently exposes no
> methods or properties. It is documented here for completeness; the type and
> global exist and can be referenced, but have no callable members until the
> engine binds them.

```lua
    --- Creates a Network object. Use the global `network` instead of
    --- constructing one.
    ---
    ---@return Network
    function Network() end

    --- Networking system handle. Currently a placeholder: the engine registers
    --- the type and the global `network` object, but exposes no scriptable
    --- methods or properties yet.
    ---
    ---@class Network
    local Network = {}

    --- Use this global object to access networking (no methods are bound yet).
    ---
    ---@type Network
    network = nil
```
