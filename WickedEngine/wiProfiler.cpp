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

#if __has_include("Superluminal/PerformanceAPI_capi.h")
#include "Superluminal/PerformanceAPI_capi.h"
#include "Superluminal/PerformanceAPI_loader.h"
#endif // superluminal

#include <string>
#include <stack>
#include <mutex>
#include <atomic>
#include <sstream>

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
	GPUBuffer queryResultBuffer[GraphicsDevice::GetBufferCount() + 1];
	std::atomic<uint32_t> nextQuery{ 0 };
	int queryheap_idx = 0;
	bool drawn_this_frame = false;
	wi::Color background_color = wi::Color(20, 20, 20, 230);
	wi::Color text_color = wi::Color::White();

#if PERFORMANCEAPI_ENABLED
	PerformanceAPI_ModuleHandle superluminal_handle = {};
	PerformanceAPI_Functions superluminal_functions = {};
#endif // PERFORMANCEAPI_ENABLED

	struct Range
	{
		bool in_use = false;
		std::string name;
		float times[20] = {};
		int avg_counter = 0;
		float time = 0;
		CommandList cmd;

		wi::Timer cpuTimer;

		int gpuBegin[arraysize(queryResultBuffer)];
		int gpuEnd[arraysize(queryResultBuffer)];

		bool IsCPURange() const { return !cmd.IsValid(); }
	};
	wi::unordered_map<size_t, Range> ranges;

	void BeginFrame()
	{
		if (ENABLED_REQUEST != ENABLED)
		{
			ranges.clear();
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

		cpu_frame = BeginRangeCPU("CPU Frame");

		GraphicsDevice* device = wi::graphics::GetDevice();
		CommandList cmd = device->BeginCommandList();

		device->QueryReset(
			&queryHeap,
			0,
			queryHeap.desc.query_count,
			cmd
		);

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

		double gpu_frequency = (double)device->GetTimestampFrequency() / 1000.0;

		device->QueryResolve(
			&queryHeap,
			0,
			nextQuery.load(),
			&queryResultBuffer[queryheap_idx],
			0ull,
			cmd
		);

		nextQuery.store(0);
		queryheap_idx = (queryheap_idx + 1) % arraysize(queryResultBuffer);
		uint64_t* queryResults = (uint64_t*)queryResultBuffer[queryheap_idx].mapped_data;

		for (auto& x : ranges)
		{
			auto& range = x.second;

			if (!range.IsCPURange())
			{
				int begin_query = range.gpuBegin[queryheap_idx];
				int end_query = range.gpuEnd[queryheap_idx];
				if (queryResultBuffer[queryheap_idx].mapped_data != nullptr && begin_query >= 0 && end_query >= 0)
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

		range_id id = wi::helper::string_hash(name);

		lock.lock();

		// If one range name is hit multiple times, differentiate between them!
		size_t differentiator = 0;
		while (ranges[id].in_use)
		{
			wi::helper::hash_combine(id, differentiator++);
		}
		ranges[id].in_use = true;
		ranges[id].name = name;
		ranges[id].cpuTimer.record();

		lock.unlock();

		return id;
	}
	range_id BeginRangeGPU(const char* name, CommandList cmd)
	{
		if (!ENABLED || !initialized)
			return 0;

		range_id id = wi::helper::string_hash(name);

		lock.lock();

		// If one range name is hit multiple times, differentiate between them!
		size_t differentiator = 0;
		while (ranges[id].in_use)
		{
			wi::helper::hash_combine(id, differentiator++);
		}
		ranges[id].in_use = true;
		ranges[id].name = name;
		ranges[id].cmd = cmd;

		ranges[id].gpuBegin[queryheap_idx] = nextQuery.fetch_add(1);
		wi::graphics::GetDevice()->QueryEnd(&queryHeap, ranges[id].gpuBegin[queryheap_idx], cmd);

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
				it->second.time = (float)it->second.cpuTimer.elapsed();

#if PERFORMANCEAPI_ENABLED
				if (superluminal_handle)
				{
					superluminal_functions.EndEvent();
				}
#endif // PERFORMANCEAPI_ENABLED
			}
			else
			{
				ranges[id].gpuEnd[queryheap_idx] = nextQuery.fetch_add(1);
				wi::graphics::GetDevice()->QueryEnd(&queryHeap, it->second.gpuEnd[queryheap_idx], it->second.cmd);
			}
		}
		else
		{
			assert(0);
		}

		lock.unlock();
	}


	PipelineState pso_linestrip;
	PipelineState pso_linelist;
	const uint32_t graph_vertex_count = 120;
	float cpu_graph[graph_vertex_count] = {};
	float gpu_graph[graph_vertex_count] = {};
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
	}

	struct Hits
	{
		uint32_t num_hits = 0;
		float total_time = 0;
	};
	wi::unordered_map<std::string, Hits> time_cache_cpu;
	wi::unordered_map<std::string, Hits> time_cache_gpu;
	void DrawData(
		const wi::Canvas& canvas,
		float x,
		float y,
		CommandList cmd,
		ColorSpace colorspace
	)
	{
		if (!ENABLED || !ENABLED_REQUEST || !initialized || drawn_this_frame)
			return;
		drawn_this_frame = true;

		const XMFLOAT2 graph_size = XMFLOAT2(190, 100);
		const float graph_left_offset = 45;
		const float graph_padding_y = 40;

		wi::image::SetCanvas(canvas);
		wi::font::SetCanvas(canvas);

		std::stringstream ss("");
		ss.precision(2);

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

		wi::font::Params params = wi::font::Params(x, y + graph_size.y + graph_padding_y, wi::font::WIFONTSIZE_DEFAULT - 4, wi::font::WIFALIGN_LEFT, wi::font::WIFALIGN_TOP, text_color);

		// Background:
		wi::image::Params fx;
		fx.pos.x = x - 10;
		fx.pos.y = y - 10;
		fx.siz.x = std::max(graph_size.x, (float)wi::font::TextWidth(ss.str(), params)) + 100;
		fx.siz.y = (float)wi::font::TextHeight(ss.str(), params) + graph_size.y + graph_padding_y + 20;
		fx.color = background_color;

		if (colorspace != ColorSpace::SRGB)
		{
			params.enableLinearOutputMapping(9);
			fx.enableLinearOutputMapping(9);
		}

		wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);
		wi::font::Draw(ss.str(), params, cmd);


		// Graph:
		{
			static bool shaders_loaded = false;
			if (!shaders_loaded)
			{
				shaders_loaded = true;
				static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
				LoadShaders();
			}

			float graph_max = 33;
			for (uint32_t i = graph_vertex_count - 1; i > 0; --i)
			{
				cpu_graph[i] = cpu_graph[i - 1];
				gpu_graph[i] = gpu_graph[i - 1];
				graph_max = std::max(graph_max, cpu_graph[i]);
				graph_max = std::max(graph_max, gpu_graph[i]);
			}
			cpu_graph[0] = ranges[cpu_frame].time;
			gpu_graph[0] = ranges[gpu_frame].time;
			graph_max = std::max(graph_max, cpu_graph[0]);
			graph_max = std::max(graph_max, gpu_graph[0]);

			struct Vertex
			{
				XMFLOAT4 position;
				XMFLOAT4 color;
			};
			Vertex graph_info[] = {
				// axes:
				{XMFLOAT4(0, 0, 0, 1), text_color},
				{XMFLOAT4(0, 1, 0, 1), text_color},
				{XMFLOAT4(0, 1, 0, 1), text_color},
				{XMFLOAT4(1, 1, 0, 1), text_color},

				// markers:
				{XMFLOAT4(0, 0, 0, 1), text_color}, // graph_max
				{XMFLOAT4(-10.0f / graph_size.x, 0, 0, 1), text_color}, // graph_max
				{XMFLOAT4(0, 1 - 16.6f / graph_max, 0, 1), text_color}, // 16.6
				{XMFLOAT4(-10.0f / graph_size.x, 1 - 16.6f / graph_max, 0, 1), text_color}, // 16.6
				{XMFLOAT4(60.0f / float(graph_vertex_count),1,0,1), text_color}, // 60 frames
				{XMFLOAT4(60.0f / float(graph_vertex_count),1 + 10.0f / graph_size.x,0,1), text_color}, // 60 frames
				{XMFLOAT4(1,1,0,1), text_color}, // 0 frames
				{XMFLOAT4(1,1 + 10.0f / graph_size.x,0,1), text_color}, // 0 frames
			};

			GraphicsDevice* device = wi::graphics::GetDevice();
			GraphicsDevice::GPUAllocation allocation = device->AllocateGPU(sizeof(Vertex) * graph_vertex_count * 2 + sizeof(graph_info), cmd);

			for (uint32_t i = 0; i < graph_vertex_count; ++i)
			{
				Vertex vert;
				vert.color = XMFLOAT4(1, 0.2f, 0.2f, 1);
				vert.position = XMFLOAT4(float(graph_vertex_count - 1 - i) / float(graph_vertex_count), 1 - cpu_graph[i] / graph_max, 0, 1);
				std::memcpy((Vertex*)allocation.data + i, &vert, sizeof(vert));

				vert.color = XMFLOAT4(0.2f, 1, 0.2f, 1);
				vert.position = XMFLOAT4(float(graph_vertex_count - 1 - i) / float(graph_vertex_count), 1 - gpu_graph[i] / graph_max, 0, 1);
				std::memcpy((Vertex*)allocation.data + graph_vertex_count + i, &vert, sizeof(vert));
			}
			std::memcpy((Vertex*)allocation.data + graph_vertex_count * 2, graph_info, sizeof(graph_info));

			device->BindPipelineState(&pso_linestrip, cmd);

			MiscCB cb;
			cb.g_xColor = XMFLOAT4(1, 1, 1, 1);
			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixScaling(graph_size.x - graph_left_offset, graph_size.y, 1) *
				XMMatrixTranslation(x + graph_left_offset, y, 0) *
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
			device->Draw(sizeof(graph_info) / sizeof(Vertex), graph_vertex_count * 2, cmd);

			wi::font::Params params;
			params.size = 10;
			params.v_align = wi::font::WIFALIGN_CENTER;
			params.color = text_color;
			params.position.x = x;
			params.position.y = y;
			std::stringstream ss;
			ss.precision(1);
			ss << std::fixed << graph_max << " ms";
			wi::font::Draw(ss.str(), params, cmd);
			params.position.y = y + graph_size.y - (16.6f / graph_max) * graph_size.y;
			wi::font::Draw("16.6 ms", params, cmd);

			params.position.x = x + graph_size.x + 5;
			params.position.y = y + graph_size.y - cpu_graph[0] / graph_max * graph_size.y;
			params.color = wi::Color::fromFloat4(XMFLOAT4(1, 0.2f, 0.2f, 1));
			ss.str("");
			ss.clear();
			ss << "cpu: " << cpu_graph[0] << " ms";
			wi::font::Draw(ss.str(), params, cmd);

			float cpu_position = params.position.y;
			params.position.x = x + graph_size.x + 5;
			params.position.y = y + graph_size.y - gpu_graph[0] / graph_max * graph_size.y;
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
			ss.str("");
			ss.clear();
			ss << "gpu: " << gpu_graph[0] << " ms";
			wi::font::Draw(ss.str(), params, cmd);

			params.h_align = wi::font::WIFALIGN_CENTER;
			params.color = text_color;
			params.position.x = x + graph_left_offset + (graph_size.x - graph_left_offset) * 0.5f;
			params.position.y = y + graph_size.y + 10;
			wi::font::Draw("60 frames", params, cmd);
			params.position.x = x + graph_size.x;
			wi::font::Draw("current frame", params, cmd);
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
