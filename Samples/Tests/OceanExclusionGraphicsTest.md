# Ocean exclusion graphics integration tests

The CPU-only `OceanExclusionTest` does not require a graphics device. The opt-in
`OceanExclusionGraphicsTest` exercises the same GPU and physics cases on each
selected backend, with a real device, shader compilation, dispatch and readback.
It is a compute integration test, not an image comparison or a full render test.

Enable it with `-DWICKED_OCEAN_GPU_TEST=ON` when configuring CMake, build the
`OceanExclusionGraphicsTest` target, then run `ctest -L graphics --output-on-failure`.
By default the registered backends are Vulkan on Linux, DX12 and Vulkan on
Windows, and Metal on Apple platforms. Override the defaults with e.g.
`-DWICKED_OCEAN_GPU_BACKENDS=dx12` or
`-DWICKED_OCEAN_GPU_BACKENDS='dx12;vulkan'` on Windows. Each requested backend
requires a compatible device/driver and the engine's shader compiler runtime;
Metal additionally requires the engine's Metal shader conversion dependencies.
An unavailable backend is not silently skipped: select only backends available
on the machine. To claim cross-backend coverage, run each on an appropriate host.

For a direct invocation use `OceanExclusionGraphicsTest vulkan`, `dx12`, or
`metal` as appropriate. Unknown or unsupported backends return failure.
