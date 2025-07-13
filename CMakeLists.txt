cmake_minimum_required(VERSION 3.19)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR
        "In-source builds are not supported!\n"
        "Run `git clean -d -f` to clean up the files CMake has created (stash "
        "your changes first, if you have made any), then run `cmake -B build "
        "<other_options>` followed by `cmake --build build --parallel`"
    )
endif()

set(WICKED_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

option(WICKED_DYNAMIC_LIBRARY "Build WickedEngine as a dynamic library" OFF)
option(WICKED_PIC "Build WickedEngine as position-independent code" WICKED_DYNAMIC_LIBRARY)
option(USE_LIBCXX "Link WickedEngine to llvm libc++ library - only available with the Clang compiler" OFF)

option(WICKED_EDITOR "Build WickedEngine editor" ON)
option(WICKED_TESTS "Build WickedEngine tests" ON)
option(WICKED_IMGUI_EXAMPLE "Build WickedEngine imgui example" ON)

include(CMakeDependentOption)

if(UNIX)
    option(WICKED_LINUX_TEMPLATE "Build WickedEngine Linux template" ON)
elseif(WIN32)
    option(WICKED_WINDOWS_TEMPLATE "Build WickedEngine Windows template" ON)
endif()

# Configure CMake global variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE WICKED_PIC)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(WickedEngine)

if (WIN32)
    set(PLATFORM "Windows")
    add_compile_definitions(WIN32=1)
    # add_compile_definitions(_WIN32=1) this is a given from the compiler
elseif(UNIX)
    set(PLATFORM "SDL2")
    add_compile_definitions(SDL2=1)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -fdeclspec
        -fms-extensions

        -Wuninitialized
        #-Wwrite-strings
        #-Winit-self
        #-Wreturn-type
        #-Wreorder
        #-Werror=delete-non-virtual-dtor
        #-Werror
        #uncomment this to stop the compilation at the first error
        # -Wfatal-errors

        # turn off some warnings we don't want
        -Wno-nullability-completeness   # _Nonnull etc. are only supported on Clang
    )
    if (USE_LIBCXX)
        add_compile_options(
            $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>
        )
        add_link_options(
            -stdlib=libc++
        )
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# add some warnings and set them as errors
# read more details here: https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
    add_compile_options(
        -Wuninitialized
        -Wmaybe-uninitialized
        -Wwrite-strings
        -Winit-self
        -Wreturn-type
        $<$<COMPILE_LANGUAGE:CXX>:-Wreorder>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=delete-non-virtual-dtor>
        -Werror

		# some GCC versions have bugs triggering this warning
	    -Wno-stringop-overflow
		#uncomment this to stop the compilation at the first error
        # -Wfatal-errors
    )
endif()


add_subdirectory(WickedEngine)
add_subdirectory(Content)

if (WICKED_EDITOR)
    add_subdirectory(Editor)
endif()

if (WICKED_TESTS)
    add_subdirectory(Samples/Tests)
endif()

if (WICKED_IMGUI_EXAMPLE)
    add_subdirectory(Samples/Example_ImGui)
    add_subdirectory(Samples/Example_ImGui_Docking)
endif()

if (WICKED_LINUX_TEMPLATE)
    add_subdirectory(Samples/Template_Linux)
endif()

if (WICKED_WINDOWS_TEMPLATE)
    add_subdirectory(Samples/Template_Windows)
endif()
