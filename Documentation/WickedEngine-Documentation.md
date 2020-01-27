# WickedEngine Documentation
This is a reference for the C++ features of Wicked Engine

## Contents
1. [High Level Interface](#high-level-interface)
	1. [MainComponent](#maincomponent)
	2. [RenderPath](#renderpath)
	3. [RenderPath2D](#renderpath2d)
	4. [RenderPath3D](#renderpath3d)
	5. [RenderPath3D_Forward](#renderpath3d_forward)
	6. [RenderPath3D_Deferred](#renderpath3d_deferred)
	7. [RenderPath3D_TiledForward](#renderpath3d_tiledforward)
	8. [RenderPath3D_TiledDeferred](#renderpath3d_tileddeferred)
	9. [RenderPath3D_Pathtracing](#renderpath3d_pathtracing)
	10. [LoadingScreen](#loadingscreen)
2. [System](#system)
	1. [wiECS (Entity-Component System)](#wiecs)
	2. [wiScene](#wiscene)
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
		18. [AnimationComponent](#animationcomponent)
		19. [WeatherComponent](#weathercomponent)
		20. [SoundComponent](#soundcomponent)
		21. [Scene](#scene)
	3. [wiJobSystem](#wijobsystem)
	4. [wiInitializer](#wiinitializer)
	5. [wiPlatform](#wiplatform)
3. [Graphics](#graphics)
	1. [wiGraphics](#wigraphics)
		1. [GraphicsDevice](#wigraphicsdevice)
		2. [GraphicsDevice_DX11](#wigraphicsdevice_dx11)
		3. [GraphicsDevice_DX12](#wigraphicsdevice_dx12)
		4. [GraphicsDevice_Vulkan](#wigraphicsdevice_vulkan)
		5. [Graphics Descriptors](#graphics-descriptors)
		6. [Graphics Resources](#graphics-resources)
		7. [GPUMapping](#gpumapping)
			1. [ConstantBufferMapping](#constantbuffermapping)
			2. [ResourceMapping](#resourcemapping)
			3. [SamplerMapping](#samplermapping)
			4. [ShaderInterop](#shaderinterop)
	2. [wiRenderer](#wirenderer)
	3. [wiEnums](#wienums)
	4. [wiImage](#wiimage)
	5. [wiFont](#wifont)
	6. [wiEmittedParticle](#wiemittedparticle)
	7. [wiHairParticle](#wihairparticle)
	8. [wiOcean](#wiocean)
	9. [wiGPUSortLib](#wigpusortlib)
	10. [wiGPUBVH](#wigpubvh)
4. [GUI](#gui)
	1. [wiGUI](#wigui)
	2. [wiWidget](#wiwidget)
	3. [wiButton](#wibutton)
	4. [wiLabel](#wilabel)
	5. [wiTextInputField](#witextinputfield)
	6. [wiSlider](#wislider)
	7. [wiCheckBox](#wicheckbox)
	8. [wiComboBox](#wicombobox)
	9. [wiWindow](#wiwindow)
	10. [wiColorPicker](#wicolorpicker)
5. [Helpers](#helpers)
	1. [wiAllocators](#wiallocators)
		1. [LinearAllocator](#linearallocator)
	2. [wiArchive](#wiarchive)
	3. [wiColor](#wicolor)
	4. [wiContainers](#wicontainers)
		1. [ThreadSafeRingBuffer](#threadsaferingbuffer)
	5. [wiFadeManager](#wifademanager)
	6. [wiHashString](#wihashstring)
	7. [wiHelper](#wihelper)
	8. [wiIntersect](#wiintersect)
		1. [AABB](#aabb)
		2. [SPHERE](#sphere)
		3. [RAY](#ray)
		4. [Frustum](#frustum)
		5. [Hitbox2D](#hitbox2d)
	9. [wiMath](#wimath)
	10. [wiRandom](#wirandom)
	11. [wiRectPacker](#wirectpacker)
	12. [wiResourceManager](#wiresourcemanager)
	13. [wiSpinLock](#wispinlock)
	14. [wiStartupArguments](#wistartuparguments)
	15. [wiTimer](#witimer)
6. [Input](#input)
7. [Audio](#audio)
	1. [wiAudio](#wiaudio)
	2. [Sound](#sound)
	3. [SoundInstance](#soundinstance)
	4. [SoundInstance3D](#soundinstance3d)
	5. [SUBMIX_TYPE](#submix_type)
	6. [REVERB_PRESET](#reverb_preset)
8. [Physics](#physics)
	1. [wiPhysicsEngine](#wiphysicsengine)
	2. [wiPhysicsEngine_BULLET](#wiphysicsengine_bullet)
9. [Network](#network)
	1. [wiNetwork](#winetwork)
	2. [Socket](#socket)
	3. [Connection](#connection)
10. [Scripting](#scripting)
	1. [wiLua](#wilua)
	2. [wiLua_Globals](#wilua_globals)
	3. [wiLuna](#wiluna)
11. [Tools](#tools)
	1. [wiBacklog](#wibacklog)
	2. [wiProfiler](#wiprofiler)
12. [Shaders](#shaders)

## High Level Interface
The high level interface consists of classes that allow for extending the engine with custom functionality. This is usually done by overriding the classes.
### MainComponent
[[Header]](../WickedEngine/MainComponent.h) [[Cpp]](../WickedEngine/MainComponent.cpp)
This is the main runtime component that has the Run() function. It should be included in the application entry point while calling Run() in an infinite loop. <br/>
The MainComponent has many uses that the user is not necessarily interested in. The most important part is that it manages the RenderPaths. There can be one active RenderPath at a time, which will be updated and rendered to the screen every frame. However, because a RenderPath is a highly customizable class, there is no limitation what can be done within the RenderPath, for example supporting multiple concurrent RenderPaths if required. RenderPaths can be switched wit ha Fade out screen easily. Loading Screen can be activated as an active Renderpath and it will load and switch to an other RenderPath if desired. A RenderPath can be simply activated with the MainComponent::ActivatePath() function.<br/>
The MainComponent does the following every frame while it is running:<br/>
1. FixedUpdate() <br/>
Calls FixedUpdate for the active RenderPath and wakes up scripts that are waiting for fixedupdate(). The frequency off calls will be determined by MainComponent::setTargetFrameRate(float framespersecond). By default (parameter = 60), FixedUpdate will be called 60 times per second.
2. Update(float deltatime) <br/>
Calls Update for the active RenderPath and wakes up scripts that are waiting for update()
3. Render() <br/>
Calls Render for the active RenderPath and wakes up scripts that are waiting for render()
4. Compose()
Calls Compose for the active RenderPath
### RenderPath
[[Header]](../WickedEngine/RenderPath.h) [[Cpp]](../WickedEngine/RenderPath.cpp)
This is an empty base class that can be activated with a MainComponent. It calls its Start(), Update(), FixedUpdate(), Render(), Compose(), Stop() functions as needed. Override this to perform custom gameplay or rendering logic. <br/>
The order in which the functions are executed every frame: <br/>
1. FixedUpdate() <br/>
This will be called in a manner that is deterministic, so logic will be running in the frequency that is specified with MainComponent::setTargetFrameRate(float framespersecond)
2. Update(float deltatime) <br/>
This will be called once per frame, and the elapsed time in seconds since the last Update() is provided as parameter
3. Render() const <br/>
This will be called once per frame. It is const, so it shouldn't modify state. When running this, it is not defined which thread it is running on. Multiple threads and job system can be used within this. The main purpose is to record mass rendering commands in multiple threads and command lists. Command list can be safely retrieved at this point from the graphics device. 
4. Compose(CommandList cmd) const <br/>
It is called once per frame. It is running on a single command list that it receives as a parameter. These rendering commands will directly record onto the last submitted command list in the frame. The render target is the back buffer at this point, so rendering will happen to the screen.

Apart from the functions that will be run every frame, the RenderPath has the following functions:
1. Initialize() <br/>
This is an optional function, that the user can call when something needs to be initialized before Load() function will be triggered.
2. Load() <br/> 
Intended for loading resources before the first time the RenderPath will be used.
3. Start() <br/>
Start will always be called when a RenderPath is activated by the MainComponent
4. Stop() <br/>
Stop will be always called when the current RenderPath was the active one in MainComponent, but an other one was activated.
5. Unload() <br/>
Before a RenderPath is destroyed, Unload will be called, so deleting resources can take place if required.

### RenderPath2D
[[Header]](../WickedEngine/RenderPath2D.h) [[Cpp]](../WickedEngine/RenderPath2D.cpp)
Capable of handling 2D rendering to offscreen buffer in Render() function, or just the screen in Compose() function. It has some functionality to render wiSprite and wiFont onto rendering layers and stenciling with 3D rendered scene.
### RenderPath3D
[[Header]](../WickedEngine/RenderPath3D.h) [[Cpp]](../WickedEngine/RenderPath3D.cpp)
Base class for implementing 3D rendering paths. It supports everything that the Renderpath2D does.
### RenderPath3D_Forward
[[Header]](../WickedEngine/RenderPath3D_Forward.h) [[Cpp]](../WickedEngine/RenderPath3D_Forward.cpp)
Implements simple Forward rendering. It uses few render targets, small memory footprint, but not very efficient with many lights.
### RenderPath3D_Defered
[[Header]](../WickedEngine/RenderPath3D_Deferred.h) [[Cpp]](../WickedEngine/RenderPath3D_Deferred.cpp)
Implements "old school" Deferred rendering. It uses many render targets, capable of advanced post processing effects, and good to render many lights.
### RenderPath3D_TiledForward
[[Header]](../WickedEngine/RenderPath3D_TiledForward.h) [[Cpp]](../WickedEngine/RenderPath3D_TiledForward.cpp)
Implements an advanced method of Forward rendering to be able to render many lights efficiently. It uses fewer render targets, and less memory than deferred. One downside is that it is not capable of Subsurface scattering, because light buffers are not separated from rendering result.
### RenderPath3D_TiledDeferred
[[Header]](../WickedEngine/RenderPath3D_TiledDeferred.h) [[Cpp]](../WickedEngine/RenderPath3D_TiledDeferred.cpp)
Implements an advanced method of Deferred rendering to be able to render many lights with reduced memory bandwidth requirements. This has the largest memory footprint overall, but less bandwidth consummation than old school deferred approach, because lighting is computed in a single pass instead of additive blending. 
### RenderPath3D_PathTracing
[[Header]](../WickedEngine/RenderPath3D_PathTracing.h) [[Cpp]](../WickedEngine/RenderPath3D_PathTracing.cpp)
Implements a compute shader based path tracing solution. In a static scene, the rendering will converge to ground truth. When something changes in the scene (something moves, ot material changes, etc...), the convergence will be restarted from the beginning.
### LoadingScreen
[[Header]](../WickedEngine/LoadingScreen.h) [[Cpp]](../WickedEngine/LoadingScreen.cpp)
Render path that can be easily used for loading screen. It is able to load content or RenderPath in the background and switch to an other RenderPath onceit finished.

## System
You can find out more about the Entity-Component system and other engine-level systems under ENGINE/System filter in the solution.
### wiECS
[[Header]](../WickedEngine/wiECS.h)
#### ComponentMnager
This is the core entity-component relationship handler class. The purpose of this is to efficiently store, remove, add and sort components. Components can be any movable C++ structure. The best components are simple POD (plain old data) structures.
#### Entity
Entity is a number, it can reference components through ComponentManager containers. An entity is always valid if it exists. It's not required that an entity has any components. An entity has a component, if there is a ComponentManager that has a component which is associated with the same entity.
### wiScene
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
The logical scene representation using the Entity-Component System
- GetScene <br/>
Returns a global scene instance. The wiRenderer will use this scene instance to render the scene. The user can create multiple scenes as well, and merge those into the global scene so that those will be rendered as well.
- LoadModel() <br/>
There are two flavours to this. One of them immediately loads into the global scene. The other loads into a custom scene, which is usefult to manage the contents separately. This function will return an Entity that represents the root transform of the scene - if the attached parameter was true, otherwise it will return INVALID_ENTITY and no root transform will be created.
- Pick <br/>
Allows to pick the closest object with a RAY (closest ray intersection hit to the ray origin). The user can provide a custom scene or layermask to filter the objects to be checked.
#### NameComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
A string to identify an entity with a human readable name.
#### LayerComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
A bitmask that can be used to filter entities.
#### TransformComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
Orientation in 3D space, which supports various common operations on itself.
#### PreviousFrameTransformComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
Absolute orientation in the previous frame (a matrix).
#### HierarchyComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
An entity can be part of a transform hierarchy by having this component. Some other properties can also be inherieted, such as layer bitmask. If an entity has a parent, then it has a HierarchyComponent, otherwise it's not part of a hierarchy.
#### MaterialComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
Several properties that define a material, like color, textures, etc...
#### MeshComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
A mesh is an array of triangles. A mesh can have multiple parts, called MeshSubsets. Each MeshSubset has a material and it is using a range of triangles of the mesh. This can also have GPU resident data for rendering.
#### ImpostorComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
Supports efficient rendering of the same mesh multiple times (but as an approximation, such as a billboard cutout). A mesh can be rendered as impostors for example when it is not important, but has a large number of copies.
#### ObjectComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
An ObjectComponent is an instance of a mesh that has physical location in a 3D world. Multiple ObjectComponents can have the same mesh, and in this case the mesh will be rendered multiple times efficiently. It is expected that an entity that has ObjectComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.
#### RigidBodyPhysicsComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
Stores properties required for rigid body physics simulation and a handle that will be used by the physicsengine internally.
#### SoftBodyPhysicsComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
Stores properties required for soft body physics simulation and a handle that will be used by the physicsengine internally.
#### ArmatureComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
A skeleton used for skinning deformation of meshes.
#### LightComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
A light in the scene that can shine in the darkness. It is expected that an entity that has LightComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.
#### CameraComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
#### EnvironmentProbeComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
It is expected that an entity that has EnvironmentProbeComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.
#### ForceFieldComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
#### DecalComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
It is expected that an entity that has DecalComponent, also has TransformComponent and [AABB](#aabb) (axis aligned bounding box) component.
#### AnimationComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
#### WeatherComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
#### SoundComponent
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
Holds a Sound and a SoundInstance, and it can be placed into the scene via a TransformComponent. It can have a 3D audio effect it has a TransformComponent.
#### Scene
[[Header]](../WickedEngine/wiScene.h) [[Cpp]](../WickedEngine/wiScene.cpp)
A scene is a collection of component arrays. The scene is updating all the components in an efficient manner using the [job system](#wijobsystem). It can be serialized and saved/loaded from disk efficiently.
- Update(float deltatime) <br/>
This function runs all the requied systems to update all components contained within the Scene.
### wiJobSystem
[[Header]](../WickedEngine/wiJobSystem.h) [[Cpp]](../WickedEngine/wiJobSystem.cpp)
Manages the execution of concurrent tasks
- context <br/>
Defines a single workload that can be synchronized. It is used to issue jobs from within jobs and properly wait for completion. A context can be simply created on the stack because it is a simple atomic counter.
- Execute <br/>
This will schedule a task for execution on a separate thread for a given workload
- Dispatch <br/>
This will schedule a task for execution on multiple parallel threads for a given workload
- Wait <br/>
This function will block until all jobs have finished for a given workload. The current thread starts working on any work left to be finished.
### wiInitializer
[[Header]](../WickedEngine/wiInitializer.h) [[Cpp]](../WickedEngine/wiInitializer.cpp)
Initializes all engine systems either in a blocking or an asynchronous way.
### wiPlatform
[[Header]](../WickedEngine/wiPlatform.h)
You can get native platform specific handles here, such as window handle.
- GetWindow <br/>
Returns the platform specific window handle
- IsWindowActive <br/>
Returns true if the current window is the topmost one, false if it is not in focus


## Graphics
Everything related to rendering graphics will be discussed below
### wiGraphics
The graphics API wrappers
#### wiGraphicsDevice
[[Header]](../WickedEngine/wiGraphicsDevice.h) [[Cpp]](../WickedEngine/wiGraphicsDevice.cpp)
This is the interface for the graphics API abstraction. It is higher level than a graphics API, but these are the lowest level of rendering commands the engine offers.
#### wiGraphicsDevice_DX11
[[Header]](../WickedEngine/wiGraphicsDevice_DX11.h) [[Cpp]](../WickedEngine/wiGraphicsDevice_DX11.cpp)
DirectX11 rendering interface
#### wiGraphicsDevice_DX12
[[Header]](../WickedEngine/wiGraphicsDevice_DX12.h) [[Cpp]](../WickedEngine/wiGraphicsDevice_DX12.cpp)
DirectX12 rendering interface
#### wiGraphicsDevice_Vulkan
[[Header]](../WickedEngine/wiGraphicsDevice_Vulkan.h) [[Cpp]](../WickedEngine/wiGraphicsDevice_Vulkan.cpp)
Vulkan rendering interface (It is only compiled if Vulkan SDK is installed and the following environment variable is available: **$(VULKAN_SDK)**)
#### GraphicsDescriptors
[[Header]](../WickedEngine/wiGraphicsDescriptors.h) [[Cpp]](../WickedEngine/wiGraphicsDescriptors.cpp)
The place for graphics types like COMPARISON_FUNC, STENCIL_OP and descriptors like TextureDesc, GPUBufferDesc, etc. These types are used to create [graphics resources](#graphics-resources)
#### Graphics Resources
[[Header]](../WickedEngine/wiGraphicsResource.h) [[Cpp]](../WickedEngine/wiGraphicsResource.cpp)
Graphics resource wrappers for Texture, Shader, GPUBuffer, etc.
#### ConstantBufferMapping
[[Header]](../WickedEngine/ConstantBufferMapping.h)
Used to declare shared global constant buffer bind points between C++ code and shaders
#### ResourceMapping
[[Header]](../WickedEngine/ResourceMapping.h)
Used to declare shared global resource view bind points between C++ code and shaders
#### SamplerMapping
[[Header]](../WickedEngine/SamplerMapping.h)
Used to declare shared global sampler bind points between C++ code and shaders
#### ShaderInterop
[[Header]](../WickedEngine/ShaderInterop.h)
Shader Interop is used for declaring shared structures or values between C++ Engine code and shader code. There are several ShaderInterop files, prefixed by the subsystem they are used for to keep them minimal and more readable: <br/>
[ShaderInterop_BVH.h](../WickedEngine/ShaderInterop_BVH.h) <br/>
[ShaderInterop_EmittedParticle.h](../WickedEngine/ShaderInterop_EmittedParticle.h) <br/>
[ShaderInterop_FFTGenerator.h](../WickedEngine/ShaderInterop_FFTGenerator.h) <br/>
[ShaderInterop_Font.h](../WickedEngine/ShaderInterop_Font.h) <br/>
[ShaderInterop_GPUSortLib.h](../WickedEngine/ShaderInterop_GPUSortLib.h) <br/>
[ShaderInterop_HairParticle.h](../WickedEngine/ShaderInterop_HairParticle.h) <br/>
[ShaderInterop_Image.h](../WickedEngine/ShaderInterop_Image.h) <br/>
[ShaderInterop_Ocean.h](../WickedEngine/ShaderInterop_Ocean.h) <br/>
[ShaderInterop_Postprocess.h](../WickedEngine/ShaderInterop_Postprocess.h) <br/>
[ShaderInterop_Raytracing.h](../WickedEngine/ShaderInterop_Raytracing.h) <br/>
[ShaderInterop_Renderer.h](../WickedEngine/ShaderInterop_Renderer.h) <br/>
[ShaderInterop_Skinning.h](../WickedEngine/ShaderInterop_Skinning.h) <br/>
[ShaderInterop_Utility.h](../WickedEngine/ShaderInterop_Utility.h) <br/>
[ShaderInterop_Vulkan.h](../WickedEngine/ShaderInterop_Vulkan.h) <br/>
The ShaderInterop also contains the resource macros to help share code between C++ and HLSL, and portability between shader compilers. Read more about macros in the [Shaders section](#shaders)
### wiRenderer
[[Header]](../WickedEngine/wiRenderer.h) [[Cpp]](../WickedEngine/wiRenderer.cpp)
This is a collection of graphics technique implentations and functions to draw a scene, shadows, post processes and other things. It is also the manager of the GraphicsDevice instance, and provides other helper functions to load shaders from files on disk.<br/>
Apart from graphics helper functions that are mostly independent of each other, the renderer also provides facilities to render a Scene. This can be done via the high level DrawScene, DrawSceneTransparent, etc. functions. These don't set up render passes or viewports by themselves, but they expect that they are set up from outside. Most other render state will be handled internally, such as constant buffers, stencil, blendstate, etc.. Please see how the scene rendering functions are used in the High level interface RenderPath3D implementations (for example [RenderPath3D_TiledForward.cpp](../WickedEngine/RenderPath3D_TiledForward.cpp)) <br/>
Other points of interest here are utility graphics functions, such as CopyTexture2D, GenerateMipChain, and the like, which provide customizable operations, such as border expand mode, Gaussian mipchain generation, and things that wouldn't be supported by a graphics API.<br/>
The renderer also hosts post process implementations. These functions are independent and have clear input/output parameters. They shouldn't rely on any other extra setup from outside.<br/>
### wiEnums
[[Header]](../WickedEngine/wiEnums.h) [[Cpp]](../WickedEngine/wiEnums.cpp)
This is a collection of enum values used by the wiRenderer to identify graphics resources. Usually arrays of the same resource type are declared and the XYZENUM_COUNT values tell the length of the array. The other XYZENUM_VALUE represents a single element within that array. This makes the code easy to manage, for example:
```cpp
enum CBTYPE
{
	CBTYPE_MESH, // = 0
	CBTYPE_APPLESANDORANGES, // = 1
	CBTYPE_TAKEITEASY, // = 2
	CBTYPE_COUNT // = 3
};
GPUBuffer buffers[CBTYPE_COUNT]; // this example array contains 3 elements
//...
device->BindConstantBuffer(&buffers[CBTYPE_MESH], 0, 1, cmd); // makes it easy to reference an element
```
This is widely used to make code straight forward and easy to add new objects, without needing to create additional declarations, except for the enum values.
### wiImage
[[Header]](../WickedEngine/wiImage.h) [[Cpp]](../WickedEngine/wiImage.cpp)
This can render images to the screen in a simple manner. You can draw an image to the screen with a simple one liner:
```cpp
wiImage::Draw(myTexture, wiImageParams(10, 20, 256, 128), cmd);
```
The example will draw a 2D texture image to the position (10, 20), with a size of 256 x 128 pixels to the screen (or the curernt render pass). There are a lot of other parameters to customize the rendered image, for more information see wiImageParams structure.
- wiImageParams <br/>
Describe all parameters of how and where to draw the image on the screen. 
### wiFont
[[Header]](../WickedEngine/wiFont.h) [[Cpp]](../WickedEngine/wiFont.cpp)
This can render fonts to the screen in a simple manner. You can render a font as simple as this:
```cpp
wiFont("write this!", wiFontParams(10, 20)).Draw(cmd);
```
Which will write the text <i>write this!</i> to 10, 20 pixel position onto the screen. There are many other parameters to describe the font's position, size, color, etc. See the wiFontParams structure for more details.
- wiFontParams <br/>
Describe all parameters of how and where to draw the font on the screen. 
### wiEmittedParticle
[[Header]](../WickedEngine/wiEmittedParticle.h) [[Cpp]](../WickedEngine/wiEmittedParticle.cpp)
GPU driven emitter particle system, used to draw large amount of camera facing quad billboards. Supports simulation with force fields and fluid simulation based on Smooth Particle Hydrodynamics computation.
### wiHairParticle
[[Header]](../WickedEngine/wiHairParticle.h) [[Cpp]](../WickedEngine/wiHaorParticle.cpp)
GPU driven particles that are attached to a mesh surface. It can be used to render vegetation. It participates in force fields simulation.
### wiOcean
[[Header]](../WickedEngine/wiOcean.h) [[Cpp]](../WickedEngine/wiOcean.cpp)
Ocean renderer using Fast Fourier Transforms simulation. The ocean surface is always rendered relative to the camera, like an infinitely large water body.
### wiSprite
[[Header]](../WickedEngine/wiSprite.h) [[Cpp]](../WickedEngine/wiSprite.cpp)
A helper facility to render and animate images. It uses the [wiImage](#wiimage) renderer internally
- Anim <br/>
Several different simple animation utilities, like animated textures, wobbling, rotation, fade out, etc...
### wiTextureHelper
[[Header]](../WickedEngine/wiTextureHelper.h) [[Cpp]](../WickedEngine/wiTextureHelper.cpp)
This is used to generate procedural textures, such as uniform colors, noise, etc...
### wiGPUSortLib
[[Header]](../WickedEngine/wiGPUSortLib.h) [[Cpp]](../WickedEngine/wiGPUSortLib.cpp)
This is a GPU sorting facility using the Bitonic Sort algorithm. It can be used to sort an index list based on a list of floats as comparison keys entirely on the GPU.
### wiGPUBVH
[[Header]](../WickedEngine/wiGPUBVH.h) [[Cpp]](../WickedEngine/wiGPUBVH.cpp)
This facility can generate a BVH (Bounding Volume Hierarcy) on the GPU for a [Scene](#scene). The BVH structure can be used to perform efficient RAY-triangle intersections on the GPU, for example in ray tracing.


## GUI
The custom GUI, implemented with engine features
### wiGUI
[[Header]](../WickedEngine/wiGUI.h) [[Cpp]](../WickedEngine/wiGUI.cpp)
The wiGUI is responsible to run a GUI interface and manage widgets
### wiEventArgs
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
This will be sent to widget callbacks to provide event arguments in different formats
### wiWidget
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
The base widget interface can be extended with specific functionality. The GUI will store and process widgets by this interface. 
#### wiButton
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
A simple clickable button. Supports the OnClick event callback.
#### wiLabel
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
A simple static text field. 
#### wiTextInputField
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
Supports text input when activated. Pressing Enter will accept the input and fire the OnInputAccepted callback. Pressing the Escape key while active will cancel the text input and restoe the previous state. There can be only one active text input field at a time.
#### wiSlider
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
A slider, that can represent float or integer values. The slider also accepts text input to input number values. If the number input is outside of the slider's range, it will expand its range to support the newly assigned value. Upon changing the slider value, whether by text input or sliding, the OnSlide event callback will be fired. 
#### wiCheckBox
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
The checkbox is a two state item which can represent true/false state. The OnClick event callback will be fired upon changing the value (by clicking on it)
#### wiComboBox
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
Supports item selection from a list of text. Can set the maximum number of visible items. If the list of items is greater than the max allowed visible items, then a vertical scrollbar will be presented to allow showing hidden items. Upon selection, the OnSelect event callback will be triggered.
#### wiWindow
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
A window widget is able to hold any number of other widgets. It can be moved across the screen, minimized and resized by the user. If a window holds a widget, it will manage its lifetime, so if the window is destroyed, all its children will be destroyed as well.
#### wiColorPicker
[[Header]](../WickedEngine/wiWidget.h) [[Cpp]](../WickedEngine/wiWidget.cpp)
Supports picking a HSV (or HSL) color, displaying its RGB values. On selection, the OnColorChanged event callback will be fired and the user can read the new RGB value from the event argument.


## Helpers
A collection of engine-level helper classes
### wiAllocators
[[Header]](../WickedEngine/wiAllocators.h)
#### LinearAllocator
[[Header]](../WickedEngine/wiAllocators.h)
The linear allocator is used to allocate contiguous blocks of memory. The amount of maximum allocations is defined at creation time. Blocks then can be allocated from beginning to end until there is free space left. Blocks can be freed from the end until the allocator is not empty. This is not thread safe.
### wiArchive
[[Header]](../WickedEngine/wiArchive.h) [[Cpp]](../WickedEngine/wiArchive.cpp)
This is used for serializing binary data to disk or memory. An archive file always starts with the 64-bit version number that it was serialized with. An archive of greater version number than the current archive version of the engine can't be opened safely, so an error message will be shown if this happens. A certain archive version will not be forward compatible with the current engine version if the current archive version barrier number is greater than the archive's own version number.
### wiColor
[[Header]](../WickedEngine/wiColor.h)
Utility to convert to/from float color data to 32-bit RGBA data (stored in a uint32_t as RGBA, where each channel is 8 bits)
### wiContainers
[[Header]](../WickedEngine/wiContainers.h)
#### ThreadSafeRingBuffer
[[Header]](../WickedEngine/wiContainers.h)
This is a thread safe container that can hold elements of one certain data type at once. Elements are added to the end of container, and they are removed from the beginning. The container wraps around, so if the last element would be put after the last valid array index, then it will be put to the first instead (if the first array index is not occupied).
### wiFadeManager
[[Header]](../WickedEngine/wiFadeManager.h) [[Cpp]](../WickedEngine/wiFadeManager.cpp)
Simple helper to manage a fadeout screen. Fadeout starts at transparent, then fades smoothly to an opaque color (such as black, in most cases), then a callback occurs which the user can handle with their own event. After that, the color will fade back to transperent. This is used by the [MainComponent](#maincomponent) to fade from one RenderPath to an other.
### wiHashString
[[Header]](../WickedEngine/wiHashString.h)
### wiHelper
[[Header]](../WickedEngine/wiHelper.h) [[Cpp]](../WickedEngine/wiHelper.cpp)
Many helper utility functions, like screenshot, readfile, messagebox, splitpath, sleep, etc...
### wiIntersect
[[Header]](../WickedEngine/wiIntersect.h) [[Cpp]](../WickedEngine/wiIntersect.cpp)
Primitives that can be intersected with each other
#### AABB
[[Header]](../WickedEngine/wiIntersect.h) [[Cpp]](../WickedEngine/wiIntersect.cpp)
#### SPHERE
[[Header]](../WickedEngine/wiIntersect.h) [[Cpp]](../WickedEngine/wiIntersect.cpp)
#### RAY
[[Header]](../WickedEngine/wiIntersect.h) [[Cpp]](../WickedEngine/wiIntersect.cpp)
#### Frustum
[[Header]](../WickedEngine/wiIntersect.h) [[Cpp]](../WickedEngine/wiIntersect.cpp)
#### Hitbox2D
[[Header]](../WickedEngine/wiIntersect.h) [[Cpp]](../WickedEngine/wiIntersect.cpp)
### wiMath
[[Header]](../WickedEngine/wiMath.h) [[Cpp]](../WickedEngine/wiMath.cpp)
Math related helper functions, like lerp, triangleArea, HueToRGB, etc...
### wiRandom
[[Header]](../WickedEngine/wiRandom.h) [[Cpp]](../WickedEngine/wiRandom.cpp)
Uniform random number generator with a good balance
### wiRectPacker
[[Header]](../WickedEngine/wiRectPacker.h) [[Cpp]](../WickedEngine/wiRectPacker.cpp)
### wiResourceManager
[[Header]](../WickedEngine/wiResourceManager.h) [[Cpp]](../WickedEngine/wiResourceManager.cpp)
This can load images and sounds. It will hold on to resources until there is at least something that is referencing them, otherwise deletes them. One resource can have multiple owners, too. This is thread safe.
### wiSpinLock
[[Header]](../WickedEngine/wiSpinLock.h) [[Cpp]](../WickedEngine/wiSpinLock.cpp)
This can be used to guarantee exclusive access to a block in multithreaded race condition scenario instead of a mutex. The difference to a mutex that this doesn't let the thread to yield, but instead spin on an atomic flag until the spinlock can be locked.
### wiStartupArguments
[[Header]](../WickedEngine/wiStartupArguments.h) [[Cpp]](../WickedEngine/wiStartupArguments.cpp)
This is to store the startup parameters that were passed to the application from the operating system "command line". The user can query these arguments by name.
### wiTimer
[[Header]](../WickedEngine/wiTimer.h) [[Cpp]](../WickedEngine/wiTimer.cpp)
High resolution stopwatch timer


## Input
The input interface can be found here
### wiInput
[[Header]](../WickedEngine/wiInput.h) [[Cpp]](../WickedEngine/wiInput.cpp)
This is the high level input interface
- Initialize
- Update
- Down
- Press
- Hold
- GetPointer
- SetPointer
- HidePointer
- GetAnalog
- SetControllerFeedback
### BUTTON
### KeyboardState
### MouseState
### ControllerState
### ControllerFeedback
### Touch
- GetTouches
### wiXInput
[[Header]](../WickedEngine/wiXInput.h) [[Cpp]](../WickedEngine/wiXInput.cpp)
Low level wrapper for XInput API (capable of handling Xbox controllers). This functionality is used by the more generic wiInput interface, so the user probably doesn't need to use this.
### wiRawInput
[[Header]](../WickedEngine/wiRawInput.h) [[Cpp]](../WickedEngine/wiRawInput.cpp)
Low level wrapper for RAWInput API (capable of handling human interface devices, such as mouse, keyboard, controllers). This functionality is used by the more generic wiInput interface, so the user probably doesn't need to use this.



## Audio
Handles audio playback and spatial audio.
### wiAudio
[[Header]](../WickedEngine/wiAudio.h) [[Cpp]](../WickedEngine/wiAudio.cpp)
The namespace that is a collection of audio related functionality. It is currently implemented with XAudio2
- CreateSound
- CreateSoundInstance
- Destroy
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
You can find the physics system related functionality under ENGINE/Physics filter in the solution.
It uses the entity-component system to perform updating all physics components in the world.
### wiPhysicsEngine
[[Header]](../WickedEngine/wiPhysicsEngine.h) [[Cpp]](../WickedEngine/wiPhysicsEngine.cpp)
- Initialize<br/>
This must be called before using the physics system, but it is automatically done by [wiInitializer](#wiinitializer)
- IsEnabled
- SetEnabled
- RunPhysicsUpdateSystem
### wiPhysicsEngine_Bullet
[[Header]](../WickedEngine/wiPhysicsEngine_BULLET.h) [[Cpp]](../WickedEngine/wiPhysicsEngine_BULLET.cpp)
Bullet physics engine implementation of the physics update system


## Network
### wiNetwork
[[Header]](../WickedEngine/wiNetwork.h) [[Cpp]](../WickedEngine/wiNetwork.cpp)
Simple interface that provides UDP networking features.
- Initialize
- CreateSocket
- Destroy
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
### wiLua
[[Header]](../WickedEngine/wiLua.h) [[Cpp]](../WickedEngine/wiLua.cpp)
The Lua scripting interface on the C++ side. This allows to execute lua commands from the C++ side and manipulate the lua stack, such as pushing values to lua and getting values from lua, among other things.
### wiLua_Globals
[[Header]](../WickedEngine/wiLua_Globals.h)
Hardcoded lua script in text format. This will be always executed and provides some commonly used helper functionality for lua scripts.
### wiLuna
[[Header]](../WickedEngine/wiLuna.h)
Helper to allow bind engine classes from C++ to Lua


## Tools
This is the place for tools that use engine-level systems
### wiBackLog
[[Header]](../WickedEngine/wiBacklog.h) [[Cpp]](../WickedEngine/wiBackLog.cpp)
Used to log any messages by any system, from any thread. It can draw itself to the screen. It can execute Lua scripts.
### wiProfiler
[[Header]](../WickedEngine/wiProfiler.h) [[Cpp]](../WickedEngine/wiProfiler.cpp)
Used to time specific ranges in execution. Support CPU and GPU timing. Can write the result to the screen as simple text at this time.


## Shaders
There is a separate project file for shaders in the solution. Shaders are written in pure HLSL, although there are some macros used to keep them more manageable and easier to build with different shader compilers. The macros are used to declare resources, for example:
- CBUFFER(name, slot) -> cbuffer name : register(b slot) <br/>

You can see them all in [ShaderInterop.h](../WickedEngine/ShaderInterop.h)

Note: When creating constant buffers, the structure must be strictly padded to 16-byte boundaries according to HLSL rules. Apart from that, the following limitation is in effect for Vulkan compatibility:

Incorrect padding to 16-byte:
```cpp
struct cbuf
{
	float padding;
	float3 value; // <- the larger type can start a new 16-byte slot, so the C++ and shader side structures could mismatch
	float4 value2;
};
```

Correct padding:
```cpp
struct cbuf
{
	float3 value; // <- the larger type must be placed before the padding
	float padding;
	float4 value2;
};
```
