# Building for linux

### External dependencies needed:
- cmake
- g++ compiler
- sdl2 lib and headers
- Vukan SDK >= 1.2.141
    - You can use Vulkan 1.1 but you need disable the ray tracing
- Directx Shader Compiler

### Ubuntu
Ubuntu specific instructions for dependencies.
They were written with Ubuntu 20.04 in mind, so your milage may vary. 
- `sudo apt install libsdl2-dev`
- go to https://vulkan.lunarg.com/sdk/home and follow the instructions for ubuntu.
You are encouraged to download the latest version (>= 1.2.141).

### Archlinux
- `sudo pacman -S sdl2 vulkan-devel`
- `your_aur_helper -S directx-shader-compiler-git`

## Installing the DirectX Shader Compiler
- `git clone https://github.com/microsoft/DirectXShaderCompiler.git --recursive`
- `cd DirectXShaderCompiler`
- `mkdir build && cd build`
- `cmake .. -DCMAKE_BUILD_TYPE=Release $(cat ../utils/cmake-predefined-config-params) -DCMAKE_INSTALL_LIBDIR=/opt/directx-shader-compiler/lib -DCMAKE_INSTALL_PREFIX=/opt/directx-shader-compiler`
- `make -j8`
- `sudo make install`

## Compiling WickedEngine
- `mkdir build && cd build`
- `cmake ..` (`-DCMAKE_BUILD_TYPE=Release` if you want to compile in release mode )
