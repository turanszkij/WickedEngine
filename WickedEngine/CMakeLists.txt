cmake_minimum_required(VERSION 3.19)

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
    # TODO: Choose whether to use SDL2 on windows as well
    set(TARGET_NAME WickedEngine_Windows)

    if (MSVC)
        add_compile_options(
            /bigobj
        )
    endif()
else ()
    if(APPLE)
        set(TARGET_NAME WickedEngine_macOS)
    else()
        set(TARGET_NAME WickedEngine_Linux)
    endif()
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

# Note: if you remove this glob, the wiShaderDump.h hack
# will stop working, see below for more information
file(GLOB HEADER_FILES CONFIGURE_DEPENDS *.h)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/offlineshadercompiler.cpp)

add_library(${TARGET_NAME} ${WICKED_LIBRARY_TYPE}
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    # Make shim force-include PUBLIC so dependent targets (Editor) also see NEON compatibility wrappers
    target_compile_options(${TARGET_NAME} PUBLIC -include ${CMAKE_CURRENT_SOURCE_DIR}/platform/NeonIntrinsicsAppleShim.h)
endif()

# Force UUID emulation on non-Windows (e.g. macOS/Linux)
if(NOT WIN32)
    target_compile_definitions(${TARGET_NAME} PUBLIC __EMULATE_UUID)
endif()

target_precompile_headers(${TARGET_NAME} PRIVATE WickedEngine.h)

add_library(WickedEngine ALIAS ${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES
    PUBLIC_HEADER "${HEADER_FILES}"
    POSITION_INDEPENDENT_CODE ${WICKED_PIC}
)

target_include_directories(${TARGET_NAME} SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine>
)

set(WICKEDENGINE_STATIC_LIBRARIES
    Jolt
    LUA
    Utility
)

if (USE_AVX512 OR USE_AVX2)
    target_compile_definitions(${TARGET_NAME} PUBLIC _XM_AVX2_INTRINSICS_)
endif()
if (USE_AVX)
    target_compile_definitions(${TARGET_NAME} PUBLIC _XM_AVX_INTRINSICS_)
endif()
if (USE_SSE4_1 OR USE_SSE4_2)
    target_compile_definitions(${TARGET_NAME} PUBLIC _XM_SSE4_INTRINSICS_)
endif()
if (USE_F16C)
    target_compile_definitions(${TARGET_NAME} PUBLIC _XM_F16C_INTRINSICS_)
endif()
if (USE_FMADD)
    target_compile_definitions(${TARGET_NAME} PUBLIC _XM_FMA3_INTRINSICS_)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86" OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        if (USE_AVX512)
            target_compile_options(${TARGET_NAME} PUBLIC /arch:AVX512)
        elseif (USE_AVX2)
            target_compile_options(${TARGET_NAME} PUBLIC /arch:AVX2)
        elseif (USE_AVX)
            target_compile_options(${TARGET_NAME} PUBLIC /arch:AVX)
        endif()
    endif()
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "amd64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i386")
    if (USE_AVX512)
        target_compile_options(${TARGET_NAME} PUBLIC -mavx512f -mavx512vl -mavx512dq -mavx2 -mbmi -mpopcnt -mlzcnt -mf16c)
    elseif (USE_AVX2)
        target_compile_options(${TARGET_NAME} PUBLIC -mavx2 -mbmi -mpopcnt -mlzcnt -mf16c)
    elseif (USE_AVX)
        target_compile_options(${TARGET_NAME} PUBLIC -mavx -mpopcnt)
    elseif (USE_SSE4_2)
        target_compile_options(${TARGET_NAME} PUBLIC -msse4.2 -mpopcnt)
    elseif (USE_SSE4_1)
        target_compile_options(${TARGET_NAME} PUBLIC -msse4.1)
    else()
        target_compile_options(${TARGET_NAME} PUBLIC -msse2)
    endif()
    if (USE_LZCNT)
        target_compile_options(${TARGET_NAME} PUBLIC -mlzcnt)
    endif()
    if (USE_TZCNT)
        target_compile_options(${TARGET_NAME} PUBLIC -mbmi)
    endif()
    if (USE_F16C)
        target_compile_options(${TARGET_NAME} PUBLIC -mf16c)
    endif()
    if (USE_FMADD)
        target_compile_options(${TARGET_NAME} PUBLIC -mfma)
    endif()

    target_compile_options(${TARGET_NAME} PUBLIC -mfpmath=sse)
endif()

target_compile_definitions(${TARGET_NAME} PUBLIC WICKED_CMAKE_BUILD)

# wiRenderer.cpp gets compiled differently depending on whether or not
# wiShaderDump.h exists. If it does, it includes it and defines
# SHADERDUMP_ENABLED. CMake doesn't support optional dependencies,
# so we check if the file exists and define it ourselves if it does,
# this will cause the file to be recompiled if necessary.
# IMPORTANT: this only works because we are using GLOB on header files
# and therefore CMake will re-run if those change. Without a GLOB
# the check will only be done once and stop working reliably.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wiShaderDump.h)
    set_property(
        SOURCE wiRenderer.cpp
        PROPERTY COMPILE_DEFINITIONS "SHADERDUMP_ENABLED="
    )
endif()

if (WIN32)
    target_compile_definitions(${TARGET_NAME} PUBLIC
        UNICODE _UNICODE
    )

    set(LIBDXCOMPILER "dxcompiler.dll")
else ()
    # `ska::flat_hash_map` has issues on linux because of the hash function being identity
    # in same cases. Use `robin_hood::unordered_flat_map` instead
    target_compile_definitions(${TARGET_NAME} PUBLIC WI_UNORDERED_MAP_TYPE=2)

    target_link_libraries(${TARGET_NAME} PUBLIC
        Threads::Threads
        SDL2::SDL2
        $<$<BOOL:${OpenImageDenoise_FOUND}>:OpenImageDenoise> # links OpenImageDenoise only if it's found
    )

    set_target_properties(FAudio PROPERTIES
        POSITION_INDEPENDENT_CODE ${WICKED_PIC}
        UNITY_BUILD NO
    )
    set(WICKEDENGINE_STATIC_LIBRARIES ${WICKEDENGINE_STATIC_LIBRARIES} FAudio)

    target_link_libraries(${TARGET_NAME} PRIVATE dl)

    if(APPLE)
        set(LIBDXCOMPILER "libdxcompiler.dylib")
    else()
        set(LIBDXCOMPILER "libdxcompiler.so")
    endif()
endif()

target_link_libraries(${TARGET_NAME} PUBLIC ${WICKEDENGINE_STATIC_LIBRARIES})

if (PLATFORM MATCHES "SDL2")
    target_compile_definitions(${TARGET_NAME} PUBLIC SDL2=1)
endif()

add_library(dxcompiler SHARED IMPORTED)
set_property(TARGET dxcompiler PROPERTY
        IMPORTED_LOCATION ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} )

# Copy the shader library next to the executable
add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
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

# OFFLINE SHADER COMPILER
add_executable(offlineshadercompiler
        offlineshadercompiler.cpp
)

if(WICKED_USE_IPO)
set_target_properties(${WICKEDENGINE_STATIC_LIBRARIES} ${TARGET_NAME} offlineshadercompiler PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()


target_link_libraries(offlineshadercompiler
        PUBLIC ${TARGET_NAME})

install(TARGETS offlineshadercompiler
        RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

install(DIRECTORY "${WICKED_ROOT_DIR}/Content"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

#### LOCAL FOLDER INSTALLATION
# Create importable target here

#set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")
set_target_properties("${TARGET_NAME}" PROPERTIES EXPORT_NAME "${TARGET_NAME}")


install(TARGETS "${TARGET_NAME}" ${WICKEDENGINE_STATIC_LIBRARIES}
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
