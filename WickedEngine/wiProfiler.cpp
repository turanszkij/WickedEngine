#include "wiProfiler.h"
#include "wiGraphicsDevice.h"
#include "wiRenderer.h"
#include "wiFont.h"
#include "wiTimer.h"
#include "wiGraphicsResource.h"

#include <sstream>
#include <unordered_map>
#include <stack>

using namespace std;
using namespace wiGraphicsTypes;

namespace wiProfiler
{
	bool ENABLED = false;
	bool initialized = false;

	struct Range
	{
		PROFILER_DOMAIN domain;
		std::string name;
		float time;

		wiTimer cpuBegin, cpuEnd;
		wiGraphicsTypes::GPUQuery gpuBegin, gpuEnd;

		Range() :time(0), domain(DOMAIN_CPU) {}
		~Range() {}
	};
	std::unordered_map<std::string, Range*> ranges;
	std::stack<std::string> rangeStack;
	wiGraphicsTypes::GPUQuery disjoint;

	void BeginFrame()
	{
		if (!ENABLED)
			return;

		if (!initialized)
		{
			initialized = true;

			ranges.reserve(100);

			GPUQueryDesc desc;
			desc.async_latency = 4;
			desc.MiscFlags = 0;
			desc.Type = GPU_QUERY_TYPE_TIMESTAMP_DISJOINT;
			wiRenderer::GetDevice()->CreateQuery(&desc, &disjoint);
		}

		wiRenderer::GetDevice()->QueryBegin(&disjoint, GRAPHICSTHREAD_IMMEDIATE);
	}
	void EndFrame()
	{
		if (!ENABLED || !initialized)
			return;

		wiRenderer::GetDevice()->QueryEnd(&disjoint, GRAPHICSTHREAD_IMMEDIATE);
		while (!wiRenderer::GetDevice()->QueryRead(&disjoint, GRAPHICSTHREAD_IMMEDIATE));

		assert(rangeStack.empty() && "There was a range which was not ended!");

		if (disjoint.result_disjoint == FALSE)
		{
			for (auto& x : ranges)
			{
				x.second->time = 0;
				switch (x.second->domain)
				{
				case wiProfiler::DOMAIN_CPU:
					x.second->time = (float)abs(x.second->cpuEnd.elapsed() - x.second->cpuBegin.elapsed());
					break;
				case wiProfiler::DOMAIN_GPU:
					while (!wiRenderer::GetDevice()->QueryRead(&x.second->gpuBegin, GRAPHICSTHREAD_IMMEDIATE));
					while (!wiRenderer::GetDevice()->QueryRead(&x.second->gpuEnd, GRAPHICSTHREAD_IMMEDIATE));
					x.second->time = abs((float)(x.second->gpuEnd.result_timestamp - x.second->gpuBegin.result_timestamp) / disjoint.result_timestamp_frequency * 1000.0f);
					break;
				default:
					assert(0);
					break;
				}
			}
		}
	}

	void BeginRange(const std::string& name, PROFILER_DOMAIN domain, GRAPHICSTHREAD threadID)
	{
		if (!ENABLED || !initialized)
			return;

		if (ranges.find(name) == ranges.end())
		{
			Range* range = new Range;
			range->name = name;
			range->domain = domain;
			range->time = 0;

			switch (domain)
			{
			case wiProfiler::DOMAIN_CPU:
				range->cpuBegin.Start();
				range->cpuEnd.Start();
				break;
			case wiProfiler::DOMAIN_GPU:
			{
				GPUQueryDesc desc;
				desc.async_latency = 4;
				desc.MiscFlags = 0;
				desc.Type = GPU_QUERY_TYPE_TIMESTAMP;
				wiRenderer::GetDevice()->CreateQuery(&desc, &range->gpuBegin);
				wiRenderer::GetDevice()->CreateQuery(&desc, &range->gpuEnd);
			}
			break;
			default:
				assert(0);
				break;
			}


			ranges.insert(make_pair(name, range));
		}

		switch (domain)
		{
		case wiProfiler::DOMAIN_CPU:
			ranges[name]->cpuBegin.record();
			break;
		case wiProfiler::DOMAIN_GPU:
			wiRenderer::GetDevice()->QueryEnd(&ranges[name]->gpuBegin, threadID);
			break;
		default:
			assert(0);
			break;
		}

		rangeStack.push(name);
	}
	void EndRange(GRAPHICSTHREAD threadID)
	{
		if (!ENABLED || !initialized)
			return;

		assert(!rangeStack.empty() && "There is no range to end!");
		const std::string& top = rangeStack.top();

		auto& it = ranges.find(top);
		if (it != ranges.end())
		{
			switch (it->second->domain)
			{
			case wiProfiler::DOMAIN_CPU:
				it->second->cpuEnd.record();
				break;
			case wiProfiler::DOMAIN_GPU:
				wiRenderer::GetDevice()->QueryEnd(&it->second->gpuEnd, threadID);
				break;
			default:
				assert(0);
				break;
			}
		}
		else
		{
			assert(0);
		}

		rangeStack.pop();
	}

	void DrawData(int x, int y, GRAPHICSTHREAD threadID)
	{
		if (!ENABLED || !initialized)
			return;

		stringstream ss("");
		ss.precision(2);
		ss << "Frame Profiler Ranges:" << endl << "----------------------------" << endl;

		for (int domain = DOMAIN_CPU; domain < DOMAIN_COUNT; ++domain)
		{
			for (auto& x : ranges)
			{
				if (x.second->domain == domain)
				{
					ss << x.first << ": " << fixed << x.second->time << " ms" << endl;
				}
			}
			ss << endl;
		}

		wiFont(ss.str(), wiFontParams(x, y, WIFONTSIZE_DEFAULT, WIFALIGN_LEFT, WIFALIGN_TOP, 0, 0, wiColor(255, 255, 255, 255), wiColor(0, 0, 0, 255))).Draw(threadID);
	}

	void SetEnabled(bool value)
	{
		ENABLED = value;
	}

}
