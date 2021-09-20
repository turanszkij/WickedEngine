# C++ / COM Interface for FAudio
Use FAudio as a substitute for XAudio2 without changing or recompiling the host application.
Works on:
- Windows
- Wine on Linux as a native (= Windows binary) DLL

## Building

### Building Windows DLLs with Visual Studio
A Visual Studio 2010 solution is included in the `cpp/visualc` directory.

Newer Visual Studio versions should be able to upgrade the 2010 solution (e.g. Visual Studio 2015: right-click the solution and choose `Retarget solution`). If there are any issues check if the Platform Toolset is set to something reasonable in the general properties of the project.

The resulting DLLs can also be used as native DLLs with Wine.

### Building Windows DLLs with MSYS / mingw-w64 on Windows
The included `Makefile` has been tested to work with msys and mingw-w64 for both 32-bit and 64-bit builds.

### Building native Wine DLLs with mingw-w64 on Linux
Because cross-compiling is fun.

#### Prerequisites
- mingw-w64 has to be present on your system (the C and C++ compiler)
- install the SDL2 development libraries for mingw (download: [here](http://libsdl.org/download-2.0.php))

#### Building
Use your cross compilation cmake toolchain to configure the build. Use `i686-w64-mingw32-cmake` to build 32-bit and `x86_64-w64-mingw32-cmake` to build 64-bit libraries.
```
x86_64-w64-mingw32-cmake -H. -B_build_mingw64 -DCMAKE_INSTALL_PREFIX="${PWD}/_faudio_mingw64" -DBUILD_CPP=ON -DINSTALL_MINGW_DEPENDENCIES=ON
i686-w64-mingw32-cmake -H. -B_build_mingw32 -DCMAKE_INSTALL_PREFIX="${PWD}/_faudio_mingw32" -DBUILD_CPP=ON -DINSTALL_MINGW_DEPENDENCIES=ON
```
- `-DBUILD_CPP=ON` enables compilation of the C++/COM wrapper
- `-DINSTALL_MINGW_DEPENDENCIES` adds runtime dynamic libraries like `SDL2.dll` and `winpthread-1.dll` to the install target

Optional:

- See README.gstreamer for more information about WMA support.

After the configuration is done the following command(s) starts the cross-compilation of both FAudio and C++/COM wrapper:
```
cmake --build _build_mingw64 --target install -- -j
cmake --build _build_mingw32 --target install -- -j
```

- `-- -j` passes the `-j` flag to the `make` command to speed up compilation
- the results are installed to the `_install_mingw64/32` subdirectories


## Using the wrapper

### Windows - XAudio version 2.8+
COM support was dropped in XAudio 2.8, it's still using the same interfaces but they aren't registered any more. Applications link directly with an import library and use a plain function to create an IXAudio2 instance.

Overriding the system XAudio2 DLLs is just a matter of taking advantage of the search path used by Windows when loading DLLs. Place `XAudio2_(8|9).dll` and its dependencies (FAudio.dll and SDL2.dll) in the same directory as the executable you want to test with FAudio. (This will only work if the application does not load the library with an absolute path.)

Doing this also enables the XAudio2FX and X3DAudio wrappers because they are included in the main XAudio2 DLL starting from version 2.8.

### Windows - XAudio version 2.7 or earlier
To use XAudio 2.7 or earlier we'll have to hijack to COM registration in the Windows registry. COM information is stored under `HKEY_CLASSES_ROOT` but in modern (Vista+?) Windows versions this is actually a merged view of `HKEY_LOCAL_MACHINE/Software/Classes` (for system defaults) and `HKEY_CURRENT_USER/Software/Classes` for user specific configuration. We use this to our advantage to register the FAudio wrapper DLLs for the current user. The standard Windows tool regsvr32 can be used to register or unregister the DLLs. Just make sure to specify an absolute path to the wrapper DLLs to prevent regsvr32 from using the standard Windows versions.

The `cpp/scripts` directory has Windows cmd-scripts to (un)register all the DLLs in one step. Run `windows_register.bat` from the directory containing the wrapper DLLs to register and `windows_unregister.bat` to remove the overrides.

X3DAudio is not a COM-interface, even in these earlier versions of XAudio2, use the version 2.8+ instructions.

### Wine - native DLLs
A Wine prefix is a directory that contains a Wine configuration, registry, and native Windows DLLs for compatibility. It's trivial to replace the XAudio2 DLLs in a prefix with the FAudio wrapper DLLs and still use another prefix with the original DLLs.

The `wine_setup_native` script (in the `cpp/scripts` subdirectory) does just this. Run it from a directory containing the wrapper DLLs (32 or 64 bit) and it will create symbolic links in the Wine prefix and modify the Wine registry to make sure Wine only uses the native DLLs.

### How can I check if the wrapper DLLs are actually being used?
- Build the wrapper DLLs with tracing enabled (at the top of `xaudio2.cpp`)
    - check to see if log entries are added when running the application
- Remove a dependency (e.g. `FAudio.dll`), the application will fail to load the wrapper and hopefully show a nice error message.
    - Some applications (e.g. games using Wwise) will just try to use a different version of XAudio2 :-)
- Set `WINEDEBUG=+loaddll` to check which DLLs are loaded by the application
- On Windows, use [Sysinternals Process Monitor](https://docs.microsoft.com/en-us/sysinternals/downloads/procmon) to check which DLLs are loaded by the application

## Redistributing the DLLs
The COM wrapper DLLs depend on both FAudio.dll and SDL2.dll. During the build process they are copied to the output directory. Don't forget to include these DLLs when you want to use the binaries from another directory or on another computer.

# Known Issues
1. The application hangs when using the wrapper DLLs
    - An assertion might have been triggered with the message box hidden behind the fullscreen application
    - Alt-Tab until you can access the message box and dismiss it
2. The sound is crackling when using the native DLLs with Wine
    - Are you using PulseAudio? Try setting the `PULSE_LATENCY_MSEC` environment variable to 60 to fix this.
3. Wine games are silent after installing the native DLLs
    - Some MinGW setups will silently link to libwinpthread-1.dll, resulting in failure to load the DLLs if this is not present. Usually this is quickly solved with a line like `ln -sf /usr/i686-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll $WINEPREFIX/drive_c/windows/system32/libwinpthread-1.dll`
4. Wine audio may sound choppy with the native DLLs
    - Set `SDL_AUDIODRIVER=directsound`. For Proton, this is `SDL_AUDIODRIVER=directsound %command%` in the Steam Launch Options.
