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
		10. [LightComponent](#lightcomponent)
		11. [ObjectComponent](#objectcomponent)
		12. [InverseKinematicsComponent](#inversekinematicscomponent)
		13. [SpringComponent](#springcomponent)
	10. [High Level Interface](#high-level-interface)
		1. [MainComponent](#maincomponent)
		2. [RenderPath](#renderpath)
			1. [RenderPath2D](#renderpath2d)
			2. [RenderPath3D](#renderpath3d)
			3. [LoadingScreen](#loadingscreen)
	11. [Input](#input)
		
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
- SetResolutionScale(float scale)
- SetGamma(float gamma)
- SetGameSpeed(float speed)
- GetScreenWidth() : float result
- GetScreenHeight() : float result
- GetRenderWidth() : float result
- GetRenderHeight(): float result
- GetCamera() : Camera result		-- returns the main camera
- AttachCamera(Entity entity)	-- attaches camera to an entity in the current frame
- HairParticleSettings(opt int lod0, opt int lod1, opt int lod2)
- SetShadowProps2D(int resolution, int count, int softShadowQuality)
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
- PutWaterRipple(String imagename, Vector position)
- PutDecal(Decal decal)
- PutEnvProbe(Vector pos)
- ClearWorld()
- ReloadShaders(opt string path)

### Sprite
Render images on the screen.
- [constructor]Sprite(opt string texture, opt string maskTexture)
- SetParams(ImageParams effects)
- GetParams() : ImageParams result
- SetAnim(SpriteAnim anim)
- GetAnim() : SpriteAnim result

#### ImageParams
Specify Sprite properties, like position, size, etc.
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

### SpriteFont
Gives you the ability to render text with a custom font.
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
- GetText() : string result
- GetSize() : int result
- GetPos() : Vector result
- GetSpacing() : Vector result
- GetAlign() : WIFALIGN halign,valign
- GetColor() : Vector result
- GetShadowColor() : Vector result

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
- [outer]LoadModel(string fileName, opt Matrix transform) : int rootEntity	-- Load Model from file. returns a root entity that everything in this model is attached to
- [outer]LoadModel(Scene scene, string fileName, opt Matrix transform) : int rootEntity	-- Load Model from file into specified scene. returns a root entity that everything in this model is attached to
- [outer]Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified.
- [outer]SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified.
- [outer]SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) : int entity, Vector position,normal, float distance		-- Perform ray-picking in the scene. pickType is a bitmask specifying object types to check against. layerMask is a bitmask specifying which layers to check against. Scene parameter is optional and will use the global scene if not specified.
- Update()  -- updates the scene and every entity and component inside the scene
- Clear()  -- deletes every entity and component inside the scene
- Merge(Scene other)  -- moves contents from an other scene into this one. The other scene will be empty after this operation (contents are moved, not copied)

- Entity_FindByName(string value) : int entity  -- returns an entity ID if it exists, and 0 otherwise
- Entity_Remove(Entity entity)  -- removes an entity and deletes all its components if it exists
- Entity_Duplicate(Entity entity) : int entity  -- duplicates all of an entity's components and creates a new entity with them. Returns the clone entity handle

- Component_CreateName(Entity entity) : NameComponent result  -- attach a name component to an entity. The returned NameComponent is associated with the entity and can be manipulated
- Component_CreateLayer(Entity entity) : LayerComponent result  -- attach a layer component to an entity. The returned LayerComponent is associated with the entity and can be manipulated
- Component_CreateTransform(Entity entity) : TransformComponent result  -- attach a transform component to an entity. The returned TransformComponent is associated with the entity and can be manipulated
- Component_CreateLight(Entity entity) : LightComponent result  -- attach a light component to an entity. The returned LightComponent is associated with the entity and can be manipulated
- Component_CreateObject(Entity entity) : ObjectComponent result  -- attach an object component to an entity. The returned ObjectComponent is associated with the entity and can be manipulated
- Component_CreateInverseKinematics(Entity entity) : InverseKinematicsComponent result  -- attach an IK component to an entity. The returned InverseKinematicsComponent is associated with the entity and can be manipulated
- Component_CreateSpring(Entity entity) : SpringComponent result  -- attach a spring component to an entity. The returned InverseKinematicsComponent is associated with the entity and can be manipulated

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
- Component_GetSpringArray() : InverseKinematicsComponent[] result  -- returns the array of all components of this type

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

- Component_Attach(Entity entity,parent)  -- attaches entity to parent (adds a hierarchy component to entity). From now on, entity will inherit certain properties from parent, such as transform (entity will move with parent) or layer (entity's layer will be a sublayer of parent's layer)
- Component_Detach(Entity entity)  -- detaches entity from parent (if hierarchycomponent exists for it). Restores entity's original layer, and applies current transformation to entity
- Component_DetachChildren(Entity parent)  -- detaches all children from parent, as if calling Component_Detach for all of its children

#### NameComponent
Holds a string that can more easily identify an entity to humans than an entity ID. 
- SetName(string value)  -- set the name
- GetName() : string result  -- query the name string

#### LayerComponent
An integer mask that can be used to group entities together for certain operations such as: picking, rendering, etc.
- SetLayerMask(int value)  -- set layer mask
- GetLayerMask() : int result  -- query layer mask

#### TransformComponent
Describes an orientation in 3D space.
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
- UpdateCamera()  -- update the camera matrices
- TransformCamera(TransformComponent transform)  -- copies the transform's orientation to the camera. Camera matrices are not updated immediately. They will be updated by the Scene::Update() (if the camera is part of the scene), or by manually calling UpdateCamera()
- GetFOV() : float result
- SetFOV(float value)
- GetNearPlane() : float result
- SetNearPlane(float value)
- GetFarPlane() : float result
- SetFarPlane(float value)
- GetView() : Matrix result
- GetProjection() : Matrix result
- GetViewProjection() : Matrix result
- GetInvView() : Matrix result
- GetInvProjection() : Matrix result
- GetInvViewProjection() : Matrix result

#### AnimationComponent
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
- SetBaseColor(Vector value)
- SetEmissiveColor(Vector value)
- SetEngineStencilRef(int value)
- SetUserStencilRef(int value)
- GetStencilRef() : int result

#### EmitterComponent
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

#### LightComponent
- SetType(int type)  -- set light type, see accepted values below (by default it is a point light)
- [outer]DIRECTIONAL : int
- [outer]POINT : int
- [outer]SPOT : int
- [outer]SPHERE : int
- [outer]DISC : int
- [outer]RECTANGLE : int
- [outer]TUBE : int
- SetRange(float value)
- SetEnergy(float value)
- SetColor(Vector value)
- SetCastShadow(bool value)

#### ObjectComponent
- GetMeshID() : Entity
- GetColor() : Vector
- GetUserStencilRef() : int
- SetMeshID(Entity entity)
- SetColor(Vector value)
- SetUserStencilRef(int value)

#### InverseKinematicsComponent
Describes an Inverse Kinematics effector.
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
- SetStiffness(float value)
- SetDamping(float value)
- SetWindAffection(float value)

## High Level Interface
### MainComponent
This is the main entry point and manages the lifetime of the application. Even though it is called a component, it is not part of the entity-component system
- [outer]main : MainComponent
- [void-constructor]MainComponent()
- GetContent() : Resource? result
- GetActivePath() : RenderPath? result
- SetActivePath(RenderPath path, opt float fadeSeconds,fadeColorR,fadeColorG,fadeColorB)
- SetFrameSkip(bool enabled)	-- enable/disable frame skipping in fixed update 
- SetTargetFrameRate(float fps)	-- set target frame rate for fixed update and variable rate update when frame rate is locked
- SetFrameRateLock(bool enabled)	-- if enabled, variable rate update will use a fixed delta time
- SetInfoDisplay(bool active)
- SetWatermarkDisplay(bool active)
- SetFPSDisplay(bool active)
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
A 3D scene can either be rendered by a Forward or Deferred render path, or path tracing. 
It inherits functions from RenderPath2D, so it can render a 2D overlay.
- [void-constructor]RenderPath3D()
- SetAO(int value)  -- Sets up the ambient occlusion effect (possible values below)
- AO_DISABLED : int  -- turn off AO computation (use in SetAO() function)
- AO_SSAO : int  -- enable simple brute force screen space ambient occlusion (use in SetAO() function)
- AO_HBAO : int  -- enable horizon based screen space ambient occlusion (use in SetAO() function)
- AO_MSAO : int  -- enable multi scale screen space ambient occlusion (use in SetAO() function)
- SetHBAOEnabled(bool value)
- SetSSREnabled(bool value)
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
- SetDepthOfFieldFocus(float value)
- SetDepthOfFieldStrength(float value)
- SetDepthOfFieldAspect(float value)
- SetMSAASampleCount(int count)
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

### Intersects

#### Ray
A ray is defined by an origin Vector and a normalized direction Vector. It can be used to intersect with other primitives or the scene
- [constructor]Ray(Vector origin,direction)
- Intersects(AABB aabb) : bool result
- Intersects(Sphere sphere) : bool result
- GetOrigin() : Vector result
- GetDirection() : Vector result

#### AABB
Axis Aligned Bounding Box. Can be intersected with other primitives.
- [constructor]AABB(opt Vector min,max)	-- if no argument is given, it will be infinitely inverse that can't intersect
- Intersects2D(AABB aabb) : bool result	-- omit the z component for intersection check for more precise 2D intersection
- Intersects(AABB aabb) : bool result
- Intersects(Sphere sphere) : bool result
- Intersects(Ray ray) : bool result
- GetMin() : Vector result
- GetMax() : Vector result
- GetCenter() : Vector result
- GetHalfExtents() : Vector result
- Transform(Matrix matrix) : AABB result  -- transforms the AABB with a matrix and returns the resulting conservative AABB
- GetAsBoxMatrix() : Matrix result	-- get a matrix that represents the AABB as OBB (oriented bounding box)

#### Sphere
Sphere defined by center Vector and radius. Can be intersected with other primitives.
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
- [constructor]Capsule(Vector base, tip, float radius)
- Intersects(Capsule other) : bool result, Vector position, indicent_normal, float penetration_depth
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
