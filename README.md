<img align="left" src="Content/logo_small.png" width="180px"/>

# Wicked Engine

[![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)
[![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)
<a href="https://twitter.com/intent/follow?screen_name=turanszkij"><img src="https://img.shields.io/twitter/follow/turanszkij.svg?style=social" alt="follow on Twitter"></a>
<br/>
[![Steam](https://img.shields.io/badge/-Steam-383838.svg?style=for-the-badge&logo=steam)](https://store.steampowered.com/app/1967460/Wicked_Engine/)
[![Store](https://img.shields.io/badge/-Microsoft_Store-748FB2.svg?style=for-the-badge&logo=microsoft)](https://www.microsoft.com/store/productId/9PPMV065W9QJ)


<br/>
<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/npc.gif" width="320px"/>
Wicked Engine is an open-source C++ engine focusing on modern rendering techniques and performance. Use this as a framework for your graphics projects, or learning. Some programming skills are required for the best experience, but some simple tools like an Editor are also included. It is free to be used for anything good.<br/>
This project is hosted on <a href="https://github.com/turanszkij/WickedEngine/">GitHub</a>.

- [Documentation](Content/Documentation/WickedEngine-Documentation.md)<br/>
- [Scripting API Documentation](Content/Documentation/ScriptingAPI-Documentation.md)<br/>
- [Features](features.txt)<br/>
- [Devblog](https://wickedengine.net/)<br/>
- [Videos](https://www.youtube.com/playlist?list=PLLN-1FTGyLU_HJoC5zx6hJkB3D2XLiaxS)<br/>

You can download the source code by using Git and cloning the repository, or downloading it as zip. You can also choose to download a pre-built version of the Editor or Tests applications, which will allow you to try out features, load content and execute LUA scripts.<br/>
Tip: try loading models or scripts from the Content folder using the Editor app to see how everything works.
<br/>

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/swimming.gif" width="320px"/>

### Platforms:
- Windows 10 or newer
- Linux
- UWP

### How to build: 

#### Windows
To build Wicked Engine for Windows 10, use Visual Studio and the provided `WickedEngine.sln` solution file. There are a couple of projects that you can run up front: Editor, Tests and Template. You just have to set either as startup project and press F5 in Visual Studio to build and run. For optimal performance, choose `Release` mode, for the best debugging experience, choose `Debug` mode.

<img align="right" src="https://turanszkij.files.wordpress.com/2023/05/fighting_game.gif" width="320px"/>

If you want to develop an application that uses Wicked Engine, you can build the WickedEngine static library project for the appropriate platform, such as `WickedEngine_Windows` and link against it. Including the `"WickedEngine.h"` header will attempt to link the binaries for the appropriate platform, but search directories should be set up beforehand. For example, you can set additional library directories to `$(SolutionDir)BUILD\$(Platform)\$(Configuration)` by default. For examples, see the `Template`, `Tests`, and `Editor` projects. 

You can also download prebuilt and packaged versions of the Editor and Tests here (requires Github sign in): [![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)

If you have questions or stuck, please use the `windows` communication channel on Discord: [![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)

Note: Building 32-bit and ARM versions should be possible, but no longer provided by default. You will need to configure the solution for these platforms yourself if you want this. For ARM platform, also use the `-DBT_USE_DOUBLE_PRECISION=1` definition when compiling.

Cmake: It is possible to build the windows version with Cmake, but the recommended way is to use the provided WickedEngine.sln file with Visual Studio.

#### Linux
To build the engine for Linux, use Cmake. You can find a sample build script for Ubuntu 20.04 [here](.github/workflows/build.yml) (in the linux section). You might need to install some dependencies, such as Cmake (3.7 or newer), g++ compiler (C++ 17 compliant version) and SDL2. For Ubuntu 20.04, you can use the following commands to install dependencies:

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/grass.gif" width="320px"/>

```bash
sudo apt update
sudo apt install libsdl2-dev
sudo apt install build-essential
```
To build the engine, editor and tests, use `cmake` and then `make`:
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

If you want to develop an application that uses Wicked Engine, you will have to link to libWickedEngine.a and `#include "WickedEngine.h"` into the source code. For examples, look at the Cmake files, or the Tests and the Editor applications.

You can also download prebuilt and packaged versions of the Editor and Tests here (requires Github sign in): [![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)

If you have questions or stuck, please use the `linux` communication channel on Discord: [![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)


### Examples:

#### Initialization (C++):

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/inverse_kinematics.gif" width="320px"/>

```cpp
// Include engine headers:
#include "WickedEngine.h"

// Create the Wicked Engine application:
wi::Application application;

// Assign window that you will render to:
application.SetWindow(hWnd);

// Run the application:
while(true) {
   application.Run(); 
}
```

#### Basics (C++):
```cpp
wi::initializer::InitializeComponentsImmediate(); // (Optional) allows to initialize all components immediately and block the application until finished. Otherwise the initialization will take place at the first application.Run() asynchronously. This is useful if you want to start using other parts of the engine before application.Run() is called.

wi::RenderPath3D myGame; // Declare a game screen component, aka "RenderPath" (you could also override its Update(), Render() etc. functions). 
application.ActivatePath(&myGame); // Register your game to the application. It will call Start(), Update(), Render(), etc. from now on...

wi::scene::LoadModel("myModel.wiscene"); // Simply load a model into the current global scene
wi::scene::GetScene(); // Get the current global scene

wi::scene::Scene scene2; // create a separate scene
wi::scene::LoadModel(scene2, "myModel2.wiscene"); // Load model into a separate scene
wi::scene::GetScene().Merge(scene2); // Combine separate scene with global scene

myGame.setFXAAEnabled(true); // You can enable post process effects this way...

wi::RenderPath2D myMenuScreen; // This is an other render path, but now a simple 2D one. It can only render 2D graphics by default (like a menu for example)
application.ActivatePath(&myMenuScreen); // activate the menu, the previous path (myGame) will be stopped

wi::Sprite mySprite("image.png"); // There are many utilities, such as a "sprite" helper class
myMenuScreen.AddSprite(&mySprite); // The 2D render path is ready to handle sprite and font rendering for you

wi::audio::Sound mySound;
wi::audio::CreateSound("explosion.wav", &mySound); // Loads a sound file
wi::audio::SoundInstance mySoundInstance;
wi::audio::CreateSoundInstance(&mySound, &mySoundInstance); // Instances the sound file, it can be played now
wi::audio::Play(&mySoundInstance); // Play the sound instance
wi::audio::SetVolume(0.6, &mySoundInstance); // Set the volume of this soundinstance
wi::audio::SetVolume(0.2); // Set the master volume

if (wi::input::Press(wi::input::KEYBOARD_BUTTON_SPACE)) { wi::audio::Stop(&mySoundInstance); } // You can check if a button is pressed or not (this only triggers once)
if (wi::input::Down(wi::input::KEYBOARD_BUTTON_SPACE)) { wi::audio::Play(&mySoundInstance); } // You can check if a button is pushed down or not (this triggers repeatedly)
```

#### Scripting (LUA):
```lua
-- Set a rendering path for the application
path = RenderPath3D;
application.SetActivePath(path);    -- "application" is created automatically by wi::Application

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

-- Print any WickedEngine class information to the backlog:
getprops(application);	-- prints the application methods
getprops(scene);	-- prints the Scene class methods
getprops(path);	-- prints the deferred render path methods

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

For more code samples and advanced use cases, please see the example projects, like the Template_Windows, Tests, or Editor project. There are also sample models and scripts included with Wicked Engine in the Content/models and Content/scripts folders. Check them out to learn about more features.

### Scripting API:

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/character_lookat.gif" width="320px"/>

You can use a great number of engine features through the Lua scripting api, which can even be used real time while the program is running. The included applications, like the Editor,
contain a scripting input method toggled by the "Home" key. A blue screen will be presented where the user can type in LUA commands. It is very minimal in regards to input methods.
For further details, please check the scripting API documentation: [Wicked Engine Scripting API](Content/Documentation/ScriptingAPI-Documentation.md)


### Model import/export:
The native model format is the <b>WISCENE</b> format. Any application using Wicked Engine can open this format efficiently.

In addition, the Editor supports the importing of some common model formats (the list will potentially grow): 
- <b>OBJ</b>
- <b>GLTF 2.0</b>
- <b>VRM</b>

The preferred workflow is to import models into the Editor, and save them as <b>WISCENE</b>, then any Wicked Engine application can open them.<br/>

### Graphics API:
The default renderer is `DirectX 12` on Windows and `Vulkan` on Linux. The `DirectX 11` renderer is no longer available starting from version 0.57.0, but it can be found on the <a href="https://github.com/turanszkij/WickedEngine/tree/dx11-backup">dx11-backup branch</a>.
You can specify command line arguments (without any prefix) to switch between render devices or other settings. Currently the list of options:
<table>
  <tr>
	<th>Argument</th>
	<th>Description</th>
  </tr>
  <tr>
	<td>dx12</td>
	<td>Use DirectX 12 rendering device</td>
  </tr>
  <tr>
	<td>vulkan</td>
	<td>Use Vulkan rendering device</td>
  </tr>
  <tr>
	<td>debugdevice</td>
	<td>Use debug layer for graphics API validation. Performance will be degraded, but graphics warnings and errors will be written to "Output" window</td>
  </tr>
  <tr>
	<td>gpuvalidation</td>
	<td>Use GPU Based Validation for graphics. This must be used together with the debugdevice argument. Currently DX12 only.</td>
  </tr>
  <tr>
	<td>gpu_verbose</td>
	<td>Enable verbose GPU validation mode.</td>
  </tr>
  <tr>
	<td>igpu</td>
	<td>Prefer integrated GPU selection for graphics. By default, dedicated GPU selection will be preferred.</td>
  </tr>
  <tr>
	<td>alwaysactive</td>
	<td>The application will not be paused when the window is in the background.</td>
  </tr>
</table>


<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/talking.gif" width="320px"/>
<br/>

### Other software using Wicked Engine
- <a href="https://www.game-guru.com/max">Game Guru MAX</a>: Easy to use game creator software
- <a href="https://www.youtube.com/watch?v=0SxXmnSQ6Q4">Flytrap</a>: Demoscene production by qop
- Your project: add your project to this readme and open a pull request

<br/>

### Troubleshooting
If you are having trouble getting the applications to run, make sure that you satisfy the following conditions:
- If you built the application with Visual Studio, run it from the Visual Studio environment, where the executable working directory is set up to be the Project directory (not the build directory where the exe will be found)
- If you want to run an application without Visual Studio, either copy the executable from the BUILD directory to the correct project directory, or set the working directory appropriately. You can also check the Working directory setting in Visual Studio to find out the right working directory of every project. 
- If you want to build UWP application, then you will first need to build the shaders into a shader dump. For that, build and run the `offlineshadercompiler` project with the `hlsl6 shaderdump` command line arguments. If the `wiShaderDump.h` file is successfully generated, rebuilding the engine will embed all the shader files so they are not loaded separately. But embedded shaders also cannot be recompiled during runtime.
- If you experience crashes, follow these steps to find out the problem:
	- make sure your environment is up to date, with latest graphics drivers and operating system updates.
	- see if there is a wiBackLog.txt in your user temp folder (for example: C:\Users\username\AppData\Local\Temp), and request help on Discord or Github issue
	- build the engine in Debug mode and try to run it, see where it crashes, provide call stack on Discord or Github issue
	- run the engine with the `debugdevice` command argument and post the text from your console output window when the crash happens
