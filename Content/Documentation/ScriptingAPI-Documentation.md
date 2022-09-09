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

### Renderer
This is the graphics renderer, which is also responsible for managing the scene graph which consists of keeping track of
parent-child relationships between the scene hierarchy, updating the world, animating armatures.
You can use the Renderer with the following functions, all of which are in the global scope:
- GetGameSpeed() : float result
- SetGameSpeed(float speed)
- GetScreenWidth() : float result  -- (deprecated, use MainComponent::GetCanvas().GetLogicalWidth() instead)
- GetScreenHeight() : float result  -- (deprecated, use MainComponent::GetCanvas().GetLogicalHeight() instead)
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
- PutWaterRipple(String imagename, Vector position)
- PutDecal(Decal decal)
- PutEnvProbe(Vector pos)
- ClearWorld(opt Scene scene) -- Clears the global scene and the associated renderer resources
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
- SetColor(Vector size)
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

### Texture
Just holds texture information in VRAM.
- [void-constructor]Texture()

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

#### SoundInstance
An audio file instance that can be played.
- [constructor]SoundInstance()  -- creates an empty soundinstance. Use the audio device to clone sounds
- SetSubmixType(int submixtype)  -- set a submix type group (default is SUBMIX_TYPE_SOUNDEFFECT)

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
- Length() : float result
- Normalize() : Vector result
- QuaternionNormalize() : Vector result
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
- [outer]Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified.
- [outer]SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified.
- [outer]SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified.
- Update()  -- updates the scene and every entity and component inside the scene
- Clear()  -- deletes every entity and component inside the scene
- Merge(Scene other)  -- moves contents from an other scene into this one. The other scene will be empty after this operation (contents are moved, not copied)

- CreateEntity() : int entity  -- creates an empty entity and returns it
- Entity_FindByName(string value) : int entity  -- returns an entity ID if it exists, and 0 otherwise
- Entity_Remove(Entity entity)  -- removes an entity and deletes all its components if it exists
- Entity_Duplicate(Entity entity) : int entity  -- duplicates all of an entity's components and creates a new entity with them. Returns the clone entity handle

- Component_CreateName(Entity entity) : NameComponent result  -- attach a name component to an entity. The returned NameComponent is associated with the entity and can be manipulated
- Component_CreateLayer(Entity entity) : LayerComponent result  -- attach a layer component to an entity. The returned LayerComponent is associated with the entity and can be manipulated
- Component_CreateTransform(Entity entity) : TransformComponent result  -- attach a transform component to an entity. The returned TransformComponent is associated with the entity and can be manipulated
- Component_CreateLight(Entity entity) : LightComponent result  -- attach a light component to an entity. The returned LightComponent is associated with the entity and can be manipulated
- Component_CreateObject(Entity entity) : ObjectComponent result  -- attach an object component to an entity. The returned ObjectComponent is associated with the entity and can be manipulated
- Component_CreateInverseKinematics(Entity entity) : InverseKinematicsComponent result  -- attach an IK component to an entity. The returned InverseKinematicsComponent is associated with the entity and can be manipulated
- Component_CreateSpring(Entity entity) : SpringComponent result  -- attach a spring component to an entity. The returned SpringComponent is associated with the entity and can be manipulated
- Component_CreateScript(Entity entity) : ScriptComponent result  -- attach a script component to an entity. The returned ScriptComponent is associated with the entity and can be manipulated
- Component_CreateRigidBodyPhysics(Entity entity) : RigidBodyPhysicsComponent result  -- attach a RigidBodyPhysicsComponent to an entity. The returned RigidBodyPhysicsComponent is associated with the entity and can be manipulated
- Component_CreateSoftBodyPhysics(Entity entity) : SoftBodyPhysicsComponent result  -- attach a SoftBodyPhysicsComponent to an entity. The returned SoftBodyPhysicsComponent is associated with the entity and can be manipulated
- Component_CreateForceField(Entity entity) : ForceFieldComponent result  -- attach a ForceFieldComponent to an entity. The returned ForceFieldComponent is associated with the entity and can be manipulated
- Component_CreateWeather(Entity entity) : WeatherComponent result  -- attach a WeatherComponent to an entity. The returned WeatherComponent is associated with the entity and can be manipulated
- Component_CreateSound(Entity entity) : SoundComponent result  -- attach a SoundComponent to an entity. The returned SoundComponent is associated with the entity and can be manipulated
- Component_CreateCollider(Entity entity) : ColliderComponent result  -- attach a script to an entity. The returned ColliderComponent is associated with the entity and can be manipulated

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

- Entity_GetNameArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetLayerArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetTransformArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetCameraArray() : Entity[] result  -- returns the array of all entities that have this component type
- Entity_GetAnimationArray() : Entity[] result  -- returns the array of all entities that have this component type
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

- Component_RemoveName(Entity entity) : NameComponent? result  -- remove the name component of the entity (if exists)
- Component_RemoveLayer(Entity entity) : LayerComponent? result  -- remove the layer component of the entity (if exists)
- Component_RemoveTransform(Entity entity) : TransformComponent? result  -- remove the transform component of the entity (if exists)
- Component_RemoveCamera(Entity entity) : CameraComponent? result  -- remove the camera component of the entity (if exists)
- Component_RemoveAnimation(Entity entity) : AnimationComponent? result  -- remove the animation component of the entity (if exists)
- Component_RemoveMaterial(Entity entity) : MaterialComponent? result  -- remove the material component of the entity (if exists)
- Component_RemoveEmitter(Entity entity) : EmitterComponent? result  -- remove the emitter component of the entity (if exists)
- Component_RemoveLight(Entity entity) : LightComponent? result  -- remove the light component of the entity (if exists)
- Component_RemoveObject(Entity entity) : ObjectComponent? result  -- remove the object component of the entity (if exists)
- Component_RemoveInverseKinematics(Entity entity) : InverseKinematicsComponent? result  -- remove the IK component of the entity (if exists)
- Component_RemoveSpring(Entity entity) : SpringComponent? result  -- remove the spring component of the entity (if exists)
- Component_RemoveScript(Entity entity) : ScriptComponent? result  -- remove the script component of the entity (if exists)
- Component_RemoveRigidBodyPhysics(Entity entity) : RigidBodyPhysicsComponent? result  -- remove the RigidBodyPhysicsComponent of the entity (if exists)
- Component_RemoveSoftBodyPhysics(Entity entity) : SoftBodyPhysicsComponent? result  -- remove the SoftBodyPhysicsComponent of the entity (if exists)
- Component_RemoveForceField(Entity entity) : ForceFieldComponent? result  -- remove the ForceFieldComponent of the entity (if exists)
- Component_RemoveWeather(Entity entity) : WeatherComponent? result  -- remove the WeatherComponent of the entity (if exists)
- Component_RemoveSound(Entity entity) : SoundComponent? result  -- remove the SoundComponent of the entity (if exists)
- Component_RemoveCollider(Entity entity) : ColliderComponent? result  -- remove the ColliderComponent of the entity (if exists)

- Component_Attach(Entity entity,parent)  -- attaches entity to parent (adds a hierarchy component to entity). From now on, entity will inherit certain properties from parent, such as transform (entity will move with parent) or layer (entity's layer will be a sublayer of parent's layer)
- Component_Detach(Entity entity)  -- detaches entity from parent (if hierarchycomponent exists for it). Restores entity's original layer, and applies current transformation to entity
- Component_DetachChildren(Entity parent)  -- detaches all children from parent, as if calling Component_Detach for all of its children

- GetBounds() : AABB result  -- returns an AABB fully containing objects in the scene. Only valid after scene has been updated.

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
- Rotate(Vector vectorRollPitchYaw)  -- Applies rotation as roll,pitch,yaw
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

#### CameraComponent
- FOV : float
- NearPlane : float
- FarPlane : float
- FocalDistance : float
- ApertureSize : float
- ApertureShape : float

</br>

- UpdateCamera()  -- update the camera matrices
- TransformCamera(TransformComponent transform)  -- copies the transform's orientation to the camera. Camera matrices are not updated immediately. They will be updated by the Scene::Update() (if the camera is part of the scene), or by manually calling UpdateCamera()
- GetFOV() : float result
- SetFOV(float value)
- GetNearPlane() : float result
- SetNearPlane(float value)
- GetFarPlane() : float result
- SetFarPlane(float value)
- GetFocalDistance() : float result
- SetFocalDistance(float value)
- GetApertureSize() : float result
- SetApertureSize(float value)
- GetApertureShape() : float result
- SetApertureShape(Vector value)
- GetView() : Matrix result
- GetProjection() : Matrix result
- GetViewProjection() : Matrix result
- GetInvView() : Matrix result
- GetInvProjection() : Matrix result
- GetInvViewProjection() : Matrix result
- GetPosition() : Vector result
- GetLookDirection() : Vector result
- GetUpDirection() : Vector result

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
- SetTexture(int textureindex, string texturefile)
- SetTextureUVSet(int textureindex, int uvset)
- GetTexture(int textureindex) : string texturefile
- GetTextureUVSet(int textureindex) : int uvset

#### MeshComponent
- _flags : int
- TessellationFactor : float
- ArmatureID : int
- SubsetsPerLOD : int

</br>

- SetMeshSubsetMaterialID(int subsetindex, Entity materialID)
- GetMeshSubsetMaterialID(int subsetindex)

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
- GetEmissiveColor(Vector value)
- SetUserStencilRef(int value)
- GetLodDistanceMultiplier(float value)
- GetDrawDistance(float value)

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
- SphereParams_Radius : floatd
- CapsuleParams_Radius : float
- CapsuleParams_Reight : float
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
- Gravity : Vector

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
- SetLoop(bool value = true) -- Sets if the sound is looping when playing
- SetDisable3D(bool value = true) -- Disable/Enable 3D sounds
- IsPlaying() : bool result -- Check if sound is playing
- IsLooped() : bool result -- Check if sound is looping
- IsDisabled() : bool result -- Check if sound is disabled

#### ColliderComponent
Describes a Sound object.
- Shape : int -- Shape of the collider
- Radius : float
- Offset : Vector
- Tail : Vector

- SetCPUEnabled(bool value)
- SetGPUEnabled(bool value)

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

#### RenderPath3D
This is the default scene render path. 
It inherits functions from RenderPath2D, so it can render a 2D overlay.
- [constructor]RenderPath3D()
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

- [constructor]Ray(Vector origin,direction)
- Intersects(AABB aabb) : bool result
- Intersects(Sphere sphere) : bool result
- Intersects(Capsule capsule) : bool result
- GetOrigin() : Vector result
- GetDirection() : Vector result
- SetOrigin(Vector vector)
- SetDirection(Vector vector)

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

#### Capsule
It's like two spheres connected by a cylinder. Base and Tip are the two endpoints, radius is the cylinder's radius.
- Base : Vector
- Tip : Vector
- Radius : float

</br>

- [constructor]Capsule(Vector base, tip, float radius)
- Intersects(Capsule other) : bool result, Vector position, indicent_normal, float penetration_depth
- Intersects(Ray ray) : bool result
- GetBase() : Vector result
- GetTip() : Vector result
- GetRadius() : float result
- SetBase(Vector value)
- SetTip(Vector value)
- SetRadius(float value)

### Input
Query input devices
- [outer]input : Input
- [void-constructor]Input()
- Down(int code, opt int playerindex = 0) : bool result  -- Check whether a button is currently being held down
- Press(int code, opt int playerindex = 0) : bool result  -- Check whether a button has just been pushed that wasn't before
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
- [outer]GAMEPAD_BUTTON_1 : int
- [outer]GAMEPAD_BUTTON_2 : int
- [outer]GAMEPAD_BUTTON_3 : int
- [outer]GAMEPAD_BUTTON_4 : int
...
- [outer]GAMEPAD_14 : int

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
- SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity)	-- Set the linear velocity manually
- SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity)	-- Set the angular velocity manually
- ApplyForce(RigidBodyPhysicsComponent component, Vector force)	-- Apply force at body center
- ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at)	-- Apply force at body local position
- ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse)	-- Apply impulse at body center
- ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at)	-- Apply impulse at body local position
- ApplyTorque(RigidBodyPhysicsComponent component, Vector torque)	-- Apply torque at body center
- ApplyTorqueImpulse(RigidBodyPhysicsComponent component, Vector torque)	-- Apply torque impulse at body center
- SetActivationState(RigidBodyPhysicsComponent component, int state)	-- Force set activation state to rigid body. Use a value ACTIVATION_STATE_ACTIVE or ACTIVATION_STATE_INACTIVE
- SetActivationState(SoftBodyPhysicsComponent component, int state)	-- Force set activation state to soft body. Use a value ACTIVATION_STATE_ACTIVE or ACTIVATION_STATE_INACTIVE
- [outer]ACTIVATION_STATE_ACTIVE : int
- [outer]ACTIVATION_STATE_INACTIVE : int
