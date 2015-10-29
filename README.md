# Wicked Engine
## Created by Turánszki János

### Overview:

Wicked Engine is an open-source game engine written in C++ for Windows PC. For list of features, see features.txt.
Documentation is not yet available, but hopefully it will be someday.
Demos are available at: https://github.com/turanszkij/WickedEngineDemos
From now on you can easily set up a game project by using the component templates. (see the demos for examples)


### Requirements:

Visual Studio 2013+ (earlier versions may be compatible)
WindowsSDK
DirectX11 SDK (included in Windows SDK)(legacy SDK not supported)
DirectX11 June 2010 Redist (Only if you want to support Windows 7 because it uses Xaudio 2.7!)


### Hardware: 

DirectX 10+ Graphics card


### Usage: 

Set the following dependencies to this library in Visual Studio this way in the implementing project:

1.) Open Project Properties -> Configuration Properties
2.) C/C++ -> General -> Additional Include Directories: 
		./WickedEngine
		./WickedEngine/BULLET
3.) Linker -> General -> Additional Library Directories:
		Directory of your built .lib file (Debug or Release directory in the solution by default)
		./WickedEngine/mysql

Editor: Use Blender 2.72 as the editor of this engine. Set up your scene and export it with the provided script

Windows 7 support: define _WIN32_WINNT=0x0601 preprocessor on the whole project

Windows 8.1 Store support: define WINSTORE_SUPPORT preprocessor for the whole project (incomplete)


### Contents:

	./WickedEngine.sln 						- Visual Studio Solution;
	./WickedEngine/WickedEngine.vcxproj		- Visual Studio Project;
	./WickedEngine/wiBULLET/					- Bullet 2.82 Source files;
	./WickedEngine/mysql/					- MYSQL C++ libraries;
	./WickedEngine/shaders/					- Binary shaders output;
	./WickedEngine/ 						- C++ and shader source files;
	./io_export_wicked_wi_bin.py 			- Blender 2.72 script to export scene;


### TODOs:

	Priority:
		- Lua script bindings for most engine features
		- Optimize image rendering
			The Drawing fuctions are bloated, imageVS is no longer readable by humans, 
			possible batching optimizations
		- Decouple API from wiRenderer
			The renderer has become too large, unmanageable, hardly readable. Aside from that it
			is a good idea to decouple them for possible support of multiple apis in the future...
	Other:
		- Cleanup
			No comment
		- Documentation
			Probably not in the near future
		- HDR pipeline
			Already using floating point rendertargets, but everything is saturated (except bloom).
			It needs tone mapping and not much else
		- Forward rendering pipeline
			It should support everything that the deferred renderer supports. Now it has only a directional light
			without shadows, but the lighting fuctions are reusable and already written in separate headers.
		- Precalculated Ambient Occlusion
			It has everything it needs in the engine, just need to put it together.
		- Optimize Meshes
			Remove duplicate vertices and improve cache coherency
		- Port to Windows Phone and Win RT
		- Windows RT controls helper
		- Texture helper various texture generators (fractal, perlin, etc.) (low priority)


### Editor:

For the time being, Blender software is the editor program for Wicked Engine. I provide an export script which
you can use to export to the wicked scene format. The script is also going to be updated from time to time to
remove the known hiccups and bugs.
	- Names should not contain spaces inside Blender
		The problem is the c++ side code which parses the text files such as it breaks parsing on spaces. 
		Mesh files are already exported as binary, so those are okay
		Suggested fix: write binary export for everything
	- Separate files generated
		I've written the exporter to write different categories of the scene to different files for easier debugging
		from a text editor. If the exporter is rewritten to write binary for everything, such debugging will
		not be possible so might as well merge the files (except mesh files and error message file)
	- Only animation supported is skeletal animation
	- Animation Action names should contain their armature's name so that the exporter matches them correctly
		Suggested fix: find a better way of matching armatures and actions
	- Animation only with linear curves (so no curves)
		Suggested fix: implement curves support into the engine and the exporter
	- Only one uv map support
		Light maps and other effects requiring multiple uv maps are not possible yet.
