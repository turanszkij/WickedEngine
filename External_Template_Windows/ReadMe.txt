This is a template project showing the minumum setup required to set up an application using Wicked Engine with CMake.

For CMake to be able to find WickedEngine, either specifiy WickedEngine_ROOT as CMake definition or as environment variable pointing to the root directory of the WickedEngine git clone.

IMPORTANT: WickedEngine must have been built with CMake for FinsWickedEngine to be able to find the binaries.

Contents of interest:
	- stdafx.h
		This is the precompiled header file, WickedEngine.h has been included here
	- main.cpp
		This is the windows application entry point. The MainComponent has been created here, this is the engine runtime component.
		main.SetWindow() and main.Run() functions are called here which are necessary to setting up an application.

