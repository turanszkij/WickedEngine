##################################
## Wicked Engine		##
## Created by Turánszki János	##
##################################

________________
Requirements:   |
_____________________________________________________________________

Visual Studio 2012+
WindowsSDK
DirectX11 SDK (included in Windows SDK)(legacy SDK not supported)
_____________________________________________________________________

___________
Hardware:  |
__________________________________

DirectX 10+ Graphics card
__________________________________

________
Usage:  |
_______________________________________________________________________________________________________________

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

Windows 8.1 Store support: define WINSTORE_SUPPORT preprocessor for the whole project
_______________________________________________________________________________________________________________

__________
Contents: |
________________________________________________________________________________________________

	./WickedEngine.sln 						- Visual Studio Solution;
	./WickedEngine/WickedEngine.vcxproj		- Visual Studio Project;
	./WickedEngine/BULLET/					- Bullet 2.82 Source files;
	./WickedEngine/mysql/					- MYSQL C++ libraries;
	./WickedEngine/shaders/					- Binary shaders output;
	./WickedEngine/ 						- C++ and shader source files;
	./io_export_wicked_wi_bin.py 			- Blender 2.72 script to export scene;
________________________________________________________________________________________________


__________
Overview: |
________________________________________________________________________________________________________________________

Wicked Engine is an open-source game engine written in C++ for Windows PC. For list of features, see features.txt.
Documentation is not yet available, but hopefully it will be someday.
________________________________________________________________________________________________________________________


_______
TODOs: |
_________________________________________________________

	Priority:
		- FIX XINPUT
		- Optimize image rendering
		- Rewrite camera class
		- Decouple API from wiRenderer
	Other:
		- Cleanup
		- Documentation
		- HDR pipeline
		- Forward rendering pipeline
		- Precalculated Ambient Occlusion
		- Windows RT controls helper
		- Texture helper various texture generators (fractal, perlin, etc.) (low priority)
__________________________________________________________
