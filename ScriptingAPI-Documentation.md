# Wicked Engine Scripting API Documentation
This is a collection and explanation of scripting features in Wicked Engine.
The documentation completion is still pending....

## Contents
1. Introduction and usage
2. Engine manipulation
	1. BackLog (Console)
	2. Renderer
3. Utility Tools
	1. Font
	2. Sprite
		1. ImageEffects
		2. SpriteAnim
	3. Texture
	4. Sound
		1. SoundEffect
		2. Music
	5. Vector
	6. Matrix
	7. Scene
		1. Node
		2. Transform
		3. Cullable
		4. Object
		5. Armature
		6. Ray
		7. AABB
	8. Renderable Components
		1. Renderable2DComponent
		2. Renderable3DComponent
			1. ForwardRenderableComponent
			2. DeferredRenderableComponent
		4. LoadingScreenComponent
	9. Network
		1. Server
		2. Client
	10. Input Handling
		
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

## Engine manipulation
The scripting API provides functions for the developer to manipulate engine behaviour or query it for information.

### BackLog
The scripting console of the engine. Input text with the keyboard, run the input with the RETURN key. The script errors
are also displayed here.
The scripting API provides some functions which manipulate the BackLog. These functions are in he global scope:
- backlog_clear()
- backlog_post(string params,,,)
- backlog_fontsize(float size)
- backlog_isactive() : boolean result
- backlog_fontrowspacing(float spacing)

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
- GetMeshes() : string result
- GetLights() : string result
- GetMaterials() : string result
- GetGameSpeed() : float result
- SetGameSpeed(float speed)
- GetScreenWidth() : float result
- GetScreenHeight() : float result
- GetRenderWidth() : float result
- GetRenderHeight(): float result
- GetCamera() : Transform? result
- LoadModel(string directory, string name, opt string identifier, opt Matrix transform)
- LoadWorldInfo(string directory, string name)
- FinishLoading()
- SetEnvironmentMap(Texture cubemap)
- SetColorGrading(Texture texture2D)
- HairParticleSettings(opt int lod0, opt int lod1, opt int lod2)
- SetDirectionalLightShadowProps(int resolution, int softshadowQuality)
- SetPointLightShadowProps(int shadowMapCount, int resolution)
- SetSpotLightShadowProps(int shadowMapCount, int resolution)
- SetDebugBoxesEnabled(bool enabled)
- SetVSyncEnabled(opt bool enabled)
- SetPhysicsParams(opt bool rigidBodyPhysicsEnabled, opt bool softBodyPhysicsEnabled, opt int softBodyIterationCount)
- Pick(Ray ray, opt PICKTYPE pickType) : Object? object, Vector position,normal
- DrawLine(Vector origin,end, opt Vector color)
- PutWaterRipple(String imagename, Vector position)
- ClearWorld()
- ReloadShaders()

## Utility Tools
The scripting API provides certain engine features to be used in development.
- Tip: You can inspect any object's functionality by calling 
getprops(YourObject) on them (where YourObject is the object which is to be inspected). The result will be displayed on the BackLog.

- Tags:
	- [constructor]				: The method is a constructor for the same type of object.
	- [void-constructor]		: The method is a constructor for the same type of object, but the object it creates is empty, so cannot be used.
	- [outer]					: The method is in the global scope, so not a method of any objects.

### Font
Gives you the ability to render text with a custom font.
- [constructor]Font(opt string text)
- SetText(opt string text)
- SetSize(float size)
- SetPos(Vector pos)
- SetSpacing(Vector spacing)
- SetAlign(WIFALIGN Halign, opt WIFALIGN Valign)
	- WIFALIGN_LEFT
	- WIFALIGN_CENTER
	- WIFALIGN_MID
	- WIFALIGN_RIGHT
	- WIFALIGN_TOP
	- WIFALIGN_BOTTOM
- GetText() : string result
- GetSize() : float result
- GetPos() : Vector result
- GetSpacing() : Vector result
- GetAlign() : WIFALIGN halign,valign
- Destroy()

### Sprite
Render images on the screen.
- [constructor]Sprite(opt string texture, opt string maskTexture, opt string normalMapTexture)
- SetEffects(ImageEffects effects)
- GetEffects() : ImageEffects result
- SetAnim(SpriteAnim anim)
- GetAnim() : SpriteAnim result
- Destroy()

#### ImageEffects
Specify Sprite properties, like position, size, etc.
- [constructor]ImageEffects(opt float width, opt float height)
- [constructor]ImageEffects(float posX, float posY, float width, opt float height)
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
- SetRot(float rotation)

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
- Dot(Vector v1,v2) : Vector result
- Cross(Vector v1,v2) : Vector result
- Lerp(Vector v1,v2, float t) : Vector result
- QuaternionMultiply(Vector v1,v2) : Vector result
- QuaternionFromAxis(Vector v1,v2) : Vector result
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
Manipulate the 3D scene with these objects. 

#### Node
The basic entity in the scene. It has a name.
- [constructor]Node()
- GetName() : string
- SetName(string name)

#### Transform
Everything in the scene is a transform. It defines a point in the space by location, size, and rotation.
It provides several key features of parenting. 
It inherits functions from Node.
- [constructor]Transform()
- AttachTo(Transform parent, opt boolean translation,rotation,scale)
- Detach(opt boolean eraseFromParent) [warning: Leave the parameter unless you know what you are doing]
- Scale(Vector vector)
- Rotate(Vector vectorRollPitchYaw)
- Translate(Vector vector)
- MatrixTransform(Matrix matrix)
- GetMatrix() : Matrix result
- ClearTransform()
- SetTransform(Transform t)
- GetPosition() : Vector result
- GetRotation() : Vector resultQuaternion
- GetScale() : Vector resultXYZ

#### Cullable
Can be tested againt the view frustum, AABBs, rays, space partitioning trees. 
It inherits functions from Transform.
- [constructor]Cullable()
- Intersects(Cullable cullable)
- Intersects(Ray cullable)
- Intersects(Vector cullable)
- GetAABB() : AABB result
- SetAABB(AABB aabb)

#### Object
It is a renderable entity (optionally), which contains a Mesh.
It inherits functions from Cullable.
- [void-constructor]Object()
- SetTransparency(float value)
- GetTransparency() : float? result
- SetColor(float r,g,b)
- GetColor() : float? r,g,b
- IsValid() : boolean result

#### Armature
It animates meshes.
It inherits functions from Transform.
- [void-constructor]Armature()
- GetAction() : string? result
- GetActions() : string? result
- GetBones() : string? result
- GetBone(String name) : Transform? result
- GetFrame() : float? result
- GetFrameCount() : float? result
- IsValid() : boolean result
- ChangeAction(String name)
- StopAction()
- PauseAction()
- PlayAction()
- ResetAction()

#### Ray
Can intersect with AABBs, Cullables.
- [constructor]Ray(opt Vector origin, direction)
- GetOrigin() : Vector result
- GetDirection() : Vector result

#### AABB
Axis Aligned Bounding Box. Can be intersected with any shape, or ray.
- [constructor]AABB(opt Vector min,max)
- Intersects(AABB aabb)
- Transform(Matrix mat)
- GetMin() : Vector result
- GetMax() : Vector result

### Renderable Components
A RenderableComponent describes a scene wich can render itself.

#### Renderable2DComponent
It can hold Sprites and Fonts and can sort them by layers, update and render them.

#### Renderable3DComponent
A 3D scene can either be rendered by a Forward or Deferred render path.

##### ForwardRenderableComponent
It renders the scene contained by the Renderer in a forward render path. The component does not hold the scene information, 
only the effects to render the scene. The scene is managed and ultimately rendered by the Renderer.

##### DeferredRenderableComponent
It renders the scene contained by the Renderer in a deferred render path. The component does not hold the scene information, 
only the effects to render the scene. The scene is managed and ultimately rendered by the Renderer.

#### LoadingScreenComponent
It is a Renderable2DComponent but one that internally manages resource loading and can display information about the process.

### Network
Here are the network communication features.

#### Server
A TCP host to which clients can connect and communicate with each other or the server.

#### Client
A TCP client which provides features to communicate with other clients over the internet or local area network connection.

### Input Handling
These provide functions to check the state of the input devices.
- [outer]input : InputManager
- [void-constructor]InputManager()
- Down(int code, opt int type = KEYBOARD)
- Press(int code, opt int type = KEYBOARD)
- Hold(int code, opt int duration = 30, opt boolean continuous = false, opt int type = KEYBOARD)

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
- You can also generate a key code by calling string.byte(char uppercaseLetter) where the parameter represents the desired key of the keyboard

#### Mouse Key Codes
- [outer]VK_LBUTTON : int
- [outer]VK_MBUTTON : int
- [outer]VK_RBUTTON : int