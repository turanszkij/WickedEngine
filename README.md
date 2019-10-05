<img align="left" src="images/logo_small.png" width="128px"/>

# Wicked Engine

[![Build status][s1]][av] [![License: MIT][s3]][li] [![Discord chat][s2]][di] 
<a href="https://twitter.com/intent/follow?screen_name=turanszkij">
        <img src="https://img.shields.io/twitter/follow/turanszkij.svg?style=social"
            alt="follow on Twitter"></a><br/>
Latest builds: 
[![Editor][s4]][do64] [![Editor32][s5]][do32]
[![Tests][s6]][dt64] [![Tests32][s7]][dt32] <br/>

[s1]: https://ci.appveyor.com/api/projects/status/3dbcee5gd6i7qh7v?svg=true
[s2]: https://img.shields.io/discord/602811659224088577?logo=discord
[s3]: https://img.shields.io/badge/License-MIT-orange.svg
[s4]: https://img.shields.io/badge/editor-64bit-blue.svg
[s5]: https://img.shields.io/badge/editor-32bit-blue.svg
[s6]: https://img.shields.io/badge/tests-64bit-blue.svg
[s7]: https://img.shields.io/badge/tests-32bit-blue.svg

[av]: https://ci.appveyor.com/project/turanszkij/wickedengine
[di]: https://discord.gg/CFjRYmE
[li]: https://opensource.org/licenses/MIT
[do64]: https://ci.appveyor.com/api/projects/turanszkij/wickedengine/artifacts/WickedEngineEditor.zip?branch=master&job=Platform%3A%20x64
[do32]: https://ci.appveyor.com/api/projects/turanszkij/wickedengine/artifacts/WickedEngineEditor.zip?branch=master&job=Platform%3A%20Win32
[dt64]: https://ci.appveyor.com/api/projects/turanszkij/wickedengine/artifacts/WickedEngineTests.zip?branch=master&job=Platform%3A%20x64
[dt32]: https://ci.appveyor.com/api/projects/turanszkij/wickedengine/artifacts/WickedEngineTests.zip?branch=master&job=Platform%3A%20Win32
[ba]: https://github.com/turanszkij/WickedEngine/tree/old-system-backup

<br/>
<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/gltfanim.gif"/>
Wicked Engine is an open-source game engine written in C++. It is easy to use, high performance and feature rich. There are no external dependencies, but some free libraries are included as part of the source code. The MIT license means that anyone is free to download, modify, share or do anything with it. <br/>
This project is hosted on GitHub. For any questions, please open an issue at: https://github.com/turanszkij/WickedEngine<br/>
Because everything is changing rapidly, the documentation is sparse at the moment. <br/>

[Documentation](Documentation/WickedEngine-Documentation.md)<br/>
[Scripting API Documentation](Documentation/ScriptingAPI-Documentation.md)<br/>
[Features](features.txt)<br/>
[Devblog](https://turanszkij.wordpress.com/)<br/>
[Videos](https://www.youtube.com/playlist?list=PLLN-1FTGyLU_HJoC5zx6hJkB3D2XLiaxS)<br/>

You can download the engine by using Git and cloning the repository, or downloading it as zip, which will give you the full C++ source code that you must build for yourself. Building is simply pressing F5 in Visual Studio. You can also choose to download a pre-built version of the Editor or Tests applications, which will allow you to load content and write LUA scripts out of the box. <br/>

![Emitter](https://turanszkij.files.wordpress.com/2019/02/emitterskinned2.gif) ![Drone](https://turanszkij.files.wordpress.com/2018/11/drone_anim.gif) <br/>

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/physics.gif"/>

### Platforms:
- Windows PC Desktop (x86, x64)
- Universal Windows (x86, x64, ARM, Phone, XBOX One)

### Requirements:

- Windows 10
- Visual Studio 2019

### Getting started: 

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/trace.gif"/>

Wicked Engine is provided as a static library. This means, that when creating a new project, the developer has to link against the compiled library before using its features. 
For this, you must first compile the engine library project for the desired platform. For Windows Desktop, this is the WickedEngine_Windows project. 
Then set the following dependencies to this library in Visual Studio this way in the implementing project (paths are as if your project is inside the engine root folder):

1. Open Project Properties -> Configuration Properties
2. C/C++ -> General -> Additional Include Directories: 
	- ./WickedEngine
3. Linker -> General -> Additional Library Directories:
	- Directory of your built .lib file (For example ./x64/Release)
4. Also be sure to compile with a non-DLL runtime library for Release builds:
	- Project settings -> C/C++ -> Code Generation -> Runtime Library -> Multi threaded
5. If you want to create an UWP application, #define WINSTORE_SUPPORT preprocessor for the whole implementing project and link against the WickedEngine_UWP library.
	
When your project settings are set up, time to #include "WickedEngine.h" in your source. This will enable the use of all the engine features and link the necessary binaries. After this, you should already be able to build your project. If you are having difficulties, there are some projects that  you can compare against, such as the Editor, Tests or Template projects.
Once the build is succesful, you can start using the engine.

Initialization example (C++):

<img align="right" src="https://turanszkij.files.wordpress.com/2018/05/sphinit.gif"/>

```cpp
// Include engine headers:
#include "WickedEngine.h"

// Declare main component once per application:
MainComponent main;

// If you want to render, interface with Windows API like this:
main.SetWindow(hWnd, hInst);

// Run the application:
while(true) {
   main.Run(); 
}
```

Some basic usage examples (C++):
```cpp
RenderPath3D_Deferred myGame; // Declare a game screen component, aka "RenderPath" (you could also override its Update(), Render() etc. functions). This is a 3D, Deferred path for example, but there are others
main.ActivatePath(&myGame); // Register your game to the application. It will call Start(), Update(), Render(), etc. from now on...

wiSceneSystem::LoadModel("myModel.wiscene"); // Simply load a model into the current global scene
wiSceneSystem::GetScene(); // Get the current global scene
wiRenderer::ClearWorld(); // Delete every model, etc. from the current global scene

wiSceneSystem::Scene scene2; // create a separate scene
wiSceneSystem::LoadModel(scene2, "myModel2.wiscene"); // Load model into a separate scene
wiSceneSystem::GetScene().Merge(scene2); // Combine separate scene with global scene

myGame.setSSAOEnabled(true); // You can enable post process effects this way...

RenderPath2D myMenuScreen; // This is an other render path, but now a simple 2D one. It can only render 2D graphics by default (like a menu for example)
main.ActivatePath(&myMenuScreen); // activate the menu, the previous path (myGame) will be stopped

wiSprite mySprite("image.png"); // There are many utilities, such as a "sprite" helper class
myMenuScreen.addSprite(&mySprite); // The 2D render path is ready to handle sprite and font rendering for you

wiAudio::Sound mySound;
wiAudio::CreateSound("explosion.wav", &mySound); // Loads a sound file
wiAudio::SoundInstance mySoundInstance;
wiAudio::CreateSoundInstance(&mySound, &mySoundInstance); // Instances the sound file, it can be played now
wiAudio::Play(&mySoundInstance); // Play the sound instance
wiAudio::SetVolume(0.6, &mySoundInstance); // Set the volume of this soundinstance
wiAudio::SetVolume(0.2); // Set the master volume

if (wiInputManager::press(VK_SPACE)) { soundEffect.Stop(); } // You can check if a button is pressed or not (this only triggers once)
if (wiInputManager::down(VK_SPACE)) { soundEffect.Play(); } // You can check if a button is pushed down or not (this triggers repeatedly)
```

Some scripting examples (LUA):
```lua
-- Set a rendering path for the application main component
path = RenderPath3D_Deferred;
main.SetActivePath(path);    -- "main" is created automatically

-- Load a model entity into the global scene:
entity = LoadModel("myModel.wiscene");

-- Load a model entity into a separate scene:
scene2 = Scene()
entity2 = LoadModel(scene2, "myModel2.wiscene");

-- Combine the separate scene with the global scene:
scene.Merge(scene2);

-- Get the current global scene:
scene = GetScene();

-- Move model to the right using the entity-component system:
transform = scene.Component_GetTransform(entity);
transform.Translate(Vector(2, 0, 0));

-- Clear every model from the current global scene:
ClearWorld();

-- Print any WickedEngine class information to the backlog:
getprops(main);    -- prints the main component methods
getprops(scene);    -- prints the Scene class methods
getprops(path);    -- prints the deferred render path methods

-- Play a sound:
sound = Sound()
audio.CreateSound("explosion.wav", sound)
soundinstance = SoundInstance()
audio.CreateSoundInstance(sound, soundinstance)  -- several instances can be created from one file
audio.Play(soundinstance)
audio.SetVolume(0.6, soundinstance)  -- sets the volume of this soundinstance
audio.SetVolume(0.2)  -- sets the master volume

-- Check for input:
if(input.press(VK_LEFT)) then
   sound.Play(); -- this will play the sound if you press the left arrow on the keyboard
end
```
<i>(You can enter lua scripts into the backlog (HOME button), or the startup.lua script which is always executed on application startup if it is found near the app, or load a script via dofile("script.lua") command)</i>

For more code samples and advanced use cases, please see the example projects, like the Template_Windows, Tests, or Editor project. There are also sample models and scripts included with Wicked Engine in the models and scripts folders. Check them out to learn about more features.

### Scripting API:

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/hairparticle2.gif"/>

You can use a great number of engine features through the Lua scripting api, which can even be used real time while the program is running. The included applications, like the Editor,
contain a scripting input method toggled by the "Home" key. A blue screen will be presented where the user can type in LUA commands. It is very minimal in regards to input methods.
For further details, please check the scripting API documentation: [Wicked Engine Scripting API](Documentation/ScriptingAPI-Documentation.md)


### Model import/export:

The native model format is the <b>WISCENE</b> format. Any application using Wicked Engine can open this format efficiently.

In addition, the Editor supports the importing of some common model formats (the list will potentially grow): 
- <b>OBJ</b>
- <b>GLTF 2.0</b>

The preferred workflow is to import models into the Editor, and save them as <b>WISCENE</b>, then any Wicked Engine application can open them.<br/>
<i>(The old Blender exporter script is now not supported! (from version 0.21.0), because the engine was redesigned with Entity-Component System at its core. The old object oriented version can be found [here][ba].)</i>

### Graphics API:

The default renderer is DirectX 11. There is also a DirectX12 renderer (experimental) and Vulkan renderer (experimental).
You can specify command line arguments to switch between render devices or other settings. Currently the list of options:
<table>
  <tr>
    <th>Argument</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>dx12</td>
    <td>Create DirectX 12 rendering device (Windows 10 required)</td>
  </tr>
  <tr>
    <td>vulkan</td>
    <td>Create Vulkan rendering device*. (Only if engine was built with Vulkan SDK installed)</td>
  </tr>
  <tr>
    <td>debugdevice</td>
    <td>Create rendering device with debug layer enabled for validation. Performance will be degraded.</td>
  </tr>
  <tr>
    <td>hlsl6</td>
    <td>Reroute shader loading path to use shader model 6 shaders** (DirectX 12 only)</td>
  </tr>
</table>

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/soft.gif"/>

* *Before enabling the Vulkan API, you must first also compile SPIR-V shaders. This step is not yet included in the standard build process. First, run the "generate_shader_buildtask_spirv.py"
Python script which will generate the SPIR-V shader building program "build_SPIRV.bat". Run "build_SPIRV.bat" to build all HLSL shaders as SPIR-V bytecode format for Vulkan. Shader loading after this 
is automatic if you start the application with Vulkan support.
This feature is experimental, not tested thoroughly yet.

* **To load HLSL 6 shaders, replicate the exact steps as with SPIR-V above(*), but the python script you should run is called "generate_shader_buildtask_hlsl6.py" which will generate "build_HLSL6.bat". 
This feature is experimental, not tested thoroughly yet.

<br/>

### Contributions

Contributions can be submitted on Github by following the steps below:
1) Open an issue and describe the feature or patch in detail
2) The feature or patch will be discussed in the issue, and determined if it would benefit the project
3) After the request is accepted, open a pull request and reference the issue with #issue_number
4) Code review will be conducted and the pull request will be merged when it meets the requirements
5) When the pull request passes, you will be added to the credits as a contributor and your name shall be remembered by future generations

<br/>

### Finally, take a look at some screenshots:

Sponza scene with voxel GI enabled:
![Sponza](https://turanszkij.files.wordpress.com/2018/12/sponza.png)

Damaged Helmet GLTF sample model:
![Sponza](https://turanszkij.files.wordpress.com/2019/03/damagedhelmet.png)

Path tracing in the living room:
![LivingRoom](https://turanszkij.files.wordpress.com/2019/09/livingroom.jpg)

City scene with a light map, model from <a href="https://www.cgtrader.com/michaelmilesgallie">Michael Gallie</a>:
![City](https://turanszkij.files.wordpress.com/2019/01/city1.png)

Path tracing in the city:
![Balcony](https://turanszkij.files.wordpress.com/2019/01/city2.png)

Path traced caustics:
![Caustics](https://turanszkij.files.wordpress.com/2019/01/trace.png)

Lots of instanced boxes with a light map:
![Lightmap](https://turanszkij.files.wordpress.com/2019/01/lightmap.png)

Lots of boxes path traced in the editor:
![EditorBoxes](https://turanszkij.files.wordpress.com/2019/01/boxes.png)

Bloom and post processing:
![Bloom](https://turanszkij.files.wordpress.com/2019/01/bloom.png)

Bistro scene from Amazon Lumberyard (from <a href="http://casual-effects.com/data/index.html">Morgan McGuire's graphics archive</a>):
![Bistro_out](https://turanszkij.files.wordpress.com/2019/01/bistro_out_0.png)

Bistro scene from the inside:
![Bistro_in](https://turanszkij.files.wordpress.com/2019/01/bistro_in_2.png)

Parallax occlusion mapping:
![ParallxOcclusionMapping](https://turanszkij.files.wordpress.com/2019/01/pom.png)

Large scale particle simulation on the GPU:
![ParticleSimulation](https://turanszkij.files.wordpress.com/2019/01/gpuparticles3.png)

Tiled light culling in the Bistro:
![TiledLightCulling](https://turanszkij.files.wordpress.com/2019/02/bistro_heatmap-1.png)

Physically based rendering test:
![PBRTest](https://turanszkij.files.wordpress.com/2019/03/roughness.png)

GPU-based BVH builder:
![GPU_BVH](https://turanszkij.files.wordpress.com/2019/07/bvh_livingroom.png)
