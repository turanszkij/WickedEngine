# Wicked Engine Scripting API Documentation
This is a collection and explanation of scripting features in Wicked Engine.
The documentation completion is still pending....

## Contents
1. Introduction and usage
2. Common Tools
3. Engine manipulation
	1. BackLog (Console)
	2. Renderer
4. Utility Tools
	1. Font
	2. Sprite
		1. ImageParams
		2. SpriteAnim
		3. MovingTexAnim
		4. DrawRecAnim
	3. Texture
	4. Sound
		1. SoundEffect
		2. Music
	5. Vector
	6. Matrix
	7. Scene
		TODO
	8. MainComponent
	9. RenderPath
		1. RenderPath2D
		2. RenderPath3D
			1. RenderPath3D_Forward
			2. RenderPath3D_Deferred
		4. LoadingScreen
	10. Network
		1. Server
		2. Client
	11. Input Handling
	12. ResourceManager
		
## Introduction and usage
Scripting in Wicked Engine is powered by Lua, meaning that the user can make use of the 
syntax and features of the widely used Lua language, accompanied by its fast interpreter.
Apart from the common features, certain engine features are also available to use.
You can load lua script files and execute them, or the engine scripting console (also known as the BackLog)
can also be used to execute single line scripts (or you can execute file scripts by the dofile command here).
Upon startup, the engine will try to load a startup script file named startup.lua in the root directory of 
the application. If not found, an error message will be thrown follwed by the normal execution by the program.
In the startup file, you can specify any startup logic, for example loading content or anything.

The setting up and usage of the BackLog is the responsibility of the target application, but the recommended way to set it up
is included in all of the demo projects. It is mapped to te HOME button by default.

## Common Tools
This section describes the common tools for scripting which are not necessarily engine features.
- signal(string name)
- waitSignal(string name)
- runProcess(function func)
- killProcesses()
- waitSeconds(float seconds)
- getprops(table object)
- len(table object)
- backlog_post_list(table list)
- fixedupdate()
- update()
- render() 
- getDeltaTime()
- math.lerp(float a,b,t)
- math.clamp(float x,min,max)
- math.saturate(float x)
- math.round(float x)

## Engine manipulation
The scripting API provides functions for the developer to manipulate engine behaviour or query it for information.

### BackLog
The scripting console of the engine. Input text with the keyboard, run the input with the RETURN key. The script errors
are also displayed here.
The scripting API provides some functions which manipulate the BackLog. These functions are in he global scope:
- backlog_clear()
- backlog_post(string params,,,)
- backlog_fontsize(int size)
- backlog_isactive() : boolean result
- backlog_fontrowspacing(int spacing)

### Renderer
This is the graphics renderer, which is also responsible for managing the scene graph which consists of keeping track of
parent-child relationships between the scene hierarchy, updating the world, animating armatures.
You can use the Renderer with the following functions, all of which are in the global scope:
- GetTransforms() : string result
- GetTransform(String name) : Transform? result
- GetArmatures() : string result
- GetArmature(String name) : Armature? result
- GetObjects() : string result
- GetObject(String name) : Object? result
- GetEmitters() : string result
- GetEmitter(string name) : Emitter? result1,result2,...
- GetMeshes() : string result
- GetLights() : string result
- GetMaterials() : string result
- GetGameSpeed() : float result
- SetResolutionScale(float scale)
- SetGamma(float gamma)
- SetGameSpeed(float speed)
- GetScreenWidth() : float result
- GetScreenHeight() : float result
- GetRenderWidth() : float result
- GetRenderHeight(): float result
- GetCameras() : string result
- GetCamera(opt String name) : Camera result		-- If string is provided, it will search a camera by name, otherwise, returns the main camera
- LoadModel(string fileName, opt Matrix transform) : int rootEntity	-- Load Model from file. returns a root entity that everything in this model is attached to
- LoadWorldInfo(string fileName)		-- Loads world information from file
- DuplicateInstance(Object object) : Object result		-- Copies the specified object in the scene as an instanced mesh
- SetEnvironmentMap(Texture cubemap)
- HairParticleSettings(opt int lod0, opt int lod1, opt int lod2)
- SetAlphaCompositionEnabled(opt bool enabled)
- SetShadowProps2D(int resolution, int count, int softShadowQuality)
- SetShadowPropsCube(int resolution, int count)
- SetDebugPartitionTreeEnabled(bool enabled)
- SetDebugBonesEnabled(bool enabled)
- SetDebugEittersEnabled(bool enabled)
- SetDebugForceFieldsEnabled(bool enabled)
- SetVSyncEnabled(opt bool enabled)
- SetOcclusionCullingEnabled(bool enabled)
- SetPhysicsParams(opt bool rigidBodyPhysicsEnabled, opt bool softBodyPhysicsEnabled, opt int softBodyIterationCount)
- Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask) : Object? object, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against
- DrawLine(Vector origin,end, opt Vector color)
- PutWaterRipple(String imagename, Vector position)
- PutDecal(Decal decal)
- PutEnvProbe(Vector pos)
- ClearWorld()
- ReloadShaders(opt string path)

## Utility Tools
The scripting API provides certain engine features to be used in development.
- Tip: You can inspect any object's functionality by calling 
getprops(YourObject) on them (where YourObject is the object which is to be inspected). The result will be displayed on the BackLog.

- Tags:
	- [constructor]				: The method is a constructor for the same type of object.
	- [void-constructor]		: The method is a constructor for the same type of object, but the object it creates is empty, so cannot be used.
	- [outer]					: The method or value is in the global scope, so not a method of any objects.

### Font
Gives you the ability to render text with a custom font.
- [constructor]Font(opt string text)
- SetStyle(string fontstyle, opt int size = 16)
- SetText(opt string text)
- SetSize(int size)
- SetPos(Vector pos)
- SetSpacing(Vector spacing)
- SetAlign(WIFALIGN Halign, opt WIFALIGN Valign)
	- [outer]WIFALIGN_LEFT : int
	- [outer]WIFALIGN_CENTER : int
	- [outer]WIFALIGN_MID : int
	- [outer]WIFALIGN_RIGHT : int
	- [outer]WIFALIGN_TOP : int
	- [outer]WIFALIGN_BOTTOM : int
- GetText() : string result
- GetSize() : int result
- GetPos() : Vector result
- GetSpacing() : Vector result
- GetAlign() : WIFALIGN halign,valign
- Destroy()

### Sprite
Render images on the screen.
- [constructor]Sprite(opt string texture, opt string maskTexture, opt string normalMapTexture)
- SetParams(ImageParams effects)
- GetParams() : ImageParams result
- SetAnim(SpriteAnim anim)
- GetAnim() : SpriteAnim result
- Destroy()

#### ImageParams
Specify Sprite properties, like position, size, etc.
- [constructor]ImageParams(opt float width, opt float height)
- [constructor]ImageParams(float posX, float posY, float width, opt float height)
- GetPos() : Vector result
- GetSize() : Vector result
- GetOpacity() : float result
- GetFade() : float result
- GetRotation() : float result
- GetMipLevel() : float result
- SetPos(Vector pos)
- SetSize(Vector size)
- SetOpacity(float opacity)
- SetFade(float fade)
- SetRotation(float rotation)
- SetMipLevel(float mipLevel)

#### SpriteAnim
Animate Sprites easily with this helper.
- [constructor]SpriteAnim()
- SetRot(float val)
- SetRotation(float val)
- SetOpacity(float val)
- SetFade(float val)
- SetRepeatable(boolean val)
- SetVelocity(Vector val)
- SetScaleX(float val)
- SetScaleY(float val)
- SetMovingTexAnim(MovingTexAnim val)
- SetDrawRectAnim(DrawRectAnim val)
- GetRot() : float result
- GetRotation() : float result
- GetOpacity() : float result
- GetFade() : float result
- GetRepeatable() : boolean result
- GetVelocity() : Vector result
- GetScaleX() : float result
- GetScaleY() : float result
- GetMovingTexAnim() : GetMovingTexAnim result
- GetDrawRectAnim() : GetDrawRectAnim result

#### MovingTexAnim
Move texture continuously along the sprite.
- [constructor]MovingTexAnim(opt float speedX,speedY)
- SetSpeedX(float val)
- SetSpeedY(float val)
- GetSpeedX() : float result
- GetSpeedY() : float result

#### DrawRecAnim
Animate sprite frame by frame.
- [constructor]DrawRecAnim()
- SetOnFrameChangeWait(float val)
- SetFrameRate(float val)
- SetFrameCount(int val)
- SetHorizontalFrameCount(int val)
- GetFrameRate() : float result
- GetFrameCount() : int result
- GetHorizontalFrameCount() : int result

### Texture
Just holds texture information in VRAM.
- [void-constructor]Texture()

### Sound
Load a Sound file, either sound effect or music.
- [outer]SoundVolume(opt float volume)
- [outer]MusicVolume(opt float volume)
- [void-constructor]Sound()
- Play(opt int delay)
- Stop()

#### SoundEffect
Sound Effects are for playing a sound file once. Inherits the methods from Sound.
- [constructor]SoundEffect(string soundFile)

#### Music
Music is for playing sound files in the background, along with sound effects. Inherits the methods from Sound.
- [constructor]Music(string soundFile)

### Vector
A four component floating point vector. Provides efficient calculations with SIMD support.
- [outer]vector
- [constructor]Vector(opt float x, opt float y, opt float z, opt float w)
- GetX() : float result
- GetY() : float result
- GetZ() : float result
- GetW() : float result
- SetX(float value)
- SetY(float value)
- SetZ(float value)
- SetW(float value)
- Transform(Matrix matrix)
- Length() : float result
- Normalize() : Vector result
- QuaternionNormalize() : Vector result
- Add(Vector v1,v2) : Vector result
- Subtract(Vector v1,v2) : Vector result
- Multiply(Vector v1,v2) : Vector result
- Multiply(Vector v1, float f) : Vector result
- Multiply(float f, Vector v1) : Vector result
- Dot(Vector v1,v2) : Vector result
- Cross(Vector v1,v2) : Vector result
- Lerp(Vector v1,v2, float t) : Vector result
- QuaternionMultiply(Vector v1,v2) : Vector result
- QuaternionFromRollPitchYaw(Vector rotXYZ) : Vector result
- QuaternionSlerp(Vector v1,v2, float t) : Vector result

### Matrix
A four by four matrix, efficient calculations with SIMD support.
- [outer]matrix
- [constructor]Matrix(opt float m00,m01,m02,m03,m10,m11,m12,m13,m20,m21,m22,m23,m30,m31,m32,m33)
- Translation(opt Vector vector) : Matrix result
- Rotation(opt Vector rollPitchYaw) : Matrix result
- RotationX(opt float angleInRadians) : Matrix result
- RotationY(opt float angleInRadians) : Matrix result
- RotationZ(opt float angleInRadians) : Matrix result
- RotationQuaternion(opt Vector quaternion) : Matrix result
- Scale(opt Vector scaleXYZ) : Matrix result
- LookTo(Vector eye,direction, opt Vector up) : Matrix result
- LookAt(Vector eye,focusPos, opt Vector up) : Matrix result
- Multiply(Matrix m1,m2) : Matrix result
- Add(Matrix m1,m2) : Matrix result
- Transpose(Matrix m) : Matrix result
- Inverse(Matrix m) : Matrix result, float determinant

### Scene
Manipulate the 3D scene with these components. TODO: rewrite this section


### MainComponent
The main component which holds information and manages the running of the current program.
- [outer]main : MainComponent
- [void-constructor]MainComponent()
- GetContent() : Resource? result
- GetActivePath() : RenderPath? result
- SetActivePath(RenderPath path, opt float fadeSeconds,fadeColorR,fadeColorG,fadeColorB)
- SetFrameSkip(bool enabled)
- SetInfoDisplay(bool active)
- SetWatermarkDisplay(bool active)
- SetFPSDisplay(bool active)
- [outer]SetProfilerEnabled(bool enabled)

### RenderPath
A RenderPath describes a scene wich can render itself.
- [constructor]RenderPath()
- GetContent() : Resource result
- Initialize()
- Load()
- Unload()
- Start()
- Stop()
- FixedUpdate()
- Update(opt float dt = 0)
- Render()
- Compose()
- OnStart(string task)
- OnStop(string task)
- GetLayerMask() : uint result
- SetLayerMask(uint mask)

#### RenderPath2D
It can hold Sprites and Fonts and can sort them by layers, update and render them.
- [constructor]RenderPath2D()
- AddSprite(Sprite sprite, opt string layer)
- AddFont(Font font, opt string layer)
- RemoveFont(Font font)
- ClearSprites()
- ClearFonts()
- GetSpriteOrder(Sprite sprite) : int? result
- GetFontOrder(Font font) : int? result
- AddLayer(string name)
- GetLayers() : string? result
- SetLayerOrder(string name, int order)
- SetSpriteOrder(Sprite sprite, int order)
- SetFontOrder(Font font, int order)

#### RenderPath3D
A 3D scene can either be rendered by a Forward or Deferred render path. 
It inherits functions from RenderPath2D, so it can render a 2D overlay.
- [void-constructor]RenderPath3D()
- SetSSAOEnabled(bool value)
- SetSSREnabled(bool value)
- SetShadowsEnabled(bool value)
- SetReflectionsEnabled(bool value)
- SetFXAAEnabled(bool value)
- SetBloomEnabled(bool value)
- SetColorGradingEnabled(bool value)
- SetColorGradingTexture(Texture value)
- SetEmitterParticlesEnabled(bool value)
- SetHairParticlesEnabled(bool value)
- SetHairParticlesReflectionEnabled(bool value)
- SetVolumeLightsEnabled(bool value)
- SetLightShaftsEnabled(bool value)
- SetLensFlareEnabled(bool value)
- SetMotionBlurEnabled(bool value)
- SetSSSEnabled(bool value)
- SetDepthOfFieldEnabled(bool value)
- SetDepthOfFieldFocus(float value)
- SetDepthOfFieldStrength(float value)
- SetTessellationEnabled(bool value)
- SetMSAASampleCount(int count)
- SetStereogramEnabled(bool value)
- SetSharpenFilterEnabled(bool value)
- SetSharpenFilterAmount(float value)
- SetExposure(float value)

##### RenderPath3D_Forward
It renders the scene contained by the Renderer in a forward render path. The component does not hold the scene information, 
only the effects to render the scene. The scene is managed and ultimately rendered by the Renderer.
It inherits functions from RenderPath3D.
- [constructor]RenderPath3D_Forward()

##### RenderPath3D_TiledForward
It renders the scene contained by the Renderer in a tiled forward render path. The component does not hold the scene information, 
only the effects to render the scene. The scene is managed and ultimately rendered by the Renderer.
It inherits functions from RenderPath3D_Forward3D.
- [constructor]RenderPath3D_TiledForward()

##### RenderPath3D_Deferred
It renders the scene contained by the Renderer in a deferred render path. The component does not hold the scene information, 
only the effects to render the scene. The scene is managed and ultimately rendered by the Renderer.
It inherits functions from RenderPath3D.
- [constructor]RenderPath3D_Deferred()

#### LoadingScreen
It is a RenderPath2D but one that internally manages resource loading and can display information about the process.
It inherits functions from RenderPath2D.
- [constructor]LoadingScreen()
- AddLoadingTask(string taskScript)
- OnFinished(string taskScript)

### Network
Here are the network communication features.
- TODO

#### Server
A TCP host to which clients can connect and communicate with each other or the server.
- TODO

#### Client
A TCP client which provides features to communicate with other clients over the internet or local area network connection.
- TODO

### Input Handling
These provide functions to check the state of the input devices.

#### InputManager
Query input devices
- [outer]input : InputManager
- [void-constructor]InputManager()
- Down(int code, opt int type = KEYBOARD)
- Press(int code, opt int type = KEYBOARD)
- Hold(int code, opt int duration = 30, opt boolean continuous = false, opt int type = KEYBOARD)
- GetPointer() : Vector result
- SetPointer(Vector pos)
- HidePointer(bool visible)
- GetTouches() : Touch result[]

#### Touch
Describes a touch contact point
- [constructor]Touch()
- GetState() : TOUCHSTATE result
- GetPos() : Vector result

#### TOUCHSTATE
- [outer]TOUCHSTATE_PRESSED : int
- [outer]TOUCHSTATE_RELEASED : int
- [outer]TOUCHSTATE_MOVED : int

#### Input types
- [outer]DIRECTINPUT_JOYPAD : int
- [outer]XINPUT_JOYPAD : int
- [outer]KEYBOARD : int

#### Keyboard Key codes
- [outer]VK_UP : int
- [outer]VK_DOWN : int
- [outer]VK_LEFT : int
- [outer]VK_RIGHT : int
- [outer]VK_SPACE : int
- [outer]VK_RETURN : int
- [outer]VK_RSHIFT : int
- [outer]VK_LSHIFT : int
- [outer]VK_F1 : int
- [outer]VK_F2 : int
- [outer]VK_F3 : int
- [outer]VK_F4 : int
- [outer]VK_F5 : int
- [outer]VK_F6 : int
- [outer]VK_F7 : int
- [outer]VK_F8 : int
- [outer]VK_F9 : int
- [outer]VK_F10 : int
- [outer]VK_F11 : int
- [outer]VK_F12 : int
- You can also generate a key code by calling string.byte(char uppercaseLetter) where the parameter represents the desired key of the keyboard

#### Mouse Key Codes
- [outer]VK_LBUTTON : int
- [outer]VK_MBUTTON : int
- [outer]VK_RBUTTON : int

### ResourceManager
Stores and manages resources such as textures, sounds and shaders.
- [outer]globalResources : Resource
- [void-constructor]Resource()
- Get(string name)
- Add(string name)
- Del(string name)
- List() : string result
