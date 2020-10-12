<img align="left" src="images/logo_small.png" width="128px"/>

# Wicked Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-orange.svg)](https://opensource.org/licenses/MIT)
[![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)
<a href="https://twitter.com/intent/follow?screen_name=turanszkij"><img src="https://img.shields.io/twitter/follow/turanszkij.svg?style=social" alt="follow on Twitter"></a><br/>
[![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/3dbcee5gd6i7qh7v?svg=true)](https://ci.appveyor.com/project/turanszkij/wickedengine)
[![Store](https://img.shields.io/badge/-Microsoft_Store-9cf.svg)](https://www.microsoft.com/store/productId/9PPMV065W9QJ)


<br/>
<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/gltfanim.gif"/>
Wicked Engine is an open-source game engine written in C++. It is easy to use, high performance and feature rich. There are no external dependencies, but some free libraries are included as part of the source code. The MIT license means that anyone is free to download, modify, and share it. This project is hosted on <a href="https://github.com/turanszkij/WickedEngine/">GitHub</a>.

- [Documentation](Documentation/WickedEngine-Documentation.md)<br/>
- [Scripting API Documentation](Documentation/ScriptingAPI-Documentation.md)<br/>
- [Features](features.txt)<br/>
- [Devblog](https://turanszkij.wordpress.com/)<br/>
- [Videos](https://www.youtube.com/playlist?list=PLLN-1FTGyLU_HJoC5zx6hJkB3D2XLiaxS)<br/>

You can download the engine by using Git and cloning the repository, or downloading it as zip. This will give you the full C++ source code that you must build for yourself. Building is simply pressing F5 in Visual Studio. You can also choose to download a pre-built version of the Editor or Tests applications, which will allow you to load content and write LUA scripts. <br/>

![Emitter](https://turanszkij.files.wordpress.com/2019/02/emitterskinned2.gif) ![Drone](https://turanszkij.files.wordpress.com/2018/11/drone_anim.gif) <br/>

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/physics.gif"/>

### Platforms:
- Windows 10 Desktop (x86, x64) [Visual Studio 2019]
- UWP (x86, x64, ARM, Phone, XBOX One) [Visual Studio 2019]
- Linux [CMake 3.7]

### How to build: 

#### Windows
To build Wicked Engine for Windows 10, use Visual Studio and the provided solution file. <b>Make sure that you have the latest Windows SDK and updated operating system</b>. There are a couple of projects that you can run up front: Editor, Tests and Template. You just have to set either as startup project and press F5 in Visual Studio to build and run. For optimal performance, choose `Release` mode, for the best debugging experience, choose `Debug` mode.

<img align="right" src="https://turanszkij.files.wordpress.com/2020/08/fighting_game_small-1.gif"/>

If you want to develop an application that uses Wicked Engine, you will have to link to WickedEngine_Windows.lib or WickedEngine_UWP.lib.lib and `#include "WickedEngine.h"` into the source code. For examples, look at the Template, Editor and Tests projects in Visual Studio that do this.

You can also dowload prebuilt and packaged versions of the Editor and Tests here: [![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)

If you have questions or stuck, please use the `windows` communication channel on Discord: [![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)


#### Linux
The Linux support is experimental. You can find a sample build script for Ubuntu 20.04 [here](.github/workflows/build.yml) (in the linux section). You might need to install some dependencies, such as Vulkan SDK 1.2 or greater, SDL2, cmake 3.7 and g++ compiler (C++ 17 compliant version). For Ubuntu 20.04, you can use the following commands to install dependencies:
```bash
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.148-focal.list https://packages.lunarg.com/vulkan/1.2.148/lunarg-vulkan-1.2.148-focal.list
sudo apt update
sudo apt install vulkan-sdk
sudo apt install libsdl2-dev
sudo apt install build-essential
```
 - Note: The Vulkan SDK for Ubuntu contains DXC (<a href="https://github.com/microsoft/DirectXShaderCompiler">DirectXShaderCompiler</a>) which is required to build the shaders. If you are using an other Linux distribution, make sure that you have DirectXShaderCompiler.
To build the engine, editor and tests, use Cmake and make:
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

If you want to develop an application that uses Wicked Engine, you will have to link to libWickedEngine.a and `#include "WickedEngine.h"` into the source code. For examples, look at the Cmake files.

You can also dowload prebuilt and packaged versions of the Editor and Tests here: [![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)

If you have questions or stuck, please use the `linux` communication channel on Discord: [![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)


### Examples:

#### Initialization (C++):

<img align="right" src="https://turanszkij.files.wordpress.com/2018/05/sphinit.gif"/>

```cpp
// Include engine headers:
#include "WickedEngine.h"

// Declare main component once per application:
MainComponent main;

// Set the window to render to like this:
main.SetWindow(hWnd);

// Run the application:
while(true) {
   main.Run(); 
}
```

#### Basics (C++):
```cpp
RenderPath3D myGame; // Declare a game screen component, aka "RenderPath" (you could also override its Update(), Render() etc. functions). 
main.ActivatePath(&myGame); // Register your game to the application. It will call Start(), Update(), Render(), etc. from now on...

wiScene::LoadModel("myModel.wiscene"); // Simply load a model into the current global scene
wiScene::GetScene(); // Get the current global scene
wiRenderer::ClearWorld(); // Delete every model, etc. from the current global scene

wiScene::Scene scene2; // create a separate scene
wiScene::LoadModel(scene2, "myModel2.wiscene"); // Load model into a separate scene
wiScene::GetScene().Merge(scene2); // Combine separate scene with global scene

myGame.setFXAAEnabled(true); // You can enable post process effects this way...

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

if (wiInput::Press(wiInput::KEYBOARD_BUTTON_SPACE)) { wiAudio::Stop(&mySoundInstance); } // You can check if a button is pressed or not (this only triggers once)
if (wiInput::Down(wiInput::KEYBOARD_BUTTON_SPACE)) { wiAudio::Play(&mySoundInstance); } // You can check if a button is pushed down or not (this triggers repeatedly)
```

#### Scripting (LUA):
```lua
-- Set a rendering path for the application main component
path = RenderPath3D;
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
if(input.Press(KEYBOARD_BUTTON_LEFT)) then
   audio.Play(soundinstance); -- this will play the sound if you press the left arrow on the keyboard
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
<i>(The old Blender exporter script is now not supported! (from version 0.21.0), because the engine was redesigned with Entity-Component System at its core. The old object oriented version can be found <a href ="https://github.com/turanszkij/WickedEngine/tree/old-system-backup">[here]</a>.)</i>

### Graphics API:

The default renderer is DirectX 11 on Windows and Vulkan on Linux. There is also an optional DirectX12 renderer for Windows.
You can specify command line arguments (without any prefix) to switch between render devices or other settings. Currently the list of options:
<table>
  <tr>
    <th>Argument</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>dx12</td>
    <td>Use DirectX 12 rendering device*</td>
  </tr>
  <tr>
    <td>vulkan</td>
    <td>Use Vulkan rendering device**</td>
  </tr>
  <tr>
    <td>debugdevice</td>
    <td>Use debug layer for graphics API validation. Performance will be degraded, but graphics warnings and errors will be written to "Output" window</td>
  </tr>
</table>

<img align="right" src="https://turanszkij.files.wordpress.com/2018/11/soft.gif"/>

* *DX12 will try to load shaders from WickedEngine/shaders/hlsl6 directory. 
HLSL6 shaders can be compiled by Rebuilding the Shaders_HLSL6 project from within Visual Studio (Python 3 required for building).

* **Vulkan support will be built into the application if the Vulkan SDK is installed on the build machine. 
Vulkan will try to load shaders from WickedEngine/shaders/spirv directory. 
SPIRV shaders can be compiled by Rebuilding the Shaders_SPIRV project from within Visual Studio (Python 3 required for building).
For Linux builds, spirv shaders will be built by using Cmake system.

<br/>

### Contributions

Contributions can be submitted on Github by following the steps below:
1) Open an issue and describe the feature or patch in detail
2) The feature or patch will be discussed in the issue, and determined if it would benefit the project
3) After the request is accepted, open a pull request and reference the issue with #issue_number
4) Code review will be conducted and the pull request will be merged when it meets the requirements
5) When the pull request passes, you will be added to the credits as a contributor and your name shall be remembered by future generations

<br/>

### Other software using Wicked Engine

- <a href="https://www.game-guru.com/max">Game Guru MAX</a>: Easy to use game creator software
- <a href="https://www.youtube.com/watch?v=0SxXmnSQ6Q4">Flytrap</a>: Demoscene production by qop
- Your project: add your project to this readme and open a pull request

<br/>

### Finally, take a look at some screenshots:

Sponza scene with voxel GI enabled:
![Sponza](https://turanszkij.files.wordpress.com/2020/08/vxgi_sponza_small.png)

Damaged Helmet sample model imported from GLTF:
![Sponza](https://turanszkij.files.wordpress.com/2019/03/damagedhelmet.png)

Bokeh Depth of Field (Lain model by <a href="https://sketchfab.com/3d-models/lain-20-bf255be16da34df08d48abb5443a6706">woopoodle at Sketchfab</a>):
![DepthOfField](https://turanszkij.files.wordpress.com/2020/08/dof_bokeh_small.png)

Motion blur (fighting game sample):
![MotionBlur](https://turanszkij.files.wordpress.com/2019/12/motionblur.png)

Stochastic Screen Space Reflections:
![ScreenSpaceReflections](https://turanszkij.files.wordpress.com/2020/08/ssr.png)

Real time ray traced shadows and ambient occlusion (DXR, VK_KHR_raytracing):
![RaytracedShadows](https://turanszkij.files.wordpress.com/2020/08/dxr_rtao_rtshadow_small.png)

Bloom:
![Bloom](https://turanszkij.files.wordpress.com/2020/08/bloom_new.png)

Path tracing in the living room (model from <a href="http://casual-effects.com/data/index.html">Morgan McGuire's graphics archive</a>):
![LivingRoom](https://turanszkij.files.wordpress.com/2019/09/livingroom.jpg)

City scene with a light map (model by <a href="https://www.cgtrader.com/michaelmilesgallie">Michael Gallie at CgTrader</a>):
![City](https://turanszkij.files.wordpress.com/2019/01/city1.png)

Path tracing in the city:
![Balcony](https://turanszkij.files.wordpress.com/2019/01/city2.png)

Path traced caustics:
![Caustics](https://turanszkij.files.wordpress.com/2019/01/trace.png)

Vegetation particle system and depth of field:
![Vegetation](https://turanszkij.files.wordpress.com/2020/08/grass.png)

Bistro scene from Amazon Lumberyard (model from <a href="http://casual-effects.com/data/index.html">Morgan McGuire's graphics archive</a>):
![Bistro_out](https://turanszkij.files.wordpress.com/2019/01/bistro_out_0.png)

Bistro scene from the inside:
![Bistro_in](https://turanszkij.files.wordpress.com/2019/01/bistro_in_2.png)

Parallax occlusion mapping:
![ParallxOcclusionMapping](https://turanszkij.files.wordpress.com/2019/01/pom.png)

Large scale particle simulation on the GPU:
![ParticleSimulation](https://turanszkij.files.wordpress.com/2020/08/particles_2.png)

Tiled light culling in the Bistro:
![TiledLightCulling](https://turanszkij.files.wordpress.com/2019/02/bistro_heatmap-1.png)

GPU-based BVH builder:
![GPU_BVH](https://turanszkij.files.wordpress.com/2019/07/bvh_livingroom.png)
