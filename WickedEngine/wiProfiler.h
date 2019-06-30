#pragma once
#include "wiGraphicsDevice.h"
#include "wiHashString.h"

#include <string>

namespace wiProfiler
{
	enum PROFILER_DOMAIN
	{
		DOMAIN_CPU,
		DOMAIN_GPU,
		DOMAIN_COUNT
	};
	typedef size_t range_id;

	// Begin collecting profiling data for the current frame
	void BeginFrame(GRAPHICSTHREAD threadID);

	// Finalize collecting profiling data for the current frame
	void EndFrame(GRAPHICSTHREAD threadID);

	// Start a profiling range
	//	If this is a GPU profiling range, you must provide a correct GRAPHICSTHREAD as last parameter!
	range_id BeginRange(const wiHashString& name, PROFILER_DOMAIN domain, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_COUNT);

	// End a profiling range
	void EndRange(range_id id);

	// Renders a basic text of the Profiling results to the (x,y) screen coordinate
	void DrawData(int x, int y, GRAPHICSTHREAD threadID);

	// Enable/disable profiling
	void SetEnabled(bool value);
};

