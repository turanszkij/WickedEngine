cmake_minimum_required(VERSION 3.8)

# Configure CMake global variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(WickedEngine)

if (WIN32)
    set(PLATFORM "Windows")
    add_compile_definitions(WIN32=1)
    # add_compile_definitions(_WIN32=1) this is a given from the compiler
    set(DXC_TARGET "${CMAKE_CURRENT_SOURCE_DIR}/WickedEngine/dxc.exe")
elseif(UNIX)
    set(PLATFORM "SDL2")
    add_compile_definitions(SDL2=1)
    set(DXC_TARGET "dxc")
endif()

add_subdirectory(WickedEngine)

option(WICKED_EDITOR "Build WickedEngine editor" ON)
if (WICKED_EDITOR)
    add_subdirectory(Editor)
endif()

option(WICKED_TESTS "Build WickedEngine tests" ON)
if (WICKED_TESTS)
    add_subdirectory(Tests)
endif()

option(WICKED_IMGUI_EXAMPLE "Build WickedEngine imgui example" ON)
if (WICKED_TESTS)
    add_subdirectory(Example_ImGui)
endif()
