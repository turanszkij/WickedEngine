#pragma once
#include "wiEnums.h"

#include <string>

namespace wiProfiler
{
	enum PROFILER_DOMAIN
	{
		DOMAIN_CPU,
		DOMAIN_GPU,
		DOMAIN_COUNT
	};

	void BeginFrame();
	void EndFrame();
	void BeginRange(const std::string& name, PROFILER_DOMAIN domain, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);
	void EndRange(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);

	// Renders a basic text of the Profiling results to the (x,y) screen coordinate
	void DrawData(int x, int y, GRAPHICSTHREAD threadID);

	void SetEnabled(bool value);
};

