# High Level Interface

[← Back to Scripting API index](../index.md)

**This section must only be used from standalone lua scripts, and must not be
used from a ScriptComponent.** This is because ScriptComponent is always running
inside scene.Update(), and paths can not be switched at that time safely. On the
other hand, a standalone lua script can define its own update logic and render
path and change application behavior.

## Application

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

## RenderPath

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

### RenderPath2D : RenderPath

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

### RenderPath3D : RenderPath2D

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

### LoadingScreen : RenderPath2D

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
