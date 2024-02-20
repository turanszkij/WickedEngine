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
		result_path_goal_to_start_simplified.clear();
		process_startpos = startpos;
		debugvoxelsize = voxelgrid.voxelSize;

		auto is_voxel_valid = [&](XMUINT3 coord) {
			if (!voxelgrid.check_voxel(coord))
				return false;
			// Check blocking from above 2 voxels:
			if (voxelgrid.check_voxel(XMUINT3(coord.x, coord.y - 1, coord.z)))
				return false;
			if (voxelgrid.check_voxel(XMUINT3(coord.x, coord.y - 2, coord.z)))
				return false;
			return true;
		};
		auto is_voxel_valid2 = [&](XMUINT3 coord) {
			for (int x = -1; x <= 1; ++x)
			{
				for (int z = -1; z <= 1; ++z)
				{
					XMUINT3 neighbor_coord = coord;
					neighbor_coord.x += x;
					neighbor_coord.z += z;
					if (!is_voxel_valid(neighbor_coord))
						return false;
				}
			}
			return true;
		};
		auto cost_to_goal = [&](XMUINT3 coord, XMUINT3 goal) {
			// manhattan distance:
			return std::abs(int(coord.x) - int(goal.x)) + std::abs(int(coord.y) - int(goal.y)) + std::abs(int(coord.z) - int(goal.z));
		};
		auto dda = [&](const XMUINT3& start, const XMUINT3& goal)
		{
			const int dx = int(goal.x) - int(start.x);
			const int dy = int(goal.y) - int(start.y);
			const int dz = int(goal.z) - int(start.z);

			static int method = 0;

			if (method == 0)
			{
				// Thick-line (conservative) DDA:
				const int stepX = dx >= 0 ? 1 : -1;
				const int stepY = dy >= 0 ? 1 : -1;
				const int stepZ = dz >= 0 ? 1 : -1;

				const float tDeltaX = float(stepX) / dx;
				const float tDeltaY = float(stepY) / dy;
				const float tDeltaZ = float(stepZ) / dz;

				float tMaxX = tDeltaX;
				float tMaxY = tDeltaY;
				float tMaxZ = tDeltaZ;

				int x = start.x;
				int y = start.y;
				int z = start.z;

				do {
					if (tMaxX < tMaxY)
					{
						if (tMaxX < tMaxZ)
						{
							x += stepX;
							tMaxX += tDeltaX;
						}
						else
						{
							z += stepZ;
							tMaxZ += tDeltaZ;
						}
					}
					else
					{
						if (tMaxY < tMaxZ)
						{
							y += stepY;
							tMaxY += tDeltaY;
						}
						else
						{
							z += stepZ;
							tMaxZ += tDeltaZ;
						}
					}
					XMUINT3 coord = XMUINT3(uint32_t(x), uint32_t(y), uint32_t(z));
					if (!is_voxel_valid(coord))
						return false;
				} while (x != goal.x || y != goal.y || z != goal.z);
			}
			else
			{
				// Thin-line DDA:
				const int abs_dx = std::abs(dx);
				const int abs_dy = std::abs(dy);
				const int abs_dz = std::abs(dz);

				int step = abs_dx > abs_dy ? abs_dx : abs_dy;
				step = abs_dz > step ? abs_dz : step;

				const float x_incr = float(dx) / step;
				const float y_incr = float(dy) / step;
				const float z_incr = float(dz) / step;

				float x = float(start.x);
				float y = float(start.y);
				float z = float(start.z);

				for (int i = 0; i < step; i++)
				{
					XMUINT3 coord = XMUINT3(uint32_t(std::round(x)), uint32_t(std::round(y)), uint32_t(std::round(z)));
					if (!is_voxel_valid(coord))
						return false;
					x += x_incr;
					y += y_incr;
					z += z_incr;
				}
			}
			return true;
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

			VoxelGrid::NeighborQueryFlags neighbor_flags = VoxelGrid::NeighborQueryFlags::None;
			//neighbor_flags |= VoxelGrid::NeighborQueryFlags::MustBeValid;
			//neighbor_flags |= VoxelGrid::NeighborQueryFlags::DisableDiagonal;
			wi::VoxelGrid::Neighbors neighbors = voxelgrid.get_neighbors(current.coord(), neighbor_flags);
			for (uint32_t i = 0; i < neighbors.count; ++i)
			{
				if (!is_voxel_valid(neighbors.coords[i]))
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

		// Simplification:
		if (!result_path_goal_to_start.empty())
		{
			size_t i = 0;
			while (i < result_path_goal_to_start.size())
			{
				result_path_goal_to_start_simplified.push_back(result_path_goal_to_start[i]);
				Node current = Node::create(voxelgrid.world_to_coord(result_path_goal_to_start[i]));
				size_t maxi = i;
				for (maxi = i + 1; maxi < result_path_goal_to_start.size() - 1; ++maxi)
				{
					Node next = Node::create(voxelgrid.world_to_coord(result_path_goal_to_start[maxi]));
					if (!dda(current.coord(), next.coord()))
						break;
				}
				i = maxi;
			}
		}

		wi::profiler::EndRange(range);
	}

	XMFLOAT3 PathQuery::get_first_waypoint() const
	{
		const wi::vector<XMFLOAT3>& results = result_path_goal_to_start_simplified.empty() ? result_path_goal_to_start : result_path_goal_to_start_simplified;
		if (results.size() < 2)
			return process_startpos;
		return results[results.size() - 2];
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
		const wi::vector<XMFLOAT3>& results = result_path_goal_to_start_simplified.empty() ? result_path_goal_to_start : result_path_goal_to_start_simplified;

		if (results.size() < 2)
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
		const float resolution_rcp = 1.0f / resolution;
		uint32_t numSegments = uint32_t(results.size()) * resolution;
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
		int count = (int)results.size();
		for (int i = 0; i < count; ++i)
		{
			P0 = XMLoadFloat3(&results[std::max(0, std::min(i - 1, count - 1))]) + topalign;
			P1 = XMLoadFloat3(&results[std::max(0, std::min(i, count - 1))]) + topalign;
			P2 = XMLoadFloat3(&results[std::max(0, std::min(i + 1, count - 1))]) + topalign;
			P3 = XMLoadFloat3(&results[std::max(0, std::min(i + 2, count - 1))]) + topalign;
			XMFLOAT4X4 boxmat;
			XMStoreFloat4x4(&boxmat, XMMatrixScaling(debugvoxelsize.x, debugvoxelsize.y, debugvoxelsize.z)* XMMatrixTranslationFromVector(P1));
			wi::renderer::DrawBox(boxmat);
			for (uint32_t j = 0; j < (i == (count - 1) ? 1 : resolution); ++j)
			{
				float t = float(j) / float(resolution);
#if 1
				XMVECTOR P = XMVectorCatmullRom(P0, P1, P2, P3, t);
				XMVECTOR P_prev = XMVectorCatmullRom(P0, P1, P2, P3, t - resolution_rcp);
				XMVECTOR P_next = XMVectorCatmullRom(P0, P1, P2, P3, t + resolution_rcp);
#else
				XMVECTOR P = XMVectorLerp(P1, P2, t);
				XMVECTOR P_prev = XMVectorLerp(P0, P1, t - resolution_rcp);
				XMVECTOR P_next = XMVectorLerp(P1, P2, t + resolution_rcp);
#endif

				XMVECTOR T = XMVector3Normalize(P_next - P_prev);
				XMVECTOR B = XMVector3Normalize(XMVector3Cross(T, XMVectorSet(0, 1, 0, 0)));
				B *= width;

				Vertex vert;
				vert.color = wi::math::Lerp(color0, color1, wi::math::saturate(std::sin(segmenti + debugtimer) * 0.5f + 0.5f));

				XMStoreFloat4(&vert.position, XMVectorSetW(P - B, 1));
				std::memcpy(vertices + numVertices, &vert, sizeof(vert));
				numVertices++;

				XMStoreFloat4(&vert.position, XMVectorSetW(P + B, 1));
				std::memcpy(vertices + numVertices, &vert, sizeof(vert));
				numVertices++;

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
