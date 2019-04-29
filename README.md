<img align="left" src="images/logo_small.png" width="128px"/>

# Wicked Engine

[![Build status][s1]][av] [![License: MIT][s3]][li] [![Join the chat at https://gitter.im/WickedEngine/Lobby][s2]][gi]
<a href="https://twitter.com/intent/follow?screen_name=turanszkij">
        <img src="https://img.shields.io/twitter/follow/turanszkij.svg?style=social"
            alt="follow on Twitter"></a><br/>
[![DownloadEditor][s4]][do] [![DownloadTests][s5]][dt] 

[s1]: https://ci.appveyor.com/api/projects/status/3dbcee5gd6i7qh7v?svg=true
[s2]: https://badges.gitter.im/WickedEngine/Lobby.svg
[s3]: https://img.shields.io/badge/License-MIT-yellow.svg
[s4]: https://img.shields.io/badge/download%20build-editor-blue.svg
[s5]: https://img.shields.io/badge/download%20build-tests-blue.svg

[av]: https://ci.appveyor.com/project/turanszkij/wickedengine
[gi]: https://gitter.im/WickedEngine/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge
[li]: https://opensource.org/licenses/MIT
[do]: https://ci.appveyor.com/api/projects/turanszkij/wickedengine/artifacts/WickedEngineEditor.zip?branch=master
[dt]: https://ci.appveyor.com/api/projects/turanszkij/wickedengine/artifacts/WickedEngineTests.zip?branch=master
[ba]: https://github.com/turanszkij/WickedEngine/tree/old-system-backup


Wicked Engine is an open-source game engine written in C++. The main focus is to be easy to set up and use, light weight, high performance, and graphically advanced.
The full source code is provided with the MIT license, which means, anyone is free to use it for anything without additional considerations. The code will not contain any parts with other licensing. The code is hosted on GitHub: https://github.com/turanszkij/WickedEngine For any questions, please open an issue there.

From version <b>0.21.0</b> onwards, the engine was changed to use Entity-Component System and the old Object-Oriented system was dropped. <b>It is not backwards-compatible</b>, so assets/scripts made with the old system are unfortunately not usable.
You can find a snapshot of the old Object-Oriented codebase (0.20.6) [here][ba], but it will not be updated anymore.

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/gltfanim.gif"/>

[Documentation](Documentation/WickedEngine-Documentation.md)<br/>
[Scripting API Documentation](Documentation/ScriptingAPI-Documentation.md)<br/>
[Features](features.txt)<br/>
[Devblog](https://turanszkij.wordpress.com/)<br/>
[Videos](https://www.youtube.com/playlist?list=PLLN-1FTGyLU_HJoC5zx6hJkB3D2XLiaxS)<br/>

To test the engine, this solution contains several projects which you can build out of the box with Visual Studio 2017. There shall be no external dependencies. You can now also [download the latest Editor build!][do]
- You can run the "Editor" or the "Tests" project to check out features and play around in a 3D environment, load models, scripts, etc. 
- You can find a "Template_Windows" project which is a minimal set up to get the engine up and running in a clean Windows 10 application.
- Be sure to build in "Release" mode for optimal performance. Debugging support will be reduced.
- Be sure to set an appropriate startup project such as the "Editor" or "Tests" for example.
- There are multiple sample models in the "models" folder. You can load any of them inside the Editor.
- There are multiple sample LUA scripts in the "scripts" folder. You can load any of them inside the Editor, but please check how to use them first by reading the first few lines of the scripts.
- Please open an Issue here on GitHub if you encounter any difficulties.

![Emitter](https://turanszkij.files.wordpress.com/2019/02/emitterskinned2.gif) ![Drone](https://turanszkij.files.wordpress.com/2018/11/drone_anim.gif)

The default renderer is DirectX 11. The DirectX 12 renderer is now available (experimental). Vulkan renderer is now available (experimental).
You can specify command line arguments for each application to switch between render devices or other settings. Currently the list of options:
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

<img align="left" src="https://turanszkij.files.wordpress.com/2018/11/soft.gif"/>

* *Before enabling the Vulkan API, you must first also compile SPIR-V shaders. This step is not yet included in the standard build process. First, run the "generate_shader_buildtask_spirv.py"
Python script which will generate the SPIR-V shader building program "build_SPIRV.bat". Run "build_SPIRV.bat" to build all HLSL shaders as SPIR-V bytecode format for Vulkan. Shader loading after this 
is automatic if you start the application with Vulkan support.
This feature is experimental, not tested thoroughly yet.

* **To load HLSL 6 shaders, replicate the exact steps as with SPIR-V above(*), but the python script you should run is called "generate_shader_buildtask_hlsl6.py" which will generate "build_HLSL6.bat". 
This feature is experimental, not tested thoroughly yet.

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/physics.gif"/>

### Platforms:
- Windows PC Desktop (x86, x64)
- Universal Windows (PC, Phone, XBOX One)

### Requirements:

- Windows 10
- DirectX 11 compatible GPU
- Visual Studio 2017
- Windows 10 SDK

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/trace.gif"/>

### Getting started: 

The interface is designed to be somewhat similar to the XNA framework, with overridable Load, Update, Render methods, switchable rendering components, content managers. 
However, it makes use of the C++ programming language instead of C#, which enables lower level and more performant code in the hand of experienced developers. On the other hand, the developer can also make use of the 
popular Lua scripting language for faster iteration times and more flexible code structure.

Wicked Engine is provided as a static library. This means, that when creating a new project, the developer has to link against the compiled library before using its features. 
For this, you must first compile the engine library project for the desired platform. For Windows Desktop, this is the WickedEngine_Windows project. 
Then set the following dependencies to this library in Visual Studio this way in the implementing project (paths are as if your project is inside the engine root folder):
<img align="right" src="https://turanszkij.files.wordpress.com/2018/05/sphinit.gif"/>

1. Open Project Properties -> Configuration Properties
2. C/C++ -> General -> Additional Include Directories: 
	- ./WickedEngine
3. Linker -> General -> Additional Library Directories:
	- Directory of your built .lib file (For example ./x64/Release)
4. Also be sure to compile with a non-DLL runtime library for Release builds:
	- Project settings -> C/C++ -> Code Generation -> Runtime Library -> Multi threaded
	
When your project settings are set up, time to #include "WickedEngine.h" in your source. I recommend to include this
in the precompiled header file. This will enable the use of all the engine features and link the necessary binaries. After this, you should already be able to build your project. If you are having difficulties, there are some projects that  you can compare against, such as the Editor, Tests or Template projects.
Once the build is succesful, you can start using the engine. Here is some basic sample code, just to get an idea:

Initialization example (C++):
```
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
```
RenderPath3D_Deferred myGame; // Declare a game screen component, aka "RenderPath" (you could also override its Update(), Render() etc. functions). This is a 3D, Deferred path for example, but there are others
main.ActivatePath(&myGame); // Register your game to the application. It will call Start(), Update(), Render(), etc. from now on...

wiSceneSystem::LoadModel("myModel.wiscene"); // Simply load a model into the current global scene
wiSceneSystem::GetScene(); // Get the current global scene
wiRenderer::ClearWorld(); // Delete every model, etc. from the current global scene

myGame.setSSAOEnabled(true); // You can enable post process effects this way...

RenderPath2D myMenuScreen; // This is an other render path, but now a simple 2D one. It can only render 2D graphics by default (like a menu for example)
main.ActivatePath(&myMenuScreen); // activate the menu, the previous path (myGame) will be stopped

wiSprite mySprite("image.png"); // There are many utilities, such as a "sprite" helper class
myMenuScreen.addSprite(&mySprite); // The 2D render path is ready to handle sprite and font rendering for you

wiSoundEffect soundEffect("explosion.wav"); // you can load sound effects, or music
soundEffect.Play(); // you can play sounds
soundEffect.Stop(); // you can stop sounds

wiMusic myMusic("music.wav"); // music is the same as sound effects, but they can be controlled independently
myMusic.Play(); // plays a music
wiMusic::SetVolume(0.5f); // set volume for every music, but don't modify sound effect volume

if (wiInputManager::press(VK_SPACE)) { soundEffect.Stop(); } // You can check if a button is pressed or not (this only triggers once)
if (wiInputManager::down(VK_SPACE)) { soundEffect.Play(); } // You can check if a button is pushed down or not (this triggers repeatedly)
```

Some scripting examples (LUA): <br/>
(You can enter lua scripts into the backlog (HOME button), or the startup.lua script which is always executed on application startup if it is found near the app, or load a script via dofile("script.lua") command)
```
-- Set a rendering path for the application main component
path = RenderPath3D_Deferred;
main.SetActivePath(path);    -- "main" is created automatically

-- Load a model entity:
entity = LoadModel("myModel.wiscene");

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
sound = SoundEffect("whoosh.wav");
sound.Play();

-- Check for input:
if(input.press(VK_LEFT)) then
   sound.Play(); -- this will play the sound if you press the left arrow on the keyboard
end
```

For more code samples and advanced use cases, please see the example projects, like the Template_Windows, Tests, or Editor project.

If you want to create an UWP application, #define WINSTORE_SUPPORT preprocessor for the whole implementing project and link against the WickedEngine_UWP library.

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/hairparticle2.gif"/>

### Contents:

- ./Documentation/						- Documentation files
- ./logo/								- Logo artwork images
- ./models/								- Sample model files
- ./scripts/							- Sample LUA script files
- ./WickedEngine/						- Wicked Engine Library project
- ./Editor/								- Editor project
- ./Tests/								- Testing framework project
- ./Template_Windows					- Template project for Windows applications
- ./WickedEngine.sln 					- Visual Studio Solution

### Scripting API:

You can use a great number of engine features through the Lua scripting api, which can even be used real time while the program is running. The included applications, like the Editor,
contain a scripting input method toggled by the "Home" key. A blue screen will be presented where the user can type in LUA commands. It is very minimal in regards to input methods.
For further details, please check the scripting API documentation: [Wicked Engine Scripting API](Documentation/ScriptingAPI-Documentation.md)


### Model import/export:

The Editor supports the importing of some common model formats (the list will potentially grow): <b>OBJ</b>, <b>GLTF 2.0</b> <br/>
The Engine itself can open the serialized model format (<b>WISCENE</b>) only. The preferred workflow is to import models into the editor, and save them out to <b>WISCENE</b>, then any WickedEngine application can open them.<br/>
The old Blender exporter script is now not supported! (from version 0.21.0)

### Take a look at some screenshots:

Sponza scene with voxel GI enabled:
![Sponza](https://turanszkij.files.wordpress.com/2018/12/sponza.png)

Damaged Helmet GLTF sample model:
![Sponza](https://turanszkij.files.wordpress.com/2019/03/damagedhelmet.png)

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
![Bloom](https://turanszkij.files.wordpress.com/2019/01/bistro_out_0.png)

Bistro scene from the inside:
![Bloom](https://turanszkij.files.wordpress.com/2019/01/bistro_in_2.png)

Parallax occlusion mapping:
![Bloom](https://turanszkij.files.wordpress.com/2019/01/pom.png)

Large scale particle simulation on the GPU:
![Bloom](https://turanszkij.files.wordpress.com/2019/01/gpuparticles3.png)

Tiled light culling in the Bistro:
![Bloom](https://turanszkij.files.wordpress.com/2019/02/bistro_heatmap-1.png)
