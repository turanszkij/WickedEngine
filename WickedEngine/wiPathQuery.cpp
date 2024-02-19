#include "wiPathQuery.h"
#include "wiRenderer.h"
#include "wiEventHandler.h"
#include "wiProfiler.h"

using namespace wi::graphics;

namespace wi
{
	void PathQuery::process(const XMFLOAT3& startpos, const XMFLOAT3& goalpos, const wi::VoxelGrid& voxelgrid)
	{
		auto range = wi::profiler::BeginRangeCPU("PathQuery");
		frontier = {};
		came_from.clear();
		cost_so_far.clear();
		result_path_goal_to_start.clear();
		process_startpos = startpos;
		debugvoxelsize = voxelgrid.voxelSize;

		auto is_coord_valid = [&](XMUINT3 coord) {
			if (voxelgrid.check_voxel(XMUINT3(coord.x, coord.y - 1, coord.z)))
				return false; // if blocked from above, then it is not traversible
			if (voxelgrid.check_voxel(XMUINT3(coord.x, coord.y - 2, coord.z)))
				return false; // if blocked from above, then it is not traversible
			return true;
		};
		auto cost_to_goal = [&](XMUINT3 coord, XMUINT3 goal) {
			// manhattan distance:
			return std::abs(int(coord.x) - int(goal.x)) + std::abs(int(coord.y) - int(goal.y)) + std::abs(int(coord.z) - int(goal.z));
		};

		// A* explanation at: https://www.redblobgames.com/pathfinding/a-star/introduction.html
		Node start = Node::create(voxelgrid.world_to_coord(startpos));
		Node goal = Node::create(voxelgrid.world_to_coord(goalpos));
		frontier.emplace(start);
		came_from[start] = Node::invalid();
		cost_so_far[start] = 0;

		while (!frontier.empty())
		{
			Node current = frontier.top();
			frontier.pop();

			if (current == goal)
				break;

			wi::VoxelGrid::Neighbors neighbors = voxelgrid.get_neighbors(current.coord());
			for (uint32_t i = 0; i < neighbors.count; ++i)
			{
				if (!is_coord_valid(neighbors.coords[i]))
					continue;
				Node next = Node::create(neighbors.coords[i]);
				uint16_t new_cost = cost_so_far[current] + cost_to_goal(current.coord(), next.coord());
				if (cost_so_far.find(next) == cost_so_far.end() || new_cost < cost_so_far[next])
				{
					cost_so_far[next] = new_cost;
					next.cost = new_cost + cost_to_goal(neighbors.coords[i], goal.coord());
					frontier.push(next);
					came_from[next] = current;
				}
			}
		}

		if (came_from[goal] != Node::invalid())
		{
			// If goal is reachable, add that as the first result waypoint:
			result_path_goal_to_start.push_back(voxelgrid.coord_to_world(goal.coord()));
		}

		// Add rest of the path to result waypoints:
		Node current = goal;
		while (came_from[current] != Node::invalid())
		{
			Node node = came_from[current];
			result_path_goal_to_start.push_back(voxelgrid.coord_to_world(node.coord()));
			current = node;
		}

		wi::profiler::EndRange(range);
	}

	XMFLOAT3 PathQuery::get_first_waypoint() const
	{
		if (result_path_goal_to_start.size() < 2)
			return process_startpos;
		return result_path_goal_to_start[result_path_goal_to_start.size() - 2];
	}

	namespace PathQuery_internal
	{
		PipelineState pso_solidpart;
		static void LoadShaders()
		{
			PipelineStateDesc desc;
			desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
			desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
			desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHREAD);
			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
			desc.pt = PrimitiveTopology::TRIANGLESTRIP;

			GraphicsDevice* device = GetDevice();
			device->CreatePipelineState(&desc, &pso_solidpart);
		}
	}
	using namespace PathQuery_internal;

	void PathQuery::debugdraw(const XMFLOAT4X4& ViewProjection, CommandList cmd) const
	{
		if (result_path_goal_to_start.size() < 2)
			return;

		static bool shaders_loaded = false;
		if (!shaders_loaded)
		{
			shaders_loaded = true;
			static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
			LoadShaders();
		}

		struct Vertex
		{
			XMFLOAT4 position;
			XMFLOAT4 color;
		};

		static uint32_t resolution = 10;
		uint32_t numSegments = uint32_t(result_path_goal_to_start.size()) * resolution;
		uint32_t numVertices = numSegments * 2;

		GraphicsDevice* device = GetDevice();

		auto mem = device->AllocateGPU(sizeof(Vertex) * numVertices, cmd);

		static float width = std::max(debugvoxelsize.x, debugvoxelsize.z) * 0.2f;
		debugtimer += 0.16f;
		float segmenti = 0;
		static float gradientsize = 5.0f / resolution;
		static XMFLOAT4 color0 = wi::Color(10, 10, 20, 255);
		static XMFLOAT4 color1 = wi::Color(70, 150, 170, 255);

		static const XMVECTOR topalign = XMVectorSet(0, debugvoxelsize.y, 0, 0); // align line to top of voxels
		
		numVertices = 0;
		Vertex* vertices = (Vertex*)mem.data;
		XMVECTOR P0, P1, P2, P3;
		XMVECTOR P_prev = XMLoadFloat3(&result_path_goal_to_start[0]) + topalign;
		int count = (int)result_path_goal_to_start.size();
		for (int i = 0; i < count - 1; ++i)
		{
			P0 = XMLoadFloat3(&result_path_goal_to_start[std::max(0, std::min(i - 1, count - 1))]) + topalign;
			P1 = XMLoadFloat3(&result_path_goal_to_start[std::max(0, std::min(i, count - 1))]) + topalign;
			P2 = XMLoadFloat3(&result_path_goal_to_start[std::max(0, std::min(i + 1, count - 1))]) + topalign;
			P3 = XMLoadFloat3(&result_path_goal_to_start[std::max(0, std::min(i + 2, count - 1))]) + topalign;
			for (uint32_t j = 0; j < resolution; ++j)
			{
				float t = float(j) / float(resolution);
				XMVECTOR P = XMVectorCatmullRom(P0, P1, P2, P3, t);

				XMVECTOR T = XMVector3Normalize(P - P_prev);
				XMVECTOR B = XMVector3Normalize(XMVector3Cross(T, XMVectorSet(0, 1, 0, 0)));
				B *= width;

				Vertex vert;
				vert.color = wi::math::Lerp(color0, color1, wi::math::saturate(std::sin(segmenti + debugtimer) * 0.5f + 0.5f));

				XMStoreFloat4(&vert.position, XMVectorSetW(P_prev - B, 1));
				std::memcpy(vertices + numVertices, &vert, sizeof(vert));
				numVertices++;

				XMStoreFloat4(&vert.position, XMVectorSetW(P_prev + B, 1));
				std::memcpy(vertices + numVertices, &vert, sizeof(vert));
				numVertices++;

				P_prev = P;
				segmenti += gradientsize;
			}
		}

		device->EventBegin("PathQuery::debugdraw", cmd);
		device->BindPipelineState(&pso_solidpart, cmd);

		const GPUBuffer* vbs[] = {
			&mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint64_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		MiscCB sb;
		sb.g_xTransform = ViewProjection;
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);

		device->Draw(numVertices, 0, cmd);

		device->EventEnd(cmd);
	}
}
