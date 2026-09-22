## General notes

- Avoid big refactorings, keep changes minimally invasive and well-reasoned.
- External dependencies should be kept to a minimum, and all libraries should be
  vendored in under the `WickedEngine/Utility` directory. Prefer small single-header libraries
  (eg. stb_truetype over FreeType)
- Try to avoid locks and mutexes in render paths, consider doing the work in the main
  thread only so locks won't be necessary.

## What this is

Wicked Engine: an open-source, single-repo C++17 3D game engine (DirectX 12 / Vulkan / Metal) with a Lua
scripting layer and an ImGui-free, custom-GUI standalone Editor. It's usable as a static/dynamic C++
library, a standalone Editor application, or via Lua scripts. Platforms: Windows, Linux, macOS, iOS, Xbox
Series X|S, PlayStation 5 (console extensions are private/not in this repo).

## Build

### Windows (primary/native workflow)
Open `WickedEngine.sln` in Visual Studio and build/run. Key projects in the solution:
- `WickedEngine_Windows` — the engine static library
- `Editor_Windows` — the standalone Editor app
- `OfflineShaderCompiler` — compiles `.hlsl`/`.hlsli` shaders under `WickedEngine/shaders/` to target
  formats (`hlsl6` for DX12 via dxcompiler, `spirv` for Vulkan, `hlsl6_xs` for Xbox)
- Sample/template projects: `Samples/Template_Windows`, `Samples/Tests`, etc.

Shaders are normally compiled at runtime by the engine on first use (cached to disk). CI instead does an
offline "shader dump" build: build `OfflineShaderCompiler`, run it with `hlsl6 spirv shaderdump
strip_reflection` from inside `WickedEngine/`, then rebuild the Editor — this embeds precompiled shaders
into the binary so no runtime compiler/SDK is needed. Use this flow if you need a build that doesn't
depend on the shader compiler DLLs at runtime.

### CMake (Windows/Linux, also used in CI)
```
cmake -B build
cmake --build build --config Release
```
Useful `-D` options (see top-level `CMakeLists.txt`):
- `WICKED_EDITOR` (default ON) — build the Editor
- `WICKED_TESTS` (default ON) — build `Samples/Tests`
- `WICKED_EMBED_SHADERS` (default NO) — embed shaders into the library instead of compiling/loading at
  runtime (Linux CI builds with this ON)
- `WICKED_ENABLE_RTTI`, `WICKED_ENABLE_IPO`, and (Unix only)
  `WICKED_ENABLE_ASAN`/`WICKED_ENABLE_UBSAN`/`WICKED_ENABLE_TSAN`
- `WICKED_DYNAMIC_LIBRARY` is currently semi-broken and should not be used; Wicked doesn't support
  being built as a DLL

Linux additionally needs SDL2: `sudo apt install libsdl2-dev build-essential`. In-source builds are
rejected — always build into a separate `build/` directory.

### macOS / iOS
Use the `.xcodeproj` files directly: `WickedEngine/WickedEngine.xcodeproj` (static lib),
`Editor/Editor.xcodeproj`, `Samples/Template_MacOS`, `Samples/Template_iOS`.

### Tests
`Samples/Tests` is a small sample/smoke-test application (not a unit test framework) exercising engine
features interactively (`Tests.cpp`, driven by `test_script.lua`); it builds as its own executable via
the `.sln`/CMake and is not run headlessly in CI.

### C++ compiler settings that affect how code must be written
Both MSVC and Clang/GCC builds disable **exceptions** and **RTTI** by default (`/EHsc-` `/GR-` on MSVC,
`-fno-exceptions` `-fno-rtti` on Clang/GCC — see `CMakeLists.txt`). Do not introduce code that throws/
catches exceptions or relies on `dynamic_cast`/RTTI unless `WICKED_ENABLE_RTTI` is explicitly required.
C++ standard is C++17.

### Runtime command-line arguments (not build flags)
The built executables accept plain (no-dash) arguments, e.g. `vulkan` (use Vulkan on Windows),
`debugdevice` (enable graphics validation layer), `gpuvalidation`, `igpu`/`amdgpu`/`nvidiagpu`/`intelgpu`
(GPU vendor preference). See `README.md` for the full list.

## Architecture

### Namespace/module layout
Nearly everything lives under the `wi::` namespace, one subsystem per `wi<Name>.h/.cpp` pair at the top of
`WickedEngine/` (e.g. `wiScene`, `wiRenderer`, `wiAudio`, `wiInput`, `wiPhysics`, `wiJobSystem`). The
umbrella header `WickedEngine/WickedEngine.h` includes all public modules — start there to find the entry
point for any subsystem. A minimal embedding app just includes `WickedEngine.h`, constructs a
`wi::Application`, calls `SetWindow()`, and drives `Run()` in a loop.

### Entity-Component System (`wiECS.h`, `wiScene*`)
The scene graph is a lightweight ECS. `wi::ecs::Entity` is just a `uint64_t` handle (globally unique,
created via `CreateEntity()`); components are plain structs with no base class. `wi::scene::Scene`
(`wiScene.h`/`wiScene_Components.h`) is the top-level container holding `ComponentManager<T>` arrays per
component type (transform, mesh, material, light, etc.) keyed by `Entity`. Scenes can be created
independently of the global scene (`wi::scene::GetScene()`) and merged (`Scene::Merge()`). Serialization
of the whole ECS goes through `wiArchive.h` + `EntitySerializer`, with per-component version numbers — if
you change a component's layout, bump its version and handle backward-compat in its
`Serialize()`/`wiScene_Serializers.cpp` path, since existing `.wiscene` files must keep loading.

### Rendering
- Graphics API abstraction: `wiGraphicsDevice.h` defines the interface; `wiGraphicsDevice_DX12`,
  `_Vulkan`, and `_Metal` are the backend implementations. New rendering features should go through this
  abstraction rather than calling a specific backend API directly, unless the feature is inherently
  backend-specific.
- `wiRenderPath` / `wiRenderPath2D` / `wiRenderPath3D` (+ `wiRenderPath3D_PathTracing`) form the
  pipeline/screen abstraction an application activates via `wi::Application::ActivatePath()` — each
  RenderPath owns its `Start()`/`Update()`/`Render()`/`Compose()` lifecycle and can fade-transition to
  another.
- `wiRenderer.h/.cpp` is the large central module coordinating per-frame scene rendering (culling,
  draw call submission, lighting, post-processing dispatch, etc.).
- Shaders live in `WickedEngine/shaders/` (`.hlsl`/`.hlsli`), written in HLSL and cross-compiled to SPIR-V
  for Vulkan/Metal via dxcompiler. `ShaderInterop*.h` files define CPU/GPU-shared structs/constant buffer
  layouts — keep these in sync manually when changing a shader's inputs, since there is no reflection-
  based auto-binding for these structured buffers.

### Lua scripting
Most public engine classes have a matching `wiXxx_BindLua.h/.cpp` file exposing them to Lua (via the
bundled Lua interpreter, `WickedEngine/LUA/`). When adding a new public API you likely want a symmetric
Lua binding if the class is meant to be scriptable — follow the pattern of an existing `_BindLua` pair.
Scripting API surface is documented in `Content/Documentation/ScriptingAPI-Documentation.md`; C++ API in
`Content/Documentation/WickedEngine-Documentation.md`.

### Third-party code
`WickedEngine/Utility/` vendors third-party single-file/small libraries (DirectXMath, meshoptimizer,
pugixml, stb_*, miniaudio, FAudio, zstd, volk, etc.) directly in-tree rather than via package manager —
don't "clean up" or modify these except for integration fixes; check `third_party_software.txt` for
licensing context before touching them.

### The Editor
`Editor/` is a full application built on top of the engine (not part of the core library), organized as
one `*Window.h/.cpp` pair per inspector panel (e.g. `MaterialWindow`, `AnimationWindow`,
`ComponentsWindow`). It supports importing OBJ/FBX/GLTF/GLB/VRM/VRMA/PLY and exporting to the engine's
native `.wiscene` format.

## Code style
- Tabs for indentation (size 4), UTF-8, CRLF line endings, trailing whitespace trimmed, final newline
  required (see `.editorconfig`; YAML files use 2-space indent).
- Header guards use `#pragma once` in engine code (ECS/interop headers may use explicit `#ifndef` guards
  for other reasons — match the surrounding file).
- Some files have different code styles, maintain the current style when editing a file

