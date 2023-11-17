<img src="../logo_small.png" width="128px"/>

# Wicked Engine Scripting API Documentation
This is a reference and explanation of Lua scripting features in Wicked Engine.

## Contents
1. [Introduction and usage](#introduction-and-usage)
2. [Utility Tools](#utility-tools)
4. [Engine Bindings](#engine-bindings)
	1. [BackLog (Console)](#backlog)
	2. [Renderer](#renderer)
	3. [Sprite](#sprite)
		1. [ImageParams](#imageparams)
		2. [SpriteAnim](#spriteanim)
		3. [MovingTexAnim](#movingtexanim)
		4. [DrawRecAnim](#drawrecanim)
	4. [SpriteFont](#spritefont)
	5. [Texture](#texture)
	6. [Audio](#audio)
		1. [Sound](#sound)
		2. [SoundInstance](#soundinstance)
		3. [SoundInstance3D](#soundinstance3d)
	7. [Vector](#vector)
	8. [Matrix](#matrix)
	9. [Scene](#scene)
		1. [Entity](#entity)
		2. [Scene](#scene)
		3. [NameComponent](#namecomponent)
		4. [LayerComponent](#layercomponent)
		5. [TransformComponent](#transformcomponent)
		6. [CameraComponent](#cameracomponent)
		7. [AnimationComponent](#animationcomponent)
		8. [MaterialComponent](#materialcomponent)
		9. [EmitterComponent](#emittercomponent)
		9. [HairParticleSystem](#hairparticlesystem)
		10. [LightComponent](#lightcomponent)
		11. [ObjectComponent](#objectcomponent)
		12. [InverseKinematicsComponent](#inversekinematicscomponent)
		13. [SpringComponent](#springcomponent)
		14. [RigidBodyPhsyicsComponent](#rigidbodyphysicscomponent)
		15. [SoftBodyPhsyicsComponent](#softbodyphysicscomponent)
		16. [ForceFieldComponent](#forcefieldcomponent)
		17. [WeatherComponent](#weathercomponent)
		18. [SoundComponent](#soundcomponent)
		19. [ColliderComponent](#collidercomponent)
		19. [ExpressionComponent](#expressioncomponent)
		19. [HumanoidComponent](#humanoidcomponent)
		19. [DecalComponent](#decalcomponent)
	10. [Canvas](#canvas)
	11. [High Level Interface](#high-level-interface)
		1. [MainComponent](#maincomponent)
		2. [RenderPath](#renderpath)
			1. [RenderPath2D](#renderpath2d)
			2. [RenderPath3D](#renderpath3d)
			3. [LoadingScreen](#loadingscreen)
	12. [Primitives](#primitives)
		1. [Ray](#ray)
		2. [AABB](#aabb)
		3. [Sphere](#sphere)
		4. [Capsule](#capsule)
	12. [Input](#input)
	13. [Physics](#physics)
		
## Introduction and usage
Scripting in Wicked Engine is powered by Lua, meaning that the user can make use of the 
syntax and features of the flexible and powerful Lua language.
Apart from the common features, certain engine features are also available to use.
You can load lua script files and execute them, or the engine scripting console (also known as the BackLog)
can also be used to execute single line scripts (or you can execute file scripts by the dofile command here).
Upon startup, the engine will try to load a startup script file named startup.lua in the root directory of 
the application. If not found, an error message will be thrown followed by the normal execution by the program.
In the startup file, you can specify any startup logic, for example loading content or anything.

The Backlog is mapped to the HOME button on the keyboard. This will bring down an interface where lua commands can be entered with the keyboard. The ENTER button will execute the command that was entered. Pressing the HOME button again will exit the Backlog.

- Tip: You can inspect any object's functionality by calling 
getprops(YourObject) on them (where YourObject is the object which is to be inspected). The result will be displayed on the BackLog.

- Tags used in this documentation:
	- [constructor]				: The method is a constructor for the same type of object.
	- [void-constructor]		: The method is a constructor for the same type of object, but the object it creates is empty, so cannot be used.
	- [outer]					: The method or value is in the global scope, so not a method of any objects.


## Utility Tools
This section describes the common tools for scripting which are not necessarily engine features.
- signal(string name)  -- send a signal globally. This can wake up processes if there are any waiting on the same signal name
- waitSignal(string name)  -- wait until a specified signal arrives
- runProcess(function func)  -- start a new process
- killProcesses()  -- stop and remove all processes
- waitSeconds(float seconds)  -- wait until some time has passed (to be used from inside a process)
- getprops(table object)  -- get reflection data from object
- len(table object)  -- get the length of a table
- backlog_post_list(table list)  -- post table contents to the backlog
- fixedupdate()  -- wait for a fixed update step to be called (to be used from inside a process)
- update()  -- wait for variable update step to be called (to be used from inside a process)
- render()  -- wait for a render step to be called (to be used from inside a process)
- getDeltaTime()  -- returns the delta time in seconds (time passed sice previous update())
- math.lerp(float a,b,t)  -- linear interpolation
- math.clamp(float x,min,max)  -- clamp x between min and max
- math.saturate(float x)  -- clamp x between 0 and 1
- math.round(float x)  -- round x to nearest integer
- dobinaryfile(string filename) -- executes a binary LUA file
- compilebinaryfile(string filename_src, dilename_dst) -- compiles a text LUA file (filename_src) into a binary LUA file (filename_dst)

## Engine Bindings
The scripting API provides functions for the developer to manipulate engine behaviour or query it for information.

### BackLog
The scripting console of the engine. Input text with the keyboard, run the input with the RETURN key. The script errors
are also displayed here.
The scripting API provides some functions which manipulate the BackLog. These functions are in he global scope:
- backlog_clear()  -- remove all entries from the backlog
- backlog_post(string params,,,)  -- post a string to the backlog
- backlog_fontsize(int size)  -- modify the fint size of the backlog
- backlog_isactive() : boolean result  -- returns true if the backlog is active, false otherwise
- backlog_fontrowspacing(float spacing)  -- set a row spacing to the backlog
- backlog_lock() -- disable and lock the backlog so HOME key doesn't bring it up
- backlog_unlock() -- unlock the backlog so it can be toggled with the HOME key
- backlog_blocklua() -- disable LUA code execution in the backlog
- backlog_unblocklua() -- undisable LUA code execution in the backlog

### Renderer
This is the graphics renderer, which is also responsible for managing the scene graph which consists of keeping track of
parent-child relationships between the scene hierarchy, updating the world, animating armatures.
You can use the Renderer with the following functions, all of which are in the global scope:
- GetGameSpeed() : float result
- SetGameSpeed(float speed)
- GetScreenWidth() : float result  -- (deprecated, use application.GetCanvas().GetLogicalWidth() instead)
- GetScreenHeight() : float result  -- (deprecated, use application.GetCanvas().GetLogicalHeight() instead)
- HairParticleSettings(opt int lod0, opt int lod1, opt int lod2)
- SetShadowProps2D(int resolution, int count)
- SetShadowPropsCube(int resolution, int count)
- SetDebugPartitionTreeEnabled(bool enabled)
- SetDebugBonesEnabled(bool enabled)
- SetDebugEittersEnabled(bool enabled)
- SetDebugForceFieldsEnabled(bool enabled)
- SetVSyncEnabled(opt bool enabled)
- SetOcclusionCullingEnabled(bool enabled)
- DrawLine(Vector origin,end, opt Vector color)
- DrawPoint(Vector origin, opt float size, opt Vector color)
- DrawBox(Matrix boxMatrix, opt Vector color)
- DrawSphere(Sphere sphere, opt Vector color)
- DrawCapsule(Capsule capsule, opt Vector color)
- DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags)
	DrawDebugText flags, these can be combined with binary OR operator:
	[outer]DEBUG_TEXT_DEPTH_TEST		-- text can be occluded by geometry
	[outer]DEBUG_TEXT_CAMERA_FACING		-- text will be rotated to face the camera
	[outer]DEBUG_TEXT_CAMERA_SCALING	-- text will be always the same size, independent of distance to camera
- PutWaterRipple(Vector position) -- put down a water ripple with default embedded asset
- PutWaterRipple(string imagename, Vector position) -- put down water ripple texture from image asset file
- ClearWorld(opt Scene scene) -- Clears the scene and the associated renderer resources. If parmaeter is not specified, it will clear the global scene
- ReloadShaders()

### Sprite
Render images on the screen.
- Params : ImageParams
- Anim : SpriteAnim

</br>

- [constructor]Sprite(opt string texture, opt string maskTexture)
- SetParams(ImageParams effects)
- GetParams() : ImageParams result
- SetAnim(SpriteAnim anim)
- GetAnim() : SpriteAnim result
- SetTexture(Texture texture)
- GetTexture() : Texture
- SetMaskTexture(Texture texture)
- GetMaskTexture() : Texture

#### ImageParams
Specify Sprite properties, like position, size, etc.
- Pos : Vector
- Size : Vector
- Pivot : Vector
- Color : Vector
- Opacity : float
- Fade : float
- Rotation : float
- MipLevel : float
- TexOffset : Vector
- TexOffset2 : Vector

</br>

- [constructor]ImageParams(opt float width, opt float height)
- [constructor]ImageParams(float posX, float posY, float width, opt float height)
- GetPos() : Vector result
- GetSize() : Vector result
- GetPivot() : Vector result
- GetColor() : Vector result
- GetOpacity() : float result
- GetFade() : float result
- GetRotation() : float result
- GetMipLevel() : float result
- GetTexOffset() : Vector result
- GetTexOffset2() : Vector result
- GetDrawRect() : Vector result
- GetDrawRect2() : Vector result
- IsDrawRectEnabled() : bool result
- IsDrawRect2Enabled() : bool result
- IsMirrorEnabled() : bool result
- IsBackgroundBlurEnabled() : bool result
- SetPos(Vector pos)
- SetSize(Vector size)
- SetPivot(Vector value)
- SetColor(Vector value)
- SetOpacity(float opacity)
- SetFade(float fade)
- SetStencil(int stencilMode,stencilRef)
- SetStencilRefMode(int stencilrefmode)
- SetBlendMode(int blendMode)
- SetQuality(int quality)
- SetSampleMode(int sampleMode)
- SetRotation(float rotation)
- SetMipLevel(float mipLevel)
- SetTexOffset()
- SetTexOffset2()
- EnableDrawRect(Vector value)
- EnableDrawRect2(Vector value)
- DisableDrawRect()
- DisableDrawRect2()
- EnableMirror()
- DisableMirror()
- EnableBackgroundBlur(opt float mip = 0)
- DisableBackgroundBlur()

- [outer]STENCILMODE_DISABLED : int
- [outer]STENCILMODE_EQUAL : int
- [outer]STENCILMODE_LESS : int
- [outer]STENCILMODE_LESSEQUAL : int
- [outer]STENCILMODE_GREATER : int
- [outer]STENCILMODE_GREATEREQUAL : int
- [outer]STENCILMODE_NOT : int

- [outer]STENCILREFMODE_ENGINE : int
- [outer]STENCILREFMODE_USER : int
- [outer]STENCILREFMODE_ALL : int

- [outer]SAMPLEMODE_CLAMP : int
- [outer]SAMPLEMODE_WRAP : int
- [outer]SAMPLEMODE_MIRROR : int

- [outer]QUALITY_NEAREST : int
- [outer]QUALITY_LINEAR : int
- [outer]QUALITY_ANISOTROPIC : int
- [outer]QUALITY_BICUBIC : int

- [outer]BLENDMODE_OPAQUE : int
- [outer]BLENDMODE_ALPHA : int
- [outer]BLENDMODE_PREMULTIPLIED : int
- [outer]BLENDMODE_ADDITIVE : int

#### SpriteAnim
Animate Sprites easily with this helper.
- Rot : float
- Rotation : float
- Opacity : float
- Fade : float
- Repeatable : boolean
- Velocity : Vector
- ScaleX : float
- ScaleY : float
- MovingTexAnim : GetMovingTexAnim
- DrawRectAnim : GetDrawRectAnim

</br>

- [constructor]SpriteAnim()
- SetRot(float val)
- SetRotation(float val)
- SetOpacity(float val)
- SetFade(float val)
- SetWobbleAnimAmount(float val)
- SetWobbleAnimSpeed(float val)
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
- SpeedX : float
- SpeedY : float

</br>

- [constructor]MovingTexAnim(opt float speedX,speedY)
- SetSpeedX(float val)
- SetSpeedY(float val)
- GetSpeedX() : float result
- GetSpeedY() : float result

#### DrawRecAnim
Animate sprite frame by frame.
- FrameRate : float
- FrameCount : int
- HorizontalFrameCount : int

</br>

- [constructor]DrawRecAnim()
- SetFrameRate(float val)
- SetFrameCount(int val)
- SetHorizontalFrameCount(int val)
- GetFrameRate() : float result
- GetFrameCount() : int result
- GetHorizontalFrameCount() : int result

### SpriteFont
Gives you the ability to render text with a custom font.
- Text : string
- Size : int
- Pos : Vector
- Spacing : Vector
- Align : WIFALIGN halign
- Color : Vector
- ShadowColor : Vector
- Bolden : float
- Softness : float
- ShadowBolden : float
- ShadowSoftness : float
- ShadowOffset : Vector

</br>

- [constructor]SpriteFont(opt string text)
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
- SetColor(Vector color)
- SetColor(int colorHexCode)
- SetShadowColor(Vector shadowcolor)
- SetShadowColor(int colorHexCode)
- SetBolden(float value)
- SetSoftness(float value)
- SetShadowBolden(float value)
- SetShadowSoftness(float value)
- SetShadowOffset(Vector value)
- SetHorizontalWrapping(float value)
- SetHidden(bool value)
- GetText() : string result
- GetSize() : int result
- GetPos() : Vector result
- GetSpacing() : Vector result
- GetAlign() : WIFALIGN halign,valign
- GetColor() : Vector result
- GetShadowColor() : Vector result
- GetBolden() : float result
- GetSoftness() : float result
- GetShadowBolden() : float result
- GetShadowSoftness() : float result
- GetShadowOffset() : Vector result
- GetHorizontalWrapping() : float result
- IsHidden() : bool result
- TextSize() : Vector result -- returns text width and height in a Vector's X and Y components
- SetTypewriterTime(float value) -- time to fully type the text in seconds (0: disable)
- SetTypewriterLooped(bool value)) -- if true, typing starts over when finished
- SetTypewriterCharacterStart(int value) -- starting character for the animation
- SetTypewriterSound(Sound sound, SoundInstance soundinstance) -- sound effect when new letter appears
- ResetTypewriter() -- resets typewriter to first character
- TypewriterFinish() -- finished typewriter animation immediately
- IsTypewriterFinished() : bool -- returns tru if typewrites animation is finished, false otherwise


### Texture
Just holds texture information in VRAM.
- [constructor]Texture(opt string filename)	-- creates a texture from file
- [outer]texturehelper -- a global helper texture creation utility
- GetLogo() : Texture -- returns the Wicked Engine logo texture
- CreateGradientTexture(
	GradientType type = GradientType.Linear, 
	int width = 256,
	int height = 256, 
	Vector uv_start = Vector(0,0),
	Vector uv_end = Vector(0,0), 
	GradientFlags flags = GradientFlags.None, 
	string swizzle = "rgba", 
	float perlin_scale = 1,
	int perlin_seed = 1234, 
	int perlin_octaves = 8, 
	float perlin_persistence = 0.5) -- creates a gradient texture from parameters
- Save(string filename) -- saves texture into a file. Provide the extension in the filename, it should be one of the following: .JPG, .PNG, .TGA, .BMP, .DDS, .KTX2, .BASIS

```lua
GradientType = {
	Linear = 0,
	Circular = 1,
	Angular = 2,
}

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
texture = texturehelper.CreateGradientTexture(
	GradientType.Circular, -- gradient type
	256, 256, -- resolution of the texture
	Vector(0.5, 0.5), Vector(0.5, 0), -- start and end uv coordinates will specify the gradient direction and extents
	GradientFlags.Inverse | GradientFlags.Smoothstep | GradientFlags.PerlinNoise, -- modifier flags bitwise combination
	"rrr1", -- for each channel ,you can specify one of the following characters: 0, 1, r, g, b, a
	2, -- perlin noise scale
	123, -- perlin noise seed
	6, -- perlin noise octaves
	0.8 -- perlin noise persistence
)
texture.Save("gradient.png") -- you can save it to file
sprite.SetTexture(texture) -- you can set it to a sprite
material.SetTexture(TextureSlot.BASECOLORMAP, texture) -- you can set it to a material
```

### Audio
Loads and plays an audio files.
- [outer]audio : Audio  -- the audio device
- CreateSound(string filename, Sound sound) : bool  -- Creates a sound file, returns true if successful, false otherwise
- CreateSoundInstance(Sound sound, SoundInstance soundinstance) : bool  -- Creates a sound instance that can be replayed, returns true if successful, false otherwise
- Play(SoundInsance soundinstance)
- Pause(SoundInsance soundinstance)
- Stop(SoundInsance soundinstance)
- GetVolume(opt SoundInsance soundinstance) : float  -- returns the volume of a soundinstance. If soundinstance is not provided, returns the master volume
- SetVolume(float volume, opt SoundInsance soundinstance)  -- sets the volume of a soundinstance. If soundinstance is not provided, sets the master volume
- ExitLoop(SoundInsance soundinstance)  -- disable looping. By default, sound instances are looped when created.
- GetSubmixVolume(int submixtype) : float  -- returns the volume of the submix group
- SetSubmixVolume(int submixtype, float volume)  -- sets the volume for a submix group
- Update3D(SoundInstance soundinstance, SoundInstance3D instance3D)  -- adds 3D effect to the sound instance
- SetReverb(int reverbtype)  -- sets an environment effect for reverb globally. Refer to Reverb Types section for acceptable input values

#### Sound
An audio file. Can be instanced several times via SoundInstance.
- [constructor]Sound()  -- creates an empty sound. Use the audio device to load sounds from files
- IsValid() : bool -- returns whether the sound was created successfully

#### SoundInstance
An audio file instance that can be played. Note: after modifying parameters of the SoundInstance, the SoundInstance will need to be recreated from a specified sound
- [constructor]SoundInstance()  -- creates an empty soundinstance. Use the audio device to clone sounds
- SetSubmixType(int submixtype)  -- set a submix type group (default is SUBMIX_TYPE_SOUNDEFFECT)
- SetBegin(float seconds) -- beginning of the playback in seconds, relative to the Sound it will be created from (0 = from beginning)
- SetLength(float seconds) -- length in seconds (0 = until end)
- SetLoopBegin(float seconds) -- loop region begin in seconds, relative to the instance begin time (0 = from beginning)
- SetLoopLength(float seconds) -- loop region length in seconds (0 = until the end)
- SetEnableReverb(bool value) -- enable/disable reverb for the sound instance
- SetLooped(bool value) -- enable/disable looping for the sound instance
- GetSubmixType() : int
- GetBegin() : float
- GetLength() : float
- GetLoopBegin() : float
- GetLoopLength() : float
- IsEnableReverb() : bool
- IsLooped() : bool
- IsValid() : bool -- returns whether the sound instance was created successfully

#### SoundInstance3D
Describes the relation between a sound instance and a listener in a 3D world
- [constructor]SoundInstance3D()  -- creates the 3D relation object. By default, the listener and emitter are on the same position, and that disables the 3D effect
- SetListenerPos(Vector value)
- SetListenerUp(Vector value)
- SetListenerFront(Vector value)
- SetListenerVelocity(Vector value)
- SetEmitterPos(Vector value)
- SetEmitterUp(Vector value)
- SetEmitterFront(Vector value)
- SetEmitterVelocity(Vector value)
- SetEmitterRadius(float radius)

#### Submix Types
The submix types group sound instances together to be controlled together
- [outer]SUBMIX_TYPE_SOUNDEFFECT : int  -- sound effect group
- [outer]SUBMIX_TYPE_MUSIC : int  -- music group
- [outer]SUBMIX_TYPE_USER0 : int  -- user submix group
- [outer]SUBMIX_TYPE_USER1 : int  -- user submix group

#### Reverb Types
The reverb types are built in presets that can mimic a specific kind of environment
- [outer]REVERB_PRESET_DEFAULT : int
- [outer]REVERB_PRESET_GENERIC : int
- [outer]REVERB_PRESET_FOREST : int
- [outer]REVERB_PRESET_PADDEDCELL : int
- [outer]REVERB_PRESET_ROOM : int
- [outer]REVERB_PRESET_BATHROOM : int
- [outer]REVERB_PRESET_LIVINGROOM : int
- [outer]REVERB_PRESET_STONEROOM : int
- [outer]REVERB_PRESET_AUDITORIUM : int
- [outer]REVERB_PRESET_CONCERTHALL : int
- [outer]REVERB_PRESET_CAVE : int
- [outer]REVERB_PRESET_ARENA : int
- [outer]REVERB_PRESET_HANGAR : int
- [outer]REVERB_PRESET_CARPETEDHALLWAY : int
- [outer]REVERB_PRESET_HALLWAY : int
- [outer]REVERB_PRESET_STONECORRIDOR : int
- [outer]REVERB_PRESET_ALLEY : int
- [outer]REVERB_PRESET_CITY : int
- [outer]REVERB_PRESET_MOUNTAINS : int
- [outer]REVERB_PRESET_QUARRY : int
- [outer]REVERB_PRESET_PLAIN : int
- [outer]REVERB_PRESET_PARKINGLOT : int
- [outer]REVERB_PRESET_SEWERPIPE : int
- [outer]REVERB_PRESET_UNDERWATER : int
- [outer]REVERB_PRESET_SMALLROOM : int
- [outer]REVERB_PRESET_MEDIUMROOM : int
- [outer]REVERB_PRESET_LARGEROOM : int
- [outer]REVERB_PRESET_MEDIUMHALL : int
- [outer]REVERB_PRESET_LARGEHALL : int
- [outer]REVERB_PRESET_PLATE : int

### Vector
A four component floating point vector. Provides efficient calculations with SIMD support.
- X : float
- Y : float
- Z : float
- W : float

</br>

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
- Length() : float result	-- old syntax with operation on current object
- Length(Vector v) : float result
- Normalize() : Vector result	-- old syntax with operation on current object
- Normalize(Vector v) : Vector result
- QuaternionNormalize() : Vector result	-- old syntax with operation on current object
- QuaternionNormalize(Vector v) : Vector result
- Clamp(float min,max) : Vector result	-- old syntax with operation on current object
- Clamp(Vector v, float min,max) : Vector result
- Saturate() : Vector result	-- old syntax with operation on current object
- Saturate(Vector v) : Vector result
- Transform(Vector vec, Matrix matrix)
- TransformNormal(Vector vec, Matrix matrix)
- TransformCoord(Vector vec, Matrix matrix)
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
- QuaternionToRollPitchYaw(Vector quaternion) : Vector result
- QuaternionSlerp(Vector v1,v2, float t) : Vector result
- GetAngle(Vector a,b,axis, opt float max_angle = math.pi * 2) : float result	-- computes the signed angle between two 3D vectors around specified axis

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
- Scale(float scaleXYZ) : Matrix result
- LookTo(Vector eye,direction, opt Vector up) : Matrix result
- LookAt(Vector eye,focusPos, opt Vector up) : Matrix result
- Multiply(Matrix m1,m2) : Matrix result
- Add(Matrix m1,m2) : Matrix result
- Transpose(Matrix m) : Matrix result
- Inverse(Matrix m) : Matrix result, float determinant

### Scene System (using entity-component system)
Manipulate the 3D scene with these components.

#### Entity
An entity is just an int value (int in LUA and uint32 in C++) and works as a handle to retrieve associated components

#### Scene
The scene holds components. Entity handles can be used to retrieve associated components through the scene.
- [constructor]Scene() : Scene result  -- creates a custom scene
- [outer]GetScene() : Scene result  -- returns the global scene
- [outer]GetCamera() : Camera result  -- returns the global camera
- [outer]LoadModel(string fileName, opt Matrix transform) : int rootEntity	-- Load Model from file. returns a root entity that everything in this model is attached to
- [outer]LoadModel(Scene scene, string fileName, opt Matrix transform) : int rootEntity	-- Load Model from file into specified scene. returns a root entity that everything in this model is attached to

- [deprecated][outer]PICK_OPAQUE : uint	-- deprecated, you can use FILTER_ enums instead
- [deprecated][outer]PICK_TRANSPARENT : uint	-- deprecated, you can use FILTER_ enums instead
- [deprecated][outer]PICK_WATER : uint	-- deprecated, you can use FILTER_ enums instead
- [deprecated][outer]Pick(Ray ray, opt uint filterMask = ~0, opt uint layerMask = ~0, opt Scene scene = GetScene(), uint lod = 0) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified. (deprecated, you can use the scene's Intersects() function instead)
- [deprecated][outer]SceneIntersectSphere(Sphere sphere, opt uint filterMask = ~0, opt uint layerMask = ~0, opt Scene scene = GetScene(), opt uint lod = 0) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified. (deprecated, you can use the scene's Intersects() function instead)
- [deprecated][outer]SceneIntersectCapsule(Capsule capsule, opt uint filterMask = ~0, opt uint layerMask = ~0, opt Scene scene = GetScene(), opt uint lod = 0) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified. (deprecated, you can use the scene's Intersects() function instead)

- [outer]FILTER_NONE : uint	-- include nothing
- [outer]FILTER_OPAQUE : uint	-- include opaque meshes
- [outer]FILTER_TRANSPARENT : uint	-- include transparent meshes
- [outer]FILTER_WATER : uint	-- include water meshes
- [outer]FILTER_NAVIGATION_MESH : uint	-- include navigation meshes
- [outer]FILTER_OBJECT_ALL : uint	-- include all objects, meshes
- [outer]FILTER_COLLIDER : uint	-- include colliders
- [outer]FILTER_ALL : uint	-- include everything
- Intersects(Ray|Sphere|Capsule primitive, opt uint filterMask = ~0u, opt uint layerMask = ~0u, opt uint lod = 0) : int entity, Vector position,normal, float distance, Vector velocity, int subsetIndex, Matrix orientation	-- intersects a primitive with the scene and returns collision parameters
- Update()  -- updates the scene and every entity and component inside the scene
- Clear()  -- deletes every entity and component inside the scene
- Merge(Scene other)  -- moves contents from an other scene into this one. The other scene will be empty after this operation (contents are moved, not copied)
- UpdateHierarchy()	-- updates the full scene hierarchy system. Useful if you modified for example a parent transform and children immediately need up to date result in the script

- CreateEntity() : int entity  -- creates an empty entity and returns it
- FindAllEntities() : table[entities] -- returns a table with all the entities present in the given scene
- Entity_FindByName(string value, opt Entity ancestor = INVALID_ENTITY) : int entity  -- returns an entity ID if it exists, and INVALID_ENTITY otherwise. You can specify an ancestor entity if you only want to find entities that are descendants of ancestor entity
- Entity_Remove(Entity entity, bool recursive = true, bool keep_sorted = false)  -- removes an entity and deletes all its components if it exists. If recursive is specified, then all children will be removed as well (enabled by default). If keep_sorted is specified, then component order will be kept (disabled by default, slower)
- Entity_Duplicate(Entity entity) : int entity  -- duplicates all of an entity's components and creates a new entity with them. Returns the clone entity handle
- Entity_IsDescendant(Entity entity, Entity ancestor) : bool result	-- Check whether entity is a descendant of ancestor. Returns `true` if entity is in the hierarchy tree of ancestor, `false` otherwise

- Component_CreateName(Entity entity) : NameComponent result  -- attach a name component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateLayer(Entity entity) : LayerComponent result  -- attach a layer component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateTransform(Entity entity) : TransformComponent result  -- attach a transform component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateLight(Entity entity) : LightComponent result  -- attach a light component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateObject(Entity entity) : ObjectComponent result  -- attach an object component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateInverseKinematics(Entity entity) : InverseKinematicsComponent result  -- attach an IK component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateSpring(Entity entity) : SpringComponent result  -- attach a spring component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateScript(Entity entity) : ScriptComponent result  -- attach a script component to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateRigidBodyPhysics(Entity entity) : RigidBodyPhysicsComponent result  -- attach a RigidBodyPhysicsComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateSoftBodyPhysics(Entity entity) : SoftBodyPhysicsComponent result  -- attach a SoftBodyPhysicsComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateForceField(Entity entity) : ForceFieldComponent result  -- attach a ForceFieldComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateWeather(Entity entity) : WeatherComponent result  -- attach a WeatherComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateSound(Entity entity) : SoundComponent result  -- attach a SoundComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateCollider(Entity entity) : ColliderComponent result  -- attach a ColliderComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateExpression(Entity entity) : ExpressionComponent result  -- attach a ExpressionComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateHumanoid(Entity entity) : HumanoidComponent result  -- attach a HumanoidComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateDecal(Entity entity) : DecalComponent result  -- attach a DecalComponent to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateSprite(Entity entity) : Sprite result  -- attach a Sprite to an entity. The returned component is associated with the entity and can be manipulated
- Component_CreateFont(Entity entity) : SpriteFont result  -- attach a SpriteFont to an entity. The returned component is associated with the entity and can be manipulated

- Component_GetName(Entity entity) : NameComponent? result  -- query the name component of the entity (if exists)
- Component_GetLayer(Entity entity) : LayerComponent? result  -- query the layer component of the entity (if exists)
- Component_GetTransform(Entity entity) : TransformComponent? result  -- query the transform component of the entity (if exists)
- Component_GetCamera(Entity entity) : CameraComponent? result  -- query the camera component of the entity (if exists)
- Component_GetAnimation(Entity entity) : AnimationComponent? result  -- query the animation component of the entity (if exists)
- Component_GetMaterial(Entity entity) : MaterialComponent? result  -- query the material component of the entity (if exists)
- Component_GetEmitter(Entity entity) : EmitterComponent? result  -- query the emitter component of the entity (if exists)
- Component_GetLight(Entity entity) : LightComponent? result  -- query the light component of the entity (if exists)
- Component_GetObject(Entity entity) : ObjectComponent? result  -- query the object component of the entity (if exists)
- Component_GetInverseKinematics(Entity entity) : InverseKinematicsComponent? result  -- query the IK component of the entity (if exists)
- Component_GetSpring(Entity entity) : SpringComponent? result  -- query the spring component of the entity (if exists)
- Component_GetScript(Entity entity) : ScriptComponent? result  -- query the script component of the entity (if exists)
- Component_GetRigidBodyPhysics(Entity entity) : RigidBodyPhysicsComponent? result  -- query the RigidBodyPhysicsComponent of the entity (if exists)
- Component_GetSoftBodyPhysics(Entity entity) : SoftBodyPhysicsComponent? result  -- query the SoftBodyPhysicsComponent of the entity (if exists)
- Component_GetForceField(Entity entity) : ForceFieldComponent? result  -- query the ForceFieldComponent of the entity (if exists)
- Component_GetWeather(Entity entity) : WeatherComponent? result  -- query the WeatherComponent of the entity (if exists)
- Component_GetSound(Entity entity) : SoundComponent? result  -- query the SoundComponent of the entity (if exists)
- Component_GetCollider(Entity entity) : ColliderComponent? result  -- query the ColliderComponent of the entity (if exists)
- Component_GetExpression(Entity entity) : ExpressionComponent? result  -- query the ExpressionComponent of the entity (if exists)
- Component_GetHumanoid(Entity entity) : HumanoidComponent? result  -- query the HumanoidComponent of the entity (if exists)
- Component_GetDecal(Entity entity) : DecalComponent? result  -- query the DecalComponent of the entity (if exists)
- Component_GetSprite(Entity entity) : Sprite? result  -- query the Sprite of the entity (if exists)
- Component_GetFont(Entity entity) : SpriteFont? result  -- query the SpriteFont of the entity (if exists)

- Component_GetNameArray() : NameComponent[] result  -- returns the array of all components of this type
- Component_GetLayerArray() : LayerComponent[] result  -- returns the array of all components of this type
- Component_GetTransformArray() : TransformComponent[] result  -- returns the array of all components of this type
- Component_GetCameraArray() : CameraComponent[] result  -- returns the array of all components of this type
- Component_GetAnimationArray() : AnimationComponent[] result  -- returns the array of all components of this type
- Component_GetMaterialArray() : MaterialComponent[] result  -- returns the array of all components of this type
- Component_GetEmitterArray() : EmitterComponent[] result  -- returns the array of all components of this type
- Component_GetLightArray() : LightComponent[] result  -- returns the array of all components of this type
- Component_GetObjectArray() : ObjectComponent[] result  -- returns the array of all components of this type
- Component_GetInverseKinematicsArray() : InverseKinematicsComponent[] result  -- returns the array of all components of this type
- Component_GetSpringArray() : SpringComponent[] result  -- returns the array of all components of this type
- Component_GetScriptArray() : ScriptComponent[] result  -- returns the array of all components of this type
- Component_GetRigidBodyPhysicsArray() : RigidBodyPhysicsComponent[] result  -- returns the array of all components of this type
- Component_GetSoftBodyPhysicsArray() : SoftBodyPhysicsComponent[] result  -- returns the array of all components of this type
- Component_GetForceFieldArray() : ForceFieldComponent[] result  -- returns the array of all components of this type
- Component_GetWeatherArray() : WeatherComponent[] result  -- returns the array of all components of this type
- Component_GetSoundArray() : SoundComponent[] result  -- returns the array of all components of this type
- Component_GetColliderArray() : ColliderComponent[] result  -- returns the array of all components of this type
- Component_GetExpressionArray() : ExpressionComponent[] result  -- returns the array of all components of this type
- Component_GetHumanoidArray() : HumanoidComponent[] result  -- returns the array of all components of this type
- Component_GetDecalArray() : DecalComponent[] result  -- returns the array of all components of this type
- Component_GetSpriteArray() : Sprite[] result  -- returns the array of all components of this type
- Component_GetFontArray() : SpriteFont[] result  -- returns the array of all components of this type

- Entity_GetNameArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetLayerArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetTransformArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetCameraArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetAnimationArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetAnimationDataArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetMaterialArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetEmitterArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetLightArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetObjectArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetInverseKinematicsArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetSpringArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetScriptArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetRigidBodyPhysicsArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetSoftBodyPhysicsArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetForceFieldArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetWeatherArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetSoundArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetColliderArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetExpressionArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetHumanoidArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetDecalArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetSpriteArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetFontArray() : Entity[] result  -- returns the array of all entities that have this component type

- Component_RemoveName(Entity entity)  -- remove the name component of the entity (if exists)
- Component_RemoveLayer(Entity entity)  -- remove the layer component of the entity (if exists)
- Component_RemoveTransform(Entity entity)  -- remove the transform component of the entity (if exists)
- Component_RemoveCamera(Entity entity)  -- remove the camera component of the entity (if exists)
- Component_RemoveAnimation(Entity entity)  -- remove the animation component of the entity (if exists)
- Component_RemoveAnimationData(Entity entity)
- Component_RemoveMaterial(Entity entity) -- remove the material component of the entity (if exists)
- Component_RemoveEmitter(Entity entity)  -- remove the emitter component of the entity (if exists)
- Component_RemoveLight(Entity entity)  -- remove the light component of the entity (if exists)
- Component_RemoveObject(Entity entity)  -- remove the object component of the entity (if exists)
- Component_RemoveInverseKinematics(Entity entity)  -- remove the IK component of the entity (if exists)
- Component_RemoveSpring(Entity entity)  -- remove the spring component of the entity (if exists)
- Component_RemoveScript(Entity entity)  -- remove the script component of the entity (if exists)
- Component_RemoveRigidBodyPhysics(Entity entity)  -- remove the RigidBodyPhysicsComponent of the entity (if exists)
- Component_RemoveSoftBodyPhysics(Entity entity)  -- remove the SoftBodyPhysicsComponent of the entity (if exists)
- Component_RemoveForceField(Entity entity)  -- remove the ForceFieldComponent of the entity (if exists)
- Component_RemoveWeather(Entity entity) -- remove the WeatherComponent of the entity (if exists)
- Component_RemoveSound(Entity entity)  -- remove the SoundComponent of the entity (if exists)
- Component_RemoveCollider(Entity entity)  -- remove the ColliderComponent of the entity (if exists)
- Component_RemoveExpression(Entity entity)  -- remove the ExpressionComponent of the entity (if exists)
- Component_RemoveHumanoid(Entity entity)  -- remove the HumanoidComponent of the entity (if exists)
- Component_RemoveDecal(Entity entity)  -- remove the DecalComponent of the entity (if exists)
- Component_RemoveSprite(Entity entity)  -- remove the Sprite of the entity (if exists)
- Component_RemoveFont(Entity entity)  -- remove the SpriteFont of the entity (if exists)

- Component_Attach(Entity entity,parent, opt bool child_already_in_local_space = false)  -- attaches entity to parent (adds a hierarchy component to entity). From now on, entity will inherit certain properties from parent, such as transform (entity will move with parent) or layer (entity's layer will be a sublayer of parent's layer). If child_already_in_local_space is false, then child will be transformed into parent's local space, if true, it will be used as-is.
- Component_Detach(Entity entity)  -- detaches entity from parent (if hierarchycomponent exists for it). Restores entity's original layer, and applies current transformation to entity
- Component_DetachChildren(Entity parent)  -- detaches all children from parent, as if calling Component_Detach for all of its children

- GetBounds() : AABB result  -- returns an AABB fully containing objects in the scene. Only valid after scene has been updated.
- GetWeather() : WeatherComponent
- SetWeather(WeatherComponent weather)


- RetargetAnimation(Entity dst, src, bool bake_data) : Entity entity	-- Retargets an animation from a Humanoid to an other Humanoid such that the new animation will play back on the destination humanoid. dst : destination humanoid that the animation will be fit onto src : the animation to copy, it should already target humanoid bones. bake_data : if true, the retargeted data will be baked into a new animation data. If false, it will reuse the source animation data without creating a new one and retargeting will be applied at runtime on every Update. Returns entity ID of the new animation or INVALID_ENTITY if retargeting was not successful

#### NameComponent
Holds a string that can more easily identify an entity to humans than an entity ID. 
- Name : string
- SetName(string value)  -- set the name
- GetName() : string result  -- query the name string

#### LayerComponent
An integer mask that can be used to group entities together for certain operations such as: picking, rendering, etc.
- LayerMask : float
- SetLayerMask(int value)  -- set layer mask
- GetLayerMask() : int result  -- query layer mask

#### TransformComponent
Describes an orientation in 3D space.
- Translation_local : Vector XYZ  -- query the position in world space
- Rotation_local : Vector Quaternion  -- query the rotation as a quaternion in world space
- Scale_local : Vector XYZ  -- query the scaling in world space

</br>

- Scale(Vector vectorXYZ)  -- Applies scaling
- Scale(float value)  -- Applies uniform scaling
- Rotate(Vector vectorRollPitchYaw)  -- Applies rotation as roll,pitch,yaw
- RotateQuaternion(Vector quaternion)  -- Applies rotation as quaternion
- Translate(Vector vectorXYZ)  -- Applies translation (position offset)
- Lerp(TransformComponent a,b, float t)  -- Interpolates linearly between two transform components 
- CatmullRom(TransformComponent a,b,c,d, float t)  -- Interpolates between four transform components on a spline
- MatrixTransform(Matrix matrix)  -- Applies a transformation matrix
- GetMatrix() : Matrix result  -- Retrieve a 4x4 transformation matrix representing the transform component's current orientation
- ClearTransform()  -- Reset to the world origin, as in position becomes Vector(0,0,0), rotation quaternion becomes Vector(0,0,0,1), scaling becomes Vector(1,1,1)
- UpdateTransform()  -- Updates the underlying transformation matrix 
- GetPosition() : Vector resultXYZ  -- query the position in world space
- GetRotation() : Vector resultQuaternion  -- query the rotation as a quaternion in world space
- GetScale() : Vector resultXYZ  -- query the scaling in world space
- SetScale(Vector value) -- set scale in local space
- SetRotation(Vector quaternnion) -- set rotation quaternion in local space
- SetPosition(Vector value) -- set position in local space

#### CameraComponent
- FOV : float
- NearPlane : float
- FarPlane : float
- FocalLength : float
- ApertureSize : float
- ApertureShape : float

</br>

- UpdateCamera()  -- update the camera matrices
- TransformCamera(TransformComponent transform)  -- copies the transform's orientation to the camera, and sets the camera position, look direction and up direction. Camera matrices are not updated immediately. They will be updated by the Scene::Update() (if the camera is part of the scene), or by manually calling UpdateCamera()
- TransformCamera(Matrix matrix)
- GetFOV() : float result
- SetFOV(float value)	-- Sets the vertical field of view for the camera (value is an angle in radians)
- GetNearPlane() : float result
- SetNearPlane(float value)	-- Sets the near plane of the camera, which specifies the rendering cut off near the viewer. Must be a value greater than zero 
- GetFarPlane() : float result
- SetFarPlane(float value)	-- Sets the far plane (view distance) of the camera
- GetFocalLength() : float result
- SetFocalLength(float value)	-- Sets the focal distance (focus distance) of the camera. This is used by depth of field.
- GetApertureSize() : float result
- SetApertureSize(float value)	-- Sets the aperture size of the camera. Larger values will make the depth of field effect stronger.
- GetApertureShape() : float result
- SetApertureShape(Vector value)	-- Sets the aperture shape of camera, used for depth of field effect. The value's `.X` element specifies the horizontal, the `.Y` element specifies the vertical shape.
- GetView() : Matrix result
- GetProjection() : Matrix result
- GetViewProjection() : Matrix result
- GetInvView() : Matrix result
- GetInvProjection() : Matrix result
- GetInvViewProjection() : Matrix result
- GetPosition() : Vector result
- GetLookDirection() : Vector result
- GetUpDirection() : Vector result
- GetRightDirection() : Vector result
- SetPosition(Vector value)	-- Sets the position of the camera. `UpdateCamera()` should be used after this to apply the value. 
- SetLookDirection(Vector value)		-- Sets the look direction of the camera. The value must be a normalized direction `Vector`, relative to the camera position, and also perpendicular to the up direction. `UpdateCamera()` should be used after this to apply the value. This value will also be set if using the `TransformCamera()` function, from the transform's rotation.
- SetUpDirection(Vector value)		-- Sets the up direction of the camera. This must be a normalized direction `Vector`, relative to the camera position, and also perpendicular to the look direction. `UpdateCamera()` should be used after this to apply the value. This value will also be set if using the `TransformCamera()` function, from the transform's rotation.

#### AnimationComponent
- Timer : float
- Amount : float

</br>

- Play()
- Stop()
- Pause()
- SetLooped(bool value)
- IsLooped() : bool result
- IsPlaying() : bool result
- SetTimer(float value)
- GetTimer() : float result
- SetAmount(float value)
- GetAmount() : float result
- GetStart() : float result
- SetStart(float value)
- GetEnd() : float result
- SetEnd(float value)

#### MaterialComponent
- _flags : int
- BaseColor : Vector
- EmissiveColor : Vector
- EngineStencilRef : int
- UserStencilRef : int
- ShaderType : int
- UserBlendMode : int
- SpecularColor : Vector
- SubsurfaceScattering : Vector
- TexMulAdd : Vector
- Roughness : float
- Reflectance : float
- Metalness : float
- NormalMapStrength : float
- ParallaxOcclusionMapping : float
- DisplacementMapping : float
- Refraction : float
- Transmission : float
- AlphaRef : float
- SheenColor : Vector
- SheenRoughness : float
- Clearcoat : float
- ClearcoatRoughness : float
- ShadingRate : int
- TexAnimDirection : Vector
- TexAnimFrameRate : float
- texAnimElapsedTime : float
- customShaderID : int

</br>

- SetBaseColor(Vector value)
- SetEmissiveColor(Vector value)
- SetEngineStencilRef(int value)
- SetUserStencilRef(int value)
- GetStencilRef() : int result
- SetTexMulAdd(Vector vector)
- GetTexMulAdd() : Vector
- SetTexture(TextureSlot slot, string texturefile)
- SetTexture(TextureSlot slot, Texture texture)
- SetTextureUVSet(TextureSlot slot, int uvset)
- GetTexture(TextureSlot slot) : Texture
- GetTextureName(TextureSlot slot) : string
- GetTextureUVSet(TextureSlot slot) : int uvset

```lua
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
}
```

#### MeshComponent
- _flags : int
- TessellationFactor : float
- ArmatureID : int
- SubsetsPerLOD : int

</br>

- SetMeshSubsetMaterialID(int subsetindex, Entity materialID)
- GetMeshSubsetMaterialID(int subsetindex) : Entity entity

#### EmitterComponent
- _flags : int
- ShaderType : int
- Mass : float
- Velocity : Vector
- Gravity : Vector
- Drag : float
- Restitution : float
- EmitCount : float  -- emitted particle count per second
- Size : float  -- particle starting size
- Life : float  -- particle lifetime
- NormalFactor : float  -- normal factor that modulates emit velocities
- Randomness : float  -- general randomness factor
- LifeRandomness : float  -- lifetime randomness factor
- ScaleX : float  -- scaling along lifetime in X axis
- ScaleY : float  -- scaling along lifetime in Y axis
- Rotation : float  -- rotation speed
- MotionBlurAmount : float  -- set the motion elongation factor
- SPH_h : float
- SPH_K : float
- SPH_p0 : float
- SPH_e : float
- SpriteSheet_Frames_X : int
- SpriteSheet_Frames_Y : int
- SpriteSheet_Frame_Count : int
- SpriteSheet_Frame_Start : int
- SpriteSheet_Framerate : float

</br>

- Burst(int value)  -- spawns a specific amount of particles immediately
- SetEmitCount(float value)  -- set the emitted particle count per second
- SetSize(float value)  -- set particle starting size
- SetLife(float value)  -- set particle lifetime
- SetNormalFactor(float value)  -- set normal factor that modulates emit velocities
- SetRandomness(float value)  -- set general randomness factor
- SetLifeRandomness(float value)  -- set lifetime randomness factor
- SetScaleX(float value)  -- set scaling along lifetime in X axis
- SetScaleY(float value)  -- set scaling along lifetime in Y axis
- SetRotation(float value)  -- set rotation speed
- SetMotionBlurAmount(float value)  -- set the motion elongation factor

#### HairParticleSystem
- _flags : int
- StrandCount : int
- SegmentCount : int
- RandomSeed : int
- Length : float
- Stiffness : float
- Randomness : float
- ViewDistance : float
- SpriteSheet_Frames_X : int
- SpriteSheet_Frames_Y : int
- SpriteSheet_Frame_Count : int
- SpriteSheet_Frame_Start : int

#### LightComponent
- Type : int  -- light type, see accepted values below (by default it is a point light)
- [outer]DIRECTIONAL : int
- [outer]POINT : int
- [outer]SPOT : int
- Range : float
- Intensity : float -- Brightness of light in. The units that this is defined in depend on the type of light. Point and spot lights use luminous intensity in candela (lm/sr) while directional lights use illuminance in lux (lm/m2). https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- Color : Vector
- CastShadow : bool
- VolumetricsEnabled : bool
- OuterConeAngle : float -- outer cone angle for spotlight in radians
- InnerConeAngle : float -- inner cone angle for spotlight in radians

</br>

- SetType(int type)  -- set light type, see accepted values below (by default it is a point light)
- [outer]DIRECTIONAL : int
- [outer]POINT : int
- [outer]SPOT : int
- SetRange(float value)
- SetIntensity(float value) -- Brightness of light in. The units that this is defined in depend on the type of light. Point and spot lights use luminous intensity in candela (lm/sr) while directional lights use illuminance in lux (lm/m2). https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- SetColor(Vector value)
- SetCastShadow(bool value)
- SetVolumetricsEnabled(bool value)
- SetOuterConeAngle(float value) -- outer cone angle for spotlight in radians
- SetInnerConeAngle(float value) -- inner cone angle for spotlight in radians (0 <= innerConeAngle <= outerConeAngle). Value of 0 disables inner cone angle
- GetType() : int result

</br>

- SetEnergy(float value) -- kept for backwards compatibility with non physical light units (before v0.70.0)
- SetFOV(float value) -- kept for backwards compatibility with FOV angle (before v0.70.0)

#### ObjectComponent
- MeshID : Entity
- CascadeMask : int
- RendertypeMask : int
- Color : Vector
- EmissiveColor : Vector
- UserStencilRef : int
- LodDistanceMultiplier : float
- DrawDistance : float

</br>

- GetMeshID() : Entity
- GetCascadeMask() : int
- GetRendertypeMask() : int
- GetColor() : Vector
- GetEmissiveColor() : Vector
- GetUserStencilRef() : int
- GetLodDistanceMultiplier() : float
- GetDrawDistance() : float
- SetMeshID(Entity entity)
- SetCascadeMask(int value)
- SetRendertypeMask(int value)
- SetColor(Vector value)
- SetEmissiveColor(Vector value)
- SetUserStencilRef(int value)
- SetLodDistanceMultiplier(float value)
- SetDrawDistance(float value)
- SetForeground(bool value) -- enable/disable foreground object rendering. Foreground objects will be always rendered on top of regular objects, useful for FPS weapon or hands
- IsForeground() : bool
- SetNotVisibleInMainCamera(bool value) -- you can set the object to not be visible in main camera, but it will remain visible in reflections and shadows, useful for FPS character model
- IsNotVisibleInMainCamera() : bool
- SetNotVisibleInReflections(bool value) -- you can set the object to not be visible in main camera, but it will remain visible in reflections and shadows, useful for vampires
- IsNotVisibleInReflections() : bool

#### InverseKinematicsComponent
Describes an Inverse Kinematics effector.
- Target : Entity
- ChainLength : int
- IterationCount : int

</br>

- SetTarget(Entity entity) -- Sets the target entity (The IK entity and its parent hierarchy chain will try to reach the target)
- SetChainLength(int value) -- Sets the chain length, in other words, how many parents will be computed by the IK system
- SetIterationCount(int value) -- Sets the accuracy of the IK system simulation
- SetDisabled(bool value = true) -- Disable/Enable the IK simulation
- GetTarget() : Entity result
- GetChainLength() : int result
- GetIterationCount() : int result
- IsDisabled() : bool result

#### SpringComponent
Enables jiggle effect on transforms such as bones for example.
- Stiffness : float
- Damping : float
- WindAffection : float
- DragForce : float
- HitRadius : float
- GravityPower : float
- GravityDirection : Vector

</br>

- SetStiffness(float value)
- SetDamping(float value)
- SetWindAffection(float value)
- GetStiffness() : float result
- GetDamping() : float result
- GetWindAffection() : float result

#### ScriptComponent
A lua script bound to an entity
- CreateFromFile(string filename)
- Play()
- IsPlaying() : bool result
- SetPlayOnce(bool once = true)
- Stop()

#### RigidBodyPhysicsComponent
Describes a Rigid Body Physics object.
- Shape : int
- Mass : float
- Friction : float
- Restitution : float
- LinearDamping : float
- AngularDamping : float
- BoxParams_HalfExtents : Vector
- SphereParams_Radius : float
- CapsuleParams_Radius : float
- CapsuleParams_Height : float
- TargetMeshLOD : int

</br>

- IsDisableDeactivation() : bool return -- Check if the rigidbody is able to deactivate after inactivity
- IsKinematic() : bool return -- Check if the rigidbody is movable or just static
- SetDisableDeactivation(bool value = true) -- Sets if the rigidbody is able to deactivate after inactivity
- SetKinematic(bool value = true) -- Sets if the rigidbody is movable or just static

#### SoftBodyPhysicsComponent
Describes a Soft Body Physics object.
- Mass : float
- Friction : float
- Restitution : float

#### ForceFieldComponent
Describes a Force Field effector.
- Type : int
- Gravity : float
- Range : float

#### WeatherComponent
Describes a Weather
- OceanParameters : OceanParameters -- Returns a table to modify ocean parameters (if ocean is enabled)
- AtmosphereParameters : AtmosphereParameters -- Returns a table to modify atmosphere parameters
- VolumetricCloudParameters : VolumetricCloudParameters -- Returns a table to modify volumetric cloud parameters
- SkyMapName : string -- Resource name for sky texture
- ColorGradingMapName : string -- Resource name for color grading map
- sunColor : Vector
- sunDirection : Vector
- skyExposure : float
- horizon : Vector
- zenith : Vector
- ambient : Vector
- fogStart : float
- fog Density : float
- fogHeightStart : float
- fogHeightEnd : float
- windDirection : Vector
- windRandomness : float
- windWaveSize : float
- windSpeed : float
- stars : float
- rainAmount : float
- rainLenght : float
- rainSpeed : float
- rainScale : float
- rainColor : Vector
- gravity : Vector

</br>

- IsOceanEnabled() : bool return -- Check if weather's ocean simulation is enabled
- IsSimpleSky() : bool return -- Check if weather's sky is rendered in a simple, unrealistic way
- IsRealisticSky() : bool return -- Check if weather's sky is rendered in a physically correct, realistic way
- IsVolumetricClouds() : bool return -- Check if weather is rendering volumetric clouds
- IsHeightFog() : bool return -- Check if weather is rendering height fog visual effect
- SetOceanEnabled(bool value) -- Sets if weather's ocean simulation is enabled or not
- SetSimpleSky(bool value) -- Sets if weather's sky is rendered in a simple, unrealistic way or not
- SetRealisticSky(bool value) -- Sets if weather's sky is rendered in a physically correct, realistic way or not
- SetVolumetricClouds(bool value) -- Sets if weather is rendering volumetric clouds or not
- SetHeightFog(bool value) -- Sets if weather is rendering height fog visual effect or not

#### SoundComponent
Describes a Sound object.
- Filename : string
- Volume : float

</br>

- Play() -- Plays the sound
- Stop() -- Stop the sound
- SetLooped(bool value = true) -- Sets if the sound is looping when playing
- SetDisable3D(bool value = true) -- Disable/Enable 3D sounds
- IsPlaying() : bool result -- Check if sound is playing
- IsLooped() : bool result -- Check if sound is looping
- IsDisabled() : bool result -- Check if sound is disabled

#### ColliderComponent
Describes a Collider object.
- Shape : int -- Shape of the collider
- Radius : float
- Offset : Vector
- Tail : Vector

- SetCPUEnabled(bool value)
- SetGPUEnabled(bool value)
- GetCapsule() : Capsule
- GetSphere() : Sphere

[outer] ColliderShape = {
	Sphere = 0,
	Capsule = 1,
	Plane = 2,
}

#### ExpressionComponent
- FindExpressionID(string name) : int	-- Find an expression within the ExpressionComponent by name
- SetWeight(int id, float weight)	-- Set expression weight by ID. The ID can be a non-preset expression. Use FindExpressionID() to retrieve non-preset expression IDs
- GetWeight(int id) : float	-- returns current weight of expression
- SetPresetWeight(ExpressionPreset preset, float weight)	-- Set a preset expression's weight. You can get access to preset values from ExpressionPreset table
- GetPresetWeight(ExpressionPreset preset) : float	-- returns current weight of preset expression
- SetForceTalkingEnabled(bool value) -- Force continuous talking animation, even if no voice is playing
- IsForceTalkingEnabled() : bool
- SetPresetOverrideMouth(ExpressionPreset preset, ExpressionOverride override)
- SetPresetOverrideBlink(ExpressionPreset preset, ExpressionOverride override)
- SetPresetOverrideLook(ExpressionPreset preset, ExpressionOverride override)
- SetOverrideMouth(int id, ExpressionOverride override)
- SetOverrideBlink(int id, ExpressionOverride override)
- SetOverrideLook(int id, ExpressionOverride override)


[outer] ExpressionPreset = {
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
}

[outer] ExpressionOverride = {
	None = 0,
	Block = 1,
	Blend = 2,
}


#### HumanoidComponent
- GetBoneEntity(HumanoidBone bone) : int	-- Get the entity that is mapped to the specified humanoid bone. Use HumanoidBone table to get access to humanoid bone values
- SetLookAtEnabled(bool value)	-- Enable/disable automatic lookAt (for head and eyes movement)
- SetLookAt(Vector value)	-- Set a target lookAt position (for head an eyes movement)
- SetRagdollPhysicsEnabled(bool value) -- Activate dynamic ragdoll physics. Note that kinematic ragdoll physics is always active (ragdoll is animation-driven/kinematic by default).
- IsRagdollPhysicsEnabled() : bool

[outer] HumanoidBone = {
	Hips = 0,
	Spine = 1,
	Chest = 2,
	UpperChest = 3,
	Neck = 4,
	Head = 5,
	LeftEye = 6,
	RightEye = 7,
	Jaw = 8,
	LeftUpperLeg = 9,
	LeftLowerLeg = 10,
	LeftFoot = 11,
	LeftToes = 12,
	RightUpperLeg = 13,
	RightLowerLeg = 14,
	RightFoot = 15,
	RightToes = 16,
	LeftShoulder = 17,
	LeftUpperArm = 18,
	LeftLowerArm = 19,
	LeftHand = 20,
	RightShoulder = 21,
	RightUpperArm = 22,
	RightLowerArm = 23,
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
}


#### DecalComponent
The decal component is a textured sticker that can be put down onto meshes. Most of the properties can be controlled through an attached TransformComponent and MaterialComponent. 

- SetBaseColorOnlyAlpha(bool value)	-- Set decal to only use alpha from base color texture. Useful for blending normalmap-only decals
- IsBaseColorOnlyAlpha() : bool
- SetSlopeBlendPower(float value)
- GetSlopeBlendPower() : float


## Canvas
This is used to describe a drawable area
- GetDPI() : float -- pixels per inch
- GetDPIScaling() : float -- scaling factor between physical and logical size
- GetCustomScaling() : float -- a custom scaling factor on top of the DPI scaling
- SetCustomScaling(float value) -- sets a custom scaling factor that will be applied on top of DPI scaling
- GetPhysicalWidth() : int -- width in pixels
- GetPhysicalHeight() : int -- height in pixels
- GetLogicalWidth() : float -- width in dpi scaled units
- GetLogicalHeight() : float -- height in dpi scaled units

## High Level Interface
<b>This section must only be used from standalone lua scripts, and must not be used from a ScriptComponent.</b>
This is because ScriptComponent is always running inside scene.Update(), and paths can not be switched at that time safely.
On the other hand, a standalone lua script can define its own update logic and render path and cahnge application behaviour.

### Application
This is the main entry point and manages the lifetime of the application.
- [outer]application : Application
- [deprecated][outer]main : Application
- [void-constructor]Application()
- GetContent() : Resource? result
- GetActivePath() : RenderPath? result
- SetActivePath(RenderPath path, opt float fadeSeconds,fadeColorR,fadeColorG,fadeColorB)
- SetFrameSkip(bool enabled)	-- enable/disable frame skipping in fixed update 
- SetTargetFrameRate(float fps)	-- set target frame rate for fixed update and variable rate update when frame rate is locked
- SetFrameRateLock(bool enabled)	-- if enabled, variable rate update will use a fixed delta time
- SetInfoDisplay(bool active)	-- if enabled, information display will be visible in the top left corner of the application
- SetWatermarkDisplay(bool active)	-- toggle display of engine watermark, version number, etc. if info display is enabled
- SetFPSDisplay(bool active)	-- toggle display of frame rate if info display is enabled
- SetResolutionDisplay(bool active)	-- toggle display of resolution if info display is enabled
- SetLogicalSizeDisplay(bool active)	-- toggle display of logical size of canvas if info display is enabled
- SetColorSpaceDisplay(bool active)	-- toggle display of output color space if info display is enabled
- SetPipelineCountDisplay(bool active)	-- toggle display of active graphics pipeline count if info display is enabled
- SetHeapAllocationCountDisplay(bool active)	-- toggle display of heap allocation statistics if info display is enabled
- SetVRAMUsageDisplay(bool active)	-- toggle display of video memory usage if info display is enabled
- GetCanvas() : Canvas canvas  -- returns a copy of the application's current canvas
- SetCanvas(Canvas canvas)  -- applies the specified canvas to the application
- [outer]SetProfilerEnabled(bool enabled)

### RenderPath
A RenderPath is a high level system that represents a part of the whole application. It is responsible to handle high level rendering and logic flow. A render path can be for example a loading screen, a menu screen, or primary game screen, etc.
- [constructor]RenderPath()
- GetLayerMask() : uint result
- SetLayerMask(uint mask)

#### RenderPath2D
It can hold Sprites and SpriteFonts and can sort them by layers, update and render them.
- [constructor]RenderPath2D()
- AddSprite(Sprite sprite, opt string layer)
- AddFont(SpriteFont font, opt string layer)
- RemoveFont(SpriteFont font)
- ClearSprites()
- ClearFonts()
- GetSpriteOrder(Sprite sprite) : int? result
- GetFontOrder(SpriteFont font) : int? result
- AddLayer(string name)
- GetLayers() : string? result
- SetLayerOrder(string name, int order)
- SetSpriteOrder(Sprite sprite, int order)
- SetFontOrder(SpriteFont font, int order)
- CopyFrom(RenderPath other) -- copies everything from other renderpath into this

#### RenderPath3D
This is the default scene render path. 
It inherits functions from RenderPath2D, so it can render a 2D overlay.
- [constructor]RenderPath3D()
- SetResolutionScale(float value)	-- scale internal rendering resolution. This can provide major performance improvement when GPU rendering speed is the bottleneck
- SetAO(int value)  -- Sets up the ambient occlusion effect (possible values below)
- AO_DISABLED : int  -- turn off AO computation (use in SetAO() function)
	- AO_SSAO : int  -- enable simple brute force screen space ambient occlusion (use in SetAO() function)
	- AO_HBAO : int  -- enable horizon based screen space ambient occlusion (use in SetAO() function)
	- AO_MSAO : int  -- enable multi scale screen space ambient occlusion (use in SetAO() function)
- SetAOPower(float value)  -- applies AO power value if any AO is enabled
- SetSSREnabled(bool value)
- SetRaytracedDiffuseEnabled(bool value)
- SetRaytracedReflectionsEnabled(bool value)
- SetShadowsEnabled(bool value)
- SetReflectionsEnabled(bool value)
- SetFXAAEnabled(bool value)
- SetBloomEnabled(bool value)
- SetBloomThreshold(bool value)
- SetColorGradingEnabled(bool value)
- SetColorGradingTexture(Texture value)
- SetVolumeLightsEnabled(bool value)
- SetLightShaftsEnabled(bool value)
- SetLensFlareEnabled(bool value)
- SetMotionBlurEnabled(bool value)
- SetMotionBlurStrength(float value)
- SetSSSEnabled(bool value)
- SetDitherEnabled(bool value)
- SetDepthOfFieldEnabled(bool value)
- SetDepthOfFieldStrength(float value)
- SetLightShaftsStrength(float value)
- SetMSAASampleCount(int count)
- SetSharpenFilterEnabled(bool value)
- SetSharpenFilterAmount(float value)
- SetExposure(float value)
- SetOutlineEnabled(bool value)
- SetOutlineThreshold(float value)
- SetOutlineThickness(float value)
- SetOutlineColor(float r,g,b,a)
- SetFSREnabled(bool value)	-- FSR 1.0 on/off
- SetFSRSharpness(float value)	-- FSR 1.0 sharpness 0: sharpest, 2: least sharp
- SetFSR2Enabled(bool value) -- FSR 2.1 on/off
- SetFSR2Sharpness(float value) -- FSR 2.1 sharpness 0: least sharp, 1: sharpest (this is different to FSR 1.0)
- SetFSR2Preset(FSR2_Preset value) -- FSR 2.1 preset will modify resolution scaling and sampler LOD bias
- SetTonemap(Tonemap value) -- Set a tonemap type
- SetCropLeft(float value) -- Sets cropping from left of the screen in logical units
- SetCropTop(float value) -- Sets cropping from top of the screen in logical units
- SetCropRight(float value) -- Sets cropping from right of the screen in logical units
- SetCropBottom(float value) -- Sets cropping from bottom of the screen in logical units

FSR2_Preset = {
	Quality = 0,			-- 1.5x scaling, -1.58 sampler LOD bias
	Balanced = 1,			-- 1.7x scaling, -1.76 sampler LOD bias
	Performance = 2,		-- 2.0x scaling, -2.0 sampler LOD bias
	Ultra_Performance = 3,	-- 3.0x scaling, -2.58 sampler LOD bias
}

Tonemap = {
	Reinhard = 0,
	ACES = 1,
}

#### LoadingScreen
It is a RenderPath2D but one that internally manages resource loading and can display information about the process.
It inherits functions from RenderPath2D.
- [constructor]LoadingScreen()
- AddLoadingTask(string taskScript)
- OnFinished(string taskScript)

### Primitives

#### Ray
A ray is defined by an origin Vector and a normalized direction Vector. It can be used to intersect with other primitives or the scene
- Origin : Vector
- Direction : Vector

</br>

- [constructor]Ray(Vector origin,direction, opt float Tmin=0,Tmax=FLT_MAX)
- Intersects(AABB aabb) : bool result
- Intersects(Sphere sphere) : bool result
- Intersects(Capsule capsule) : bool result
- GetOrigin() : Vector result
- GetDirection() : Vector result
- SetOrigin(Vector vector)
- SetDirection(Vector vector)
- CreateFromPoints(Vector a,b)	-- creates a ray from two points. Point a will be the ray origin, pointing towards point b
- GetSurfaceOrientation(Vector position, normal) : Matrix -- compute placement orientation matrix at intersection result. This matrix can be used to place entities in the scene oriented on the surface.

#### AABB
Axis Aligned Bounding Box. Can be intersected with other primitives.
- Min : Vector
- Max : Vector

</br>

- [constructor]AABB(opt Vector min,max)	-- if no argument is given, it will be infinitely inverse that can't intersect
- Intersects2D(AABB aabb) : bool result	-- omit the z component for intersection check for more precise 2D intersection
- Intersects(AABB aabb) : bool result
- Intersects(Sphere sphere) : bool result
- Intersects(Ray ray) : bool result
- GetMin() : Vector result
- GetMax() : Vector result
- SetMin(Vector vector)
- SetMax(Vector vector)
- GetCenter() : Vector result
- GetHalfExtents() : Vector result
- Transform(Matrix matrix) : AABB result  -- transforms the AABB with a matrix and returns the resulting conservative AABB
- GetAsBoxMatrix() : Matrix result	-- get a matrix that represents the AABB as OBB (oriented bounding box)

#### Sphere
Sphere defined by center Vector and radius. Can be intersected with other primitives.
- Center : Vector
- Radius : float

</br>

- [constructor]Sphere(Vector center, float radius)
- Intersects(AABB aabb) : bool result
- Intersects(Sphere sphere) : bool result
- Intersects(Ray ray) : bool result
- GetCenter() : Vector result
- GetRadius() : float result
- SetCenter(Vector value)
- SetRadius(float value)
- GetSurfaceOrientation(Vector position, normal) : Matrix -- compute placement orientation matrix at intersection result. This matrix can be used to place entities in the scene oriented on the surface.

#### Capsule
It's like two spheres connected by a cylinder. Base and Tip are the two endpoints, radius is the cylinder's radius.
- Base : Vector
- Tip : Vector
- Radius : float

</br>

- [constructor]Capsule(Vector base, tip, float radius)
- Intersects(Capsule other) : bool result, Vector position, indicent_normal, float penetration_depth
- Intersects(Sphere sphere) : bool result, float depth, Vector normal
- Intersects(Ray ray) : bool result
- Intersects(Vector point) : bool result
- GetBase() : Vector result
- GetTip() : Vector result
- GetRadius() : float result
- SetBase(Vector value)
- SetTip(Vector value)
- SetRadius(float value)
- GetSurfaceOrientation(Vector position, normal) : Matrix -- compute placement orientation matrix at intersection result. This matrix can be used to place entities in the scene oriented on the surface.

### Input
Query input devices
- [outer]input : Input
- [void-constructor]Input()
- Down(int code, opt int playerindex = 0) : bool result  -- Check whether a button is currently being held down
- Press(int code, opt int playerindex = 0) : bool result  -- Check whether a button has just been pushed that wasn't before
- Release(int code, opt int playerindex = 0) : bool result  -- Check whether a button has just been released that was down before
- Hold(int code, opt int duration = 30, opt boolean continuous = false, opt int playerindex = 0) : bool result  -- Check whether a button was being held down for a specific duration (nunmber of frames). If continuous == true, than it will also return true after the duration was reached
- GetPointer() : Vector result  -- get mouse pointer or primary touch position (x, y). Also returns mouse wheel delta movement (z), and pen pressure (w)
- SetPointer(Vector pos)  -- set mouse poisition
- GetPointerDelta() : Vector result  -- native delta mouse movement
- HidePointer(bool visible)
- GetAnalog(int type, opt int playerindex = 0) : Vector result  -- read analog data from gamepad. type parameter must be from GAMEPAD_ANALOG values
- GetTouches() : Touch result[]
- SetControllerFeedback(ControllerFeedback feedback, opt int playerindex = 0) -- sets controller feedback such as vibration or LED color

#### ControllerFeedback
Describes controller feedback such as touch and LED color which can be replayed on a controller
- [constructor]ControllerFeedback()
- SetVibrationLeft(float value)  -- vibration amount of left motor (0: no vibration, 1: max vibration)
- SetVibrationRight(float value)  -- vibration amount of right motor (0: no vibration, 1: max vibration)
- SetLEDColor(Vector color)  -- sets the colored LED color if controller has one
- SetLEDColor(int hexcolor)  -- sets the colored LED color if controller has one (ABGR hex color code)

#### Touch
Describes a touch contact point
- [constructor]Touch()
- GetState() : TOUCHSTATE result
- GetPos() : Vector result

#### TOUCHSTATE
- [outer]TOUCHSTATE_PRESSED : int
- [outer]TOUCHSTATE_RELEASED : int
- [outer]TOUCHSTATE_MOVED : int

#### Keyboard Key codes
- [outer]KEYBOARD_BUTTON_UP				 : int
- [outer]KEYBOARD_BUTTON_DOWN			 : int
- [outer]KEYBOARD_BUTTON_LEFT			 : int
- [outer]KEYBOARD_BUTTON_RIGHT			 : int
- [outer]KEYBOARD_BUTTON_SPACE			 : int
- [outer]KEYBOARD_BUTTON_RSHIFT			 : int
- [outer]KEYBOARD_BUTTON_LSHIFT			 : int
- [outer]KEYBOARD_BUTTON_F1				 : int
- [outer]KEYBOARD_BUTTON_F2				 : int
- [outer]KEYBOARD_BUTTON_F3				 : int
- [outer]KEYBOARD_BUTTON_F4				 : int
- [outer]KEYBOARD_BUTTON_F5				 : int
- [outer]KEYBOARD_BUTTON_F6				 : int
- [outer]KEYBOARD_BUTTON_F7				 : int
- [outer]KEYBOARD_BUTTON_F8				 : int
- [outer]KEYBOARD_BUTTON_F9				 : int
- [outer]KEYBOARD_BUTTON_F10			 : int
- [outer]KEYBOARD_BUTTON_F11			 : int
- [outer]KEYBOARD_BUTTON_F12			 : int
- [outer]KEYBOARD_BUTTON_ENTER			 : int
- [outer]KEYBOARD_BUTTON_ESCAPE			 : int
- [outer]KEYBOARD_BUTTON_HOME			 : int
- [outer]KEYBOARD_BUTTON_RCONTROL		 : int
- [outer]KEYBOARD_BUTTON_LCONTROL		 : int
- [outer]KEYBOARD_BUTTON_DELETE			 : int
- [outer]KEYBOARD_BUTTON_BACKSPACE		 : int
- [outer]KEYBOARD_BUTTON_PAGEDOWN		 : int
- [outer]KEYBOARD_BUTTON_PAGEUP			 : int
- [outer]KEYBOARD_BUTTON_NUMPAD0		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD1		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD2		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD3		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD4		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD5		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD6		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD7		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD8		 : int
- [outer]KEYBOARD_BUTTON_NUMPAD9		 : int
- [outer]KEYBOARD_BUTTON_MULTIPLY		 : int
- [outer]KEYBOARD_BUTTON_ADD			 : int
- [outer]KEYBOARD_BUTTON_SEPARATOR		 : int
- [outer]KEYBOARD_BUTTON_SUBTRACT		 : int
- [outer]KEYBOARD_BUTTON_DECIMAL		 : int
- [outer]KEYBOARD_BUTTON_DIVIDE			 : int
- You can also generate a key code by calling string.byte(char uppercaseLetter) where the parameter represents the desired key of the keyboard

#### Mouse Key Codes
- [outer]MOUSE_BUTTON_LEFT	 : int
- [outer]MOUSE_BUTTON_RIGHT	 : int
- [outer]MOUSE_BUTTON_MIDDLE : int

#### Gamepad Key Codes
- [outer]GAMEPAD_BUTTON_UP : int
- [outer]GAMEPAD_BUTTON_LEFT : int
- [outer]GAMEPAD_BUTTON_DOWN : int
- [outer]GAMEPAD_BUTTON_RIGHT : int

Generic button codes:
- [outer]GAMEPAD_BUTTON_1 : int
- [outer]GAMEPAD_BUTTON_2 : int
- [outer]GAMEPAD_BUTTON_3 : int
- [outer]GAMEPAD_BUTTON_4 : int
...
- [outer]GAMEPAD_BUTTON_14 : int

Xbox button codes:
- [outer]GAMEPAD_BUTTON_XBOX_X  : GAMEPAD_BUTTON_1
- [outer]GAMEPAD_BUTTON_XBOX_A  : GAMEPAD_BUTTON_2
- [outer]GAMEPAD_BUTTON_XBOX_B  : GAMEPAD_BUTTON_3
- [outer]GAMEPAD_BUTTON_XBOX_Y  : GAMEPAD_BUTTON_4
- [outer]GAMEPAD_BUTTON_XBOX_L1 : GAMEPAD_BUTTON_5
- [outer]GAMEPAD_BUTTON_XBOX_R1 : GAMEPAD_BUTTON_6
- [outer]GAMEPAD_BUTTON_XBOX_L3 : GAMEPAD_BUTTON_7
- [outer]GAMEPAD_BUTTON_XBOX_R3 : GAMEPAD_BUTTON_8
- [outer]GAMEPAD_BUTTON_XBOX_BACK : GAMEPAD_BUTTON_9
- [outer]GAMEPAD_BUTTON_XBOX_START : GAMEPAD_BUTTON_10

Playstation button codes:
- [outer]GAMEPAD_BUTTON_PLAYSTATION_SQUARE : GAMEPAD_BUTTON_1
- [outer]GAMEPAD_BUTTON_PLAYSTATION_CROSS : GAMEPAD_BUTTON_2
- [outer]GAMEPAD_BUTTON_PLAYSTATION_CIRCLE : GAMEPAD_BUTTON_3
- [outer]GAMEPAD_BUTTON_PLAYSTATION_TRIANGLE : GAMEPAD_BUTTON_4
- [outer]GAMEPAD_BUTTON_PLAYSTATION_L1 : GAMEPAD_BUTTON_5
- [outer]GAMEPAD_BUTTON_PLAYSTATION_R1 : GAMEPAD_BUTTON_6
- [outer]GAMEPAD_BUTTON_PLAYSTATION_L3 : GAMEPAD_BUTTON_7
- [outer]GAMEPAD_BUTTON_PLAYSTATION_R3 : GAMEPAD_BUTTON_8
- [outer]GAMEPAD_BUTTON_PLAYSTATION_SHARE : GAMEPAD_BUTTON_9
- [outer]GAMEPAD_BUTTON_PLAYSTATION_OPTION : GAMEPAD_BUTTON_10

#### Gamepad Analog Codes
- [outer]GAMEPAD_ANALOG_THUMBSTICK_L : int
- [outer]GAMEPAD_ANALOG_THUMBSTICK_R : int
- [outer]GAMEPAD_ANALOG_TRIGGER_L : int
- [outer]GAMEPAD_ANALOG_TRIGGER_R : int


### Physics
- [outer]physics : Physics	-- use this global object to access physics functions
- SetEnabled(bool value)	-- Enable/disable the physics engine all together
- IsEnabled() : bool
- SetSimulationEnabled(bool value)	-- Enable/disable the physics simulation. Physics engine state will be updated but not simulated
- IsSimulationEnabeld() : bool
- SetDebugDrawEnabled(bool value)	-- Enable/disable debug drawing of physics objects
- IsDebugDrawEnabled() : bool
- SetAccuracy(int value)	-- Set the accuracy of the simulation. This value corresponds to maximum simulation step count. Higher values will be slower but more accurate.
- GetAccuracy() : int
- SetFrameRate(float value)	-- Set the frames per second resolution of physics simulation (default = 120 FPS)
- GetFrameRate() : float
- SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity)	-- Set the linear velocity manually
- SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity)	-- Set the angular velocity manually
- ApplyForce(RigidBodyPhysicsComponent component, Vector force)	-- Apply force at body center
- ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at)	-- Apply force at body local position
- ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse)	-- Apply impulse at body center
- ApplyImpulse(HumanoidComponent humanoid, HumanoidBone bone, Vector impulse)	-- Apply impulse at body center of ragdoll bone
- ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at)	-- Apply impulse at body local position
- ApplyImpulseAt(HumanoidComponent humanoid, HumanoidBone bone, Vector impulse, Vector at)	-- Apply impulse at body local position of ragdoll bone
- ApplyTorque(RigidBodyPhysicsComponent component, Vector torque)	-- Apply torque at body center
- ApplyTorqueImpulse(RigidBodyPhysicsComponent component, Vector torque)	-- Apply torque impulse at body center
- SetActivationState(RigidBodyPhysicsComponent component, int state)	-- Force set activation state to rigid body. Use a value ACTIVATION_STATE_ACTIVE or ACTIVATION_STATE_INACTIVE
- SetActivationState(SoftBodyPhysicsComponent component, int state)	-- Force set activation state to soft body. Use a value ACTIVATION_STATE_ACTIVE or ACTIVATION_STATE_INACTIVE
- [outer]ACTIVATION_STATE_ACTIVE : int
- [outer]ACTIVATION_STATE_INACTIVE : int

- Intersects(Scene scene, Ray ray) : Entity entity, Vector position,normal, Entity humanoid_ragdoll_entity, HumanoidBone humanoid_bone, Vector position_local	-- Performns physics scene intersection for closest hit with a ray

- PickDrag(Scene scene, Ray, ray, PickDragOperation op) -- pick and drag physics objects such as ragdolls and rigid bodies.

#### PickDragOperation
Tracks a physics pick drag operation. Use it with `phyiscs.PickDrag()` function. When using this object first time to PickDrag, the operation will be started and the operation will end when you call Finish() or when the object is destroyed
- [constructor]PickDragOperation() -- creates the object
- Finish() -- finish the operation, puts down the physics object
