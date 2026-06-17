# Renderer

[← Back to Scripting API index](../index.md)

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

## PaintTextureParams

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

## PaintDecalParams

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
