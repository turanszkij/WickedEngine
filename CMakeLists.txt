cmake_minimum_required(VERSION 3.19)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR
        "In-source builds are not supported!\n"
        "Run `git clean -d -f` to clean up the files CMake has created (stash "
        "your changes first, if you have made any), then run `cmake -B build "
        "<other_options>` followed by `cmake --build build --parallel`"
    )
endif()

project(WickedEngine)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)

set(WICKED_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

option(WICKED_DYNAMIC_LIBRARY "Build WickedEngine as a dynamic library" OFF)
option(WICKED_PIC "Build WickedEngine as position-independent code" WICKED_DYNAMIC_LIBRARY)
option(USE_LIBCXX "Link WickedEngine to llvm libc++ library - only available with the Clang compiler" OFF)
option(WICKED_EDITOR "Build WickedEngine editor" ON)
option(WICKED_TESTS "Build WickedEngine tests" ON)
option(WICKED_IMGUI_EXAMPLE "Build WickedEngine imgui example" ON)
option(WICKED_ENABLE_IPO "Enable IPO/LTO in non-debug builds" ${ipo_supported})
option(WICKED_ENABLE_ASAN "Enable Address Sanitizer in debug builds" OFF)
option(WICKED_ENABLE_UBSAN "Enable Undefined Behaviour Sanitizer in debug builds" OFF)
option(WICKED_ENABLE_TSAN "Enable Thread Sanitizer in debug builds" OFF)
option(WICKED_ENABLE_LSAN "Enable Leak Sanitizer in debug builds" OFF)
option(WICKED_ENABLE_SAN_RELEASE "Also enable Sanitizers in release builds" OFF)

if (CMAKE_HOST_WIN32)
    set(symlink_default OFF)
else()
    set(symlink_default ON)
endif()
option(WICKED_ENABLE_SYMLINKS "Prefer symlinking over copying directories" ${symlink_default})

# check that IPO is supported when turned on
if(WICKED_ENABLE_IPO)
    check_ipo_supported()
endif()

if(WICKED_ENABLE_SYMLINKS)
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
        message(FATAL_ERROR "Symlinks are not supported. Either disable them using -DWICKED_ENABLE_SYMLINKS=OFF or enable Windows' developer mode")
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

if(WICKED_ENABLE_ASAN AND WICKED_ENABLE_TSAN)
	message(FATAL_ERROR "WICKED_ENABLE_ASAN and WICKED_ENABLE_TSAN are mutually exclusive!")
endif()

if(WICKED_ENABLE_LSAN AND WICKED_ENABLE_TSAN)
	message(FATAL_ERROR "WICKED_ENABLE_LSAN and WICKED_ENABLE_TSAN are mutually exclusive!")
endif()

if(WICKED_ENABLE_ASAN)
	if(MSVC)
		set(SANITIZE_OPTIONS "/fsanitize=address")
	else()
		set(SANITIZE_OPTIONS "-fsanitize=address")
	endif()
endif()

if(WICKED_ENABLE_TSAN)
	if(MSVC)
		message(FATAL_ERROR "MSVC does not support TSAN, use clang or gcc")
	else()
		set(SANITIZE_OPTIONS "${SANITIZE_OPTIONS} -fsanitize=thread")
	endif()
endif()

if(WICKED_ENABLE_UBSAN)
	if (MSVC)
		message(FATAL_ERROR "MSVC does not support UBSAN, use clang or gcc")
	else()
		set(SANITIZE_OPTIONS "${SANITIZE_OPTIONS} -fsanitize=undefined")
	endif()
endif()

if(WICKED_ENABLE_LSAN)
	if (MSVC)
		message(FATAL_ERROR "MSVC does not support LSAN, use clang or gcc")
	else()
		set(SANITIZE_LINK_OPTIONS "-fsanitize=leak")
	endif()
endif()

if(WICKED_ALWAYS_USE_SAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_OPTIONS}")
	add_link_options(${SANITIZE_LINK_OPTIONS)
else()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZE_OPTIONS}")
	add_link_options($<$<CONFIG:Debug>:${SANITIZE_LINK_OPTIONS}>)
endif()

if (WIN32)
    set(PLATFORM "Windows")
    add_compile_definitions(WIN32=1)
    if (MSVC)
        add_compile_options(
            /W3
            /MP
        )
	endif()

    # add_compile_definitions(_WIN32=1) this is a given from the compiler
elseif(UNIX)
    set(PLATFORM "SDL2")
    add_compile_definitions(SDL2=1)

    # Common compiler options and warning level for CLANG and GCC:
    add_compile_options(
        -Wall

        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-sign-compare
    )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # CLANG specific compile options:
    add_compile_options(
        -fdeclspec
        -fms-extensions

        -Wno-nullability-completeness
        -Wno-unused-private-field

        # Increase Ccache hit rate, note that you also need to set config options
        # in your Ccache config, see https://ccache.dev/manual/latest.html#_precompiled_headers
        "SHELL:-Xclang -fno-pch-timestamp"
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
    # GCC specific compile options:
    add_compile_options(
        -Wno-strict-aliasing
    )
endif()


add_subdirectory(WickedEngine)
add_custom_target(Content
   COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD} ${WICKED_ROOT_DIR}/Content ${CMAKE_CURRENT_BINARY_DIR}/Content
   COMMENT "$<IF:$<BOOL:${WICKED_ENABLE_SYMLINKS}>,Symlinking,Copying> Content directory"
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
