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
using namespace wiGraphics;

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

		wiRenderer::GPUQueryRing<4> gpuBegin;
		wiRenderer::GPUQueryRing<4> gpuEnd;

		Range() :time(0), domain(DOMAIN_CPU) {}
		~Range() {}
	};
	std::unordered_map<std::string, Range*> ranges;
	std::stack<std::string> rangeStack;
	wiRenderer::GPUQueryRing<4> disjoint;

	void BeginFrame()
	{
		if (!ENABLED)
			return;

		if (!initialized)
		{
			initialized = true;

			ranges.reserve(100);

			GPUQueryDesc desc;
			desc.Type = GPU_QUERY_TYPE_TIMESTAMP_DISJOINT;
			disjoint.Create(wiRenderer::GetDevice(), &desc);
		}

		wiRenderer::GetDevice()->QueryBegin(disjoint.Get_GPU(), GRAPHICSTHREAD_IMMEDIATE);
	}
	void EndFrame()
	{
		if (!ENABLED || !initialized)
			return;

		wiRenderer::GetDevice()->QueryEnd(disjoint.Get_GPU(), GRAPHICSTHREAD_IMMEDIATE);

		GPUQueryResult disjoint_result;
		GPUQuery* disjoint_query = disjoint.Get_CPU();
		if (disjoint_query != nullptr)
		{
			while (!wiRenderer::GetDevice()->QueryRead(disjoint_query, &disjoint_result));
		}

		assert(rangeStack.empty() && "There was a range which was not ended!");

		if (disjoint_result.result_disjoint == FALSE)
		{
			for (auto& x : ranges)
			{
				auto& range = x.second;

				range->time = 0;
				switch (range->domain)
				{
				case wiProfiler::DOMAIN_CPU:
					range->time = (float)abs(range->cpuEnd.elapsed() - range->cpuBegin.elapsed());
					break;
				case wiProfiler::DOMAIN_GPU:
					{
						GPUQuery* begin_query = range->gpuBegin.Get_CPU();
						GPUQuery* end_query = range->gpuEnd.Get_CPU();
						GPUQueryResult begin_result, end_result;
						if (begin_query != nullptr && end_query != nullptr)
						{
							while (!wiRenderer::GetDevice()->QueryRead(begin_query, &begin_result));
							while (!wiRenderer::GetDevice()->QueryRead(end_query, &end_result));
						}
						range->time = abs((float)(end_result.result_timestamp - begin_result.result_timestamp) / disjoint_result.result_timestamp_frequency * 1000.0f);
					}
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
				desc.Type = GPU_QUERY_TYPE_TIMESTAMP;
				range->gpuBegin.Create(wiRenderer::GetDevice(), &desc);
				range->gpuEnd.Create(wiRenderer::GetDevice(), &desc);
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
			wiRenderer::GetDevice()->QueryEnd(ranges[name]->gpuBegin.Get_GPU(), threadID);
			break;
		default:
			assert(0);
			break;
		}

		rangeStack.push(name);
	}
	void EndRange(GRAPHICSTHREAD threadID)
	{
		if (!ENABLED || !initialized || rangeStack.empty())
			return;

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
				wiRenderer::GetDevice()->QueryEnd(it->second->gpuEnd.Get_GPU(), threadID);
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
