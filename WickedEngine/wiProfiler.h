#pragma once
#include <string>
#include <unordered_map>
#include <stack>

#include "wiEnums.h"
#include "wiTimer.h"
#include "wiGraphicsResource.h"

class wiProfiler
{
public:
	static wiProfiler& GetInstance() { static wiProfiler profiler; return profiler; }

	enum PROFILER_DOMAIN
	{
		DOMAIN_CPU,
		DOMAIN_GPU,
		DOMAIN_COUNT
	};
	struct Range
	{
		PROFILER_DOMAIN domain;
		std::string name;
		float time;

		wiTimer cpuBegin, cpuEnd;
		wiGraphicsTypes::GPUQuery gpuBegin, gpuEnd;

		Range() :time(0) {}
		~Range() {}
	};

	void BeginFrame();
	void EndFrame();
	void BeginRange(const std::string& name, PROFILER_DOMAIN domain, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);
	void EndRange(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);

	float GetRangeTime(const std::string& name) { return ranges[name]->time; }
	const std::unordered_map<string, Range*>& GetRanges() { return ranges; }

	// Renders a basic text of the Profiling results to the (x,y) screen coordinate
	void DrawData(int x, int y, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);

	bool ENABLED;

private:
	wiProfiler();
	~wiProfiler();

	std::unordered_map<std::string, Range*> ranges;
	std::stack<std::string> rangeStack;
	wiGraphicsTypes::GPUQuery disjoint;
};

