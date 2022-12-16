#pragma once
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"
#include "wiColor.h"

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

