cmake_minimum_required(VERSION 3.19)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.31)
    cmake_policy(SET CMP0171 NEW)
    set(codegen CODEGEN)
endif()

set(MIN_OpenImageDenoise_VERSION "2.0")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(INSTALL_LOCAL_CONFIGDIR "${CMAKE_BINARY_DIR}/cmake")
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/WickedEngine")

# Use same options as Jolt
# Select X86 processor features to use (if everything is off it will be SSE2 compatible)
option(USE_SSE4_1 "Enable SSE4.1" ON)
option(USE_SSE4_2 "Enable SSE4.2" ON)
option(USE_AVX "Enable AVX" ON)
option(USE_AVX2 "Enable AVX2" OFF)
option(USE_AVX512 "Enable AVX512" OFF)
option(USE_LZCNT "Enable LZCNT" ON)
option(USE_TZCNT "Enable TZCNT" ON)
option(USE_F16C "Enable F16C" ON)
option(USE_FMADD "Enable FMADD" ON)

if (WICKED_DYNAMIC_LIBRARY)
    set(WICKED_LIBRARY_TYPE SHARED)
    message(STATUS "Building WickedEngine as a shared library")
else()
    set(WICKED_LIBRARY_TYPE STATIC)
    message(STATUS "Building WickedEngine as a static library")
endif()

if (WIN32)
    if (MSVC)
        add_compile_options(
            /bigobj
        )
    endif()
else ()
    find_package(SDL2 REQUIRED)
    find_package(OpenImageDenoise "${MIN_OpenImageDenoise_VERSION}" QUIET)
    find_package(Threads REQUIRED)
    if(NOT ${OpenImageDenoise_FOUND})
        message("OpenImageDenoise>=${MIN_OpenImageDenoise_VERSION} not found, it will be disabled.")
    else()
        message("OpenImageDenoise ${OpenImageDenoise_VERSION} Found.")
    endif()

    if(NOT TARGET SDL2::SDL2)
        # using old SDL2 cmake, lets create a SDL2 target ourselves
        find_library(SDL2_LIBRARY_FILE_LOCATION SDL2 REQUIRED)

        add_library(SDL2::SDL2 SHARED IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS}
            INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES}
            IMPORTED_LOCATION ${SDL2_LIBRARY_FILE_LOCATION}
        )

        unset(SDL2_LIBRARY_FILE_LOCATION)
    endif()

    # minimum sdl version is 2.0.14 for controller LED support
    if((${SDL_VERSION_MAJOR} GREATER_EQUAL 2) AND (${SDL2_VERSION_MINOR} GREATER_EQUAL 0) AND (${SDL2_VERSION_PATCH} GREATER_EQUAL 14))
        add_compile_definitions(SDL2_FEATURE_CONTROLLER_LED=1)
    endif()
endif()

add_subdirectory(LUA)
add_subdirectory(Utility)

set(PHYSICS_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include(${PHYSICS_REPO_ROOT}/Jolt/Jolt.cmake)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Jolt triggers a bug in some GCC versions, so we need to disable the warning
    target_compile_options(Jolt PRIVATE -Wno-stringop-overflow)
endif()

set_target_properties(Jolt PROPERTIES
    POSITION_INDEPENDENT_CODE ${WICKED_PIC}
)
add_compile_definitions(Jolt JPH_DEBUG_RENDERER)

file(GLOB HEADER_FILES CONFIGURE_DEPENDS *.h)
file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
list(REMOVE_ITEM SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/offlineshadercompiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wiRenderer.cpp
)

file(GLOB SHADER_FILES CONFIGURE_DEPENDS shaders/*.hlsl shaders/*.hlsli)

# these are all Wicked files except wiRenderer.cpp, which can be compiled
# with embedded shaders or not. If embedded shaders are enabled in the build,
# we will create 2 libraries, one version without embedded shaders, and the normal
# one. The "no shaders" is needed because the offlineshadercompiler that compiles
# the shaders depends on the wicked library as well, so we have a chicken-and-egg
# problem.
add_library(WickedEngine_common OBJECT ${SOURCE_FILES} ${HEADER_FILES})

# the version without embedded shaders
add_library(WickedEngine_ext_shaders ${WICKED_LIBRARY_TYPE}
    wiRenderer.cpp
)

# the version with embedded shaders. This target might not end up being
# used if WICKED_EMBED_SHADERS is not set, though it could still be
# used in other CMakeLists as an explicit target instead of the "WickedEngine"
# alias.
# Note that we will later set specific include directory that contains the
# generated wiShaderDump.h file. Since wiRenderer.cpp uses __hasinclude
# the end result will actually differ.
# This is also the reason for the "common" object library; since include
# directories are per-target, we would compile every file twice, but this
# is only necessary for wiRenderer.cpp
add_library(WickedEngine_emb_shaders EXCLUDE_FROM_ALL ${WICKED_LIBRARY_TYPE}
    wiRenderer.cpp
	${CMAKE_CURRENT_BINARY_DIR}/wiShaderDump.h
)

# all the flag does is define what the WickedEngine target refers to
if(WICKED_EMBED_SHADERS)
    set(TARGET_NAME WickedEngine_emb_shaders)
else()
    set(TARGET_NAME WickedEngine_ext_shaders)
endif()

add_library(WickedEngine ALIAS ${TARGET_NAME})

# aliases for backwards compatibility
if(WIN32)
    add_library(WickedEngine_Windows ALIAS ${TARGET_NAME})
else()
    add_library(WickedEngine_Linux ALIAS ${TARGET_NAME})
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "WickedEngine"
)

target_precompile_headers(WickedEngine_common PRIVATE WickedEngine.h)
target_precompile_headers(WickedEngine_emb_shaders REUSE_FROM WickedEngine_common)
target_precompile_headers(WickedEngine_ext_shaders REUSE_FROM WickedEngine_common)

set_target_properties(WickedEngine_common PROPERTIES
    PUBLIC_HEADER "${HEADER_FILES}"
    POSITION_INDEPENDENT_CODE ${WICKED_PIC}
    FOLDER "WickedEngine"
    PROJECT_LABEL "common files"
)

set_target_properties(WickedEngine_ext_shaders PROPERTIES
    FOLDER "WickedEngine"
    PROJECT_LABEL "compile shaders at runtime variant"
)

set_target_properties(WickedEngine_emb_shaders PROPERTIES
    FOLDER "WickedEngine"
    PROJECT_LABEL "embedded shaders variant"
)

target_include_directories(WickedEngine_common SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine>
)

set(WICKEDENGINE_STATIC_LIBRARIES
    Jolt
    LUA
    Utility
)

if (USE_AVX512 OR USE_AVX2)
    target_compile_definitions(WickedEngine_common PUBLIC _XM_AVX2_INTRINSICS_)
endif()
if (USE_AVX)
    target_compile_definitions(WickedEngine_common PUBLIC _XM_AVX_INTRINSICS_)
endif()
if (USE_SSE4_1 OR USE_SSE4_2)
    target_compile_definitions(WickedEngine_common PUBLIC _XM_SSE4_INTRINSICS_)
endif()
if (USE_F16C)
    target_compile_definitions(WickedEngine_common PUBLIC _XM_F16C_INTRINSICS_)
endif()
if (USE_FMADD)
    target_compile_definitions(WickedEngine_common PUBLIC _XM_FMA3_INTRINSICS_)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86" OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        if (USE_AVX512)
            target_compile_options(WickedEngine_common PUBLIC /arch:AVX512)
        elseif (USE_AVX2)
            target_compile_options(WickedEngine_common PUBLIC /arch:AVX2)
        elseif (USE_AVX)
            target_compile_options(WickedEngine_common PUBLIC /arch:AVX)
        endif()
    endif()
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "amd64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i386")
    if (USE_AVX512)
        target_compile_options(WickedEngine_common PUBLIC -mavx512f -mavx512vl -mavx512dq -mavx2 -mbmi -mpopcnt -mlzcnt -mf16c)
    elseif (USE_AVX2)
        target_compile_options(WickedEngine_common PUBLIC -mavx2 -mbmi -mpopcnt -mlzcnt -mf16c)
    elseif (USE_AVX)
        target_compile_options(WickedEngine_common PUBLIC -mavx -mpopcnt)
    elseif (USE_SSE4_2)
        target_compile_options(WickedEngine_common PUBLIC -msse4.2 -mpopcnt)
    elseif (USE_SSE4_1)
        target_compile_options(WickedEngine_common PUBLIC -msse4.1)
    else()
        target_compile_options(WickedEngine_common PUBLIC -msse2)
    endif()
    if (USE_LZCNT)
        target_compile_options(WickedEngine_common PUBLIC -mlzcnt)
    endif()
    if (USE_TZCNT)
        target_compile_options(WickedEngine_common PUBLIC -mbmi)
    endif()
    if (USE_F16C)
        target_compile_options(WickedEngine_common PUBLIC -mf16c)
    endif()
    if (USE_FMADD)
        target_compile_options(WickedEngine_common PUBLIC -mfma)
    endif()

    target_compile_options(WickedEngine_common PUBLIC -mfpmath=sse)
endif()

target_compile_definitions(WickedEngine_common PUBLIC WICKED_CMAKE_BUILD)


if (WIN32)
    target_compile_definitions(WickedEngine_common PUBLIC
        UNICODE _UNICODE
    )

    set(LIBDXCOMPILER "dxcompiler.dll")
else ()
    # `ska::flat_hash_map` has issues on linux because of the hash function being identity
    # in same cases. Use `robin_hood::unordered_flat_map` instead
    target_compile_definitions(WickedEngine_common PUBLIC WI_UNORDERED_MAP_TYPE=2)

    target_link_libraries(WickedEngine_common PUBLIC
        Threads::Threads
        SDL2::SDL2
        $<$<BOOL:${OpenImageDenoise_FOUND}>:OpenImageDenoise> # links OpenImageDenoise only if it's found
    )

    set_target_properties(FAudio PROPERTIES
        POSITION_INDEPENDENT_CODE ${WICKED_PIC}
        UNITY_BUILD NO
    )
    set(WICKEDENGINE_STATIC_LIBRARIES ${WICKEDENGINE_STATIC_LIBRARIES} FAudio)

    target_link_libraries(WickedEngine_common PUBLIC dl)

    set(LIBDXCOMPILER "libdxcompiler.so")
endif()

target_link_libraries(WickedEngine_common PUBLIC ${WICKEDENGINE_STATIC_LIBRARIES})

if (PLATFORM MATCHES "SDL2")
    target_compile_definitions(WickedEngine_common PUBLIC SDL2=1)
endif()

add_library(dxcompiler SHARED IMPORTED)
set_property(TARGET dxcompiler PROPERTY
        IMPORTED_LOCATION ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} )

add_executable(offlineshadercompiler
    offlineshadercompiler.cpp
)

# Copy the shader library next to the executable
add_custom_command(
    TARGET WickedEngine_ext_shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD} ${WICKED_ROOT_DIR}/WickedEngine/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
)

set_property(SOURCE
    wiAudio.cpp
    wiOcean.cpp
    wiPhysics_Jolt.cpp
    wiGraphicsDevice_Vulkan.cpp
    wiGraphicsDevice_DX12.cpp

    PROPERTY SKIP_UNITY_BUILD_INCLUSION TRUE
)
set_property(TARGET LUA PROPERTY UNITY_BUILD NO)


if(WICKED_ENABLE_IPO)
    set_target_properties(
        ${WICKEDENGINE_STATIC_LIBRARIES}
        WickedEngine_common
        offlineshadercompiler

        PROPERTIES

        INTERPROCEDURAL_OPTIMIZATION ON
        INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()


if(DEFINED ENV{VERBOSE})
    set(verbose 1)
else()
    set(verbose $<BOOL:${CMAKE_VERBOSE_MAKEFILE}>)
endif()

# define a command that will call the offlineshadercompiler and generate wiShaderDump.h
# we simply depend on every shader file, offlineshadercompiler is smart enough to only
# recompile files that have actually changed and those that depend on them.
add_custom_command(
    ${codegen}
    OUTPUT wiShaderDump.h
    COMMAND offlineshadercompiler $<$<BOOL:${WIN32}>:hlsl6> spirv shaderdump strip_reflection $<$<NOT:${verbose}>:quiet>
    DEPENDS offlineshadercompiler ${SHADER_FILES}
    USES_TERMINAL
)

target_link_libraries(WickedEngine_ext_shaders WickedEngine_common)
target_link_libraries(WickedEngine_emb_shaders WickedEngine_common)

target_link_libraries(offlineshadercompiler
    PUBLIC WickedEngine_ext_shaders
)

# only this target will see the wiShaderDump.h file, so the _ext_shaders target will not have embedded shaders.
target_include_directories(WickedEngine_emb_shaders PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS offlineshadercompiler
        RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

install(DIRECTORY "${WICKED_ROOT_DIR}/Content"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

#### LOCAL FOLDER INSTALLATION
# Create importable target here

#set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")
set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME WickedEngine)

install(TARGETS ${TARGET_NAME} ${WICKEDENGINE_STATIC_LIBRARIES} WickedEngine_common
        EXPORT Engine-Targets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine")

install(FILES ${LIBDXCOMPILER}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

export(EXPORT Engine-Targets
        FILE "${CMAKE_BINARY_DIR}/cmake/WickedEngineTargets.cmake"
        NAMESPACE WickedEngine::
        )

install(EXPORT Engine-Targets
        FILE WickedEngineTargets.cmake
        NAMESPACE WickedEngine::
        DESTINATION ${INSTALL_CONFIGDIR})

set(_CONFIG_INSTALL_DIR_INCLUDE "${WICKED_ROOT_DIR}")
set(_CONFIG_INSTALL_DIR_LIB "${WICKED_ROOT_DIR}")
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/WickedEngineConfig.cmake.in
        ${CMAKE_BINARY_DIR}/cmake/WickedEngineConfig.cmake
        INSTALL_DESTINATION ${INSTALL_LOCAL_CONFIGDIR}
)
set(_CONFIG_INSTALL_DIR_INCLUDE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
set(_CONFIG_INSTALL_DIR_LIB "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/WickedEngineConfig.cmake.in
        ${CMAKE_BINARY_DIR}/cmake/install/WickedEngineConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES ${CMAKE_BINARY_DIR}/cmake/install/WickedEngineConfig.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
)

# Shaders
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine"
        PATTERN "spirv" EXCLUDE
        PATTERN "hlsl6" EXCLUDE
        PATTERN "*.vcxitems*" EXCLUDE
        )
