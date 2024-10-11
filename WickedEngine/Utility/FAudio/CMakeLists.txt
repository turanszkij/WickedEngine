# CMake Project for FAudio
# Written by @NeroBurner
cmake_minimum_required(VERSION 3.5)
project(FAudio C)

# Options
option(BUILD_SHARED_LIBS "Build shared library" OFF)

# C99 Requirement
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

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

set(PUBLIC_HEADERS
	include/F3DAudio.h
	include/FACT3D.h
	include/FACT.h
	include/FAPOBase.h
	include/FAPOFX.h
	include/FAPO.h
	include/FAudioFX.h
	include/FAudio.h
	)

# Source lists
add_library(FAudio STATIC
	# Public Headers
	${PUBLIC_HEADERS}
	# Internal Headers
	src/FACT_internal.h
	src/FAudio_internal.h
	src/stb.h
	src/stb_vorbis.h
	src/matrix_defaults.inl
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
	src/FAudio_platform_sdl3.c
	src/FAudio_platform_win32.c
	# Optional source files
	src/XNA_Song.c
	src/FAudio_gstreamer.c
)

install(FILES ${PUBLIC_HEADERS}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/FAudio/")

# FAudio folders as includes, for other targets to consume
target_include_directories(FAudio PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/FAudio>
)

# Soname
set_target_properties(FAudio PROPERTIES
	OUTPUT_NAME "FAudio"
	VERSION ${LIB_VERSION}
	SOVERSION ${LIB_MAJOR_VERSION}
	POSITION_INDEPENDENT_CODE ${WICKED_DYNAMIC_LIBRARY}
)

# SDL2 Dependency
if (PLATFORM_WIN32)
	message(STATUS "not using SDL3")
else()
	find_package(SDL3 CONFIG)
	if (TARGET SDL3::SDL3)
		message(STATUS "using TARGET SDL3::SDL3")
		target_link_libraries(FAudio PUBLIC SDL3::SDL3)
		if(INSTALL_MINGW_DEPENDENCIES)
			install_shared_libs(TARGETS SDL3::SDL3 DESTINATION bin NO_INSTALL_SYMLINKS REQUIRED)
		endif()
		target_compile_definitions(FAudio PUBLIC FAUDIO_SDL3_PLATFORM)
	endif()
endif()
