cmake_minimum_required(VERSION 3.8)

set(WICKED_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(CMakeDependentOption)
option(WICKED_PIC_CODE "Build WickedEngine as portable code, to include in a dynamic library" OFF)
cmake_dependent_option(WICKED_DYNAMIC_LIBRARY "Build WickedEngine as a dynamic library" OFF "WICKED_PIC_CODE" OFF)
option(USE_LIBCXX "Link WickedEngine to llvm libc++ library - only available with the Clang compiler" OFF)

option(WICKED_EDITOR "Build WickedEngine editor" ON)
option(WICKED_TESTS "Build WickedEngine tests" ON)
option(WICKED_IMGUI_EXAMPLE "Build WickedEngine imgui example" ON)
option(WICKED_LINUX_TEMPLATE "Build WickedEngine Linux template" ON)
cmake_dependent_option(BLENDER_EXPORTER "Build blender wiscene exporter plugin" OFF "WICKED_PIC_CODE" OFF)

# Configure CMake global variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

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


    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdeclspec -fms-extensions -fmodules -fbuiltin-module-map")
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdeclspec -fms-extensions -stdlib=libc++ -fmodules -fbuiltin-module-map")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
	if (CMAKE_CXX_COMPILER_VERSION LESS 16 )
		message(FATAL_ERROR "Clang supported only from version 16")
	elseif (CMAKE_CXX_COMPILER_VERSION EQUAL 16)
		# Default to C++ extensions being off. Clang16's modules support have trouble
		# with extensions right now and it is not required for any other compiler
		set(CMAKE_CXX_EXTENSIONS OFF)
	endif()
	#if (CMAKE_CXX_COMPILER_VERSION LESS 18 )
	#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wread-modules-implicitly")
	#endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    endif()
endif()


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdeclspec -fms-extensions")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} $<$<NOT:$<WICKED_DYNAMIC_LIBRARY>>:--for-linker=-no-pie>" )
	if (USE_LIBCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
	endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
else()
endif()


add_subdirectory(WickedEngine)

if (WICKED_EDITOR)
	message("Building Wicked Editor")
    add_subdirectory(Editor)
endif()

if (WICKED_TESTS)
	message("Building Wicked Tests")
    add_subdirectory(Tests)
endif()

if (WICKED_IMGUI_EXAMPLE)
	message("Building IMGUI Examples")
    add_subdirectory(Example_ImGui)
    add_subdirectory(Example_ImGui_Docking)
endif()

if (WICKED_LINUX_TEMPLATE)
	message("Building Linux Template")
    add_subdirectory(Template_Linux)
endif()

if (BLENDER_EXPORTER)
	if (NOT ${WICKED_PIC_CODE})
		message(FATAL_ERROR "WICKED_PIC_CODE required to build blender exporter")
	endif()
	message("Building Blender Exporter")
    add_subdirectory(BlenderExporter)
endif()
