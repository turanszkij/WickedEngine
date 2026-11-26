#include "wiProfiler.h"
#include "wiGraphicsDevice.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiTimer.h"
#include "wiTextureHelper.h"
#include "wiHelper.h"
#include "wiUnorderedMap.h"
#include "wiBacklog.h"
#include "wiRenderer.h"
#include "wiEventHandler.h"
#include "wiInput.h"
#include "wiMath.h"

#if __has_include("Superluminal/PerformanceAPI_capi.h")
#include "Superluminal/PerformanceAPI_capi.h"
#include "Superluminal/PerformanceAPI_loader.h"
#endif // superluminal

#include <string>
#include <stack>
#include <mutex>
#include <atomic>

using namespace wi::graphics;

namespace wi::profiler
{
	bool ENABLED = false;
	bool ENABLED_REQUEST = false;
	bool initialized = false;
	std::mutex lock;
	range_id cpu_frame;
	range_id gpu_frame;
	GPUQueryHeap queryHeap;
	GPUBuffer queryResultBuffer[GraphicsDevice::GetBufferCount()];
	std::atomic<uint32_t> nextQuery{ 0 };
	uint32_t queryheap_idx = 0;
	bool drawn_this_frame = false;
	wi::Color background_color = wi::Color(10, 10, 20, 220);
	wi::Color text_color = wi::Color(130, 210, 220, 255);
	wi::Color tooltip_color = wi::Color(30, 40, 60, 240);

#if PERFORMANCEAPI_ENABLED
	PerformanceAPI_ModuleHandle superluminal_handle = {};
	PerformanceAPI_Functions superluminal_functions = {};
#endif // PERFORMANCEAPI_ENABLED

	struct Range
	{
		bool in_use = false;
		std::string name;
		size_t name_hash = 0;
		float times[20] = {};
		int avg_counter = 0;
		float time = 0;
		CommandList cmd;

		wi::Timer cpuTimer;
		std::chrono::high_resolution_clock::time_point cpuEnd;
		bool cpu_ended = false;

		int gpuBegin[arraysize(queryResultBuffer)];
		int gpuEnd[arraysize(queryResultBuffer)];

		bool IsCPURange() const { return !cmd.IsValid(); }
	};
	wi::unordered_map<size_t, Range> ranges;

	struct TimelineEntry
	{
		range_id id = 0;
		size_t name_hash = 0;
		float begin = 0; // ms relative to frame start
		float end = 0;
		uint32_t thread = 0;
		uint32_t depth = 0;
		float rx = 0, ry = 0, rw = 0, rh = 0;
	};
	struct Timeline
	{
		wi::vector<TimelineEntry> entries;
		float frame_time = 0;
		uint32_t reserved_rows = 0; // only grows, to avoid layout jumping
		void clear()
		{
			entries.clear();
			frame_time = 0;
		}
	};
	Timeline timeline_cpu;
	Timeline timeline_gpu;
	bool timeline_freeze = false;
	float timeline_scale = 16.6f;
	wi::Timer timeline_refresh_timer;
	const double timeline_refresh_interval = 0.5;

	void ComputeTimelineDepths(Timeline& timeline)
	{
		auto& entries = timeline.entries;
		std::sort(entries.begin(), entries.end(), [](const TimelineEntry& a, const TimelineEntry& b) {
			if (a.thread != b.thread)
				return a.thread < b.thread;
			if (a.begin != b.begin)
				return a.begin < b.begin;
			return a.end > b.end; // parent first
			});

		const float eps = 0.0001f;
		static wi::vector<size_t> stack;
		static wi::vector<float> row_end;

		stack.clear();
		row_end.clear();
		for (size_t i = 0; i < entries.size(); ++i)
		{
			TimelineEntry& e = entries[i];
			if (i > 0 && e.thread != entries[i - 1].thread)
			{
				stack.clear();
				row_end.clear();
			}
			while (!stack.empty() && entries[stack.back()].end <= e.begin + eps)
			{
				stack.pop_back();
			}
			uint32_t depth = 0;
			for (size_t s = stack.size(); s > 0; --s)
			{
				const TimelineEntry& parent = entries[stack[s - 1]];
				if (parent.end + eps >= e.end)
				{
					depth = parent.depth + 1;
					break;
				}
			}
			while (depth < row_end.size() && row_end[depth] > e.begin + eps)
			{
				depth++;
			}
			if (depth >= row_end.size())
			{
				row_end.resize(depth + 1, -FLT_MAX);
			}
			row_end[depth] = e.end;
			e.depth = depth;
			stack.push_back(i);
		}

		struct Group
		{
			size_t first = 0;
			size_t count = 0;
			float begin = 0;
			float end = 0;
			uint32_t height = 0;
		};
		static wi::vector<Group> groups;
		groups.clear();
		for (size_t i = 0; i < entries.size(); ++i)
		{
			const TimelineEntry& e = entries[i];
			if (groups.empty() || e.depth == 0 || e.thread != entries[i - 1].thread)
			{
				Group& group = groups.emplace_back();
				group.first = i;
				group.begin = e.begin;
				group.end = e.end;
			}
			Group& group = groups.back();
			group.count++;
			group.end = std::max(group.end, e.end);
			group.height = std::max(group.height, e.depth + 1);
		}
		std::stable_sort(groups.begin(), groups.end(), [](const Group& a, const Group& b) {
			return a.begin < b.begin;
			});

		row_end.clear();
		for (auto& group : groups)
		{
			uint32_t base = 0;
			for (;;)
			{
				bool free = true;
				for (uint32_t r = base; r < base + group.height && r < row_end.size(); ++r)
				{
					if (row_end[r] > group.begin + eps)
					{
						free = false;
						break;
					}
				}
				if (free)
					break;
				base++;
			}
			if (base + group.height > row_end.size())
			{
				row_end.resize(base + group.height, -FLT_MAX);
			}
			for (uint32_t r = base; r < base + group.height; ++r)
			{
				row_end[r] = group.end;
			}
			for (size_t i = group.first; i < group.first + group.count; ++i)
			{
				entries[i].depth += base;
			}
		}
	}

	XMFLOAT4 GetRangeColor(size_t name_hash)
	{
		const float h = float(name_hash % 360) / 60.0f;
		const float s = 0.5f + float((name_hash >> 16) % 20) / 100.0f;
		const float v = 0.85f;
		const float c = v * s;
		const float xx = c * (1 - std::abs(std::fmod(h, 2.0f) - 1));
		const float m = v - c;
		XMFLOAT3 rgb;
		if (h < 1) rgb = XMFLOAT3(c, xx, 0);
		else if (h < 2) rgb = XMFLOAT3(xx, c, 0);
		else if (h < 3) rgb = XMFLOAT3(0, c, xx);
		else if (h < 4) rgb = XMFLOAT3(0, xx, c);
		else if (h < 5) rgb = XMFLOAT3(xx, 0, c);
		else rgb = XMFLOAT3(c, 0, xx);
		return XMFLOAT4(rgb.x + m, rgb.y + m, rgb.z + m, 1);
	}

	struct Stat
	{
		const char* name = nullptr;
		int64_t value = 0;
	};
	wi::vector<Stat> stats;

	void BeginFrame()
	{
		if (ENABLED_REQUEST != ENABLED)
		{
			stats.clear();
			ranges.clear();
			timeline_cpu.clear();
			timeline_gpu.clear();
			timeline_cpu.reserved_rows = 0;
			timeline_gpu.reserved_rows = 0;
			ENABLED = ENABLED_REQUEST;
		}

		if (!ENABLED)
			return;

		if (!initialized)
		{
			initialized = true;

			ranges.reserve(100);

			GraphicsDevice* device = wi::graphics::GetDevice();

			GPUQueryHeapDesc desc;
			desc.type = GpuQueryType::TIMESTAMP;
			desc.query_count = 1024;
			bool success = device->CreateQueryHeap(&desc, &queryHeap);
			assert(success);

			GPUBufferDesc bd;
			bd.usage = Usage::READBACK;
			bd.size = desc.query_count * sizeof(uint64_t);

			for (int i = 0; i < arraysize(queryResultBuffer); ++i)
			{
				success = device->CreateBuffer(&bd, nullptr, &queryResultBuffer[i]);
				assert(success);
			}

#if PERFORMANCEAPI_ENABLED
			superluminal_handle = PerformanceAPI_LoadFrom(L"PerformanceAPI.dll", &superluminal_functions);
			if (superluminal_handle)
			{
				wi::backlog::post("[wi::profiler] Superluminal Performance API loaded");
			}
#endif // PERFORMANCEAPI_ENABLED
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		queryheap_idx = device->GetBufferIndex();

		// Timeline origins must be read before the new "CPU Frame" range begins, because that can reuse the previous frame's range:
		const bool update_timeline = !timeline_freeze && timeline_refresh_timer.elapsed_seconds() >= timeline_refresh_interval;
		if (update_timeline)
		{
			timeline_refresh_timer.record();
		}
		timeline_freeze = false;
		bool cpu_timeline_valid = false;
		std::chrono::high_resolution_clock::time_point cpu_origin;
		if (update_timeline)
		{
			timeline_cpu.clear();
			auto it = ranges.find(cpu_frame);
			if (it != ranges.end() && it->second.cpu_ended)
			{
				cpu_timeline_valid = true;
				cpu_origin = it->second.cpuTimer.timestamp;
				timeline_cpu.frame_time = float(std::chrono::duration<double, std::milli>(it->second.cpuEnd - cpu_origin).count());
			}
		}
		const range_id prev_cpu_frame = cpu_frame;

		const uint64_t* queryResults = (const uint64_t*)queryResultBuffer[queryheap_idx].mapped_data;
		double gpu_frequency = (double)device->GetTimestampFrequency() / 1000.0;
		bool gpu_timeline_valid = false;
		uint64_t gpu_origin = 0;
		if (update_timeline)
		{
			timeline_gpu.clear();
			auto it = ranges.find(gpu_frame);
			if (queryResults != nullptr && it != ranges.end())
			{
				const int begin_idx = it->second.gpuBegin[queryheap_idx];
				const int end_idx = it->second.gpuEnd[queryheap_idx];
				if (begin_idx >= 0 && end_idx >= 0)
				{
					gpu_timeline_valid = true;
					gpu_origin = queryResults[begin_idx];
					timeline_gpu.frame_time = float(double(int64_t(queryResults[end_idx] - gpu_origin)) / gpu_frequency);
				}
			}
		}

		cpu_frame = BeginRangeCPU("CPU Frame");

		CommandList cmd = device->BeginCommandList();

		// Read results of previous timings:
		// This should be done before we begin reallocating new queries for current buffer index
		for (auto& x : ranges)
		{
			auto& range = x.second;
			if (!range.in_use)
				continue;

			if (!range.IsCPURange())
			{
				const int begin_idx = range.gpuBegin[queryheap_idx];
				const int end_idx = range.gpuEnd[queryheap_idx];
				if (queryResults != nullptr && begin_idx >= 0 && end_idx >= 0)
				{
					const uint64_t begin_result = queryResults[begin_idx];
					const uint64_t end_result = queryResults[end_idx];
					range.time = (float)abs((double)(end_result - begin_result) / gpu_frequency);
					if (range.time > 1000000.0f)
					{
						// can happen in Apple TBDR GPU, if no pixel was drawn, timestamp becomes invalid and produce huge values
						range.time = 0;
					}
					else if (gpu_timeline_valid && x.first != gpu_frame)
					{
						TimelineEntry& entry = timeline_gpu.entries.emplace_back();
						entry.id = x.first;
						entry.name_hash = range.name_hash;
						entry.begin = float(double(int64_t(begin_result - gpu_origin)) / gpu_frequency);
						entry.end = float(double(int64_t(end_result - gpu_origin)) / gpu_frequency);
					}
				}
				range.gpuBegin[queryheap_idx] = -1;
				range.gpuEnd[queryheap_idx] = -1;
			}
			else if (cpu_timeline_valid && range.cpu_ended && x.first != prev_cpu_frame && x.first != cpu_frame)
			{
				TimelineEntry& entry = timeline_cpu.entries.emplace_back();
				entry.id = x.first;
				entry.name_hash = range.name_hash;
				entry.begin = float(std::chrono::duration<double, std::milli>(range.cpuTimer.timestamp - cpu_origin).count());
				entry.end = float(std::chrono::duration<double, std::milli>(range.cpuEnd - cpu_origin).count());
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

		device->QueryReset(
			&queryHeap,
			0,
			queryHeap.desc.query_count,
			cmd
		);

		if (update_timeline)
		{
			ComputeTimelineDepths(timeline_cpu);
			ComputeTimelineDepths(timeline_gpu);
		}

		gpu_frame = BeginRangeGPU("GPU Frame", cmd);
		drawn_this_frame = false;
	}
	void EndFrame(CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return;

		GraphicsDevice* device = wi::graphics::GetDevice();

		// note: read the GPU Frame end range manually because it will be on a separate command list than start point:
		auto& gpu_range = ranges[gpu_frame];
		gpu_range.gpuEnd[queryheap_idx] = nextQuery.fetch_add(1);
		device->QueryEnd(&queryHeap, gpu_range.gpuEnd[queryheap_idx], cmd);

		EndRange(cpu_frame);

		device->QueryResolve(
			&queryHeap,
			0,
			nextQuery.load(),
			&queryResultBuffer[queryheap_idx],
			0ull,
			cmd
		);

		nextQuery.store(0);
	}

	range_id BeginRangeCPU(const char* name)
	{
		if (!ENABLED || !initialized)
			return 0;

#if PERFORMANCEAPI_ENABLED
		if (superluminal_handle)
		{
			superluminal_functions.BeginEvent(name, nullptr, 0xFF0000FF);
		}
#endif // PERFORMANCEAPI_ENABLED

		const size_t name_hash = wi::helper::string_hash(name);
		range_id id = name_hash;

		lock.lock();

		// If one range name is hit multiple times, differentiate between them!
		size_t differentiator = 0;
		while (ranges[id].in_use)
		{
			wi::helper::hash_combine(id, differentiator++);
		}
		ranges[id].in_use = true;
		ranges[id].name = name;
		ranges[id].name_hash = name_hash;
		ranges[id].cpu_ended = false;
		ranges[id].cpuTimer.record();

		lock.unlock();

		return id;
	}
	range_id BeginRangeGPU(const char* name, CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return 0;

		const size_t name_hash = wi::helper::string_hash(name);
		range_id id = name_hash;

		lock.lock();

		// If one range name is hit multiple times, differentiate between them!
		size_t differentiator = 0;
		while (ranges[id].in_use)
		{
			wi::helper::hash_combine(id, differentiator++);
		}
		ranges[id].in_use = true;
		ranges[id].name = name;
		ranges[id].name_hash = name_hash;
		ranges[id].cmd = cmd;

		GraphicsDevice* device = wi::graphics::GetDevice();
		ranges[id].gpuBegin[queryheap_idx] = nextQuery.fetch_add(1);
		device->QueryEnd(&queryHeap, ranges[id].gpuBegin[queryheap_idx], cmd);

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
				it->second.cpuEnd = std::chrono::high_resolution_clock::now();
				it->second.cpu_ended = true;
				it->second.time = (float)std::chrono::duration<double, std::milli>(it->second.cpuEnd - it->second.cpuTimer.timestamp).count();

#if PERFORMANCEAPI_ENABLED
				if (superluminal_handle)
				{
					superluminal_functions.EndEvent();
				}
#endif // PERFORMANCEAPI_ENABLED
			}
			else
			{
				GraphicsDevice* device = wi::graphics::GetDevice();
				ranges[id].gpuEnd[queryheap_idx] = nextQuery.fetch_add(1);
				device->QueryEnd(&queryHeap, it->second.gpuEnd[queryheap_idx], it->second.cmd);
			}
		}
		else
		{
			assert(0);
		}

		lock.unlock();
	}

	void AddStat(const char* name, int64_t value)
	{
		if (!ENABLED || !initialized)
			return;

		lock.lock();

		stats.push_back({ name,value });

		lock.unlock();
	}

	PipelineState pso_linestrip;
	PipelineState pso_linelist;
	PipelineState pso_triangles;
	const uint32_t graph_vertex_count = 120;
	float cpu_graph[graph_vertex_count] = {};
	float gpu_graph[graph_vertex_count] = {};
	float cpu_memory_graph[graph_vertex_count] = {};
	float gpu_memory_graph[graph_vertex_count] = {};
	void LoadShaders()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		PipelineStateDesc desc;
		desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
		desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
		desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
		desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
		desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_WIRE_SMOOTH);
		desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
		desc.pt = PrimitiveTopology::LINESTRIP;
		bool success = device->CreatePipelineState(&desc, &pso_linestrip);

		desc.pt = PrimitiveTopology::LINELIST;
		success = device->CreatePipelineState(&desc, &pso_linelist);
		assert(success);

		desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
		desc.pt = PrimitiveTopology::TRIANGLELIST;
		success = device->CreatePipelineState(&desc, &pso_triangles);
		assert(success);
	}

	struct ColorVertex
	{
		XMFLOAT4 position;
		XMFLOAT4 color;
	};
	wi::vector<ColorVertex> rect_vertices;
	void PushRect(float x, float y, float w, float h, const XMFLOAT4& color)
	{
		if (w <= 0 || h <= 0 || color.w <= 0)
			return;
		const ColorVertex verts[] = {
			{ XMFLOAT4(x, y, 0, 1), color },
			{ XMFLOAT4(x + w, y, 0, 1), color },
			{ XMFLOAT4(x, y + h, 0, 1), color },
			{ XMFLOAT4(x, y + h, 0, 1), color },
			{ XMFLOAT4(x + w, y, 0, 1), color },
			{ XMFLOAT4(x + w, y + h, 0, 1), color },
		};
		rect_vertices.insert(rect_vertices.end(), verts, verts + arraysize(verts));
	}
	void PushRectOutline(float x, float y, float w, float h, const XMFLOAT4& color, float t = 1)
	{
		PushRect(x, y, w, t, color);
		PushRect(x, y + h - t, w, t, color);
		PushRect(x, y + t, t, h - t * 2, color);
		PushRect(x + w - t, y + t, t, h - t * 2, color);
	}
	void FlushRects(const wi::Canvas& canvas, CommandList cmd)
	{
		if (rect_vertices.empty())
			return;
		GraphicsDevice* device = wi::graphics::GetDevice();
		const size_t size = sizeof(ColorVertex) * rect_vertices.size();
		GraphicsDevice::GPUAllocation allocation = device->AllocateGPU(size, cmd);
		std::memcpy(allocation.data, rect_vertices.data(), size);

		device->BindPipelineState(&pso_triangles, cmd);
		MiscCB cb;
		cb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		XMStoreFloat4x4(&cb.g_xTransform, canvas.GetProjection());
		device->BindDynamicConstantBuffer(cb, CB_GETBINDSLOT(MiscCB), cmd);
		const GPUBuffer* buffers[] = { &allocation.buffer };
		const uint32_t strides[] = { sizeof(ColorVertex) };
		const uint64_t offsets[] = { allocation.offset };
		device->BindVertexBuffers(buffers, 0, arraysize(buffers), strides, offsets, cmd);
		device->Draw((uint32_t)rect_vertices.size(), 0, cmd);
		rect_vertices.clear();
	}

	struct Hits
	{
		uint32_t num_hits = 0;
		float total_time = 0;
	};
	wi::unordered_map<std::string, Hits> time_cache_cpu;
	wi::unordered_map<std::string, Hits> time_cache_gpu;

	struct ListLine
	{
		size_t name_hash = 0; // 0 for stats
		XMFLOAT4 color = XMFLOAT4(0, 0, 0, 0);
		float fraction = 0;
	};
	struct ListColumn
	{
		const char* header = nullptr;
		char header_value[32] = {};
		wi::vector<ListLine> lines;
		std::string labels_str;
		std::string values_str;
		float x = 0;
		float label_x = 0;
		float value_x = 0;
		float width = 0;
	};
	ListColumn list_columns[3];
	size_t hovered_hash = 0;

	void DrawData(
		const wi::Canvas& canvas,
		float x,
		float y,
		CommandList cmd,
		ColorSpace colorspace,
		float max_width
	)
	{
		if (!ENABLED || !ENABLED_REQUEST || !initialized || drawn_this_frame)
			return;
		drawn_this_frame = true;

		static bool shaders_loaded = false;
		if (!shaders_loaded)
		{
			shaders_loaded = true;
			static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
			LoadShaders();
		}

		const XMFLOAT2 graph_size = XMFLOAT2(190, 100);
		const float graph_left_offset = 45;

		wi::image::SetCanvas(canvas);
		wi::font::SetCanvas(canvas);

		const XMFLOAT4 pointer4 = wi::input::GetPointer();
		const XMFLOAT2 pointer = XMFLOAT2(pointer4.x, pointer4.y);
		auto pointer_inside = [&](float rx, float ry, float rw, float rh) {
			return pointer.x >= rx && pointer.x < rx + rw && pointer.y >= ry && pointer.y < ry + rh;
			};
		hovered_hash = 0;
		const TimelineEntry* hovered_entry = nullptr;

		const XMFLOAT4 text_color4 = text_color.toFloat4();
		auto text_alpha = [&](float alpha) { return XMFLOAT4(text_color4.x, text_color4.y, text_color4.z, alpha); };
		char buf[256];

		for (auto& it : ranges)
		{
			if (!it.second.in_use)
				continue;
			if (it.second.IsCPURange())
			{
				if (it.first == cpu_frame)
					continue;
				time_cache_cpu[it.second.name].num_hits++;
				time_cache_cpu[it.second.name].total_time += it.second.time;
			}
			else
			{
				if (it.first == gpu_frame)
					continue;
				time_cache_gpu[it.second.name].num_hits++;
				time_cache_gpu[it.second.name].total_time += it.second.time;
			}
		}

		for (auto& column : list_columns)
		{
			column.lines.clear();
			column.labels_str.clear();
			column.values_str.clear();
		}
		auto add_ranges = [&](ListColumn& column, const char* header, float frame_time, wi::unordered_map<std::string, Hits>& cache) {
			column.header = header;
			snprintf(column.header_value, sizeof(column.header_value), "%.2f ms", frame_time);

			static wi::vector<std::pair<const std::string*, Hits>> sorted;
			sorted.clear();
			for (auto& it : cache)
			{
				if (it.second.num_hits > 0)
				{
					sorted.push_back({ &it.first, it.second });
				}
				it.second.num_hits = 0;
				it.second.total_time = 0;
			}
			std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
				return a.second.total_time > b.second.total_time;
				});
			for (auto& it : sorted)
			{
				ListLine& line = column.lines.emplace_back();
				line.name_hash = wi::helper::string_hash(it.first->c_str());
				line.color = GetRangeColor(line.name_hash);
				line.fraction = frame_time > 0 ? std::min(1.0f, it.second.total_time / frame_time) : 0.0f;

				column.labels_str += *it.first;
				if (it.second.num_hits > 1)
				{
					snprintf(buf, sizeof(buf), " (%ux)", it.second.num_hits);
					column.labels_str += buf;
				}
				column.labels_str += '\n';
				snprintf(buf, sizeof(buf), "%.2f ms  %4.1f%%\n", it.second.total_time, frame_time > 0 ? it.second.total_time / frame_time * 100.0f : 0.0f);
				column.values_str += buf;
			}
			};
		add_ranges(list_columns[0], "CPU Frame", ranges[cpu_frame].time, time_cache_cpu);
		add_ranges(list_columns[1], "GPU Frame", ranges[gpu_frame].time, time_cache_gpu);
		{
			ListColumn& column = list_columns[2];
			column.header = "Stats";
			column.header_value[0] = 0;
			for (auto& it : stats)
			{
				column.lines.emplace_back();
				column.labels_str += it.name;
				column.labels_str += '\n';
				snprintf(buf, sizeof(buf), "%lld\n", (long long)it.value);
				column.values_str += buf;
			}
			stats.clear();
		}

		wi::font::Params params = wi::font::Params(x, y, wi::font::WIFONTSIZE_DEFAULT - 6, wi::font::WIFALIGN_LEFT, wi::font::WIFALIGN_TOP, text_color);
		wi::font::Params header_params = params;
		header_params.size = params.size + 2;
		header_params.bolden = 0.08f;
		if (colorspace != ColorSpace::SRGB)
		{
			params.enableLinearOutputMapping(9);
			header_params.enableLinearOutputMapping(9);
		}
		const float line_height = wi::font::TextHeight("A\nA", params) - wi::font::TextHeight("A", params);
		const float swatch_size = std::max(4.0f, line_height - 4);
		const float cell_padding = 6;
		const float header_height = wi::font::TextHeight("A", header_params) + cell_padding * 2;
		const float table_spacing = 10;

		float columns_width = 0;
		size_t max_body_lines = 0;
		for (auto& column : list_columns)
		{
			const bool has_swatches = &column != &list_columns[2];
			column.x = x + columns_width;
			column.label_x = column.x + cell_padding + (has_swatches ? swatch_size + 6 : 0);
			column.value_x = column.label_x + wi::font::TextWidth(column.labels_str, params) + 16;
			column.width = column.value_x - column.x + wi::font::TextWidth(column.values_str, params) + cell_padding;
			const float header_width = cell_padding * 2 + wi::font::TextWidth(column.header, header_params) + 24 + wi::font::TextWidth(column.header_value, header_params);
			column.width = std::max(column.width, header_width);
			columns_width += column.width + 16;
			max_body_lines = std::max(max_body_lines, column.lines.size());
		}
		const float list_height = header_height + line_height * max_body_lines + cell_padding * 2;

		const float graphs_width = graph_size.x + 100 + cell_padding * 2;
		const float graph_top_margin = 6;
		const float graph_bottom_margin = 18;
		const float graph_table_height = header_height + cell_padding * 2 + graph_top_margin + graph_size.y + graph_bottom_margin;
		const float graphs_height = graph_table_height * 2 + table_spacing;
		const float time_table_y = y;
		const float memory_table_y = y + graph_table_height + table_spacing;
		const float graph_x = x + cell_padding;
		const float time_graph_y = time_table_y + header_height + cell_padding + graph_top_margin;
		const float memory_graph_y = memory_table_y + header_height + cell_padding + graph_top_margin;

		const float fg_x = x + graphs_width + table_spacing;
		const float fg_y = y;
		const float fg_w = std::max(50.0f, max_width > 0 ? (x + max_width - fg_x) : (canvas.GetLogicalWidth() - fg_x - 20));
		const float row_height = 14;
		const float tl_x = fg_x + cell_padding;
		const float tl_w = std::max(10.0f, fg_w - cell_padding * 2);

		// Lock the time scale to the 60 or 30 FPS budget, with some headroom for vsync jitter:
		const float budget_60fps = 1000.0f / 60.0f;
		const float budget_30fps = 1000.0f / 30.0f;
		const float budget_headroom = 1.06f;
		{
			const float frame_time = std::max(timeline_cpu.frame_time, timeline_gpu.frame_time);
			float target = frame_time * 1.05f;
			if (frame_time <= budget_60fps * budget_headroom)
			{
				target = budget_60fps * budget_headroom;
			}
			else if (frame_time <= budget_30fps * budget_headroom)
			{
				target = budget_30fps * budget_headroom;
			}
			timeline_scale = target > timeline_scale ? target : (timeline_scale + (target - timeline_scale) * 0.05f);
		}
		auto time_to_x = [&](float t) {
			return tl_x + std::max(0.0f, std::min(t, timeline_scale)) / timeline_scale * tl_w;
			};

		struct Section
		{
			const char* title;
			Timeline* timeline;
			float y;
			float height;
		};
		Section sections[] = {
			{ "CPU", &timeline_cpu },
			{ "GPU", &timeline_gpu },
		};
		float cursor_y = fg_y;
		for (auto& section : sections)
		{
			Timeline& timeline = *section.timeline;
			for (auto& e : timeline.entries)
			{
				timeline.reserved_rows = std::max(timeline.reserved_rows, e.depth + 1);
			}
			section.y = cursor_y;
			const float rows_y = section.y + header_height + cell_padding;
			section.height = header_height + cell_padding * 2 + timeline.reserved_rows * row_height;
			cursor_y += section.height + table_spacing;
			for (auto& e : timeline.entries)
			{
				e.rx = time_to_x(e.begin);
				e.rw = std::max(1.0f, time_to_x(e.end) - e.rx);
				e.ry = rows_y + e.depth * row_height;
				e.rh = row_height - 1;
				if (pointer_inside(e.rx, e.ry, e.rw, e.rh))
				{
					hovered_entry = &e;
				}
			}
		}
		const float fg_h = cursor_y - table_spacing - fg_y;
		if (hovered_entry != nullptr)
		{
			hovered_hash = hovered_entry->name_hash;
		}
		timeline_freeze = pointer_inside(fg_x, fg_y, fg_w, fg_h);

		const float list_y = y + std::max(graphs_height, fg_h) + table_spacing;
		const float list_body_y = list_y + header_height + cell_padding;
		for (auto& column : list_columns)
		{
			if (pointer_inside(column.x, list_body_y, column.width, line_height * max_body_lines))
			{
				const size_t idx = size_t((pointer.y - list_body_y) / line_height);
				if (idx < column.lines.size() && column.lines[idx].name_hash != 0)
				{
					hovered_hash = column.lines[idx].name_hash;
				}
			}
		}

		wi::image::Params table_background;
		table_background.color = background_color;
		if (colorspace != ColorSpace::SRGB)
		{
			table_background.enableLinearOutputMapping(9);
		}
		const XMFLOAT4 table_border_color = text_alpha(0.35f);
		auto push_table = [&](float tx, float ty, float tw, float th) {
			if (background_color.getA() > 0)
			{
				table_background.pos = XMFLOAT3(tx, ty, 0);
				table_background.siz = XMFLOAT2(tw, th);
				wi::image::Draw(nullptr, table_background, cmd);
			}
			PushRect(tx, ty, tw, header_height, text_alpha(0.12f));
			PushRect(tx, ty + header_height, tw, th - header_height, text_alpha(0.03f));
			PushRect(tx, ty + header_height - 1, tw, 1, table_border_color);
			PushRectOutline(tx, ty, tw, th, table_border_color);
			};
		push_table(x, time_table_y, graphs_width, graph_table_height);
		push_table(x, memory_table_y, graphs_width, graph_table_height);
		for (auto& column : list_columns)
		{
			push_table(column.x, list_y, column.width, list_height);
			for (size_t i = 0; i < column.lines.size(); ++i)
			{
				const ListLine& line = column.lines[i];
				const float ly = list_body_y + i * line_height;
				if (line.name_hash != 0 && line.name_hash == hovered_hash)
				{
					PushRect(column.x + 1, ly, column.width - 2, line_height, text_alpha(0.15f));
				}
				if (line.color.w > 0)
				{
					const float bar_width = column.x + column.width - cell_padding - column.label_x;
					PushRect(column.label_x, ly + 1, bar_width * line.fraction, line_height - 2, XMFLOAT4(line.color.x, line.color.y, line.color.z, 0.25f));
					PushRect(column.x + cell_padding, ly + (line_height - swatch_size) * 0.5f, swatch_size, swatch_size, line.color);
				}
			}
		}

		float tick_step = 100;
		for (float s : { 0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f })
		{
			if (timeline_scale / s <= std::max(1.0f, tl_w / 70.0f))
			{
				tick_step = s;
				break;
			}
		}
		const float budgets[] = { budget_60fps, budget_30fps };
		const XMFLOAT4 budget_color = XMFLOAT4(1, 0.85f, 0.2f, 0.5f);
		for (auto& section : sections)
		{
			push_table(fg_x, section.y, fg_w, section.height);
			const float body_y = section.y + header_height;
			const float body_h = section.height - header_height - 1;
			for (float t = 0; t <= timeline_scale; t += tick_step)
			{
				PushRect(time_to_x(t), body_y - 4, 1, 3, table_border_color);
				PushRect(time_to_x(t), body_y, 1, body_h, text_alpha(0.12f));
			}
			for (float budget : budgets)
			{
				if (budget < timeline_scale)
				{
					PushRect(time_to_x(budget), body_y - 4, 1, 3, budget_color);
					PushRect(time_to_x(budget), body_y, 1, body_h, budget_color);
				}
			}
			PushRect(tl_x, body_y + cell_padding, tl_w, section.timeline->reserved_rows * row_height, text_alpha(0.04f));
			for (auto& e : section.timeline->entries)
			{
				XMFLOAT4 color = GetRangeColor(e.name_hash);
				const bool highlighted = e.name_hash == hovered_hash;
				if (hovered_hash != 0 && !highlighted)
				{
					color.w = 0.35f;
				}
				PushRect(e.rx, e.ry, e.rw, e.rh, color);
				if (highlighted)
				{
					PushRectOutline(e.rx - 1, e.ry - 1, e.rw + 2, e.rh + 2, text_color4);
				}
			}
			const XMFLOAT4 frame_marker_color = section.timeline == &timeline_cpu ? XMFLOAT4(1, 0.2f, 0.2f, 0.8f) : XMFLOAT4(0.2f, 1, 0.2f, 0.8f);
			PushRect(time_to_x(section.timeline->frame_time), body_y, 2, body_h, frame_marker_color);
		}

		FlushRects(canvas, cmd);

		{
			wi::font::Params p = params;
			p.position.y = list_body_y;
			wi::font::Params hp = header_params;
			hp.position.y = list_y + cell_padding;
			for (auto& column : list_columns)
			{
				p.position.x = column.label_x;
				wi::font::Draw(column.labels_str, p, cmd);
				p.position.x = column.value_x;
				wi::font::Draw(column.values_str, p, cmd);

				hp.h_align = wi::font::WIFALIGN_LEFT;
				hp.position.x = column.x + cell_padding;
				wi::font::Draw(column.header, hp, cmd);
				hp.h_align = wi::font::WIFALIGN_RIGHT;
				hp.position.x = column.x + column.width - cell_padding;
				wi::font::Draw(column.header_value, hp, cmd);
			}
		}
		for (auto& section : sections)
		{
			wi::font::Params hp = header_params;
			hp.position.x = fg_x + cell_padding;
			hp.position.y = section.y + cell_padding;
			wi::font::Draw(section.title, hp, cmd);

			const float labels_left = hp.position.x + wi::font::TextWidth(section.title, hp) + 12;
			float labels_right = fg_x + fg_w - cell_padding;
			wi::font::Params p = params;
			p.size = 10;
			p.v_align = wi::font::WIFALIGN_CENTER;
			p.position.y = section.y + header_height * 0.5f;
			if (timeline_freeze && &section == &sections[0])
			{
				const char* paused_text = "paused while hovered";
				p.h_align = wi::font::WIFALIGN_RIGHT;
				p.position.x = labels_right;
				wi::font::Draw(paused_text, p, cmd);
				labels_right -= wi::font::TextWidth(paused_text, p) + 12;
			}
			p.h_align = wi::font::WIFALIGN_CENTER;
			float budget_label_left[arraysize(budgets)] = {};
			float budget_label_right[arraysize(budgets)] = {};
			int budget_label_count = 0;
			const char* budget_texts[] = { "16.6 ms (60 FPS)", "33.3 ms (30 FPS)" };
			for (int i = 0; i < arraysize(budgets); ++i)
			{
				if (budgets[i] >= timeline_scale)
					continue;
				const float half_width = wi::font::TextWidth(budget_texts[i], p) * 0.5f;
				const float cx = std::min(time_to_x(budgets[i]), labels_right - half_width);
				if (cx - half_width < labels_left)
					continue;
				wi::font::Params bp = p;
				bp.color = wi::Color::fromFloat4(XMFLOAT4(budget_color.x, budget_color.y, budget_color.z, 1));
				bp.position.x = cx;
				wi::font::Draw(budget_texts[i], bp, cmd);
				budget_label_left[budget_label_count] = cx - half_width - 8;
				budget_label_right[budget_label_count] = cx + half_width + 8;
				budget_label_count++;
			}
			for (float t = 0; t <= timeline_scale; t += tick_step)
			{
				snprintf(buf, sizeof(buf), tick_step < 1 ? "%.2f ms" : "%.0f ms", t);
				const float half_width = wi::font::TextWidth(buf, p) * 0.5f;
				const float cx = time_to_x(t);
				bool overlap = cx - half_width < labels_left || cx + half_width > labels_right;
				for (int i = 0; i < budget_label_count; ++i)
				{
					overlap |= cx + half_width > budget_label_left[i] && cx - half_width < budget_label_right[i];
				}
				if (overlap)
					continue;
				p.position.x = cx;
				wi::font::Draw(buf, p, cmd);
			}

			p.color = wi::Color::Black();
			p.size = 9;
			p.h_align = wi::font::WIFALIGN_LEFT;
			p.v_align = wi::font::WIFALIGN_TOP;
			const float char_width = p.size * 0.55f;
			for (auto& e : section.timeline->entries)
			{
				const int max_chars = std::min(int((e.rw - 4) / char_width), int(sizeof(buf)) - 1);
				if (max_chars < 3)
					continue;
				auto it = ranges.find(e.id);
				if (it == ranges.end())
					continue;
				const std::string& name = it->second.name;
				p.position.x = e.rx + 2;
				p.position.y = e.ry + 2;
				if ((int)name.size() <= max_chars)
				{
					wi::font::Draw(name, p, cmd);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%.*s..", max_chars - 2, name.c_str());
					wi::font::Draw(buf, p, cmd);
				}
			}
		}


		// Graph:
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			wi::graphics::GraphicsDevice::MemoryUsage gpu_memory_usage = device->GetMemoryUsage();
			wi::helper::MemoryUsage cpu_memory_usage = wi::helper::GetMemoryUsage();

			float graph_max = 33;
			float graph_max_gpu_memory = 0;
			float graph_max_cpu_memory = 0;
			for (uint32_t i = graph_vertex_count - 1; i > 0; --i)
			{
				cpu_graph[i] = cpu_graph[i - 1];
				gpu_graph[i] = gpu_graph[i - 1];
				cpu_memory_graph[i] = cpu_memory_graph[i - 1];
				gpu_memory_graph[i] = gpu_memory_graph[i - 1];
				graph_max = std::max(graph_max, cpu_graph[i]);
				graph_max = std::max(graph_max, gpu_graph[i]);
				graph_max_gpu_memory = std::max(graph_max_gpu_memory, gpu_memory_graph[i]);
				graph_max_cpu_memory = std::max(graph_max_cpu_memory, cpu_memory_graph[i]);
			}
			cpu_graph[0] = ranges[cpu_frame].time;
			gpu_graph[0] = ranges[gpu_frame].time;
			cpu_memory_graph[0] = float(double(cpu_memory_usage.process_physical) / (1024.0 * 1024.0 * 1024.0)); // Gigabytes
			gpu_memory_graph[0] = float(double(gpu_memory_usage.usage) / (1024.0 * 1024.0 * 1024.0)); // Gigabytes
			graph_max = std::max(graph_max, cpu_graph[0]);
			graph_max = std::max(graph_max, gpu_graph[0]);
			graph_max_gpu_memory = std::max(graph_max_gpu_memory, gpu_memory_graph[0]);
			graph_max_cpu_memory = std::max(graph_max_cpu_memory, cpu_memory_graph[0]);
			const float graph_max_memory = std::max(graph_max_cpu_memory, graph_max_gpu_memory) * 1.1f;

			struct Vertex
			{
				XMFLOAT4 position;
				XMFLOAT4 color;
			};
			const Vertex graph_info[] = {
				// axes:
				{ XMFLOAT4(0, 0, 0, 1), text_color },
				{ XMFLOAT4(0, 1, 0, 1), text_color },
				{ XMFLOAT4(0, 1, 0, 1), text_color },
				{ XMFLOAT4(1, 1, 0, 1), text_color },

				// markers:
				{ XMFLOAT4(0, 0, 0, 1), text_color }, // graph_max
				{ XMFLOAT4(-10.0f / graph_size.x, 0, 0, 1), text_color }, // graph_max
				{ XMFLOAT4(0, 1 - 16.6f / graph_max, 0, 1), text_color }, // 16.6f
				{ XMFLOAT4(-10.0f / graph_size.x, 1 - 16.6f / graph_max, 0, 1), text_color }, // 16.6f
				{ XMFLOAT4(60.0f / float(graph_vertex_count),1,0,1), text_color }, // 60 frames
				{ XMFLOAT4(60.0f / float(graph_vertex_count),1 + 10.0f / graph_size.x,0,1), text_color }, // 60 frames
				{ XMFLOAT4(1,1,0,1), text_color }, // 0 frames
				{ XMFLOAT4(1,1 + 10.0f / graph_size.x,0,1), text_color }, // 0 frames
			};
			const Vertex graph_memory_info[] = {
				// axes:
				{ XMFLOAT4(0, 0, 0, 1), text_color },
				{ XMFLOAT4(0, 1, 0, 1), text_color },
				{ XMFLOAT4(0, 1, 0, 1), text_color },
				{ XMFLOAT4(1, 1, 0, 1), text_color },

				// markers:
				{ XMFLOAT4(0, 0, 0, 1), text_color }, // graph_max_memory
				{ XMFLOAT4(-10.0f / graph_size.x, 0, 0, 1), text_color }, // graph_max_memory
				{ XMFLOAT4(0, 0.5f, 0, 1), text_color }, // half_memory_budget
				{ XMFLOAT4(-10.0f / graph_size.x, 0.5f, 0, 1), text_color }, // half_memory_budget
				{ XMFLOAT4(60.0f / float(graph_vertex_count),1,0,1), text_color }, // 60 frames
				{ XMFLOAT4(60.0f / float(graph_vertex_count),1 + 10.0f / graph_size.x,0,1), text_color }, // 60 frames
				{ XMFLOAT4(1,1,0,1), text_color }, // 0 frames
				{ XMFLOAT4(1,1 + 10.0f / graph_size.x,0,1), text_color }, // 0 frames
			};

			GraphicsDevice::GPUAllocation allocation = device->AllocateGPU(sizeof(Vertex) * graph_vertex_count * 4 + sizeof(graph_info) + sizeof(graph_memory_info), cmd);

			for (uint32_t i = 0; i < graph_vertex_count; ++i)
			{
				Vertex vert;
				vert.color = XMFLOAT4(1, 0.2f, 0.2f, 1);
				vert.position = XMFLOAT4(float(graph_vertex_count - 1 - i) / float(graph_vertex_count), 1 - cpu_graph[i] / graph_max, 0, 1);
				std::memcpy((Vertex*)allocation.data + i, &vert, sizeof(vert));

				vert.color = XMFLOAT4(0.2f, 1, 0.2f, 1);
				vert.position = XMFLOAT4(float(graph_vertex_count - 1 - i) / float(graph_vertex_count), 1 - gpu_graph[i] / graph_max, 0, 1);
				std::memcpy((Vertex*)allocation.data + graph_vertex_count + i, &vert, sizeof(vert));

				vert.color = XMFLOAT4(1, 1, 0.2f, 1);
				vert.position = XMFLOAT4(float(graph_vertex_count - 1 - i) / float(graph_vertex_count), 1 - gpu_memory_graph[i] / graph_max_memory, 0, 1);
				std::memcpy((Vertex*)allocation.data + graph_vertex_count * 2 + i, &vert, sizeof(vert));

				vert.color = XMFLOAT4(1, 0.2f, 1, 1);
				vert.position = XMFLOAT4(float(graph_vertex_count - 1 - i) / float(graph_vertex_count), 1 - cpu_memory_graph[i] / graph_max_memory, 0, 1);
				std::memcpy((Vertex*)allocation.data + graph_vertex_count * 3 + i, &vert, sizeof(vert));
			}
			std::memcpy((Vertex*)allocation.data + graph_vertex_count * 4, graph_info, sizeof(graph_info));
			std::memcpy((Vertex*)allocation.data + graph_vertex_count * 4 + sizeof(graph_info) / sizeof(Vertex), graph_memory_info, sizeof(graph_memory_info));

			device->BindPipelineState(&pso_linestrip, cmd);

			MiscCB cb;
			cb.g_xColor = XMFLOAT4(1, 1, 1, 1);
			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixScaling(graph_size.x - graph_left_offset, graph_size.y, 1) *
				XMMatrixTranslation(graph_x + graph_left_offset, time_graph_y, 0) *
				canvas.GetProjection()
			);
			device->BindDynamicConstantBuffer(cb, CB_GETBINDSLOT(MiscCB), cmd);

			const GPUBuffer* buffers[] = {
				&allocation.buffer
			};
			const uint32_t strides[] = {
				sizeof(Vertex)
			};
			const uint64_t offsets[] = {
				allocation.offset
			};
			device->BindVertexBuffers(buffers, 0, arraysize(buffers), strides, offsets, cmd);

			device->Draw(graph_vertex_count, 0, cmd);
			device->Draw(graph_vertex_count, graph_vertex_count, cmd);

			device->BindPipelineState(&pso_linelist, cmd);
			device->Draw(sizeof(graph_info) / sizeof(Vertex), graph_vertex_count * 4, cmd);

			// Memory graph:
			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixScaling(graph_size.x - graph_left_offset, graph_size.y, 1) *
				XMMatrixTranslation(graph_x + graph_left_offset, memory_graph_y, 0) *
				canvas.GetProjection()
			);
			device->BindDynamicConstantBuffer(cb, CB_GETBINDSLOT(MiscCB), cmd);
			device->BindPipelineState(&pso_linestrip, cmd);
			device->Draw(graph_vertex_count, graph_vertex_count * 2, cmd);
			device->Draw(graph_vertex_count, graph_vertex_count * 3, cmd);
			device->BindPipelineState(&pso_linelist, cmd);
			device->Draw(sizeof(graph_memory_info) / sizeof(Vertex), graph_vertex_count * 4 + sizeof(graph_info) / sizeof(Vertex), cmd);

			wi::font::Params params;
			params.size = 10;
			params.v_align = wi::font::WIFALIGN_CENTER;
			params.color = text_color;
			params.position.x = graph_x;
			params.position.y = time_graph_y;
			snprintf(buf, sizeof(buf), "%.1f ms", graph_max);
			wi::font::Draw(buf, params, cmd);
			params.position.y = time_graph_y + graph_size.y - (16.6f / graph_max) * graph_size.y;
			wi::font::Draw("16.6 ms", params, cmd);

			params.position.x = graph_x + graph_left_offset - 40;
			params.position.y = memory_graph_y;
			snprintf(buf, sizeof(buf), "%.1f GB", graph_max_memory);
			wi::font::Draw(buf, params, cmd);
			params.position.y = memory_graph_y + graph_size.y - 0.5f * graph_size.y;
			snprintf(buf, sizeof(buf), "%.1f GB", graph_max_memory * 0.5f);
			wi::font::Draw(buf, params, cmd);

			params.position.x = graph_x + graph_size.x + 5;
			params.position.y = time_graph_y + graph_size.y - cpu_graph[0] / graph_max * graph_size.y;
			params.color = wi::Color::fromFloat4(XMFLOAT4(1, 0.2f, 0.2f, 1));
			snprintf(buf, sizeof(buf), "cpu: %.1f ms", cpu_graph[0]);
			wi::font::Draw(buf, params, cmd);

			float cpu_position = params.position.y;
			params.position.x = graph_x + graph_size.x + 5;
			params.position.y = time_graph_y + graph_size.y - gpu_graph[0] / graph_max * graph_size.y;
			if (std::abs(params.position.y - cpu_position) < params.size)
			{
				if (params.position.y < cpu_position)
				{
					params.position.y = cpu_position - params.size;
				}
				else
				{
					params.position.y = cpu_position + params.size;
				}
			}
			params.color = wi::Color::fromFloat4(XMFLOAT4(0.2f, 1, 0.2f, 1));
			snprintf(buf, sizeof(buf), "gpu: %.1f ms", gpu_graph[0]);
			wi::font::Draw(buf, params, cmd);

			params.position.x = graph_x + graph_left_offset + graph_size.x - graph_left_offset + 5;
			params.position.y = memory_graph_y + graph_size.y - cpu_memory_graph[0] / graph_max_memory * graph_size.y;
			params.color = wi::Color::fromFloat4(XMFLOAT4(1, 0.2f, 1, 1));
			snprintf(buf, sizeof(buf), "RAM: %.1f GB", cpu_memory_graph[0]);
			wi::font::Draw(buf, params, cmd);

			cpu_position = params.position.y;
			params.position.y = memory_graph_y + graph_size.y - gpu_memory_graph[0] / graph_max_memory * graph_size.y;
			if (std::abs(params.position.y - cpu_position) < params.size)
			{
				if (params.position.y < cpu_position)
				{
					params.position.y = cpu_position - params.size;
				}
				else
				{
					params.position.y = cpu_position + params.size;
				}
			}
			params.color = wi::Color::fromFloat4(XMFLOAT4(1, 1, 0.2f, 1));
			snprintf(buf, sizeof(buf), "VRAM: %.1f GB", gpu_memory_graph[0]);
			wi::font::Draw(buf, params, cmd);

			params.h_align = wi::font::WIFALIGN_CENTER;
			params.color = text_color;
			params.position.x = graph_x + graph_left_offset + (graph_size.x - graph_left_offset) * 0.5f;
			params.position.y = time_graph_y + graph_size.y + 10;
			wi::font::Draw("60 frames", params, cmd);
			params.position.x = graph_x + graph_size.x;
			wi::font::Draw("current frame", params, cmd);

			// Memory graph:
			params.position.x = graph_x + graph_left_offset + (graph_size.x - graph_left_offset) * 0.5f;
			params.position.y = memory_graph_y + graph_size.y + 10;
			wi::font::Draw("60 frames", params, cmd);
			params.position.x = graph_x + graph_left_offset + graph_size.x - graph_left_offset;
			wi::font::Draw("current frame", params, cmd);

			wi::font::Params hp = header_params;
			hp.position.x = x + cell_padding;
			hp.position.y = time_table_y + cell_padding;
			wi::font::Draw("CPU / GPU time", hp, cmd);
			hp.position.y = memory_table_y + cell_padding;
			wi::font::Draw("RAM / VRAM", hp, cmd);
		}

		if (hovered_entry != nullptr)
		{
			auto it = ranges.find(hovered_entry->id);
			snprintf(buf, sizeof(buf), "%s\nduration: %.3f ms\nstart: %.3f ms\nend: %.3f ms\ndepth: %u",
				it != ranges.end() ? it->second.name.c_str() : "",
				hovered_entry->end - hovered_entry->begin,
				hovered_entry->begin,
				hovered_entry->end,
				hovered_entry->depth
			);
			wi::font::Params p = params;
			p.size = 12;
			const XMFLOAT2 text_size = wi::font::TextSize(buf, p);
			p.position.x = pointer.x + 16;
			p.position.y = pointer.y + 16;
			const float right_limit = std::min(canvas.GetLogicalWidth(), fg_x + fg_w);
			if (p.position.x + text_size.x + 4 > right_limit)
			{
				p.position.x = std::max(x + 4, pointer.x - text_size.x - 16);
			}
			if (p.position.y + text_size.y + 8 > canvas.GetLogicalHeight())
			{
				p.position.y = pointer.y - text_size.y - 16;
			}
			wi::image::Params tooltip_bg;
			tooltip_bg.pos.x = p.position.x - 4;
			tooltip_bg.pos.y = p.position.y - 4;
			tooltip_bg.siz.x = text_size.x + 8;
			tooltip_bg.siz.y = text_size.y + 8;
			tooltip_bg.color = tooltip_color;
			if (colorspace != ColorSpace::SRGB)
			{
				tooltip_bg.enableLinearOutputMapping(9);
			}
			wi::image::Draw(nullptr, tooltip_bg, cmd);
			p.color = text_color;
			wi::font::Draw(buf, p, cmd);
		}
	}
	void DisableDrawForThisFrame()
	{
		drawn_this_frame = true;
	}

	void SetEnabled(bool value)
	{
		// Don't enable/disable the profiler immediately, only on the next frame
		//	to avoid enabling inside a Begin/End by mistake
		ENABLED_REQUEST = value;
	}

	bool IsEnabled()
	{
		return ENABLED;
	}

	void SetBackgroundColor(wi::Color color)
	{
		background_color = color;
	}
	void SetTextColor(wi::Color color)
	{
		text_color = color;
	}
}
