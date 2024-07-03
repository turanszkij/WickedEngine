<img src="../logo_small.png" width="128px"/>

# WickedEngine Documentation
This is a reference for the C++ features of Wicked Engine

## Contents
1. [High Level Interface](#high-level-interface)
	1. [Application](#application)
	2. [RenderPath](#renderpath)
	3. [RenderPath2D](#renderpath2d)
	4. [RenderPath3D](#renderpath3d)
	9. [RenderPath3D_Pathtracing](#renderpath3d_pathtracing)
		1.[Denoiser](#denoiser)
	10. [LoadingScreen](#loadingscreen)
2. [System](#system)
	1. [Entity-Component System](#entity-component-system)
	2. [Scene System](#scene-system)
		1. [NameComponent](#namecomponent)
		2. [LayerComponent](#layercomponent)
		3. [TransformComponent](#transformcomponent)
		4. [PreviousFrameTransformComponent](#previousframetransformcomponent)
		5. [HierarchyComponent](#hierarchycomponent)
		6. [MaterialComponent](#materialcomponent)
		7. [MeshComponent](#meshcomponent)
		8. [ImpostorComponent](#impostorcomponent)
		9. [ObjectComponent](#objectcomponent)
		10. [RigidBodyPhysicsComponent](#rigidbodyphysicscomponent)
		11. [SoftBodyPhysicsComponent](#softbodyphysicscomponent)
		12. [ArmatureComponent](#armaturecomponent)
		13. [LightComponent](#lightcomponent)
		14. [CameraComponent](#cameracomponent)
		15. [EnvironmentProbeComponent](#environmentprobecomponent)
		16. [ForceFieldComponent](#forcefieldcomponent)
		17. [DecalComponent](#decalcomponent)
		18. [AnimationDataComponent](#animationdatacomponent)
		18. [AnimationComponent](#animationcomponent)
		19. [WeatherComponent](#weathercomponent)
		20. [SoundComponent](#soundcomponent)
		21. [InverseKinematicsComponent](#inversekinematicscomponent)
		22. [SpringComponent](#springcomponent)
		22. [ColliderComponent](#collidercomponent)
		22. [ScriptComponent](#scriptcomponent)
		23. [Scene](#scene)
	3. [Job System](#job-system)
	4. [Initializer](#initializer)
	5. [Platform](#platform)
	6. [EventHandler](#event-handler)
	7. [Canvas](#canvas)
3. [Graphics](#graphics)
	1. [Graphics API](#graphics-api)
		1. [GraphicsDevice](#wigraphicsdevice)
			1. [Debug device](#debug-device)
			2. [Creating resources](#creating-resources)
			3. [Destroying resources](#destroying-resources)
			4. [Work submission](#work-submission)
				1. [Async compute](#async-compute)
			5. [Presenting to the screen](#presenting-to-the-screen)
				1. [HDR Display](#hdr-display)
			6. [Resource binding](#resource-binding)
			6. [Bindless resources](#bindless-resources)
			7. [Subresources](#subresources)
			8. [Pipeline States and Shaders](#pipeline-states-and-shaders)
			9. [Render Passes](#render-passes)
			10. [GPU Barriers](#gpu-barriers)
			11. [Textures](#textures)
			12. [GPU Buffers](#gpu-buffers)
			13. [Updating GPU buffers](#updating-gpu-buffers)
			14. [GPU Queries](#gpu-queries)
			15. [RayTracingAccelerationStructure](#raytracingaccelerationstructure)
			16. [RayTracingPipelineState](#raytracingpipelinestate)
			17. [Variable Rate Shading](#variable-rate-shading)
			17. [Video decoding](#video-decoding)
		2. [GraphicsDevice_DX11](#graphicsdevice_dx11)
		3. [GraphicsDevice_DX12](#graphicsdevice_dx12)
		4. [GraphicsDevice_Vulkan](#graphicsdevice_vulkan)
	2. [Renderer](#renderer)
		1. [DrawScene](#drawscene)
		3. [Tessellation](#tessellation)
		4. [Occlusion Culling](#occlusion-culling)
		5. [Shadow Maps](#shadow-maps)
		6. [UpdatePerFrameData](#updateperframedata)
		7. [UpdateRenderData](#updaterenderdata)
		8. [Ray tracing (hardware accelerated)](#ray-tracing-hardware-accelerated)
		8. [Ray tracing (Legacy)](#ray-tracing-legacy)
		9. [Scene BVH](#scene-bvh)
		10. [Decals](#decals)
		11. [Environment probes](#environment-probes)
		12. [Post processing](#post-processing)
		13. [Instancing](#instancing)
		14. [Stencil](#stencil)
		15. [Loading Shaders](#loading-shaders)
		16. [Debug Draw](#debug-draw)
		17. [Animation Skinning](#animation-skinning)
		18. [Custom Shaders](#custom-shaders)
	3. [Enums](#enums)
	4. [Image Renderer](#image-renderer)
	5. [Font Renderer](#font-renderer)
	6. [Emitted Particle System](#emitted-particle-system)
	7. [Hair Particle System](#hair-particle-system)
	8. [Ocean](#ocean)
	9. [Sprite](#sprite)
	10. [SpriteFont](#spritefont)
	11. [GPUSortLib](#gpusortlib)
	12. [GPUBVH](#gpubvh)
4. [GUI](#gui)
	1. [GUI](#gui)
	2. [Widget](#widget)
	3. [Button](#button)
	4. [Label](#label)
	5. [TextInputField](#textinputfield)
	6. [Slider](#slider)
	7. [CheckBox](#checkbox)
	8. [ComboBox](#combobox)
	9. [Window](#window)
	10. [ColorPicker](#colorpicker)
	11. [TreeList](#treelist)
5. [Helpers](#helpers)
	2. [Archive](#archive)
	3. [Color](#color)
	5. [FadeManager](#fademanager)
	6. [Helper](#helper)
	7. [Primitive](#primitive)
		1. [AABB](#aabb)
		2. [Sphere](#sphere)
		2. [Capsule](#capsule)
		3. [Ray](#ray)
		4. [Frustum](#frustum)
		5. [Hitbox2D](#hitbox2d)
	8. [wiMath](#wimath)
	9. [wiRandom](#wirandom)
	10. [wiRectPacker](#wirectpacker)
	11. [wiResourceManager](#wiresourcemanager)
	12. [wiSpinLock](#wispinlock)
	13. [wiArguments](#wiarguments)
	14. [wiTimer](#witimer)
	14. [wiVoxelGrid](#wivoxelgrid)
	14. [wiPathQuery](#wipathquery)
6. [Input](#input)
7. [Audio](#audio)
	1. [Sound](#sound)
	2. [SoundInstance](#soundinstance)
	3. [SoundInstance3D](#soundinstance3d)
	4. [SUBMIX_TYPE](#submix_type)
	5. [REVERB_PRESET](#reverb_preset)
8. [Physics](#physics)
	1. [Rigid Body Physics](#rigid-body-physics)
	2. [Soft Body Physics](#soft-body-physics)
9. [Network](#network)
	2. [Socket](#socket)
	3. [Connection](#connection)
10. [Scripting](#scripting)
	1. [Lua](#lua)
	2. [Lua_Globals](#lua_globals)
	3. [Luna](#luna)
11. [Tools](#tools)
	1. [Backlog](#backlog)
	2. [Profiler](#profiler)
12. [Shaders](#shaders)
	1. [Interop](#interop)
	2. [Shader Compiler](#shader-compiler)


## High Level Interface
The high level interface consists of classes that allow for extending the engine with custom functionality. This is usually done by overriding the classes.

### Application
[[Header]](../../WickedEngine/wiApplication.h) [[Cpp]](../../WickedEngine/wiApplication.cpp)
This is the main runtime component that has the Run() function. It should be included in the application entry point while calling Run() in an infinite loop. <br/>
The user should call the SetWindow() function to associate it with a window of the operating system. This window will be used to render to.<br/>
The `Application` has many uses that the user is not necessarily interested in. The most important part is that it manages the RenderPaths. There can be one active RenderPath at a time, which will be updated and rendered to the screen every frame. However, because a RenderPath is a highly customizable class, there is no limitation what can be done within the RenderPath, for example supporting multiple concurrent RenderPaths if required. RenderPaths can be switched wit ha Fade out screen easily. Loading Screen can be activated as an active Renderpath and it will load and switch to an other RenderPath if desired. A RenderPath can be simply activated with the `Application::ActivatePath()` function.<br/>
The `Application` does the following every frame while it is running:<br/>
1. FixedUpdate() <br/>
Calls FixedUpdate for the active RenderPath and wakes up scripts that are waiting for fixedupdate(). The frequency off calls will be determined by `Application::setTargetFrameRate(float framespersecond)`. By default (parameter = 60), FixedUpdate will be called 60 times per second.
2. Update(float deltatime) <br/>
Calls Update for the active RenderPath and wakes up scripts that are waiting for update()
3. Render() <br/>
Calls Render for the active RenderPath and wakes up scripts that are waiting for render()
4. Compose()
Calls Compose for the active RenderPath

### RenderPath
[[Header]](../../WickedEngine/wiRenderPath.h)
This is an empty base class that can be activated with a `Application`. It calls its Start(), Update(), FixedUpdate(), Render(), Compose(), Stop() functions as needed. Override this to perform custom gameplay or rendering logic. The RenderPath inherits from [wiCanvas](#wicanvas), and the canvas properties will be set by the [Application](#application) when the RenderPath was activated, and while the RenderPath is active. <br/>
The order in which the functions are executed every frame: <br/>
1. PreUpdate() <br/>
This will be called once per frame before any script that calls Update().
2. FixedUpdate() <br/>
This will be called in a manner that is deterministic, so logic will be running in the frequency that is specified with `Application::setTargetFrameRate(float framespersecond)`
3. Update(float deltatime) <br/>
This will be called once per frame, and the elapsed time in seconds since the last Update() is provided as parameter
4. PostUpdate() <br/>
This will be called once per frame after any script that calls Update().
5. Render() const <br/>
This will be called once per frame. It is const, so it shouldn't modify state. When running this, it is not defined which thread it is running on. Multiple threads and job system can be used within this. The main purpose is to record mass rendering commands in multiple threads and command lists. Command list can be safely retrieved at this point from the graphics device. 
6. Compose(CommandList cmd) const <br/>
It is called once per frame. It is running on a single command list that it receives as a parameter. These rendering commands will directly record onto the last submitted command list in the frame. The render target is the back buffer at this point, so rendering will happen to the screen.

Apart from the functions that will be run every frame, the RenderPath has the following functions:
1. Load() <br/> 
Intended for loading resources before the first time the RenderPath will be used. It will be callsed by [LoadingScreen](#loadingscreen) to load resources in the background.
2. Start() <br/>
Start will always be called when a RenderPath is activated by the `Application`
3. Stop() <br/>
Stop will be always called when the current RenderPath was the active one in `Application`, but an other one was activated.

### RenderPath2D
[[Header]](../../WickedEngine/wiRenderPath2D.h) [[Cpp]](../../WickedEngine/wiRenderPath2D.cpp)
Capable of handling 2D rendering to offscreen buffer in Render() function, or just the screen in Compose() function. It has some functionality to render wiSprite and wiSpriteFont onto rendering layers and stenciling with 3D rendered scene. It has a [GUI](#gui) that is automatically updated and rendered if any elements have been added to it.

### RenderPath3D
[[Header]](../../WickedEngine/wiRenderPath3D.h) [[Cpp]](../../WickedEngine/wiRenderPath3D.cpp)
Base class for implementing 3D rendering paths. It also supports everything that the Renderpath2D does.

The post process chain is also implemented here. This means that the order of the post processes and the resources that they use are defined here, but the individual post process rendering on a lower level is implemented in the `wi::renderer` as core engine features. Read more about post process implementation in the [wi::renderer section](#post-processing). 

The post process chain is implemented in `RenderPath3D::RenderPostprocessChain()` function and divided into multiple parts:
- HDR<br/>
These are using the HDR scene render targets, they happen before tone mapping. For example: Temporal AA, Motion blur, Depth of field
- LDR<br/>
These are running after tone mapping. For example: Color grading, FXAA, chromatic aberration. The last LDR post process result will be output to the back buffer in the `Compose()` function.
- Other<br/>
These are running in more specific locations, depending on the render path. For example: SSR, SSAO, cartoon outline

The HDR and LDR post process chain are using the "ping-ponging" technique, which means when the first post process consumes texture1 and produces texture2, then the following post process will consume texture2 and produce texture1, until all post processes are rendered.

### RenderPath3D_PathTracing
[[Header]](../../WickedEngine/wiRenderPath3D_PathTracing.h) [[Cpp]](../../WickedEngine/wiRenderPath3D_PathTracing.cpp)
Implements a compute shader based path tracing solution. In a static scene, the rendering will converge to ground truth. When something changes in the scene (something moves, ot material changes, etc...), the convergence will be restarted from the beginning. The raytracing is implemented in [wi::renderer](#wi::renderer) and multiple [shaders](#shaders). The ray tracing is available on any GPU that supports compute shaders.

#### Denoiser
To enable denoising for path traced images, you can use the [Open Image Denoise library](https://github.com/OpenImageDenoise/oidn). To enable this functionality, the engine will try to include the "OpenImageDenoise/oidn.hpp" file. If this file could be included, it will attampt to link with OpenImageDenoise.lib and tbb.lib. It is also required to provide the OpenImageDenoise.dll and tbb.dll near the exe to correctly launch the application after this. If you satisfy these steps, the denoiser will work automatically after the path tracer reached the target sample count. The final path traced image will be denoised and presented to the screen.

### LoadingScreen
[[Header]](../../WickedEngine/wiLoadingScreen.h) [[Cpp]](../../WickedEngine/wiLoadingScreen.cpp)
Render path that can be easily used for loading screen. It is able to load content or RenderPath in the background and switch to an other RenderPath onceit finished.

## System
You can find out more about the Entity-Component system and other engine-level systems under ENGINE/System filter in the solution.
### Entity-Component System
[[Header]](../../WickedEngine/wiECS.h)

#### ComponentManager
This is the core entity-component relationship handler class. The purpose of this is to efficiently store, remove, add and sort components. Components can be any movable C++ structure. The best components are simple POD (plain old data) structures.

#### Entity
Entity is a number, it can reference components through ComponentManager containers. An entity is always valid if it exists. It's not required that an entity has any components. An entity has a component, if there is a ComponentManager that has a component which is associated with the same entity.

### Scene System
[[Header]](../../WickedEngine/wiScene.h) [[Cpp]](../../WickedEngine/wiScene.cpp)
The logical scene representation using the Entity-Component System
- GetScene <br/>
Returns a global scene instance. This is a convenience feature for simple applications that need a single scene. The RenderPath3D will use the global scene by default, but it can be set to a custom scene if needed.
- LoadModel() <br/>
There are two flavours to this. One of them immediately loads into the global scene. The other loads into a custom scene, which is usefult to manage the contents separately. This function will return an Entity that represents the root transform of the scene - if the attached parameter was true, otherwise it will return INVALID_ENTITY and no root transform will be created.
- Pick <br/>
Allows to pick the closest object with a RAY (closest ray intersection hit to the ray origin). The user can provide a custom scene or layermask to filter the objects to be checked.
- SceneIntersectSphere <br/>
Performs sphere intersection with all objects and returns the first occured intersection immediately. The result contains the incident normal and penetration depth and the contact object entity ID.
- SceneIntersectCapsule <br/>
Performs capsule intersection with all objects and returns the first occured intersection immediately. The result contains the incident normal and penetration depth and the contact object entity ID.

Below you will find the structures that make up the scene. These are intended to be simple strucutres that will be held in [ComponentManagers](#componentmanager). Keep these structures minimal in size to use cache efficiently when iterating a large amount of components.

<b>Note on bools: </b> using bool in C++ structures is inefficient, because they take up more space in memory than required. Instead, bitmasks will be used in every component that can store up to 32 bool values in each bit. This also makes it easier to add bool flags and not having to worry about serializing them, because the bitfields themselves are already serialized (but the order of flags must never change without handling possible side effects with serialization versioning!). C++ enums are used in the code to manage these bool flags, and the bitmask storing these is always called `uint32_t _flags;` For example:

```cpp
enum FLAGS
{
	EMPTY = 0, // always have empty as first flag (value of zero)
	RENDERABLE = 1 << 0,
	DOUBLE_SIDED = 1 << 1,
	DYNAMIC = 1 << 2,
};
uint32_t _flags = EMPTY; // default value is usually EMPTY, indicating that no flags are set

// How to query the "double sided" flag:
bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }

// How to set the "double sided" flag
void SetDoubleSided(bool value) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
```

It is good practice to not implement constructors and destructors for components. Wherever possible, initialization of values in declaration should be preferred. If desctructors are defined, move contructors, etc. will also need to be defined for compatibility with the [ComponentManager](#componentmanager), so default constructors and destructors should be preferred. Member objects should be able to desctruct themselves implicitly. If pointers need to be stored within the component that manage object lifetime, std::unique_ptr or std::shared_ptr can be used, which will be destructed implicitly.

#### NameComponent
[[Header]](../../WickedEngine/wiScene.h) [[Cpp]](../../WickedEngine/wiScene.cpp)
A string to identify an entity with a human readable name.

#### LayerComponent
[[Header]](../../WickedEngine/wiScene.h) [[Cpp]](../../WickedEngine/wiScene.cpp)
A bitmask that can be used to filter entities. Depending on what other components the entity has, the layer will be used by different systems, to for example determine visibility, filter collisions, or picking.
A bitmask of 0 will make the entity inactive in most systems.
The lower 8 bits will be used by ray tracing as an instance inclusion mask for instances (ObjectComponent, EmittedParticle, HairParticle..)

#### TransformComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
Orientation in 3D space, which supports various common operations on itself.

#### PreviousFrameTransformComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
Absolute orientation in the previous frame (a matrix).

#### HierarchyComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
An entity can be part of a transform hierarchy by having this component. Some other properties can also be inherieted, such as layer bitmask. If an entity has a parent, then it has a HierarchyComponent, otherwise it's not part of a hierarchy.

#### MaterialComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
Several properties that define a material, like color, textures, etc...

#### MeshComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
A mesh is an array of triangles. A mesh can have multiple parts, called MeshSubsets. Each MeshSubset has a material and it is using a range of triangles of the mesh. This can also have GPU resident data for rendering.

#### ImpostorComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
Supports efficient rendering of the same mesh multiple times (but as an approximation, such as a billboard cutout). A mesh can be rendered as impostors for example when it is not important, but has a large number of copies.

#### ObjectComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
An ObjectComponent is an instance of a mesh that has physical location in a 3D world. Multiple ObjectComponents can have the same mesh, and in this case the mesh will be rendered multiple times efficiently. It is expected that an entity that has ObjectComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.

#### RigidBodyPhysicsComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
Stores properties required for rigid body physics simulation and a handle that will be used by the physicsengine internally.

#### SoftBodyPhysicsComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
Stores properties required for soft body physics simulation and a handle that will be used by the physicsengine internally.

#### ArmatureComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
A skeleton used for skinning deformation of meshes.

#### LightComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
A light in the scene that can shine in the darkness. It is expected that an entity that has LightComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.

#### CameraComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)

#### EnvironmentProbeComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
It is expected that an entity that has EnvironmentProbeComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.

#### ForceFieldComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)

#### DecalComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
It is expected that an entity that has DecalComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.

#### AnimationDataComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)

#### AnimationComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)

#### WeatherComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)

#### SoundComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
Holds a Sound and a SoundInstance, and it can be placed into the scene via a TransformComponent. It can have a 3D audio effect it has a TransformComponent.

#### InverseKinematicsComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
If an entity has an `InverseKinematicComponent` (IK), and part of a transform hierarchy (has both [TransformComponent](#transformcomponent) and a [HierarchyComponent](#hierarchycomponent)), then it can be targetted to an other [ThransformComponent](#transformcomponent). The parent transforms will be computed in order to let the IK reach the target if possible. The parent transforms will be only rotated. For example, if a hand tries to reach for an object, the hand and shoulder will move accordingly to let the hand reach.
The `chain_length` can be specified to let the IK system know how many parents should be computed. It can be greater than the real chain length, in that case there will be no more simulation steps than the length of hierarchy chain.
The `iteration_count` can be specified to increase accuracy of the computation.
If animations are also playing on the affected entities, the IK system will override the animations.

#### SpringComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
An entity can have a `SpringComponent` to achieve a "jiggle" or "soft" animation effect programatically. The effect will work automatically if the transform is changed by animation system for example, or in any other way. The parameter `stiffness` specifies how fast the transform tries to go back to its initial position. The parameter `damping` specifies how fast the transform comes to rest position. The `wind_affection` parameter specifies how much the global wind applies to the spring.

#### ColiderComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
The collider component will specify a collider shape to be used in simple fake physics simulations. It will affect Springs and GPU particle effect.

#### ScriptComponent
[[Header]](../../WickedEngine/wiScene_Components.h) [[Cpp]](../../WickedEngine/wiScene_Components.cpp)
ScriptComponent can reference a lua script and run it every frame, while also providing some additional data to script like a local GetEntity() function. The script can be written to reference additional component data by using its unique GetEntity() function. A ScriptComponent can also call other scripts, which can be used to implement multiple scripts on one entity.

#### Scene
[[Header]](../../WickedEngine/wiScene.h) [[Cpp]](../../WickedEngine/wiScene.cpp)
A scene is a collection of component arrays. The scene is updating all the components in an efficient manner using the [job system](#job-system). It can be serialized and saved/loaded from disk efficiently.
- Update(float deltatime) <br/>
This function runs all the requied systems to update all components contained within the Scene.

### Job System
[[Header]](../../WickedEngine/wiJobSystem.h) [[Cpp]](../../WickedEngine/wiJobSystem.cpp)
Manages the execution of concurrent tasks
- context <br/>
Defines a single workload that can be synchronized. It is used to issue jobs from within jobs and properly wait for completion. A context can be simply created on the stack because it is a simple atomic counter.
- Execute <br/>
This will schedule a task for execution on a separate thread for a given workload
- Dispatch <br/>
This will schedule a task for execution on multiple parallel threads for a given workload
- Wait <br/>
This function will block until all jobs have finished for a given workload. The current thread starts working on any work left to be finished.

### Initializer
[[Header]](../../WickedEngine/wiInitializer.h) [[Cpp]](../../WickedEngine/wiInitializer.cpp)
Initializes all engine systems either in a blocking or an asynchronous way.

### Platform
[[Header]](../../WickedEngine/wiPlatform.h)
You can get native platform specific functionality.

### Event Handler
[[Header]](../../WickedEngine/wiEventHandler.h)
The event system can be used to execute system-wide tasks. Any system can "subscribe" to events and any system can "fire" events.
- Subscribe <br/>
The first parameter is the event ID. Core system events are negative numbers. The user can choose any positive number to create custom events. 
The second parameter is a function to be executed, with a userdata argument. The userdata argument can hold any custom data that the user desires.
The return value is a `wi::eventhandler::Handle` type. When this is object is destroyed, the event is subscription for the function will be removed.
Multiple functions can be subscribed to a single event ID.
- FireEvent <br/>
The first argument is the event id, that says which events to invoke. 
The second argument will be passed to the subscribed event's userdata parameter.
All events that are subsribed to the specified event id will run immediately at the time of the call of FireEvent. The order of execution among events is the order of which they were subscribed.

### Canvas
[[Header]](../../WickedEngine/wiCanvas.h)
The canvas specifies a DPI-aware drawing area. Use the GetLogical... functions to get parameters with DPI aware scale factors applied. Use the GetPhysical... functions to get the real pixel counts.


## Graphics
Everything related to rendering graphics will be discussed below

### Graphics API
[[Header]](../../WickedEngine/wiGraphics.h)
The graphics API wrappers

#### GraphicsDevice
[[Header]](../../WickedEngine/wiGraphicsDevice.h) [[Cpp]](../../WickedEngine/wiGraphicsDevice.cpp)
This is the interface for the graphics API abstraction. It is higher level than a graphics API, but these are the lowest level of rendering commands the engine offers.

##### Debug device
When creating a graphics device, it can be created in special debug mode to assist development. For this, the user can specify `debugdevice` as command line argument to the application, or set the `debuglayer` argument of the graphics device constructor to `true` when creating the device. The errors will be posted to the Console Output window in the development environment.

##### Creating resources
Functions like `CreateTexture()`, `CreateBuffer()`, etc. can be used to create corresponding GPU resources. Using these functions is thread safe. The resources will not necessarily be created immediately, but by the time the GPU will want to use them. These functions imediately return `false` if there were any errors, such as wrong parameters being passed to the description parameters, and they will return `true` if everything is correct. If there was an error, please use the [debug device](#debug-device) functionality to get additional information. When passing a resource to these functions that is already created, it will be destroyed, then created again with the newly provided parameters.

##### Destroying resources
Resources will be destroyed automatically by the graphics device when they are no longer used.

##### Work submission
Rendering commands that expect a `CommandList` as a parameter are not executed immediately. They will be recorded into command lists and submitted to the GPU for execution upon calling the `SubmitCommandLists()` function. The `CommandList` is a simple handle that associates rendering commands to a CPU execution timeline. The `CommandList` is not thread safe, so every `CommandList` can be used by a single CPU thread at a time to record commands. In a multithreading scenario, each CPU thread should have its own `CommandList`. `CommandList`s can be retrieved from the [GraphicsDevice](#graphicsdevice) by calling `GraphicsDevice::BeginCommandList()` that will return a `CommandList` handle that is free to be used from that point by the calling thread. All such handles will be in use until `SubmitCommandLists()` or `PresentEnd()` was called, where GPU submission takes place. The command lists will be submitted in the order they were retrieved with `GraphicsDevice::BeginCommandList()`. The order of submission correlates with the order of actual GPU execution. For example:

```cpp
CommandList cmd1 = device->BeginCommandList();
CommandList cmd2 = device->BeginCommandList();

Read_Shadowmaps(cmd2); // CPU executes these commands first
Render_Shadowmaps(cmd1); // CPU executes these commands second

//...

CommandList cmd_present = device->BeginCommandList();

device->SubmitCommandLists(cmd_present); // CPU submits work for GPU
// The GPU will execute the Render_Shadowmaps() commands first, then the Read_Shadowmaps() commands second
```

When submitting command lists with `SubmitCommandLists()`, the CPU thread can be blocked in cases when there is too much GPU work submitted already that didn't finish. It's not appropriate to start recording new command lists until `SubmitCommandLists()` finished.

Furthermore, the `BeginCommandList()` is thread safe, so the user can call it from worker threads if ordering between command lists is not a requirement (such as when they are producing workloads that are independent of each other).

###### Async compute
Async workload can be performed on the compute queue with `CommandList` granularity. The `BeginCommandList()` function has an optional `QUEUE_TYPE` parameter, which can be specified to execute the command list on the specified queue. By default, if no such parameter is specified, the work will be executed on the main graphics queue (`QUEUE_GRAPHICS`), serially to other such graphics work. In case the graphics device or API doesn't support async compute (such as DX11), then the `QUEUE_GRAPHICS` will be assumed. The main graphics queue can execute both graphics and compute work, but the `QUEUE_COMPUTE` can only execute compute work. Two queues can also be synchronized with `CommandList` granularity, with the `WaitCommandList()` function. This function inserts a dependency barrier before the first parameter command list, that synchronizes with the second parameter command list. For example:

```cpp
CommandList cmd0 = device->BeginCommandList(QUEUE_GRAPHICS);
CommandList cmd1 = device->BeginCommandList(QUEUE_COMPUTE);
device->WaitCommandList(cmd1, cmd0); // cmd1 waits for cmd0 to finish
CommandList cmd2 = device->BeginCommandList(QUEUE_GRAPHICS); // cmd2 doesn't wait, it runs async with cmd1
CommandList cmd3 = device->BeginCommandList(QUEUE_GRAPHICS);
device->WaitCommandList(cmd3, cmd1); // cmd3 waits for cmd1 to finish

device->SubmitCommandLists(); // execute all of the above
```

The `WaitCommandList()` function is a GPU wait operation, so it will not block CPU execution. Furthermore, it is not required to use this between two `CommandList`s that are on the same queue, because the synchronization between those is implicit.

Important: The `SHADER_RESOURCE` state cannot be used on the compute queue. The device could convert these to `SHADER_RESOURCE_COMPUTE` while issuing `Barrier()` commands. However, the starting resource state must be correctly specified, because those cannot be converted. Consider always choosing a `SHADER_RESOURCE_COMPUTE` starting resource state if the resource is going to be used in a compute queue, and transition them to regular `SHADER_RESOURCE` only before the resource is going to be used in a pixel shader. The graphics queue with compute commands doesn't have this limitation however.

##### Presenting to the screen
To present to the screen (an operating system window), first create a `SwapChain` with the `CreateSwapChain()` function that will be associated with a window. The SwapChain acts as a special kind of [RenderPass](#render-passes), so there is a `BeginRenderPass()` function with an overload that accepts a SwapChain parameter instead of a RenderPass. Simply use this `BeginRenderPass()` and `EndRenderPass()` to draw to the SwapChain. The final presentation will happen when calling `SubmitCommandLists()`.

###### HDR Display
To present content to a HDR display, set the `SwapChainDesc::allow_hdr` to `true` when creating the `SwapChain`. Also, select a texture format for the swapChain that can be used for HDR content. The formats that are capable of HDR, are:
- `R10G10B10A2_UNORM`, which supports both the `SRGB` (which is SDR) and `HDR10_ST2084` (HDR10) color spaces.
- `R16G16B16A16_FLOAT`, which supports the `HDR_LINEAR` color space.

If the display associated with the `SwapChain` doesn't support HDR output, HDR will be disabled and the `SwapChain` can fall back to an appropriate format that is supported. To check the final color space of the `SwapChain`, call the `GraphicsDevice::GetSwapChainColorSpace()` function providing a valid `SwapChain` as argument. The function returns the actual `COLOR_SPACE` of the `SwapChain`.
To check whether the display associated with the `SwapChain` is HDR capable, call the `GraphicsDevice::IsSwapChainSupportsHDR()` function providing a valid `SwapChain` as argument. This will return whether the display supports HDR or not, regardless of the current format of the `SwapChain`.

It is not enough to set up a HDR `SwapChain` to render correct HDR content, because great care must be taken to blend elements in linear color space, and correctly convert to the display's color space before presenting. This is a responsibility of shaders.

##### Resource binding
The resource binding model is based on DirectX 11. That means, resources are bound to slot numbers that are simple integers.

Declare a resource bind point with HLSL syntax:
```cpp
Texture2D<float4> myTexture : register(t5);
```
The slot number `t5` must be specified to avoid the HLSL compiler assign an arbitrary slot to it and will be accessible from the CPU. 

The user can bind the texture resource from the CPU side:
```cpp
Texture myTexture;
// after texture was created, etc:
device->BindResource(&myTexture, 5, cmd);
```

Other than this, resources like `Texture` can have different subresources, so an extra parameter can be specified to the `BindResources()` function called `subresource`:
```cpp
Texture myTexture;
// after texture was created, etc:
device->BindResource(&myTexture, 5, cmd, 42);
```
By default, the `subresource` parameter is `-1`, which means that the entire resource will be bound. For more information about subresources, see the [Subresources](#subresources) section.

There are different resource types regarding how to bind resources. These slots have separate binding points from each other, so they don't interfere between each other.
- Shader resources<br/>
These are read only resources. `GPUBuffer`s and `Texture`s can be bound as shader resources if they were created with a `BindFlags` in their description that has the `SHADER_RESOURCE` bit set. Use the `GraphicsDevice::BindResource()` function to bind a single shader resource, or the `GraphicsDevice::BindResources()` to bind a bundle of shader resources, occupying a number of slots starting from the bind slot. Use the `GraphicsDevice::UnbindResources()` function to unbind several shader resource slots manually, which is useful for removing debug device warnings.
- UAV<br/>
Unordered Access Views, in other words resources with read-write access. `GPUBuffer`s and `Texture`s can be bound as shader resources if they were created with a `BindFlags` in their description that has the `UNORDERED_ACCESS` bit set. Use the `GraphicsDevice::BindUAV()` function to bind a single UAV resource, or the `GraphicsDevice::BindUAVs()` to bind a bundle of UAV resources, occupying a number of slots starting from the bind slot. Use the `GraphicsDevice::UnbindUAVs()` function to unbind several UAV slots manually, which is useful for removing debug device warnings.
- Constant buffers<br/>
Only `GPUBuffer`s can be set as constant buffers if they were created with a `BindFlags` in their description that has the `CONSTANT_BUFFER` bit set. The resource can't be a constant buffer at the same time when it is also a shader resource or a UAV or a vertex buffer or an index buffer. Use the `GraphicsDevice::BindConstantBuffer()` function to bind constant buffers.
- Samplers<br/>
Only `Sampler` can be bound as sampler. Use the `GraphicsDevice::BindSampler()` function to bind samplers.

There are some limitations on the maximum value of slots that can be used, these are defined as compile time constants in [Graphics device SharedInternals](../WickedEngine/wiGraphicsDevice_SharedInternals.h). The user can modify these and recompile the engine if the predefined slots are not enough. This could slightly affect performance.

Remarks:
- Vulkan and DX12 devices make an effort to combine descriptors across shader stages, so overlapping descriptors will not be supported with those APIs to some extent. For example it is OK, to have a constant buffer on slot 0 (b0) in a vertex shader while having a Texture2D on slot 0 (t0) in pixel shader. However, having a StructuredBuffer on vertex shader slot 0 (t0) and a Texture2D in pixel shader slot 0 (t0) will not work correctly, as only one of them will be bound to a pipeline state. This is made for performance reasons.
- This slot based binding model has a CPU overhead that has to be kept in mind. Avoid binding massive amount of resources. The maximum slot numbers that should be used are: 14 for `BindConstantBuffer()`, 16 for `BindResource()`, 16 for `BindUAV()` and 8 for `BindSampler()`. Consider using a [bindless model](#bindless-resources) when you need to bind more than a couple of resources.

##### Bindless resources

Wicked Engine supports bindless resource management, this can greatly improve performance and removes resource binding constraints to allow great flexibility.

Related functions to this feature:
- `GetDescriptorIndex()` : returns an `int` that identifies the resource in bindless space. The queried resource can be a `Sampler` or a `GPUResource`. If the resource is not usable (for example if it was not created), then the function returns `-1`. **In this case, the shaders must not use the resource, but instead rely on dynamic branching to avoid it, because this would be undefined behaviour and could result in a GPU hang**. Otherwise, the index can be used by shaders to index into a descriptor heap.
- `PushConstants()` : This is an easy way to set a small amount of 32-bit values on the GPU, usable by shaders that declared a `PUSHCONSTANT(name, type)` block. There can be one push constant block per pipeline (graphics, compute or raytracing). The push constants will be bound to the last set pipeline, so only use this after binding a graphics pipeline state or compute shader.

The shaders can use bindless descriptors with the following syntax example:

```hlsl
Texture2D<float4> bindless_textures[] : register(t0, space5);
struct PushConstants
{
	uint materialIndex;
	int textureindex;
};
PUSHCONSTANT(push, PushConstants);
// ...
float4 color = bindless_textures[push.textureindex].Sample(sam, uv);
```

The corresponding data can be fed from the CPU like this:

```cpp
struct PushConstants
{
	uint materialIndex;
	int textureindex;
};
PushConstants push;
push.materialIndex = device->GetDescriptorIndex(materialCB, SRV);
push.textureindex = device->GetDescriptorIndex(texture, SRV);
device->PushConstants(&push, sizeof(push), cmd);
```

*Note: a descriptor array of ConstantBuffer<T> could be supported by some hardware/API (eg. Vulkan) in a limited form even though bindless descriptors are supported, so avoid relying on it too much.*

The regular slot based binding model will work alongside the bindless model seamlessly. The slot based bindings will always use `space0` implicitly, `space1` and greater should be used for the bindless model. Aim to keep space numbers as low as possible.

##### Subresources
Resources like textures can have different views. For example, if a texture contains multiple mip levels, each mip level can be viewed as a separate texture with one mip level, or the whole texture can be viewed as a texture with multiple mip levels. When creating resources, a subresource that views the entire resource will be created. Functions that expect a subresource parameter can be provided with the value `-1` that means the whole resource. Usually, this parameter is optional.

Other subresources can be create with the `GraphicsDevice::CreateSubresource()` function. The function will return an `int` value that can be used to refer to the subresource view that was created. In case the function returns `-1`, the subresource creation failed due to an incorrect parameter. Please use the [debug device](#debug-device) functionality to check for errors in this case. The use of `CreateSubresource()` function is thread safe on the device, but not on the resource, so only 1 thread must modify a given resource at a time.

The subresource indices are valid as long as the resource is valid that they were created with.

##### Pipeline States and Shaders
`PipelineState`s are used to define the graphics pipeline state, that includes which shaders are used, which blend mode, rasterizer state, input layout, depth stencil state and primitive topology, as well as sample mask are in effect. These states can only be bound atomically in a single call to `GraphicsDevice::SetPipelineState()`. This does not include compute shaders, which do not participate in the graphics pipeline state and can be bound individually using `GraphicsDevice::BindComputeShader()` method.

The pipeline states are subject to shader compilations. Shader compilation will happen when a pipeline state is bound inside a render pass for the first time. This is required because the render target formats are necessary information for compilation, but they are not part of the pipeline state description. This choice was made for increased flexibility of defining pipeline states. However, unlike APIs where state subsets (like RasterizerDesc, or BlendStateDesc) can be bound individually, the grouping of states is more optimal regarding CPU time, because state hashes are computed only once for the whole pipeline state at creation time, as opposed to binding time for each individual state. This approach is also less prone to user error when the developer might forget setting any subset of state and the leftover state from previous render passes are incorrect. 

Shaders still need to be created with `GraphicsDevice::CreateShader()` in a similar to CreateTexture(), etc. This could result in shader compilation/hashing in some graphics APIs like DirectX 11. The CreateShader() function expects a `wiGraphics::ShaderStage` enum value which will define the type of shader:

- `MS`: Mesh Shader
- `AS`: Amplification Shader, or Task Shader
- `VS`: Vertex Shader
- `HS`: Hull Shader, or Tessellation Control Shader
- `DS`: Domain Shader, or Tessellation Evaluation Shader
- `GS`: Geometry Shader
- `PS`: Pixel Shader
- `CS`: Compute Shader
- `LIB`: Library shader

Depending on the graphics device implementation, the shader code must be different format. For example, DirectX expects DXIL shaders, Vulkan expects SPIR-V shaders. The engine can compile HLSL shader source code to DXIL or SPIRV format with the [Shader Compiler](#shader-compiler).

Note: As an optional paramter, `CreatePipelineState()` function also accepts a `RenderPassInfo` pointer. If this is provided, then all the information will be available for pipeline creation to happen immediately. This means the pipeline will be immediately in final usable state after the function returns, but it can take longer time for this to complete. If the parameter is not provided, the pipeline could be created at a later time, when the render pass information is available on first use. This can have the effect of the longer pipeline compilation happening at an unexpected time. But in this case, the pipeline will also be compatible with more render pass types if it's used at different places.

##### Render Passes
Render passes are defining regions in GPU execution where a number of render targets or depth buffers will be used to render into them. Render targets and depth buffers are defined as `RenderPassImage`s. The `RenderPassImage`s have a pointer to the texture, state the resource type (`RENDERTARGET`, `DEPTH_STENCIL` or `RESOLVE`), state the [subresource](#subresources) index, the load and store operations, and the layout transitions for the textures.

- `RENDERTARGET`: The image will be used as a (color) render target. The order of these images define the shader color output order.
- `DEPTH_STENCIL`: The image will be used as a depth (and/or stencil) buffer.
- `RESOLVE`: The image will be used as MSAA resolve destination. The resolve source is chosen among the `RENDERTARGET` images in the same render pass, in the order they were declared in. The declaration order of the `RENDERTARGET` and `RESOLVE` images must match to correctly deduce source and destination targets for resolve operations.
- `RESOLVE_DEPTH` same as `RESOLVE` but for depth stencil. You can also specify Min or Max resolve operation with the `RenderPassImage::DepthResolveMode` enum. This feature must be supported by GPU and indicated by `GraphicsDeviceCapability::DEPTH_RESOLVE_MIN_MAX` and `GraphicsDeviceCapability::STENCIL_RESOLVE_MIN_MAX` (if image also has stencil)

- Load Operation: <br/>
Defines how the texture contents are initialized at the start of the render pass. `LoadOp::LOAD` says that the previous texture content will be retained. `LoadOp::CLEAR` says that the previous contents of the texture will be lost and instead the texture clear color will be used to fill the texture. `LoadOp::DONTCARE` says that the texture contents are undefined, so this should only be used when the developer can ensure that the whole texture will be rendered to and not leaving any region empty (in which case, undefined results will be present in the texture).
- Store operation: <br/>
Defines how the texture contents are handled after the render pass ends. `StoreOp::STORE` means that the contents will be preserved. `StoreOp::DONTCARE` means that the contents won't be necessarily preserved, they are only temporarily valid within the duration of the render pass, which can save some memory bandwidth on some platforms (specifically tile based rendering architectures, like mobile GPUs).
- Layout transition: <br/>
Define the `layout_before`, `layout` (only for `DEPTH_STENCIL`) and `layout_after` members to have an implicit transition performed as part of the render pass, that works like an [IMAGE_BARRIER](#gpu-barriers), but can be more optimal. The `layout_before` states the starting state of the resource. The resource will be transitioned from `layout_before` to `layout` within the render pass. The `layout` states how the resource is accessed within the render pass. For `DEPTH_STENCIL` type, it must be either `DEPTHSTENCIL` or `DEPTHSTENCIL_READONLY`. For `RESOLVE` type, the subpass_layout have no meaning, it is implicitly defined. At the end of the render pass, the resources will be transitioned from `layout` to `layout_after`.

Notes:
- When `RenderPassBegin()` is called, `RenderPassEnd()` must be called after on the same command list before the command list gets [submitted](#work-submission).
- It is not allowed to call `RenderPassBegin()` inside a render pass.
- It is not allowed to call `CopyResource()`, `CopyTexture()`, `CopyBuffer()` inside a render pass.
- It is not allowed to call `Dispatch()`, `DispatchIndirect()`, `DispatchMesh()`, `DispatchMeshIndirect()`, `DispatchRays()` inside a render pass.
- It is not allowed to call `UpdateBuffer()` inside the render pass.
- It is not allowed to call `Barrier()` inside the render pass.
- It is not allowed to call `ClearUAV()` inside the render pass.

##### GPU Barriers
`GPUBarrier`s can be used to state dependencies between GPU workloads. There are different kinds of barriers:

- MEMORY_BARRIER <br/>
Memory barriers are used to wait for UAV writes to finish, or in other words to wait for shaders to finish that are writing to a UNORDERED_ACCESS resource. The `GPUBarrier::memory.resource` member is a pointer to the GPUResource to wait on. If it is nullptr, than the barrier means "wait for every UAV write that is in flight to finish".
- IMAGE_BARRIER <br/>
Image barriers are stating resource state transition for [textures](#textures). The most common use case for example is to transition from `RENDERTARGET` to `SHADER_RESOURCE`, which means that the [RenderPass](#render-passes) that writes to the texture as render target must finish before the barrier, and the texture can be used as a read only shader resource after the barrier. There are other cases that can be indicated using the `GPUBarrier::image.layout_before` and `GPUBarrier::image.layout_after` states. The `GPUBarrier::image.resource` is a pointer to the resource which will have its state changed. If the texture's `layout` (as part of the TextureDesc) is not `SHADER_RESOURCE`, the layout must be transitioned to `SHADER_RESOURCE` before binding as shader resource. The image layout can also be transitioned using a [RenderPass](#render-passes), which should be preferred to `GPUBarrier`s.
- BUFFER_BARRIER <br/>
Similar to `IMAGE_BARRIER`, but for [GPU Buffer](#gpu-buffers) state transitions.

It is very important to place barriers to the right places if using low level graphics devices APIs like [DirectX 12](#graphicsdevice_dx12) or [Vulkan](#graphicsdevice_vulkan). Missing a barrier could lead to corruption of rendered results, crashes and generally undefined behaviour. However, barriers don't have an effect for [DirectX 11](#graphicsdevice_dx11), they are handled automatically. The [debug layer](#debug-device) will help to detect errors and missing barriers.

##### Textures
`Texture` type resources are used to store images which will be sampled or written by the GPU efficiently. There are a lot of texture types, such as `Texture1D`, `Texture2D`, `TextureCube`, `Texture3D`, etc. used in [shaders](#shaders), that correspond to the simple `Texture` type on the CPU. The texture type will be determined when creating the resource with `GraphicsDevice::CreateTexture(const TextureDesc*, const SubresourceData*, Texture*)` function. The first argument is the `TextureDesc` that determines the dimensions, size, format and other properties of the texture resource. The `SubresourceData` is used to initialize the texture contents, it can be left as `nullptr`, when the texture contents don't need to be initialized, such as for textures that will be rendered into. The `Texture` is the texture that will be initialized.

`SubresourceData` usage: 
- The `SubresourceData` parameter points to an array of `SubresourceData` structs. The array size is determined by the `TextureDesc::ArraySize` * `TextureDesc::MipLevels`, so one structure for every subresource. 
- The `SubresourceData::pData` pointer should point to the texture contents on the CPU that will be uploaded to the GPU.
- The `SubresourceData::rowPitch` member is indicating the distance (in bytes) from the beginning of one line of a texture to the next line.
System-memory pitch is used only for 2D and 3D texture data as it is has no meaning for the other resource types. Specify the distance from the first pixel of one 2D slice of a 3D texture to the first pixel of the next 2D slice in that texture in the `slicePitch` member.
- The `SubresourcData::slicePitch` member is indicating the distance (in bytes) from the beginning of one depth level to the next.
System-memory-slice pitch is only used for 3D texture data as it has no meaning for the other resource types.
- For more complete information, refer to the [DirectX 11 documentation](https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_subresource_data) on this topic, which should closely match.

Related topics: [Creating Resources](#creating-resources), [Destroying Resources](#destroying-resources), [Binding resources](#resource-binding), [Subresources](#subresources)

##### GPU Buffers
`GPUBuffer` type resources are used to store linear data which will be read or written by the GPU. There are a lot of buffer types, such as structured buffers, raw buffers, vertex buffers, index buffers, typed buffers, etc. used in [shaders](#shaders), that correspond to the simple `GPUBuffer` type on the CPU. The buffer type will be determined when creating the buffer with `GraphicsDevice::CreateBuffer(const GPUBufferDesc*, const void*, GPUBuffer*)` function. The first argument is the `GPUBufferDesc` that determines the dimensions, size, format and other properties of the texture resource. The `pInitialData` is used to initialize the buffer contents, it can be left as `nullptr`. The `GPUBuffer` is the buffer that will be initialized.

Related topics: [Creating Resources](#creating-resources), [Destroying Resources](#destroying-resources), [Binding resources](#resource-binding), [Subresources](#subresources), [Updating GPU Buffers](#updating-gpu-buffers)

##### Updating GPU buffers
A `GPUBuffer`'s `Usage` parameter specifies how the buffer memory is accessed.
- `DEFAULT`: The buffer memory is visible to the GPU but not the CPU. This means that it will observe maximum GPU performance, but special care needs to be taken to write the buffer contents. The GPU could write the memory from a shader or copy operation for example. You can also use the `UpdateBuffer()` function to update such a buffer from CPU (which uses a GPU copy).
- `UPLOAD`: The buffer can be written by the CPU and read by the GPU. Once such a `GPUBuffer` was created, it's memory is persistently mapped for CPU access, and can be accessed through the `GPUResource::mapped_data` pointer. It's perfect to update a `DEFAULT` buffer from this by first filling the `UPLOAD` buffer from the CPU, then let the GPU copy its contents to the `DEFAULT` buffer with a shader or copy operation.
- `READBACK`: The buffer can be written by the GPU and the contents read by the CPU. The buffer memory is persistently mapped after creation, and accessible through the `GPUResource::mapped_data` pointer.

##### GPU Queries
The `GPUQueryHeap` can be used to retrieve information from GPU to CPU. There are different query types (`GpuQueryType` enum):
- `TIMESTAMP` is used to write a timestamp value to the designated query inside the heap. Use `GraphicsDevice::QueryEnd()` to record this type.
- `OCCLUSION` is used to retrieve depth test passed sample count for drawing commands between `GraphicsDevice::QueryBegin()` and `GraphicsDevice::QueryEnd()`.
- `OCCLUSION_BINARY` is the same as `OCCLUSION`, but only returns whether any samples passed depth test or not. It can be more optimal than `OCCLUSION`.

The `GPUQueryHeap` is designed to retrieve query results in a bulk, instead of one by one, which can be implemented more optimally. However, retrieving queries one by one will still be possible if needed. The `GraphicsDevice::QueryResolve()` function will issue a GPU operation that will write the query results to a CPU visible heap. The `GraphicsDevice::QueryRead()` function can be called on the CPU timeline to read resolved query data in bulk. Reading queries must be done when the GPU finished executing the `GraphicsDevice::QueryResolve()`, which is usually after a few frames of latency. 

##### RayTracingAccelerationStructure
The acceleration strucuture can be bottom level or top level, and decided by the description strucutre's `type` field. Depending on the `type`, either the `toplevel` or `bottomlevel` member must be filled out before creating the acceleration structure. When creating the acceleration structure with the grpahics device (`GraphicsDevice::CreateRaytracingAccelerationStructure()`), sufficient backing memory is allocated, but the acceleration structure is not built. Building the acceleration structure is performed on the GPU timeline, via the `GraphicsDevice::BuildRaytracingAccelerationStructure()`. To build it from scratch, leave the `src` argument of this function to `nullptr`. To update the acceleration structure without doing a full rebuild, specify a `src` argument. The `src` can be the same acceleration structure, in this case the update will be performed in place. To be able to update an acceleration structure, it must have been created with the `RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE` flag.

##### RayTracingPipelineState
Binding a ray tracing pipeline state is required to dispatch ray tracing shaders. A ray tracing pipeline state holds a collection of shader libraries and hitgroup definitions. It also declares information about max resource usage of the pipeline.

##### Variable Rate Shading
Variable Rate Shading can be used to decrease shading quality while retaining depth testing accuracy. The shading rate can be set up in different ways:

- `BindShadingRate()`: Set the shading rate for the following draw calls. The first parameter is the shading rate, which is by default `RATE_1X1` (the best quality). The increasing enum values are standing for decreasing shading rates.
- Shading rate image: Set the shading rate for the screen via a tiled texture. The texture must be set as a RenderPassAttachment of `RATE_SOURCE` type. The texture must be using the `R8_UINT` format. In each pixel, the texture contains the shading rate value for a tile of pixels (8x8, 16x16 or 32x32). The tile size can be queried via `GetVariableRateShadingTileSize()`. The shading rate values that the texture contains are not the raw values from `ShadingRate` enum, but they must be converted to values that are native to the graphics API used using the `WriteShadingRateValue()` function. The shading rate texture must be written with a compute shader and transitioned to `RATE_SOURCE` with a [GPUBarrier](#gpu-barriers) before setting it with `BindShadingRateImage()`. It is valid to set a `nullptr` instead of the texture, indicating that the shading rate is not specified by a texture.
- Or setting the shading rate from a vertex or geometry shader with the `SV_ShadingRate` system value semantic.

The final shading rate will be determined from the above methods using the maximum shading rate (least detailed) which is applicable to the screen tile. In the future it might be considered to expose the operator to define this.

To read more about variable rate shading, refer to the [DirectX specifications.](https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading)


##### Video Decoding
Video decoding can be used to decode compressed video bitstream in real time on the GPU. Currently only the H264 format is supported. To decode a video, the following steps need to be taken:
- Provide H264 slice data in a `GPUBuffer`. The slice data offset needs to be aligned within the buffer with `GraphcisDevice::GetVideoDecodeBitstreamAlignment()`. The bitstream buffer needs to be an `UPLOAD` buffer currently.
- Create `VideoDecoder` object, while providing appropriate paraemters parsed from H264 such as resolution, picture parameters, sequence parameters. The decode format must be `Format::NV12` which is a multi planar YUV420 format.
- Create a "Decode Picture Buffer" (DPB), which is a texture array storing reference and decoded images in the native video format (currently only `Format::NV12` is supported). Indicate in the `misc_flags` that it will be used as `ResourceMiscFlags::VIDEO_DECODE`.
- Run the `GraphicsDevice::VideoDecode` operation providing correct arguments to decode a single video frame. You are responsible to provide correct H264 arguments and DPB picture indices.
- You will need to manually read from the DPB texture (eg. in a shader) and resolve the YUV format to RGB if you want.

All this is demonstrated in the [wi::video](../../WickedEngine/wiVideo.cpp) implementation.


#### GraphicsDevice_DX11
The DirectX11 interface has been removed after version 0.56

#### GraphicsDevice_DX12
[[Header]](../../WickedEngine/wiGraphicsDevice_DX12.h) [[Cpp]](../../WickedEngine/wiGraphicsDevice_DX12.cpp)
DirectX12 implementation for rendering interface

#### GraphicsDevice_Vulkan
[[Header]](../../WickedEngine/wiGraphicsDevice_Vulkan.h) [[Cpp]](../../WickedEngine/wiGraphicsDevice_Vulkan.cpp)
Vulkan implementation for rendering interface


### Renderer
[[Header]](../../WickedEngine/wiRenderer.h) [[Cpp]](../../WickedEngine/wiRenderer.cpp)
This is a collection of graphics technique implentations and functions to draw a scene, shadows, post processes and other things. It is also the manager of the GraphicsDevice instance, and provides other helper functions to load shaders from files on disk.

Apart from graphics helper functions that are mostly independent of each other, the renderer also provides facilities to render a Scene. This can be done via the high level DrawScene, DrawSky, etc. functions. These don't set up render passes or viewports by themselves, but they expect that they are set up from outside. Most other render state will be handled internally, such as constant buffers, stencil, blendstate, etc.. Please see how the scene rendering functions are used in the High level interface RenderPath3D implementations (for example [RenderPath3D_TiledForward.cpp](../WickedEngine/RenderPath3D_TiledForward.cpp))

Other points of interest here are utility graphics functions, such as CopyTexture2D, GenerateMipChain, and the like, which provide customizable operations, such as border expand mode, Gaussian mipchain generation, and things that wouldn't be supported by a graphics API.

The renderer also hosts post process implementations. These functions are independent and have clear input/output parameters. They shouldn't rely on any other extra setup from outside.

Read about the different features of the renderer in more detail below:

#### DrawScene
Renders the scene from the camera's point of view that was specified as parameter. Only the objects withing the camera [Frustum](#frustum) will be rendered. The objects will be sorted from front-to back. This is an optimization to reduce overdraw, because for opaque objects, only the closest pixel to the camera will contribute to the rendered image. Pixels behind the frontmost pixel will be culled by the GPU using the depth buffer and not be rendered. The sorting is implemented with RenderQueue internally. The RenderQueue is responsible to sort objects by distance and mesh index, so instaced rendering (batching multiple drawable objects into one draw call) and front-to back sorting can both work together. 

The `renderPass` argument will specify what kind of render pass we are using and specifies shader complexity and rendering technique.
The `cmd` argument refers to a valid [CommandList](#work-submission)
The `flags` argument can contain various modifiers that determine what kind of objects to render, or what kind of other properties to take into account:

- `DRAWSCENE_OPAQUE`: Opaque object will be rendered
- `DRAWSCENE_TRANSPARENT`: Transparent objects will be rendered. Objects will be sorted back-to front, for blending purposes
- `DRAWSCENE_OCCLUSIONCULLING`: Occluded objects won't be rendered. [Occlusion culling](#occlusion-culling) can be globally switched on/off using `wi::renderer::SetOcclusionCullingEnabled()`
- `DRAWSCENE_TESSELLATION`: Enable [tessellation](#tessellation) (if hardware supports it). [Tessellation](#tessellation) can be globally switched on/off using `wi::renderer::SetTessellationEnabled()`
- `DRAWSCENE_HAIRPARTICLE`: Allow drawing hair particles
- `DRAWSCENE_IMPOSTOR` : Allow drawing impostors
- `DRAWSCENE_OCEAN` : Allow drawing the ocean
- `DRAWSCENE_SKIP_PLANAR_REFLECTION_OBJECTS` : Don't draw the objects which are rendering planar reflections. This is to avoid rendering the reflector object itself into the planar reflections, which could produce fully occluded reflections.

#### Tessellation
Tessellation can be used when rendering objects. Tessellation requires a GPU hardware feature and can enable displacement mapping on vertices or smoothing mesh silhouettes dynamically while rendering objects. Tessellation will be used when `tessellation` parameter to the [DrawScene](#drawscene) was set to `true` and the GPU supports the tessellation feature. Tessellation level can be specified per [MeshComponent](#meshcomponent)'s `tessellationFactor` parameter. Tessellation level will be modulated by distance from camera, so that tessellation factor will fade out on more distant objects. Greater tessellation factor means more detailed geometry will be generated.

#### Occlusion Culling
Occlusion culling is a technique to determine which objects are within the camera, but are completely behind an other objects, such that they wouldn't be rendered. The depth buffer already does occlusion culling on the GPU, however, we would like to perform this earlier than submitting the mesh to the GPU for drawing, so essentially do the occlusion culling on CPU. A hybrid approach is used here, which uses the results from a previously rendered frame (that was rendered by GPU) to determine if an object will be visible in the current frame. For this, we first render the object into the previous frame's depth buffer, and use the previous frame's camera matrices, however, the current position of the object. In fact, we only render bounding boxes instead of objects, for performance reasons. Occlusion queries are used while rendering, and the CPU can read the results of the queries in a later frame. We keep track of how many frames the object was not visible, and if it was not visible for a certain amount, we omit it from rendering. If it suddenly becomes visible later, we immediately enable rendering it again. This technique means that results will lag behind for a few frames (latency between cpu and gpu and latency of using previous frame's depth buffer). These are implemented in the functions `wi::renderer::OcclusionCulling_Render()` and `wi::renderer::OcclusionCulling_Read()`. 

#### Shadow Maps
The `DrawShadowmaps()` function will render shadow maps for each active dynamic light that are within the camera [frustum](#frustum). There are two types of shadow maps, 2D and Cube shadow maps. The maximum number of usable shadow maps are set up with calling `SetShadowProps2D()` or `SetShadowPropsCube()` functions, where the parameters will specify the maximum number of shadow maps and resolution. The shadow slots for each light must be already assigned, because this is a rendering function and is not allowed to modify the state of the [Scene](#scene) and [lights](#lightcomponent). The shadow slots will be set up in the [UpdatePerFrameData()](#updateperframedata) function that is called every frame by the `RenderPath3D`.

#### UpdatePerFrameData
This function prepares the scene for rendering. It must be called once every frame. It will modify the [Scene](#scene) and other rendering related resources. It is called after the [Scene](#scene) was updated. It performs frustum culling and other management tasks, such as packing decal rects into atlas and several other things.

#### UpdateRenderData
Begin rendering the frame on GPU. This means that GPU compute jobs are kicked, such as particle simulations, texture packing, mipmap generation tasks that were queued up, updating per frame GPU buffer data, animation vertex skinning and other things.

#### Ray tracing (hardware accelerated)

Hardware accelerated ray tracing API is now available, so a variety of renderer features are available using that. If the hardware support is available, the `Scene` will allocate a top level acceleration structure, and the meshes will allocate bottom level acceleration structures for themselves. Updating these is done by simply calling `wi::renderer::UpdateRaytracingAccelerationStructures(cmd)`. The updates will happen on the GPU timeline, so provide a [CommandList](#work-submission) as argument. The top level acceleration structure will be rebuilt from scratch. The bottom level acceleration structures will be rebuilt from scratch once, and then they will be updated (refitted).

After the acceleration structures are updated, ray tracing shaders can use it after binding to a shader resource slot.

#### Ray tracing (legacy)
Ray tracing can be used in multiple ways to render the scene. The `RayTraceScene()` function will render the scene with path tracing. The result will be written to a texture that is provided as parameter. The texture must have been created with `UNORDERED_ACCESS` bind flags, because it will be written in compute shaders. The scene BVH structure must have been already built to use this, it can be accomplished by calling `wi::renderer::UpdateRaytracingAccelerationStructures()`. The [RenderPath3D_Pathracing](#renderpath3d_pathtracing) uses this ray tracing functionality to render a path traced scene.

Other than path tracing, the scene BVH can be rendered by using the `RayTraceSceneBVH` function. This will render the bounding box hierarchy to the screen as a heatmap. Blue colors mean that few boxes were hit per pixel, and with more bounding box hits the colors go to green, yellow, red, and finaly white. This is useful to determine how expensive a the scene is with regards to ray tracing performance.

The lightmap baking is also using ray tracing on the GPU to precompute static lighting for objects in the scene. [Objects](#objectcomponent) that contain lightmap atlas texture coordinate set can start lightmap rendering by setting `ObjectComponent::SetLightmapRenderRequest(true)`. Every object that have this flag set will have their lightmaps updated by performing ray traced rendering by the [wi::renderer](#wi::renderer) internally. Lightmap texture coordinates can be generated by a separate tool, such as the Wicked Engine Editor application. Lightmaps will be rendered to a global lightmap atlas that can be used by all shaders to read static lighting data. The global lightmap atlas texture contains lightmaps from all objects inside the scene in a compact format for performance. Apart from that, each object contains its own lightmap in a full precision texture format that can be post-processed and saved to disc for later use. To denoise lightmaps, follow the same steps as the path tracer denoiser setup described in the [Denoiser](#denoiser) section.

#### Scene BVH
The scene BVH can be rebuilt from scratch using the `wi::renderer::UpdateRaytracingAccelerationStructures()` function. The [ray tracing](#ray-tracing) features require the scene BVH to be built before using them. This is using the [wiGPUBVH](#wigpubvh) facility to build the BVH using compute shaders running on the GPU when hardware ray tracing is not available. Otherwise it uses ray tracing acceleration structure API objects. 

#### Decals
The [DecalComponents](#decalcomponent) in the scene can be rendered differently based on the rendering technique that is being used. The two main rendering techinques are forward and deferred rendering. 

<b>Forward rendering</b> is aimed at low bandwidth consumption, so the scene lighting is computed in one rendering pass, right when rendering the scene objects to the screen. In this case, all of the decals are also processed in the object rendering shaders automatically. In this case a decal atlas will need to be generated, because all decal textures must be available to sample in a single shader, but this is handled automatically inside the [UpdateRenderData](#updaterenderdata) function.

<b>Deferred rendering</b> techniques are outputting G-buffers, and compute lighting in a separate pass that is decoupled from the object rendering pass. Decals are rendered one by one on top of the albedo buffer using the function `wi::renderer::DrawDeferredDecals()`. All the lighting will be computed on top after the decals were rendered to the albedo in a separate render pass to complete the final scene.

#### Environment probes
[EnvironmentProbeComponents](#environmentprobecomponent) can be placed into the scene and if they are tagged as invalid using the `EnvironmentProbeComponent::SetDirty(true)` flag, they will be rendered to a cubemap array that is accessible in all shaders. They will also be rendered in real time every frame if the `EnvironmentProbeComponent::SetRealTime(true)` flag is set. The cubemaps of these contain indirect specular information from a point source that can be used as approximate reflections for close by objects.

The probes also must have [TransformComponent](#transformcomponent) associated to the same entity. This will be used to position the probes in the scene, but also to scale and rotate them. The scale/position/rotation defines an oriented bounding box (OBB) that will be used to perform parallax corrected environment mapping. This means that reflections inside the box volume will not appear as infinitely distant, but constrained inside the volume, to achieve more believable illusion.

#### Post processing
There are several post processing effects implemented in the `wi::renderer`. They are all implemented in a single function with their name starting with Postprocess_, like for example: `wi::renderer::Postprocess_SSAO()`. They are aimed to be stateless functions, with their function signature clearly indicating input-output parameters and resources.

The chain/order of post processes is not implemented here, only the single effects themselves. The order of post proceses is more of a user responsibility, so it should be implemented in the [High Ievel Interface](#high-level-interface). For reference, a default post processchain is implemented in [RenderPath3D::RenderPostProcessChain()](#renderpath3d) function.

#### Instancing
Instanced rendering will be always used automatically when rendering objects. This means, that [ObjectComponents](#objectcomponent) that share the same [mesh](#meshcomponent) will be batched together into a single draw call, even if they have different transformations, colors, or dithering parameters. This can greatly reduce CPU overhead when lots of objects are duplicated (for example trees, rocks, props).

[ObjectComponents](#objectcomponent) can override [stencil](#stencil) settings for the [material](#materialcomponent). In case multiple [ObjectComponents](#objectcomponent) share the same mesh, but some are using different stencil overrides, those could be removed from the instanced batch, so there is some overhead to modifying stencil overrides for objects.

<i>Tip: to find out more about how instancing is used to batch objects together, take a look at the `RenderMeshes()` function inside [wi::renderer.cpp](../WickedEngine/wi::renderer.cpp)</i>

#### Stencil
If a depth stencil buffer is bound when using [DrawScene()](#drawscene), the stencil buffer will be written. The stencil is a 8-bit mask that can be used to achieve different kinds of screen space effects for different objects. [MaterialComponents](#materialcomponent) and [ObjectComponents](#objectcomponent) have the ability to set the stencil value that will be rendered. The 8-bit stencil value is separated into two parts:
- The first 4 bits are reseved for engine-specific values, such as to separate skin, sky, common materials from each other. these values are contained in [wiEnums](#wienums) in the `STENCILREF` enum.
- The last 4 bits are reserved for user stencil values. The application can decide what it wants to use these for.

Please use the `wi::renderer::CombineStencilrefs()` function to specify stencil masks in a future-proof way.

The [Pipeline States](#pipeline-states) have a `DepthStencilState` type member which will control how the stencil mask is used in the graphics pipeline for any custom rendering effect. The functionality is supposed to be equivalent and closely matching of what DirectX 11 provides, so for further information, refer to the [DirectX 11 documentation of Depth Stencil States](https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil).

#### Loading Shaders
While the [GraphicsDevice is responsible of creating shaders and pipeline states](#pipeline-states-and-shaders), loading the shader files themselves are not handled by the graphics device. The `wi::renderer::LoadShader(ShaderStage::)` is a helper function that provides this feature. This is internally loading shaders from a common shader path, that is by default the "../WickedEngine/shaders" directory (relative to the application working directory), so the filename provided to this function must be relative to that path. Every system in the engine that loads shaders uses this function to load shaders from the same folder, which makes it very easy to reload shaders on demand with the `wi::renderer::ReloadShaders()` function. This is useful for when the developer modifies a shader and recompiles it, the engine can reload it while the application is running. The developer can modify the common shader path with the `wi::renderer::SetShaderPath()` to any path of preference. The developer is free to load shaders with a custom loader, from any path too, but the `wi::renderer::ReloadShaders()` functionality might not work in that case for those shaders.

#### Debug Draw
Debug geometry can be rendered by calling the `wi::renderer::DrawDebugWorld()` function and setting up debug geometries, or enabling debug features. The `DrawDebugWorld()` is already called by [RenderPath3D](#renderpath3d), so the developer can simply just worry about configure debug drawing features and add debug geometries and drawing will happen at the right place (if the developer decided to use [RenderPath3D](#renderpath3d) in their application). 

The user provided debug geometry features at present are: 
- `DrawBox()`: provide a transformation matrix and color to render a wireframe box mesh with the desired transformation and color. The box will be only rendered for a single frame.
- `DrawLine()`: provide line segment begin, end positions and a color value. The line will be only rendered for a single frame.
- `DrawPoint()`: provide a position, size and color to render a point as a "X" line geometry that is always camera facing. The point will be only rendered for a single frame.
- `DrawTriangle()`: provide 3 positions and colors that represent a colored triangle. There is an option to draw the triangle in wireframe mode instead of solid. The triangle will be only rendered for a single frame.
- `DrawSphere()`: provide a sphere and a color, its bounds will be drawn as line geometry. The sphere will be only rendered for a single frame.
- `DrawCapsule()`: provide a capsule and a color, its bounds will be drawn as line geometry. The capsule will be only rendered for a single frame.

Configuring other debug rendering functionality:
- `SetToDrawDebugBoneLines()`: Bones will be rendered as lines
- `SetToDrawDebugPartitionTree()`: Scene object bounding boxes will be rendered.
- `SetToDrawDebugEnvProbes()`: Environment probes will be rendered as reflective spheres and their affection range as oriented bounding boxes.
- `SetToDrawDebugEmitters()`: Emitter mesh geometries will be rendered as wireframe meshes.
- `SetToDrawDebugForceFields()`: Force field volumes will be rendered.
- `SetToDrawDebugCameras()`: Cameras will be rendered as oriented boxes.

#### Animation Skinning
[MeshComponents](#meshcomponent) that have their `armatureID` associated with an [ArmatureComponent](#armaturecomponent) will be skinned inside the `wi::renderer::UpdateRenderData()` function. This means that their vertex buffer will be animated with compute shaders, and the animated vertex buffer will be used throughout the rest of the frame. 


#### Custom Shaders
Apart from the built in material shaders, the developer can create a library of custom shaders from the application side and assign them to materials. The `wi::renderer::RegisterCustomShader()` function is used to register a custom shader from the application. The function returns the ID of the custom shader that can be input to the `MaterialComponent::SetCustomShaderID()` function. 

The `CustomShader` is essentially the combination of a [Pipeline State Object](#pipeline-states-and-shaders) for each `RENDERPASS` that specifies whether it is to be drawn in a transparent or opaque, or other kind of pass within a `RENDERPASS`. The developer is responsible of creating a fully valid pipeline state to render a mesh. If a pipeline state is left as empty for a combination of `RENDERPASS`, then the material will simply be skipped and not rendered in that pass. The other part of `CustomShader` is a name, that can be used as simple identifier for the user, and a `filterMask` that will be used to indicate what kind of material this is to multiple systems. You can set the `filterMask` to any combination of `wi::enums::FILTER` for your purposes, but you have to include `FILTER_OPAQUE` or `FILTER_TRANSPARENT` to indicate if this is going to be rendered in opaque or transparent passes (or both).

The `MaterialComponent::userdata` can also be used to provide a small amount of custom material data that will be available to shaders. You can access that in shaders from `ShaderMaterial::userdata`. If the built in user data is not enough for your purposes, you can create additional `GPUBuffer` objects and send descriptor indices in user data to access extended user data indirectly.

To look at an example, take a look at the built in Hologram custom shader and see how exactly to create a valid pipeline state, shader, etc.


### Enums
[[Header]](../../WickedEngine/wiEnums.h)
This is a collection of enum values used by the wi::renderer to identify graphics resources. Usually arrays of the same resource type are declared and the XYZENUM_COUNT values tell the length of the array. The other XYZENUM_VALUE represents a single element within that array. This makes the code easy to manage, for example:

```cpp
enum CBTYPE
{
	CBTYPE_MESH, // = 0
	CBTYPE_SOMETHING_ELSE, // = 1
	CBTYPE_AN_OTHER_THING, // = 2
	CBTYPE_COUNT // = 3
};
GPUBuffer buffers[CBTYPE_COUNT]; // this example array contains 3 elements
//...
device->BindConstantBuffer(&buffers[CBTYPE_MESH], 0, cmd); // makes it easy to reference an element
```

This is widely used to make code straight forward and easy to add new objects, without needing to create additional declarations, except for the enum values.

### Image Renderer
[[Header]](../../WickedEngine/wiImage.h) [[Cpp]](../../WickedEngine/wiImage.cpp)
This can render images to the screen in a simple manner. You can draw an image like this:
```cpp
wi::image::SetCanvas(canvas); // setting the canvas area is required to set the drawing area and perform DPI scaling (this is only for the current thread)
wi::image::Draw(myTexture, wiImageParams(10, 20, 256, 128), cmd);
```
The example will draw a 2D texture image to the position (10, 20), with a size of 256 x 128 pixels to the current render pass. There are a lot of other parameters to customize the rendered image, for more information see wiImageParams structure.
- wi::image::Params <br/>
Describe all parameters of how and where to draw the image on the screen.

### Font Renderer
[[Header]](../../WickedEngine/wiFont.h) [[Cpp]](../../WickedEngine/wiFont.cpp)
This can render fonts to the screen in a simple manner. You can render a font as simple as this:
```cpp
wi::font::SetCanvas(canvas); // setting the canvas area is required to set the drawing area and perform DPI scaling (this is only for the current thread)
wi::font::Draw("write this!", wiFontParams(10, 20), cmd);
```
Which will write the text <i>write this!</i> to 10, 20 pixel position onto the screen. There are many other parameters to describe the font's position, size, color, etc. See the wiFontParams structure for more details.
- wiFontParams <br/>
Describe all parameters of how and where to draw the font on the screen.

The wiFont can load and render .ttf (TrueType) fonts. The default "Liberation Sans" (Arial compatible) font style is embedded into the engine ([[liberation_sans.h]](../WickedEngine/Utility/liberation_sans.h) file). The developer can load additional fonts from files by using `wiFont::AddFontStyle()` functions. These can either load from a file, or take a provided byte data for the font. The `AddFontStyle()` will return an `int` that will indicate the font ID within the loaded font library. The `wiFontParams::style` can be set to the font ID to use a specific font that was previously loaded. If the developer added a font before wiFont::Initialize was called, then that will be the default font and the "Liberation Sans" font will not be created.

### Emitted Particle System
[[Header]](../../WickedEngine/wiEmittedParticle.h) [[Cpp]](../../WickedEngine/wiEmittedParticle.cpp)
GPU driven emitter particle system, used to draw large amount of camera facing quad billboards. Supports simulation with force fields and fluid simulation based on Smooth Particle Hydrodynamics computation.

### Hair Particle System
[[Header]](../../WickedEngine/wiHairParticle.h) [[Cpp]](../../WickedEngine/wiHairParticle.cpp)
GPU driven particles that are attached to a mesh surface. It can be used to render vegetation. It participates in force fields simulation.

### Ocean
[[Header]](../../WickedEngine/wiOcean.h) [[Cpp]](../../WickedEngine/wiOcean.cpp)
Ocean renderer using Fast Fourier Transforms simulation. The ocean surface is always rendered relative to the camera, like an infinitely large water body.

### Sprite
[[Header]](../../WickedEngine/wiSprite.h) [[Cpp]](../../WickedEngine/wiSprite.cpp)
A helper facility to render and animate images. It uses the [wiImage](#wiimage) renderer internally
- Anim <br/>
Several different simple animation utilities, like animated textures, wobbling, rotation, fade out, etc...

### SpriteFont
[[Header]](../../WickedEngine/wiSprite.h) [[Cpp]](../../WickedEngine/wiSprite.cpp)
A helper facility to render fonts. It uses the [wiFont](#wifont) renderer internally. It performs string conversion

### TextureHelper
[[Header]](../../WickedEngine/wiTextureHelper.h) [[Cpp]](../../WickedEngine/wiTextureHelper.cpp)
This is used to generate procedural textures, such as uniform colors, noise, etc...

### GPUSortLib
[[Header]](../../WickedEngine/wiGPUSortLib.h) [[Cpp]](../../WickedEngine/wiGPUSortLib.cpp)
This is a GPU sorting facility using the Bitonic Sort algorithm. It can be used to sort an index list based on a list of floats as comparison keys entirely on the GPU.

### GPUBVH
[[Header]](../../WickedEngine/wiGPUBVH.h) [[Cpp]](../../WickedEngine/wiGPUBVH.cpp)
This facility can generate a BVH (Bounding Volume Hierarcy) on the GPU for a [Scene](#scene). The BVH structure can be used to perform efficient RAY-triangle intersections on the GPU, for example in ray tracing. This is not using the ray tracing API hardware acceleration, but implemented in compute, so it has wide hardware support.


## GUI
The custom GUI, implemented with engine features

### GUI
[[Header]](../../WickedEngine/wiGUI.h) [[Cpp]](../../WickedEngine/wiGUI.cpp)
The wiGUI is responsible to run a GUI interface and manage widgets. 

<b>GUI Scaling:</b> To ensure correct GUI scaling, GUI elements should be designed for the current window size. If they are placed inside `RenderPath2D::ResizeLayout()` function according to current screen size, it will ensure that GUI will be scaled on a Resolution or DPI change event, which is recommended.

### EventArgs
This will be sent to widget callbacks to provide event arguments in different formats

### wiWidget
The base widget interface can be extended with specific functionality. The GUI will store and process widgets by this interface. 

#### Button
A simple clickable button. Supports the OnClick event callback.

#### Label
A simple static text field.

#### TextInputField
Supports text input when activated. Pressing Enter will accept the input and fire the OnInputAccepted callback. Pressing the Escape key while active will cancel the text input and restoe the previous state. There can be only one active text input field at a time.

#### Slider
A slider, that can represent float or integer values. The slider also accepts text input to input number values. If the number input is outside of the slider's range, it will expand its range to support the newly assigned value. Upon changing the slider value, whether by text input or sliding, the OnSlide event callback will be fired.

#### CheckBox
The checkbox is a two state item which can represent true/false state. The OnClick event callback will be fired upon changing the value (by clicking on it)

#### ComboBox
Supports item selection from a list of text. Can set the maximum number of visible items. If the list of items is greater than the max allowed visible items, then a vertical scrollbar will be presented to allow showing hidden items. Upon selection, the OnSelect event callback will be triggered.

#### Window
A window widget is able to hold any number of other widgets. It can be moved across the screen, minimized and resized by the user. The window does not manage lifetime of attached widgets since 0.49.0!

#### ColorPicker
Supports picking a HSV (or HSL) color, displaying its RGB values. On selection, the OnColorChanged event callback will be fired and the user can read the new RGB value from the event argument.


## Helpers
A collection of engine-level helper classes

### Archive
[[Header]](../../WickedEngine/wiArchive.h) [[Cpp]](../../WickedEngine/wiArchive.cpp)
This is used for serializing binary data to disk or memory. An archive file always starts with the 64-bit version number that it was serialized with. An archive of greater version number than the current archive version of the engine can't be opened safely, so an error message will be shown if this happens. A certain archive version will not be forward compatible with the current engine version if the current archive version barrier number is greater than the archive's own version number.

### Color
[[Header]](../../WickedEngine/wiColor.h)
Utility to convert to/from float color data to 32-bit RGBA data (stored in a uint32_t as RGBA, where each channel is 8 bits)

### FadeManager
[[Header]](../../WickedEngine/wiFadeManager.h) [[Cpp]](../../WickedEngine/wiFadeManager.cpp)
Simple helper to manage a fadeout screen. Fadeout starts at transparent, then fades smoothly to an opaque color (such as black, in most cases), then a callback occurs which the user can handle with their own event. After that, the color will fade back to transperent. This is used by the [Application](#application) to fade from one RenderPath to an other.

### Helper
[[Header]](../../WickedEngine/wiHelper.h) [[Cpp]](../../WickedEngine/wiHelper.cpp)
Many helper utility functions, like screenshot, readfile, messagebox, splitpath, sleep, etc...

### Primitive
[[Header]](../../WickedEngine/wiPrimitive.h) [[Cpp]](../../WickedEngine/wiPrimitive.cpp)
Primitives that can be intersected with each other

#### AABB
[[Header]](../../WickedEngine/wiPrimitive.h) [[Cpp]](../../WickedEngine/wiPrimitive.cpp)
Axis aligned bounding box. There are multiple ways to construct it, for example from min-max corners, or center and halfextent. 

#### Sphere
[[Header]](../../WickedEngine/wiPrimitive.h) [[Cpp]](../../WickedEngine/wiPrimitive.cpp)
Sphere with a center and radius.

#### Capsule
[[Header]](../../WickedEngine/wiPrimitive.h) [[Cpp]](../../WickedEngine/wiPrimitive.cpp)
It's like two spheres connected by a cylinder. Base and Tip are the two endpoints, radius is the cylinder's radius.

#### Ray
[[Header]](../../WickedEngine/wiPrimitive.h) [[Cpp]](../../WickedEngine/wiPrimitive.cpp)
Line with a starting point (origin) and direction. The direction's reciprocal is precomputed to perform fast intersection of many primitives with one ray.

#### Frustum
[[Header]](../../WickedEngine/wiPrimitive.h) [[Cpp]](../../WickedEngine/wiPrimitive.cpp)
Six planes, most commonly used for checking if an intersectable primitive is inside a camera.

#### Hitbox2D
[[Header]](../../WickedEngine/wiPrimitive.h) [[Cpp]](../../WickedEngine/wiPrimitive.cpp)
A rectangle, essentially an 2D AABB.

### Math
[[Header]](../../WickedEngine/wiMath.h) [[Cpp]](../../WickedEngine/wiMath.cpp)
Math related helper functions, like lerp, triangleArea, HueToRGB, etc...

### Random
[[Header]](../../WickedEngine/wiRandom.h) [[Cpp]](../../WickedEngine/wiRandom.cpp)
Uniform random number generator with a good distribution.

### RectPacker
[[Header]](../../WickedEngine/wiRectPacker.h) [[Cpp]](../../WickedEngine/wiRectPacker.cpp)
Provides the ability to pack multiple rectangles into a bigger rectangle, while taking up the least amount of space from the containing rectangle.

### ResourceManager
[[Header]](../../WickedEngine/wiResourceManager.h) [[Cpp]](../../WickedEngine/wiResourceManager.cpp)
This can load images and sounds. It will hold on to resources until there is at least something that is referencing them, otherwise deletes them. One resource can have multiple owners, too. This is thread safe.

- `Load()` : Load a resource, or return a resource handle if it already exists. The resources are identified by file names. The user can specify import flags (optional). The user can provide a file data buffer that was loaded externally (optional). This function will return a resource handle. The resource handle equals to `nullptr` if it was not loaded successfully, otherwise a valid handle is returned.
- `Contains()` : Check whether a resource exists or not.
- `Clear()` : Clear all resources. This will clear the resource library, but resources that are still used somewhere will remain usable. 

The resource manager can support different modes that can be set with `SetMode(MODE param)` function:
- `DISCARD_FILEDATA_AFTER_LOAD` : this is the default behaviour. The resource will not hold on to file data, even if the user specified `IMPORT_RETAIN_FILEDATA` flag when loading the resource. This will result in the resource manager unable to serialize (save) itself.
- `ALLOW_RETAIN_FILEDATA` : this mode can be used to keep the file data buffers alive inside resources. This way the resource manager can be serialized (saved). Only the resources that still hold onto their file data will be serialized (saved). When loading a resource, the user can specify `IMPORT_RETAIN_FILEDATA` flag to keep the file data for a specific resource instead of discarding it.
- `ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING` : Keep all file data, but don't write them while serializing. This is useful to disable resource embedding temporarily without destroying file data buffers.

The resource manager can always be serialized in read mode. File data retention will be based on existing file import flags and the global resource manager mode.

### SpinLock
[[Header]](../../WickedEngine/wiSpinLock.h) [[Cpp]](../../WickedEngine/wiSpinLock.cpp)
This can be used to guarantee exclusive access to a block in multithreaded race condition scenario instead of a mutex. The difference to a mutex that this doesn't let the thread to yield, but instead spin on an atomic flag until the spinlock can be locked.

### Arguments
[[Header]](../../WickedEngine/wiArguments.h) [[Cpp]](../../WickedEngine/wiArguments.cpp)
This is to store the startup parameters that were passed to the application from the operating system "command line". The user can query these arguments by name.

### Timer
[[Header]](../../WickedEngine/wiTimer.h) [[Cpp]](../../WickedEngine/wiTimer.cpp)
High resolution stopwatch timer

### wiVoxelGrid
[[Header]](../../WickedEngine/wiVoxelGrid.h) [[Cpp]](../../WickedEngine/wiVoxelGrid.cpp)
An axis aligned bounding box volume that contains a 3D grid of voxels. It can be used to voxelize the scene, and used by [PathQuery](#wipathquery) for path finding.

The voxel grid is created with the `init()` function, providing the dimensions of the 3D grid. This function will allocate the necessary memory to store the grid, and fill every voxel to empty. To simply clear the grid, use `cleardata()` function, which will not allocate memory, only clear all voxels to empty.

The volume of the grid is specified by the `center` and `voxelSize` members. To set the `voxelSize`, use the `set_voxelsize()` function instead of directly modifying the `voxelSize`. You can also align it easily to an existing AABB with the `from_aabb()` function. Setting these doesn't modify voxel data, so it probably needs to be revoxelized after modifications.

The main feature is that you can inject primitives into the voxel grid, this is the voxelization process. You can inject triangles, AABB, Sphere and Capsule primitives with the `inject_triangle()`, `inject_aabb()`, `inject_sphere()` and the `inject_capsule()` functions, respectively. All the primitives are input to these functions in world space, and they will be mapped into the voxel grid internally by these functions based on the grid center, voxelSize and resolution. These functions can also accept a bool option called `subtract` which - if set to `true` - will modify the voxelization process to remove voxels intead of add them. The voxelization process is performance sensitive, but it can be done from different threads as it is thread safe, using atomic operations to modify voxel data.

To access individual voxels, you can use the `check_voxel()` function to check if a voxel is empty or validm and the `set_voxel()` function to set a voxel to empty or valid. These operations are not thread safe! Also these functions will both accept world position, or voxel coordinates. You can additionally convert world positions to voxel coordinates by using the `world_to_coord()` function, or do the reverse with the `coord_to_world()` function.

Note: There are helper functions to voxelize a whole object or the whole scene, accessible from the [Scene](#scene) object. These are called `VoxelizeObject()` and `VoxelizeScene()`.

### wiPathQuery
[[Header]](../../WickedEngine/wiPathQuery.h) [[Cpp]](../../WickedEngine/wiPathQuery.cpp)
Path query can find paths from start to goal position within a voxel grid. To run a path finding query, first prepare a [voxel grid](#wivoxelgrid) with scene data, then process it with the path query.

The usage of the path query at minimum is to call the `process()` function, with start and goal positions and a [voxel grid](#wivoxelgrid). When the function returns, you can check path finding success with the `is_successful()` function, and `get_next_waypoint()` function to find the next waypoint in the path from the start that goes towards the goal.

The path query can be additionally configured with the `flying` bool param. If you set `flying` to `true`, then the path finding will search a path in empty voxels (in the air), otherwise by default it will try to find a path that goes on the ground.

The traversing entity can have a logical size, configured by the `agent_height` and `agent_width` parameters, that specify the approcimate size of traversing entity in voxels. `agent_height` will specify how many empty voxels must be above the traversed path at any waypoint. `agent_width` specifies how many empty voxels must be in the horizontal directions. With these you can specify how far the path should keep away from walls and obstacles, and also to not allow going in between tight openings that the logical size would not allow.

Note: processing a path query can take a long time, depending on how far the goal is from the start. Consider doing multiple path queries on multiple threads, or doing them asynchronously across the frame, the [Job System](#job-system) can be used to track completion of asynchronous tasks like this.


## Input
[[Header]](../../WickedEngine/wiInput.h) [[Cpp]](../../WickedEngine/wiInput.cpp)
This is the high level input interface. Use this to read input devices in a platform-independent way.
- Initialize <br/>
Creates all necessary resources, the engine initialization already does this via [wiInitializer](#wiinitializer), so the user probably doesn't have to worry about this.
- Update <br/>
This is called once per frame and necessary to process inputs. This is automatically called in [Application](#application), so the user doesn't need to worry about it, unless they are reimplementing the high level interface for themselves.
- Down <br/>
Checks whether a button is currently held down.
- Press <br/>
Checks whether a button was pressed in the current frame.
- Hold <br/>
Checks whether a button was held down for a specified amount of time. It can be checked whether it was held down exactly for the set amount of time, or at least for that long.
- GetPointer <br/>
Returns the mouse position. The first two values of the returned vector are mouse pointer coordinates in window space. The third value is the relative scroll button difference since last frame.
- SetPointer <br/>
Sets the mouse pointer position in window-relative coordinates.
- HidePointer <br/>
Hides or shows the mouse pointer.
- GetAnalog <br/>
Returns the gamepad's analog position.
- SetControllerFeedback <br/>
Utilize various features of the gamepad, such as vibration, or LED color.

### BUTTON
This collection of enums containing all the usable digital buttons, either on keyboard or a controller. If you wish to specify a letter key on the keyboard, you can use the plain character value and cast to this enum, like:

```cpp
bool is_P_key_pressed = wiInput::Press((wiInput::BUTTON)'P');
```

### KeyboardState
The keyboard's full state, where each button's state is stored as either pressed or released. Can be queried via [wiRawInput](#wirawinput)

### MouseState
The mouse pointer's absolute and relative position. Can be queried via [wiRawInput](#wirawinput)

### ControllerState
The controller's button and analog states. Can be queried via [wiRawInput](#wirawinput) or [wiXInput](#wixinput)

### ControllerFeedback
The controller force feedback features that might be supported. Can be provided to [wiRawInput](#wirawinput) or [wiXInput](#wixinput)

### Touch
A touch contact point. Currently it is supported in UWP (Universal Windows Platform) applications.
- GetTouches <br/>
Get a vector containing current Touch contact points

### XInput
[[Header]](../../WickedEngine/wiXInput.h) [[Cpp]](../../WickedEngine/wiXInput.cpp)
Low level wrapper for XInput API (capable of handling Xbox controllers). This functionality is used by the more generic wiInput interface, so the user probably doesn't need to use this.

### RawInput
[[Header]](../../WickedEngine/wiRawInput.h) [[Cpp]](../../WickedEngine/wiRawInput.cpp)
Low level wrapper for RAWInput API (capable of handling human interface devices, such as mouse, keyboard, controllers). This functionality is used by the more generic wiInput interface, so the user probably doesn't need to use this.



## Audio
[[Header]](../../WickedEngine/wiAudio.h) [[Cpp]](../../WickedEngine/wiAudio.cpp)
The namespace that is a collection of audio related functionality.
- CreateSound
- CreateSoundInstance
- Play
- Pause
- Stop
- SetVolume
- GetVolume
- ExitLoop
- SetSubmixVolume
- GetSubmixVolume
- Update3D
- SetReverb
### Sound
Represents a sound file in memory. Load a sound file via wiAudio interface.
### SoundInstance
An instance of a sound file that can be played and controlled in various ways through the wiAudio interface.
### SoundInstance3D
This structure describes a relation between listener and sound emitter in 3D space. Used together with a SoundInstance in wiAudio::Update3D() function
### SUBMIX_TYPE
Groups sounds so that different properties can be set for a whole group, such as volume for example
### REVERB_PRESET
Can make different sounding 3D reverb effect globally
- REVERB_PRESET_DEFAULT
- REVERB_PRESET_GENERIC
- REVERB_PRESET_FOREST
- REVERB_PRESET_PADDEDCELL
- REVERB_PRESET_ROOM
- REVERB_PRESET_BATHROOM
- REVERB_PRESET_LIVINGROOM
- REVERB_PRESET_STONEROOM
- REVERB_PRESET_AUDITORIUM
- REVERB_PRESET_CONCERTHALL
- REVERB_PRESET_CAVE
- REVERB_PRESET_ARENA
- REVERB_PRESET_HANGAR
- REVERB_PRESET_CARPETEDHALLWAY
- REVERB_PRESET_HALLWAY
- REVERB_PRESET_STONECORRIDOR
- REVERB_PRESET_ALLEY
- REVERB_PRESET_CITY
- REVERB_PRESET_MOUNTAINS
- REVERB_PRESET_QUARRY
- REVERB_PRESET_PLAIN
- REVERB_PRESET_PARKINGLOT
- REVERB_PRESET_SEWERPIPE
- REVERB_PRESET_UNDERWATER
- REVERB_PRESET_SMALLROOM
- REVERB_PRESET_MEDIUMROOM
- REVERB_PRESET_LARGEROOM
- REVERB_PRESET_MEDIUMHALL
- REVERB_PRESET_LARGEHALL
- REVERB_PRESET_PLATE


## Physics
[[Header]](../../WickedEngine/wiPhysics.h) [[Cpp]](../../WickedEngine/wiPhysics_Bullet.cpp)
You can find the physics system related functionality under ENGINE/Physics filter in the solution.
It uses the entity-component system to perform updating all physics components in the world.
- Initialize<br/>
This must be called before using the physics system, but it is automatically done by [wiInitializer](#wiinitializer)
- IsEnabled<br/>
Is the physics system running?
- SetEnabled<br/>
Enable or disable physics system
- RunPhysicsUpdateSystem<br/>
Run physics simulation on input components.

#### Rigid Body Physics
Rigid body simulation requires [RigidBodyPhysicsComponent](#rigidbodyphysicscomponent) for entities and [TransformComponent](#transformcomponent). It will modify TransformComponents with physics simulation data, so after simulation, TransformComponents will contain absolute world matrix.

<b>Kinematic</b> rigid bodies are those that are not simulated by the physics system, but they will affect the rest of the simulation. For example, an animated kinematic rigid body will strictly follow the animation, but affect any other rigid bodies it comes in contact with. Set a rigid body to kinematic by having the `RigidBodyPhysicsComponent::KINEMATIC` flag in the `RigidBodyPhysicsComponent::_flags` bitmask.

<b>Deactivation</b> happens after a while for rigid bodies that participated in the simulation for a while, this means after that they will no longer participate in the simulation. This behaviour can be disabled with the `RigidBodyPhysicsComponent::DISABLE_DEACTIVATION` flag.

There are multiple <b>Collision Shapes</b> of rigid bodies that can be used:
- `BOX`: simple oriented bounding box, fast.
- `SPHERE`: simple sphere, fast.
- `CAPSULE`: Two spheres with a cylinder between them.
- `CONVEX_HULL`: A simplified mesh.
- `TRIANGLE_MESH`: The original mesh. It is always kinematic.

#### Soft Body Physics
Soft body simulation requires [SoftBodyPhysicsComponent](#softbodyphysicscomponent) for entities as well as [MeshComponent](#meshcomponent). When creating a soft body, the simulation mesh will be computed from the MeshComponent vertices and mapping tables from physics to graphics indices that associate graphics vertices with physics vertices. The physics vertices will be simulated in world space and copied to the `SoftBodyPhysicsComponent::vertex_positions_simulation` array as graphics vertices. This array can be uploaded as vertex buffer as is. 

The [ObjectComponent](#objectcomponent)'s transform matrix is used as a manipulation matrix to the soft body, if the ObjectComponent's mesh has a soft body associated with it. The manipulation matrix means, that pinned soft body physics vertices will receive that transformation matrix. The simulated vertices will follow those manipulated vertices.

<b>Pinning</b> the soft body vertices means that those physics vertices have zero weights, so they are not simulated, but instead drive the simulation, as in other vertices will follow them, a bit like how kinematic rigid bodies manipulate simulated rigid bodies.

The pinned vertices can also be manipulated via <b>skinning animation</b>. If the soft body mesh is skinned and an animation is playing, then the pinned vertices will follow the animation.




## Network
[[Header]](../../WickedEngine/wiNetwork.h) [[Cpp]](../../WickedEngine/wiNetwork.cpp)
Simple interface that provides UDP networking features.
- Initialize
- CreateSocket
- Send
- ListenPort
- CanReceive
- Receive
#### Socket
This is a handle that must be created in order to send or receive data. It identifies the sender/recipient.
#### Connection
An IP address and a port number that identifies the target of communication


## Scripting
This is the place for the Lua scipt interface. For a complete reference about Lua scripting interface, please see the [ScriptingAPI-Documentation](ScriptingAPI-Documentation.md)
### LuaBindings
The systems that are bound to Lua have the name of the system as their filename, postfixed by _BindLua.
### Lua
[[Header]](../../WickedEngine/wiLua.h) [[Cpp]](../../WickedEngine/wiLua.cpp)
The Lua scripting interface on the C++ side. This allows to execute lua commands from the C++ side and manipulate the lua stack, such as pushing values to lua and getting values from lua, among other things.
### Lua_Globals
[[Header]](../../WickedEngine/wiLua_Globals.h)
Hardcoded lua script in text format. This will be always executed and provides some commonly used helper functionality for lua scripts.
### Luna
[[Header]](../../WickedEngine/wiLuna.h)
Helper to allow bind engine classes from C++ to Lua


## Tools
This is the place for tools that use engine-level systems
### Backlog
[[Header]](../../WickedEngine/wiBacklog.h) [[Cpp]](../../WickedEngine/wiBacklog.cpp)
Used to log any messages by any system, from any thread. It can draw itself to the screen. It can execute Lua scripts.
If there was a `wii:backlog::LogLevel::Error` or higher severity message posted on the backlog, the contents of the log will be saved to the temporary user directory as wiBacklog.txt.
### Profiler
[[Header]](../../WickedEngine/wiProfiler.h) [[Cpp]](../../WickedEngine/wiProfiler.cpp)
Used to time specific ranges in execution. Support CPU and GPU timing. Can write the result to the screen as simple text at this time.


## Shaders
Shaders are written in HLSL shading language and [compiled](#shader-compiler) into the native shader binary format that can vary based on platform and graphics device requirements. 

There are some macros used to declare resources with binding slots that can be read from the C++ application via code sharing. These macros are used to declare resources:

- CBUFFER(name, slot)<br/>
Declares a constant buffer
```cpp
CBUFFER(myCbuffer, 0);
{
	float4 values0;
	uint4 values2;
};

// Load as if reading a global value
float loaded_value = values0.y;
```

Note: When creating constant buffers, the structure must be strictly padded to 16-byte boundaries according to HLSL rules. Apart from that, the following limitation is in effect for Vulkan compatibility:

Incorrect padding to 16-byte:
```cpp
CBUFFER(cbuf, 0)
{
	float padding;
	float3 value; // <- the larger type can start a new 16-byte slot, so the C++ and shader side structures could mismatch
	float4 value2;
};
```

Correct padding:
```cpp
CBUFFER(cbuf, 0)
{
	float3 value; // <- the larger type must be placed before the padding
	float padding;
	float4 value2;
};
```

### Interop
The interop between shaders and C++ code is handled by shared header (.h) files. Modifying the shared header files will need recompilation of both engine and shaders, otherwise undefined behaviour will occur. For cases when only shaders need visibility, consider using shader header files (.hlsli), which should not be shared with the engine C++ code.

[[Header]](../../WickedEngine/shaders/ShaderInterop.h)
Shader Interop is used for declaring shared structures or values between C++ Engine code and shader code. There are several ShaderInterop files, postfixed by the subsystem they are used for to keep them minimal and more readable: <br/>
[ShaderInterop_BVH.h](../../WickedEngine/shaders/ShaderInterop_BVH.h) <br/>
[ShaderInterop_DDGI.h](../../WickedEngine/shaders/ShaderInterop_DDGI.h) <br/>
[ShaderInterop_EmittedParticle.h](../../WickedEngine/shaders/ShaderInterop_EmittedParticle.h) <br/>
[ShaderInterop_FFTGenerator.h](../../WickedEngine/shaders/ShaderInterop_FFTGenerator.h) <br/>
[ShaderInterop_Font.h](../../WickedEngine/shaders/ShaderInterop_Font.h) <br/>
[ShaderInterop_FSR2.h](../../WickedEngine/shaders/ShaderInterop_FSR2.h) <br/>
[ShaderInterop_GPUSortLib.h](../../WickedEngine/shaders/ShaderInterop_GPUSortLib.h) <br/>
[ShaderInterop_HairParticle.h](../../WickedEngine/shaders/ShaderInterop_HairParticle.h) <br/>
[ShaderInterop_Image.h](../../WickedEngine/shaders/ShaderInterop_Image.h) <br/>
[ShaderInterop_Ocean.h](../../WickedEngine/shaders/ShaderInterop_Ocean.h) <br/>
[ShaderInterop_Postprocess.h](../../WickedEngine/shaders/ShaderInterop_Postprocess.h) <br/>
[ShaderInterop_Raytracing.h](../../WickedEngine/shaders/ShaderInterop_Raytracing.h) <br/>
[ShaderInterop_Renderer.h](../../WickedEngine/shaders/ShaderInterop_Renderer.h) <br/>
[ShaderInterop_SurfelGI.h](../../WickedEngine/shaders/ShaderInterop_SurfelGI.h) <br/>
[ShaderInterop_Terrain.h](../../WickedEngine/shaders/ShaderInterop_Terrain.h) <br/>
[ShaderInterop_VoxelGrid.h](../../WickedEngine/shaders/ShaderInterop_VoxelGrid.h) <br/>
[ShaderInterop_VXGI.h](../../WickedEngine/shaders/ShaderInterop_VXGI.h) <br/>
[ShaderInterop_Weather.h](../../WickedEngine/shaders/ShaderInterop_Weather.h) <br/>

The ShaderInterop also contains the resource macros to help share code between C++ and HLSL, and portability between shader compilers. Read more about macros in the [Shaders section](#shaders)

### Shader Compiler
The built in Visual Studio shader compiler can compile HLSL shaders with the usual visual studio build process. This can be used to some extent, but the recommended way is to use the [[Wicked Engine Shader Compiler Interface (wiShaderCompiler)]](../WickedEngine/wiShaderCompiler.h). This supports various tools to compile shaders to different shader formats (hlsl5, hlsl6, spirv), and can be used at runtime, for example to compile shader permutations easily, and also use engine features like the [job system](wijobsystem) to parallelize compile tasks with more control than a common build system. 

Offline Shader Compilation:
The OfflineShaderCompiler tool can be built and used to compile shaders in a command line process. It can also be used to generate a shader dump, which is a header file that can be included into C++ code and compiled, so all shaders will be embedded into the executable, this way they won't be loaded as separate files by applications. However, the shader reload feature will not work in this case for those shaders that are embedded. The shader dump will be contained in `wiShaderDump.h` file when generated by the offline shader compiler using the `shaderdump` command line argument. If this file is detected by the time the engine is compiled, shaders will be embedded inside the compiled executable. The offline shader compiler can also be used to compile shader normally into separate .cso files with .wishadermeta metadata files that will be used to detect when each shader needs to be rebuilt automatically.
