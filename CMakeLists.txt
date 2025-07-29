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
if (CMAKE_HOST_WIN32)
	set(symlink_default OFF)
else()
	set(symlink_default ON)
endif()
option(WICKED_USE_SYMLINKS "Prefer symlinking over copying directories" ${symlink_default})


if(WICKED_USE_SYMLINKS)
    # check for symlink support (on windows it requires admin or developer mode)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/symlink-test
        ERROR_QUIET
        RESULT_VARIABLE symlink_check_result
    )
    if(symlink_check_result EQUAL 0)
        file(REMOVE ${CMAKE_BINARY_DIR}/symlink-test)
        set(COPY_OR_SYMLINK_DIR_CMD create_symlink)
    else()
        message(FATAL_ERROR "Symlinks are not supported. Either disable them using -DWICKED_USE_SYMLINKS=OFF or enable Windows' developer mode")
    endif()
else()
    if(CMAKE_VERSION VERSION_LESS "3.26.0")
        set(COPY_OR_SYMLINK_DIR_CMD copy_directory)
    else()
        set(COPY_OR_SYMLINK_DIR_CMD copy_directory_if_different)
    endif()
endif()

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
	
	# Recommendation for Visual Studio Level 3 warning match:
	add_compile_options(
		-Wall
		-Wextra
	 
	 	-Wno-unused-parameter
	)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -fdeclspec
        -fms-extensions

        #-Wuninitialized
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
        #-Wuninitialized
        #-Wmaybe-uninitialized
        #-Wwrite-strings
        #-Winit-self
        #-Wreturn-type
        #$<$<COMPILE_LANGUAGE:CXX>:-Wreorder>
        #$<$<COMPILE_LANGUAGE:CXX>:-Werror=delete-non-virtual-dtor>
        #-Werror

        # some GCC versions have bugs triggering this warning
        #-Wno-stringop-overflow
        # this is just silly
        #-Wno-format-zero-length

        #uncomment this to stop the compilation at the first error
        # -Wfatal-errors
    )
endif()


add_subdirectory(WickedEngine)
add_custom_target(Content
   COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD} ${WICKED_ROOT_DIR}/Content ${CMAKE_CURRENT_BINARY_DIR}/Content
   COMMENT "$<IF:$<BOOL:${WICKED_USE_SYMLINKS}>,Symlinking,Copying> Content directory"
   VERBATIM
)

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
