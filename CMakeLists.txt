cmake_minimum_required(VERSION 3.7)

project(WickedEngine)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    set(PLATFORM "Windows")
    add_compile_definitions(WIN32=1)
    # add_compile_definitions(_WIN32=1) this is a given from the compiler
elseif(UNIX)
    find_package(SDL2 REQUIRED)
    set(PLATFORM "SDL2")
    add_compile_definitions(SDL2=1)
endif()

add_subdirectory(WickedEngine)
add_subdirectory(Tests)
add_subdirectory(Editor)
