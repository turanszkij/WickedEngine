#include "wiProfiler.h"
#include "wiGraphicsDevice.h"
#include "wiRenderer.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiTimer.h"
#include "wiTextureHelper.h"
#include "wiHelper.h"

#include <sstream>
#include <unordered_map>
#include <stack>
#include <mutex>
#include <atomic>

using namespace std;
using namespace wiGraphics;

namespace wiProfiler
{
	bool ENABLED = false;
	bool initialized = false;
	std::mutex lock;
	range_id cpu_frame;
	range_id gpu_frame;
	GPUQueryHeap queryHeap[wiGraphics::GraphicsDevice::GetBackBufferCount() + 1];
	std::vector<uint64_t> queryResults;
	std::atomic<uint32_t> nextQuery{ 0 };
	uint32_t writtenQueries[arraysize(queryHeap)] = {};
	int queryheap_idx = 0;

	struct Range
	{
		std::string name;
		float times[20] = {};
		int avg_counter = 0;
		float time = 0;
		CommandList cmd = COMMANDLIST_COUNT;

		wiTimer cpuBegin, cpuEnd;

		int gpuBegin[arraysize(queryHeap)];
		int gpuEnd[arraysize(queryHeap)];

		bool IsCPURange() const { return cmd == COMMANDLIST_COUNT; }
	};
	std::unordered_map<size_t, Range> ranges;

	void BeginFrame()
	{
		if (!ENABLED)
			return;

		if (!initialized)
		{
			initialized = true;

			ranges.reserve(100);

			GPUQueryHeapDesc desc;
			desc.type = GPU_QUERY_TYPE_TIMESTAMP;
			desc.queryCount = 1024;
			for (int i = 0; i < arraysize(queryHeap); ++i)
			{
				bool success = wiRenderer::GetDevice()->CreateQueryHeap(&desc, &queryHeap[i]);
				assert(success);
			}

			queryResults.resize(desc.queryCount);
		}

		cpu_frame = BeginRangeCPU("CPU Frame");

		CommandList cmd = wiRenderer::GetDevice()->BeginCommandList();
		gpu_frame = BeginRangeGPU("GPU Frame", cmd);
	}
	void EndFrame(CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return;

		GraphicsDevice* device = wiRenderer::GetDevice();

		// note: read the GPU Frame end range manually because it will be on a separate command list than start point:
		auto& gpu_range = ranges[gpu_frame];
		gpu_range.gpuEnd[queryheap_idx] = nextQuery.fetch_add(1);
		device->QueryEnd(&queryHeap[queryheap_idx], gpu_range.gpuEnd[queryheap_idx], cmd);

		EndRange(cpu_frame);

		double gpu_frequency = (double)device->GetTimestampFrequency() / 1000.0;

		device->QueryResolve(&queryHeap[queryheap_idx], 0, nextQuery.load(), cmd);

		writtenQueries[queryheap_idx] = nextQuery.load();
		nextQuery.store(0);
		queryheap_idx = (queryheap_idx + 1) % arraysize(queryHeap);
		if (writtenQueries[queryheap_idx] > 0)
		{
			wiRenderer::GetDevice()->QueryRead(&queryHeap[queryheap_idx], 0, writtenQueries[queryheap_idx], queryResults.data());
		}

		for (auto& x : ranges)
		{
			auto& range = x.second;

			range.time = 0;
			if (range.IsCPURange())
			{
				range.time = (float)abs(range.cpuEnd.elapsed() - range.cpuBegin.elapsed());
			}
			else
			{
				int begin_query = range.gpuBegin[queryheap_idx];
				int end_query = range.gpuEnd[queryheap_idx];
				if (begin_query >= 0 && end_query >= 0)
				{
					uint64_t begin_result = queryResults[begin_query];
					uint64_t end_result = queryResults[end_query];
					range.time = (float)abs((double)(end_result - begin_result) / gpu_frequency);
				}
				range.gpuBegin[queryheap_idx] = -1;
				range.gpuEnd[queryheap_idx] = -1;
			}
			range.times[range.avg_counter++ % arraysize(range.times)] = range.time;

			if (range.avg_counter > arraysize(range.times))
			{
				float avg_time = 0;
				for (int i = 0; i < arraysize(range.times); ++i)
				{
					avg_time += range.times[i];
				}
				range.time = avg_time / arraysize(range.times);
			}
		}
	}

	range_id BeginRangeCPU(const char* name)
	{
		if (!ENABLED || !initialized)
			return 0;

		range_id id = wiHelper::string_hash(name);

		lock.lock();
		if (ranges.find(id) == ranges.end())
		{
			Range range;
			range.name = name;
			range.time = 0;

			ranges[id] = range;
		}

		ranges[id].cpuBegin.record();

		lock.unlock();

		return id;
	}
	range_id BeginRangeGPU(const char* name, CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return 0;

		range_id id = wiHelper::string_hash(name);

		lock.lock();
		if (ranges.find(id) == ranges.end())
		{
			Range range;
			range.name = name;
			range.time = 0;

			std::fill(range.gpuBegin, range.gpuBegin + arraysize(queryHeap), -1);
			std::fill(range.gpuEnd, range.gpuEnd + arraysize(queryHeap), -1);

			ranges[id] = range;
		}

		ranges[id].cmd = cmd;

		ranges[id].gpuBegin[queryheap_idx] = nextQuery.fetch_add(1);
		wiRenderer::GetDevice()->QueryEnd(&queryHeap[queryheap_idx], ranges[id].gpuBegin[queryheap_idx], cmd);

		lock.unlock();

		return id;
	}
	void EndRange(range_id id)
	{
		if (!ENABLED || !initialized)
			return;

		lock.lock();

		auto it = ranges.find(id);
		if (it != ranges.end())
		{
			if (it->second.IsCPURange())
			{
				it->second.cpuEnd.record();
			}
			else
			{
				ranges[id].gpuEnd[queryheap_idx] = nextQuery.fetch_add(1);
				wiRenderer::GetDevice()->QueryEnd(&queryHeap[queryheap_idx], it->second.gpuEnd[queryheap_idx], it->second.cmd);
			}
		}
		else
		{
			assert(0);
		}

		lock.unlock();
	}

	void DrawData(float x, float y, CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return;

		stringstream ss("");
		ss.precision(2);
		ss << "Frame Profiler Ranges:" << endl << "----------------------------" << endl;

		// Print CPU ranges:
		for (auto& x : ranges)
		{
			if (x.second.IsCPURange())
			{
				ss << x.second.name << ": " << fixed << x.second.time << " ms" << endl;
			}
		}
		ss << endl;

		// Print GPU ranges:
		for (auto& x : ranges)
		{
			if (!x.second.IsCPURange())
			{
				ss << x.second.name << ": " << fixed << x.second.time << " ms" << endl;
			}
		}

		wiFontParams params = wiFontParams(x, y, WIFONTSIZE_DEFAULT - 4, WIFALIGN_LEFT, WIFALIGN_TOP, wiColor(255, 255, 255, 255), wiColor(0, 0, 0, 255));

		wiImageParams fx;
		fx.pos.x = (float)params.posX;
		fx.pos.y = (float)params.posY;
		fx.siz.x = (float)wiFont::textWidth(ss.str(), params);
		fx.siz.y = (float)wiFont::textHeight(ss.str(), params);
		fx.color = wiColor(20, 20, 20, 230);
		wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);

		wiFont::Draw(ss.str(), params, cmd);
	}

	void SetEnabled(bool value)
	{
		if (value != ENABLED)
		{
			initialized = false;
			ranges.clear();
			ENABLED = value;
		}
	}

	bool IsEnabled()
	{
		return ENABLED;
	}

}
