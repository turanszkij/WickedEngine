#include "wiProfiler.h"
#include "wiGraphicsDevice.h"
#include "wiRenderer.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiTimer.h"
#include "wiTextureHelper.h"
#include "wiHelper.h"

#include <string>
#include <unordered_map>
#include <stack>
#include <mutex>
#include <atomic>
#include <sstream>

using namespace wiGraphics;

namespace wiProfiler
{
	bool ENABLED = false;
	bool initialized = false;
	std::mutex lock;
	range_id cpu_frame;
	range_id gpu_frame;
	GPUQueryHeap queryHeap[wiGraphics::GraphicsDevice::GetBufferCount() + 1];
	GPUBuffer queryResultBuffer[arraysize(queryHeap)];
	std::atomic<uint32_t> nextQuery{ 0 };
	uint32_t writtenQueries[arraysize(queryHeap)] = {};
	int queryheap_idx = 0;

	struct Range
	{
		bool in_use = false;
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

			GraphicsDevice* device = wiRenderer::GetDevice();

			GPUQueryHeapDesc desc;
			desc.type = GPU_QUERY_TYPE_TIMESTAMP;
			desc.queryCount = 1024;

			GPUBufferDesc bd;
			bd.Usage = USAGE_STAGING;
			bd.ByteWidth = desc.queryCount * sizeof(uint64_t);
			bd.CPUAccessFlags = CPU_ACCESS_READ;

			for (int i = 0; i < arraysize(queryHeap); ++i)
			{
				bool success = device->CreateQueryHeap(&desc, &queryHeap[i]);
				assert(success);

				success = device->CreateBuffer(&bd, nullptr, &queryResultBuffer[i]);
				assert(success);
			}
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

		device->QueryResolve(
			&queryHeap[queryheap_idx],
			0,
			nextQuery.load(),
			&queryResultBuffer[queryheap_idx],
			0ull,
			cmd
		);

		writtenQueries[queryheap_idx] = nextQuery.load();
		nextQuery.store(0);
		queryheap_idx = (queryheap_idx + 1) % arraysize(queryHeap);
		uint64_t* queryResults = nullptr;
		if (writtenQueries[queryheap_idx] > 0)
		{
			Mapping mapping;
			mapping._flags |= Mapping::FLAG_READ;
			mapping.offset = 0;
			mapping.size = sizeof(uint64_t) * writtenQueries[queryheap_idx];
			device->Map(&queryResultBuffer[queryheap_idx], &mapping);
			queryResults = (uint64_t*)mapping.data;
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
				if (queryResults != nullptr && begin_query >= 0 && end_query >= 0)
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

			range.in_use = false;
		}

		if (queryResults != nullptr)
		{
			device->Unmap(&queryResultBuffer[queryheap_idx]);
		}
	}

	range_id BeginRangeCPU(const char* name)
	{
		if (!ENABLED || !initialized)
			return 0;

		range_id id = wiHelper::string_hash(name);

		lock.lock();

		// If one range name is hit multiple times, differentiate between them!
		size_t differentiator = 0;
		while (ranges[id].in_use)
		{
			wiHelper::hash_combine(id, differentiator++);
		}
		ranges[id].in_use = true;
		ranges[id].name = name;

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

		// If one range name is hit multiple times, differentiate between them!
		size_t differentiator = 0;
		while (ranges[id].in_use)
		{
			wiHelper::hash_combine(id, differentiator++);
		}
		ranges[id].in_use = true;
		ranges[id].name = name;

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

	struct Hits
	{
		uint32_t num_hits = 0;
		float total_time = 0;
	};
	std::unordered_map<std::string, Hits> time_cache_cpu;
	std::unordered_map<std::string, Hits> time_cache_gpu;
	void DrawData(const wiCanvas& canvas, float x, float y, CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return;

		wiImage::SetCanvas(canvas, cmd);
		wiFont::SetCanvas(canvas, cmd);

		std::stringstream ss("");
		ss.precision(2);
		ss << "Frame Profiler Ranges:" << std::endl << "----------------------------" << std::endl;


		for (auto& x : ranges)
		{
			if (x.second.IsCPURange())
			{
				if (x.first == cpu_frame)
					continue;
				time_cache_cpu[x.second.name].num_hits++;
				time_cache_cpu[x.second.name].total_time += x.second.time;
			}
			else
			{
				if (x.first == gpu_frame)
					continue;
				time_cache_gpu[x.second.name].num_hits++;
				time_cache_gpu[x.second.name].total_time += x.second.time;
			}
		}

		// Print CPU ranges:
		ss << ranges[cpu_frame].name << ": " << std::fixed << ranges[cpu_frame].time << " ms" << std::endl;
		for (auto& x : time_cache_cpu)
		{
			if (x.second.num_hits > 1)
			{
				ss << "\t" << x.first << " (" << x.second.num_hits << "x)" << ": " << std::fixed << x.second.total_time << " ms" << std::endl;
			}
			else
			{
				ss << "\t" << x.first << ": " << std::fixed << x.second.total_time << " ms" << std::endl;
			}
			x.second.num_hits = 0;
			x.second.total_time = 0;
		}
		ss << std::endl;

		// Print GPU ranges:
		ss << ranges[gpu_frame].name << ": " << std::fixed << ranges[gpu_frame].time << " ms" << std::endl;
		for (auto& x : time_cache_gpu)
		{
			if (x.second.num_hits > 1)
			{
				ss << "\t" << x.first << " (" << x.second.num_hits << "x)" << ": " << std::fixed << x.second.total_time << " ms" << std::endl;
			}
			else
			{
				ss << "\t" << x.first << ": " << std::fixed << x.second.total_time << " ms" << std::endl;
			}
			x.second.num_hits = 0;
			x.second.total_time = 0;
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
