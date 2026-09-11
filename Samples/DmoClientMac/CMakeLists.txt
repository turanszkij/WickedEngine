cmake_minimum_required(VERSION 3.19)
project(DmoClientMac)

# MACRENDER-01 — DMO WICKED client macOS window bring-up.
# Links the in-tree WickedEngine target (transitive Metal/SDL2/dxc/LUA deps),
# mirroring the Template_* samples. macOS-only (AppKit + Carbon).

# DMO repo root. The engine (CMAKE_SOURCE_DIR) is repo/external/WickedEngine, so
# the DMO client repo root is two levels up.
get_filename_component(DMO_CLIENT_ROOT "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)

# WICKED-CLIENT-LAYERS-01: the DMO source lists live in the CLIENT repo, next to
# the code, not here in the engine submodule. Both the macOS and Windows app
# targets include this same file, so a new client module is added ONCE.
include("${DMO_CLIENT_ROOT}/cmake/DmoClientLayers.cmake")

# WICKED-CLIENT-LAYERS-04: THE portable client layer, converged.
#
# Renamed from dmo_wicked_ui_min, which built a curated 112-source subset from the
# days when this was a UI-only sample. It is the macOS client now and it builds the
# SAME 519-source list as the Windows client, from the one declaration in
# cmake/DmoClientLayers.cmake.
#
# Deliberately the same target name as the client project's library: they are the
# same library. If the two CMake projects are ever merged, CMake will refuse the
# duplicate target -- the correct outcome, because it forces the merge to be decided
# rather than silently producing two divergent copies again.
add_library(dmo_wicked_core STATIC ${DMO_CLIENT_PORTABLE_SOURCES})

# The include dirs travel WITH the source list. Two sufficed for the old subset; the
# full portable layer needs all 57.
# ⚠ PRIVATE, NOT PUBLIC. The app target links this library, and PUBLIC include dirs
# would put Source/Support -- the SHIM -- on the app's include path, making its
# WickedEngine.h resolve to the shim instead of the real engine. That would silently
# convert the entire app layer into stub calls. The original curated target used
# PRIVATE for precisely this reason; converging the source list must not lose it.
target_include_directories(dmo_wicked_core PRIVATE ${DMO_CLIENT_PORTABLE_INCLUDE_DIRS})

# Third-party include roots the portable layer needs. The client project supplies
# these through its own vendoring targets; the engine project has no knowledge of
# them, so they are named explicitly here.
#
# ⚠ This list is the price of two CMake projects. Each new vendored dependency the
# portable layer picks up must be added here as well, and forgetting shows up as a
# macOS-only "file not found". Converging the two PROJECTS (not just the source
# list) is what finally removes this hazard -- see the debt note in the layers file.
target_include_directories(dmo_wicked_core PRIVATE
    ${DMO_CLIENT_ROOT}/external/spz
)

# ⚠ FetchContent DEPENDENCIES CANNOT BE SHARED ACROSS TWO CMAKE PROJECTS. yojimbo
# and netcode are fetched into the CLIENT project's build tree, so the engine
# project can only reach them by path -- which means the client must be configured
# first. This is the sharpest remaining edge of having two projects, and it is the
# thing that finally justifies merging them (see cmake/DmoClientLayers.cmake).
#
# If the path is absent we do NOT fail the configure: the two networking
# translation units simply will not compile, and the error names the missing
# header. Silently dropping them would give macOS a quietly smaller client than
# Windows, which is the exact class of divergence this convergence exists to end.
set(DMO_CLIENT_YOJIMBO_SRC "${DMO_CLIENT_ROOT}/build/_deps/dmo_wicked_yojimbo-src"
    CACHE PATH "yojimbo sources fetched by the client project's CMake configure")
if(EXISTS "${DMO_CLIENT_YOJIMBO_SRC}/netcode/netcode.h")
    target_include_directories(dmo_wicked_core PRIVATE
        ${DMO_CLIENT_YOJIMBO_SRC}
        ${DMO_CLIENT_YOJIMBO_SRC}/include     # yojimbo.h
        ${DMO_CLIENT_YOJIMBO_SRC}/netcode
        ${DMO_CLIENT_YOJIMBO_SRC}/reliable
        ${DMO_CLIENT_YOJIMBO_SRC}/serialize
        ${DMO_CLIENT_YOJIMBO_SRC}/sodium
    )
    target_compile_definitions(dmo_wicked_core PRIVATE NETCODE_RELEASE)
else()
    message(WARNING
        "DmoClientMac: yojimbo sources not found at ${DMO_CLIENT_YOJIMBO_SRC}. "
        "Configure the client project first (cmake -S . -B build) so FetchContent "
        "populates them, or the networking translation units will not compile.")
endif()

# ⚠ The engine project disables exceptions globally (-fno-exceptions in
# external/WickedEngine/CMakeLists.txt). The portable client layer uses try/throw in a
# few validation paths, so it needs exceptions back for ITS translation units only --
# PRIVATE, so the engine and the app target keep the engine's setting.
#
# This single flag is what the previous convergence attempt died on: it stopped at
# WickedTerrainProjection.cpp's `throw` and the cause looked like a source problem
# rather than a build-configuration one.
if(NOT MSVC)
    # RTTI as well as exceptions: the GodClient feature registry uses dynamic_cast.
    target_compile_options(dmo_wicked_core PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-frtti>)
endif()

target_compile_definitions(dmo_wicked_core PRIVATE DMO_WICKED_SHIM_INCLUDED=1)
set_target_properties(dmo_wicked_core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

set(SOURCE_FILES
    main.mm
    ${DMO_CLIENT_RENDER_PATH_SOURCES}
    ${DMO_CLIENT_APP_LAYER_SOURCES}
)


# macOS runtime shader compiler is the arm64 .dylib (the .so is Linux x86-64).
# wiShaderCompiler dlopens "./libdxcompiler.dylib" relative to the run cwd.
set(LIB_DXCOMPILER "libdxcompiler.dylib")

add_executable(DmoClientMac ${SOURCE_FILES})

add_executable(DmoClientMacGodClientPanelProbe
    ${DMO_CLIENT_ROOT}/Tests/WickedGodClientPanelWindowFactorySmoke.cpp
)

# ARC + Obj-C++ for the AppKit window shell.
set_source_files_properties(main.mm PROPERTIES
    COMPILE_FLAGS "-fobjc-arc"
)

# WICKED-MOVE-06I: the engine project sets CMAKE_CXX_STANDARD 17 globally, which the
# app target inherits -- and Support/WickedEngineCompat.h uses a `requires` expression
# (C++20). The Windows target already compiles the same headers at cxx_std_20, so the
# two platforms were silently on different standards. Align them.
set_target_properties(DmoClientMac PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

target_link_libraries(DmoClientMac PUBLIC
    WickedEngine
    dmo_wicked_core
    "-framework AppKit"
    "-framework Carbon"
)
if(DMO_WICKED_ENABLE_TEST_ENV)
    target_compile_definitions(DmoClientMac PRIVATE DMO_WICKED_ENABLE_TEST_ENV=1)
endif()

target_link_libraries(DmoClientMacGodClientPanelProbe PUBLIC
    WickedEngine
)

# Only the ENGINE-NEUTRAL POD headers (WickedUIVisualSink.h / WickedDemoScreen.h)
# are included by the app; NOT Source/Support (shim), so the app's own
# "WickedEngine.h" resolves to the real engine, not the shim.
target_include_directories(DmoClientMac PRIVATE
    ${DMO_CLIENT_ROOT}/Source
    # App-layer GLTF import compiles Editor/ModelImporter_GLTF.cpp and
    # Source/Client/WickedGltfSceneImporter.cpp (#include "ModelImporter.h").
    # Windows dmo_client_ui already adds this path; omitting it breaks Mac only.
    ${DMO_CLIENT_ROOT}/external/WickedEngine/Editor
    # The shared render path still lives in this directory (see the debt note in
    # cmake/DmoClientLayers.cmake). App-layer sources compiled from Source/Client
    # include "DmoClientRenderPath.h", so the dir must be a real -I, not just the
    # target's own source dir. The Windows target already did this.
    "${DMO_CLIENT_RENDER_PATH_DIR}"
)

target_include_directories(DmoClientMacGodClientPanelProbe PRIVATE
    ${DMO_CLIENT_ROOT}/Source
)

if(WICKED_ENABLE_IPO)
    set_target_properties(DmoClientMac PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ON
        INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
    )
endif()

# Bring the runtime shader toolchain dylibs next to the executable so wiLoadLibrary
# (bare-name dlopen) resolves them: the HLSL compiler (dxc) AND Apple's Metal
# Shader Converter (metal_irconverter), which turns DXIL into a valid .metallib.
# Without the converter, Release builds skip conversion (assert compiled out) and
# Metal rejects raw DXIL with "Invalid library file".
set(LIBDXCOMPILER_PATH "${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER}")
set(LIBMETALIR_PATH "${WICKED_ROOT_DIR}/WickedEngine/libmetalirconverter.dylib")
foreach(RUNTIME_DYLIB "${LIBDXCOMPILER_PATH}" "${LIBMETALIR_PATH}")
    if(EXISTS ${RUNTIME_DYLIB})
        add_custom_command(
            TARGET DmoClientMac POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUNTIME_DYLIB} ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()
endforeach()
