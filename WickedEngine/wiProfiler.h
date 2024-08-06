#pragma once
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"
#include "wiColor.h"


// QoL macros, allows writing just ScopedXxxProfiling without needing to declare a variable manually
#define ScopedCPUProfiling(name) wi::profiler::ScopedRangeCPU WI_PROFILER_CONCAT(_wi_profiler_cpu_range,__LINE__)(name)
#define ScopedGPUProfiling(name, cmd) wi::profiler::ScopedRangeGPU WI_PROFILER_CONCAT(_wi_profiler_gpu_range,__LINE__)(name, cmd)

// same as ScopedXxxProfiling, just will automatically use the function name as name, should only be used at the beginning of a function
#define ScopedCPUProfilingF ScopedCPUProfiling(__FUNCTION__)
#define ScopedGPUProfilingF(cmd) ScopedGPUProfiling(__FUNCTION__, cmd)

// internal helper macros to make somewhat unique variable names based on line numbers to prevent some compilers
// warning about shadowed variables
#define WI_PROFILER_CONCAT(x,y) WI_PROFILER_CONCAT_INDIRECT(x,y)
#define WI_PROFILER_CONCAT_INDIRECT(x,y) x##y

namespace wi::profiler
{
	typedef size_t range_id;

	// Begin collecting profiling data for the current frame
	void BeginFrame();

	// Finalize collecting profiling data for the current frame
	void EndFrame(wi::graphics::CommandList cmd);

	// Start a CPU profiling range
	range_id BeginRangeCPU(const char* name);

	// Start a GPU profiling range
	range_id BeginRangeGPU(const char* name, wi::graphics::CommandList cmd);

	// End a profiling range
	void EndRange(range_id id);

	// helper using RAII to avoid having to manually call BeginRangeCPU/EndRange at beginning/end
	struct ScopedRangeCPU
	{
		range_id id;
		inline ScopedRangeCPU(const char* name) { id = BeginRangeCPU(name); }
		inline ~ScopedRangeCPU() { EndRange(id); }
	};

	// same for BeginRangeGPU
	struct ScopedRangeGPU
	{
		range_id id;
		inline ScopedRangeGPU(const char* name, wi::graphics::CommandList cmd) { id = BeginRangeGPU(name, cmd); }
		inline ~ScopedRangeGPU() { EndRange(id); }
	};

	// Renders a basic text of the Profiling results to the (x,y) screen coordinate
	void DrawData(
		const wi::Canvas& canvas,
		float x,
		float y,
		wi::graphics::CommandList cmd,
		wi::graphics::ColorSpace colorspace = wi::graphics::ColorSpace::SRGB
	);
	void DisableDrawForThisFrame();

	// Enable/disable profiling
	void SetEnabled(bool value);

	bool IsEnabled();

	void SetBackgroundColor(wi::Color color);
	void SetTextColor(wi::Color color);
};

