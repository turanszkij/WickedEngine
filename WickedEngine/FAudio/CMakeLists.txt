# CMake Project for FAudio
# Written by @NeroBurner
cmake_minimum_required(VERSION 2.8.12)
project(FAudio C)

# Options
option(GSTREAMER "Enable GStreamer support (WMA, XMA)" OFF)
option(BUILD_UTILS "Build utils/ folder" OFF)
option(BUILD_TESTS "Build tests/ folder for unit tests to be executed on the host against FAudio" OFF)
if(WIN32)
option(BUILD_CPP "Build cpp/ folder (COM wrapper for XAudio2)" OFF)
option(PLATFORM_WIN32 "Enable native Win32 platform instead of SDL2" OFF)
endif()
option(XNASONG "Build with XNA_Song.c" ON)
option(LOG_ASSERTIONS "Bind FAudio_assert to log, instead of platform's assert" OFF)
option(FORCE_ENABLE_DEBUGCONFIGURATION "Enable DebugConfiguration in all build types" OFF)
option(DUMP_VOICES "Dump voices to RIFF WAVE files" OFF)
option(BUILD_SHARED_LIBS "Build shared library" ON)
if(WIN32)
option(INSTALL_MINGW_DEPENDENCIES "Add dependent libraries to MinGW install target" OFF)
endif()

# C99 Requirement
if(${CMAKE_VERSION} VERSION_LESS "3.1.3")
	message(WARNING "Your CMake version is too old, set -std=c99 yourself!")
else()
	set(CMAKE_C_STANDARD 99)
	set(CMAKE_C_EXTENSIONS OFF)
endif()

# Version
SET(LIB_MAJOR_VERSION "0")
SET(LIB_MINOR_VERSION "21")
SET(LIB_REVISION "09")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_REVISION}")

# Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	# By default, we use Release
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE "Release" CACHE
		STRING "Choose the type of build." FORCE
	)

	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
		STRINGS "Debug" "Release" "RelWithDebInfo"
	)
endif()

# Add our repository's module path to CMake's module path list
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
elseif(WIN32)
	# "FAudio.dll", not "libFAudio.dll"
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Helper function for finding/installing MinGW libs
if(INSTALL_MINGW_DEPENDENCIES)
	include(cmake/install_shared_libs.cmake)
endif()

# Source lists
add_library(FAudio
	# Public Headers
	include/F3DAudio.h
	include/FACT3D.h
	include/FACT.h
	include/FAPOBase.h
	include/FAPOFX.h
	include/FAPO.h
	include/FAudioFX.h
	include/FAudio.h
	# Internal Headers
	src/FACT_internal.h
	src/FAudio_internal.h
	src/stb.h
	src/stb_vorbis.h
	# Source Files
	src/F3DAudio.c
	src/FACT3D.c
	src/FACT.c
	src/FACT_internal.c
	src/FAPOBase.c
	src/FAPOFX.c
	src/FAPOFX_echo.c
	src/FAPOFX_eq.c
	src/FAPOFX_masteringlimiter.c
	src/FAPOFX_reverb.c
	src/FAudio.c
	src/FAudioFX_reverb.c
	src/FAudioFX_volumemeter.c
	src/FAudio_internal.c
	src/FAudio_internal_simd.c
	src/FAudio_operationset.c
	src/FAudio_platform_sdl2.c
	src/FAudio_platform_win32.c
	# Optional source files
	src/XNA_Song.c
	src/FAudio_gstreamer.c
)

if(PLATFORM_WIN32)
	target_link_libraries(FAudio PRIVATE -ldxguid -luuid -lwinmm -lole32 -ladvapi32 -luser32 -lmfplat -lmfreadwrite -lmfuuid -lpropsys)
	target_compile_definitions(FAudio PUBLIC FAUDIO_WIN32_PLATFORM)
	target_compile_definitions(FAudio PRIVATE HAVE_WMADEC=1)
	set(PLATFORM_CFLAGS "-DFAUDIO_WIN32_PLATFORM")
	set(GSTREAMER OFF)
	set(XNASONG OFF)
else()
	set(PLATFORM_CFLAGS)
endif()

# Only disable DebugConfiguration in release builds
if(NOT FORCE_ENABLE_DEBUGCONFIGURATION)
	target_compile_definitions(FAudio PRIVATE $<$<CONFIG:Release>:FAUDIO_DISABLE_DEBUGCONFIGURATION>)
endif()

# FAudio_assert Customization
if(LOG_ASSERTIONS)
	target_compile_definitions(FAudio PUBLIC FAUDIO_LOG_ASSERTIONS)
endif()

# FAudio folders as includes, for other targets to consume
target_include_directories(FAudio PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# MinGW builds should statically link libgcc
if(MINGW)
	target_link_libraries(FAudio PRIVATE -static-libgcc)
endif()

# Soname
set_target_properties(FAudio PROPERTIES OUTPUT_NAME "FAudio"
	VERSION ${LIB_VERSION}
	SOVERSION ${LIB_MAJOR_VERSION}
)

# XNA_Song Support
if(NOT XNASONG)
	target_compile_definitions(FAudio PRIVATE DISABLE_XNASONG)
endif()

# GStreamer Support
if(GSTREAMER)
	# Requires for gstreamer in the pkgconfig file
	set(FAUDIO_GSTREAMER_DEPS " gstreamer-app-1.0 gstreamer-audio-1.0")

	# Add the extra definition...
	target_compile_definitions(FAudio PRIVATE HAVE_WMADEC=1 HAVE_GSTREAMER=1)

	# Find GStreamer
	find_package(PkgConfig)
	pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.9.2)
	pkg_check_modules(GSTAUDIO REQUIRED gstreamer-audio-1.0>=1.9.2)
	pkg_check_modules(GSTAPP REQUIRED gstreamer-app-1.0>=1.9.2)

	# Include/Link GStreamer...
	target_include_directories(FAudio PRIVATE
		${GST_INCLUDE_DIRS}
		${GSTAUDIO_INCLUDE_DIRS}
		${GSTAPP_INCLUDE_DIRS}
	)
	target_link_libraries(FAudio PRIVATE
            ${GST_LDFLAGS}
            ${GSTAUDIO_LDFLAGS}
            ${GSTAPP_LDFLAGS}
        )
else()
	set(FAUDIO_GSTREAMER_DEPS "")
endif(GSTREAMER)

# Dump source voices to RIFF WAVE files
if(DUMP_VOICES)
	target_compile_definitions(FAudio PRIVATE FAUDIO_DUMP_VOICES)
endif()

# SDL2 Dependency
if (PLATFORM_WIN32)
	message(STATUS "not using SDL2")
elseif (DEFINED SDL2_INCLUDE_DIRS AND DEFINED SDL2_LIBRARIES)
	message(STATUS "using pre-defined SDL2 variables SDL2_INCLUDE_DIRS and SDL2_LIBRARIES")
	target_include_directories(FAudio PUBLIC "$<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>")
	target_link_libraries(FAudio PUBLIC ${SDL2_LIBRARIES})
	if(INSTALL_MINGW_DEPENDENCIES)
		install_shared_libs(${SDL2_LIBRARIES} DESTINATION bin NO_INSTALL_SYMLINKS)
	endif()
else()
	# Only try to autodetect if both SDL2 variables aren't explicitly set
	find_package(SDL2 CONFIG)
	if (TARGET SDL2::SDL2)
		message(STATUS "using TARGET SDL2::SDL2")
		target_link_libraries(FAudio PUBLIC SDL2::SDL2)
		if(INSTALL_MINGW_DEPENDENCIES)
			install_shared_libs(TARGETS SDL2::SDL2 DESTINATION bin NO_INSTALL_SYMLINKS REQUIRED)
		endif()
	elseif (TARGET SDL2)
		message(STATUS "using TARGET SDL2")
		target_link_libraries(FAudio PUBLIC SDL2)
		if(INSTALL_MINGW_DEPENDENCIES)
			install_shared_libs(TARGETS SDL2 DESTINATION bin NO_INSTALL_SYMLINKS REQUIRED)
		endif()
	else()
		message(STATUS "no TARGET SDL2::SDL2, or SDL2, using variables")
		target_include_directories(FAudio PUBLIC "$<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>")
		target_link_libraries(FAudio PUBLIC ${SDL2_LIBRARIES})
		if(INSTALL_MINGW_DEPENDENCIES)
			install_shared_libs(${SDL2_LIBRARIES} DESTINATION bin NO_INSTALL_SYMLINKS)
		endif()
	endif()
endif()

# utils/ Folder
if(BUILD_UTILS)
	enable_language(CXX)

	# Shared ImGui Framework
	add_library(uicommon STATIC
		utils/uicommon/FAudioUI_main.cpp
		utils/uicommon/FAudioUI_ui.cpp
		utils/uicommon/glfuncs.h
		utils/uicommon/glmacros.h
		utils/uicommon/imconfig.h
		utils/uicommon/imgui.cpp
		utils/uicommon/imgui_demo.cpp
		utils/uicommon/imgui_draw.cpp
		utils/uicommon/imgui_widgets.cpp
		utils/uicommon/imgui.h
		utils/uicommon/imgui_internal.h
		utils/uicommon/imstb_rectpack.h
		utils/uicommon/imstb_textedit.h
		utils/uicommon/imstb_truetype.h
	)
	target_link_libraries(uicommon PUBLIC FAudio)

	# Shared WAV Resources
	add_library(wavs STATIC utils/wavcommon/wavs.cpp)
	target_compile_definitions(wavs PUBLIC
		RESOURCE_PATH="${CMAKE_SOURCE_DIR}/utils/wavcommon/resources"
	)

	# These tools do NOT use uicommon
	add_executable(testparse utils/testparse/testparse.c)
	target_link_libraries(testparse PRIVATE FAudio)
	add_executable(testxwma utils/testxwma/testxwma.cpp)
	target_link_libraries(testxwma PRIVATE FAudio)
	add_executable(showriffheader utils/showriffheader/showriffheader.cpp)
	target_link_libraries(showriffheader PRIVATE FAudio)

	# These tools use uicommon, but NOT wavs
	add_executable(facttool utils/facttool/facttool.cpp)
	target_link_libraries(facttool PRIVATE uicommon)
	add_executable(testfilter
		utils/testfilter/audio.cpp
		utils/testfilter/audio_faudio.cpp
		utils/testfilter/audio.h
		utils/testfilter/audio_player.h
		utils/testfilter/audio_xaudio.cpp
		utils/testfilter/oscillator.cpp
		utils/testfilter/oscillator.h
		utils/testfilter/testfilter.cpp
	)
	target_link_libraries(testfilter PRIVATE uicommon)

	# These tools use both uicommon and wavs
	add_executable(testreverb
		utils/testreverb/audio.cpp
		utils/testreverb/audio_faudio.cpp
		utils/testreverb/audio.h
		utils/testreverb/audio_xaudio.cpp
		utils/testreverb/testreverb.cpp
	)
	target_link_libraries(testreverb PRIVATE uicommon wavs)
	add_executable(testvolumemeter
		utils/testvolumemeter/audio.cpp
		utils/testvolumemeter/audio_faudio.cpp
		utils/testvolumemeter/audio.h
		utils/testvolumemeter/testvolumemeter.cpp
	)
	target_link_libraries(testvolumemeter PRIVATE uicommon wavs)
endif()

# define install directories
# on mingw-w64 cross compilation $CMAKE_INSTALL_LIBDIR is set to an absolute
# path. Work around that by hard coding the directories on windows
if(WIN32)
	set(FAudio_INSTALL_INCLUDEDIR include)
	set(FAudio_INSTALL_BINDIR bin)
	set(FAudio_INSTALL_LIBDIR lib)
else()
	include(GNUInstallDirs)
	set(FAudio_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
	set(FAudio_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
	set(FAudio_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
endif()

# cpp/ Folder
if(BUILD_CPP)
	enable_language(CXX)

	# XAPOBase, used by XAudio2 and XAPOFX
	add_library(XAPOBase STATIC
		cpp/XAPOBase.cpp
	)
	target_compile_definitions(XAPOBase PRIVATE
		FAUDIOCPP_EXPORTS
		_WIN32_WINNT=0x0600
	)
	target_link_libraries(XAPOBase PRIVATE FAudio)
	if(MINGW)
		target_link_libraries(XAPOBase PRIVATE
			"-Wl,--enable-stdcall-fixup"
		)
		# Enable various compiler warnings
		target_link_libraries(XAPOBase PRIVATE
			-Wall -pedantic
			-Wsign-compare
			-Wshadow
		)
	endif()

	# We're about to make a whole bunch of DLLs...
	set(FAudio_CPP_libs "")

	# XAudio2 2.0-2.7
	foreach(lib_version RANGE 0 7)
		set(faudio_cpp_target "xaudio2_${lib_version}")
		add_library(${faudio_cpp_target} SHARED
			cpp/com_utils.cpp
			cpp/xaudio2.cpp
			cpp/XAudio2fx.cpp
			cpp/win_registry.cpp
		)
		target_compile_definitions(${faudio_cpp_target} PRIVATE
			"XAUDIO2_VERSION=${lib_version}"
		)
		target_link_libraries(${faudio_cpp_target} PRIVATE XAPOBase)
		list(APPEND FAudio_CPP_libs ${faudio_cpp_target})
	endforeach()

	# XAudio2 2.8-2.9
	foreach(lib_version RANGE 8 9)
		set(faudio_cpp_target "xaudio2_${lib_version}")
		add_library(${faudio_cpp_target} SHARED
			cpp/com_utils.cpp
			cpp/xaudio2.cpp
			cpp/XAudio2fx.cpp
			cpp/win_device.cpp
			cpp/X3DAudio.cpp
			cpp/XAPOFX.cpp
		)
		target_compile_definitions(${faudio_cpp_target} PRIVATE
			"XAUDIO2_VERSION=${lib_version}"
			UNICODE
		)
		target_link_libraries(${faudio_cpp_target} PRIVATE XAPOBase setupapi)
		list(APPEND FAudio_CPP_libs ${faudio_cpp_target})
	endforeach()

	# X3DAudio 1.3-1.7
	foreach(lib_version RANGE 3 7)
		set(faudio_cpp_target "x3daudio1_${lib_version}")
		add_library(${faudio_cpp_target} SHARED cpp/X3DAudio.cpp)
		list(APPEND FAudio_CPP_libs ${faudio_cpp_target})
	endforeach()

	# XAPOFX 1.1-1.5
	foreach(lib_version RANGE 1 5)
		set(faudio_cpp_target "xapofx1_${lib_version}")
		add_library(${faudio_cpp_target} SHARED
			cpp/com_utils_xapofx.cpp
			cpp/XAPOFX.cpp
		)
		target_link_libraries(${faudio_cpp_target} PRIVATE XAPOBase)
		list(APPEND FAudio_CPP_libs ${faudio_cpp_target})
	endforeach()

	# XACTEngine3 3.0-3.7
	foreach(lib_version RANGE 0 7)
		set(faudio_cpp_target "xactengine3_${lib_version}")
		add_library(${faudio_cpp_target} SHARED
			cpp/com_utils_xact.cpp
			cpp/xact3.cpp
			cpp/win_registry.cpp
		)
		target_compile_definitions(${faudio_cpp_target} PRIVATE
			"XACT3_VERSION=${lib_version}"
		)
		list(APPEND FAudio_CPP_libs ${faudio_cpp_target})
	endforeach()

	# Common Libraries/Definitions
	foreach(faudio_cpp_target ${FAudio_CPP_libs})
		target_link_libraries(${faudio_cpp_target} PRIVATE
			ole32
			FAudio
		)
		target_compile_definitions(${faudio_cpp_target} PRIVATE
			FAUDIOCPP_EXPORTS
			_WIN32_WINNT=0x0600
		)

		# MinGW builds should statically link libgcc/libstdc++
		if(MINGW)
			target_link_libraries(${faudio_cpp_target} PRIVATE
				"-Wl,--enable-stdcall-fixup"
				"-static-libgcc -static-libstdc++"
			)
			# Enable various compiler warnings
			target_link_libraries(${faudio_cpp_target} PRIVATE
				-Wall -pedantic
				-Wsign-compare
				-Wshadow
			)
		endif()
	endforeach()

	# Add libraries to install target
	install(TARGETS ${FAudio_CPP_libs}
		RUNTIME DESTINATION ${FAudio_INSTALL_BINDIR}
		LIBRARY DESTINATION ${FAudio_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${FAudio_INSTALL_LIBDIR}
	)
	# Add Wine install script to install target
	install(PROGRAMS cpp/scripts/wine_setup_native
		DESTINATION ${FAudio_INSTALL_BINDIR}
	)
	# Install runtime dependencies
	if(INSTALL_MINGW_DEPENDENCIES)
		install_shared_libs(-lwinpthread-1 -lwinpthread -liconv DESTINATION bin NO_INSTALL_SYMLINKS)
	endif()
endif()

# tests/ Folder
if(BUILD_TESTS)
	add_executable(faudio_tests tests/xaudio2.c)
	target_link_libraries(faudio_tests PRIVATE FAudio)
endif()

# Installation

# Public Headers...
install(
	DIRECTORY include/
	DESTINATION ${FAudio_INSTALL_INCLUDEDIR}
)
# Libraries...
install(
	TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-targets
	INCLUDES DESTINATION ${FAudio_INSTALL_INCLUDEDIR}
	RUNTIME DESTINATION ${FAudio_INSTALL_BINDIR}
	LIBRARY DESTINATION ${FAudio_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${FAudio_INSTALL_LIBDIR}
)

# Generate a pkgconfig file
include(cmake/JoinPaths.cmake)
join_paths(FAUDIO_PKGCONF_LIBDIR "\${prefix}" "${CMAKE_INSTALL_LIBDIR}")
join_paths(FAUDIO_PKGCONF_INCLUDEDIR "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}")
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}.pc.in"
	${PROJECT_BINARY_DIR}/generated/${PROJECT_NAME}.pc
	@ONLY
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/${PROJECT_NAME}.pc
	DESTINATION ${FAudio_INSTALL_LIBDIR}/pkgconfig
)

# Generate cmake-config file, install CMake files
include(CMakePackageConfigHelpers)
configure_package_config_file(
	cmake/config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/generated/${PROJECT_NAME}Config.cmake
	INSTALL_DESTINATION ${FAudio_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/${PROJECT_NAME}Config.cmake
	DESTINATION ${FAudio_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
install(
	EXPORT ${PROJECT_NAME}-targets
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION ${FAudio_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
