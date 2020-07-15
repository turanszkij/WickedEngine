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

using namespace std;
using namespace wiGraphics;

namespace wiProfiler
{
	bool ENABLED = false;
	bool initialized = false;
	std::mutex lock;
	range_id cpu_frame;
	range_id gpu_frame;

	struct Range
	{
		std::string name;
		float times[20] = {};
		int avg_counter = 0;
		float time = 0;
		CommandList cmd = COMMANDLIST_COUNT;

		wiTimer cpuBegin, cpuEnd;

		wiRenderer::GPUQueryRing<wiGraphics::GraphicsDevice::GetBackBufferCount() + 3> gpuBegin;
		wiRenderer::GPUQueryRing<wiGraphics::GraphicsDevice::GetBackBufferCount() + 3> gpuEnd;

		bool IsCPURange() const { return cmd == COMMANDLIST_COUNT; }
	};
	std::unordered_map<size_t, Range> ranges;
	wiRenderer::GPUQueryRing<wiGraphics::GraphicsDevice::GetBackBufferCount() + 3> disjoint;

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

		CommandList cmd = wiRenderer::GetDevice()->BeginCommandList(); // it would be a good idea to not start a new command list just for these couple of queries!
		wiRenderer::GetDevice()->QueryBegin(disjoint.Get_GPU(), cmd);
		wiRenderer::GetDevice()->QueryEnd(disjoint.Get_GPU(), cmd); // this should be at the end of frame, but the problem is that there will be other command lists submitted in between and it doesn't work that way in DX11

		cpu_frame = BeginRangeCPU("CPU Frame");
		gpu_frame = BeginRangeGPU("GPU Frame", cmd);
	}
	void EndFrame(CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return;

		// note: read the GPU Frame end range manually because it will be on a separate command list than start point:
		wiRenderer::GetDevice()->QueryEnd(ranges[gpu_frame].gpuEnd.Get_GPU(), cmd);

		EndRange(cpu_frame);

		GPUQueryResult disjoint_result;
		GPUQuery* disjoint_query = disjoint.Get_CPU();
		if (disjoint_query != nullptr)
		{
			wiRenderer::GetDevice()->QueryRead(disjoint_query, &disjoint_result);
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
				GPUQuery* begin_query = range.gpuBegin.Get_CPU();
				GPUQuery* end_query = range.gpuEnd.Get_CPU();
				GPUQueryResult begin_result, end_result;
				if (begin_query != nullptr && end_query != nullptr)
				{
					wiRenderer::GetDevice()->QueryRead(begin_query, &begin_result);
					wiRenderer::GetDevice()->QueryRead(end_query, &end_result);
				}
				range.time = abs((float)(end_result.result_timestamp - begin_result.result_timestamp) / disjoint_result.result_timestamp_frequency * 1000.0f);
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

			range.cpuBegin.Start();
			range.cpuEnd.Start();

			ranges.insert(make_pair(id, range));
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

			GPUQueryDesc desc;
			desc.Type = GPU_QUERY_TYPE_TIMESTAMP;
			range.gpuBegin.Create(wiRenderer::GetDevice(), &desc);
			range.gpuEnd.Create(wiRenderer::GetDevice(), &desc);

			ranges.insert(make_pair(id, range));
		}

		ranges[id].cmd = cmd;
		wiRenderer::GetDevice()->QueryEnd(ranges[id].gpuBegin.Get_GPU(), cmd);

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
				wiRenderer::GetDevice()->QueryEnd(it->second.gpuEnd.Get_GPU(), it->second.cmd);
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
