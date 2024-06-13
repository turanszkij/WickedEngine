#include "wiPathQuery.h"
#include "wiRenderer.h"
#include "wiEventHandler.h"
#include "wiProfiler.h"
#include "wiPrimitive.h"

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi
{

	void PathQuery::process(
		const XMFLOAT3& startpos,
		const XMFLOAT3& goalpos,
		const wi::VoxelGrid& voxelgrid
	)
	{
		frontier = {};
		came_from.clear();
		cost_so_far.clear();
		result_path_goal_to_start.clear();
		result_path_goal_to_start_simplified.clear();
		process_startpos = startpos;
		Node start = Node::create(voxelgrid.world_to_coord(startpos));
		Node goal = Node::create(voxelgrid.world_to_coord(goalpos));
		debugstartnode = voxelgrid.coord_to_world(start.coord());
		debuggoalnode = voxelgrid.coord_to_world(goal.coord());
		debugvoxelsize = voxelgrid.voxelSize;

		auto cost_to_goal = [&](XMUINT3 coord, XMUINT3 goal) {
			// manhattan distance:
			return std::abs(int(coord.x) - int(goal.x)) + std::abs(int(coord.y) - int(goal.y)) + std::abs(int(coord.z) - int(goal.z));
		};
		auto dda = [&](const XMUINT3& start, const XMUINT3& goal)
		{
			const int dx = int(goal.x) - int(start.x);
			const int dy = int(goal.y) - int(start.y);
			const int dz = int(goal.z) - int(start.z);

			const int step = std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz)));

			const float x_incr = float(dx) / step;
			const float y_incr = float(dy) / step;
			const float z_incr = float(dz) / step;

			float x = float(start.x);
			float y = float(start.y);
			float z = float(start.z);

			for (int i = 0; i < step; i++)
			{
				XMUINT3 coord = XMUINT3(uint32_t(std::round(x)), uint32_t(std::round(y)), uint32_t(std::round(z)));
				if (!is_voxel_valid(voxelgrid, coord))
					return false;
				x += x_incr;
				y += y_incr;
				z += z_incr;
			}
			return true;
		};

		if (!is_voxel_valid(voxelgrid, goal.coord()))
		{
			// If goal is unreachable because it is not a valid voxel, check immediate neighborhood:
			//	This works better than abandoning when goal happens to be in an invalid voxel because
			//	that happens often because mismatching voxel resolution from real geometry
			bool found = false;
			const int allow_width = agent_width + 1;
			const int allow_height = agent_height + 1;
			for (int x = -allow_width; x <= allow_width && !found; ++x)
			{
				for (int y = -allow_height; y <= allow_height && !found; ++y)
				{
					for (int z = -allow_width; z <= allow_width && !found; ++z)
					{
						if (x == 0 && y == 0 && z == 0)
						{
							continue;
						}
						XMUINT3 neighbor_coord = XMUINT3(uint32_t(goal.x + x), uint32_t(goal.y + y), uint32_t(goal.z + z));
						if (is_voxel_valid(voxelgrid, neighbor_coord))
						{
							goal = Node::create(neighbor_coord);
							found = true;
							break;
						}
					}
				}
			}
			if (!found)
			{
				// if neighborhood was not valid at all, then abandon the search:
				return;
			}
		}

		// A* explanation at: https://www.redblobgames.com/pathfinding/a-star/introduction.html
		frontier.emplace(start);
		came_from[start] = Node::invalid();
		cost_so_far[start] = 0;

		while (!frontier.empty())
		{
			Node current = frontier.top();
			frontier.pop();

			if (current == goal)
				break;

			XMUINT3 coord = current.coord();

#if 1
			// The neighbors to which traversal can happen from the current cell:
			//	Horizontal diagonal is not allowed, only vertical (to support stairs)
			//	Note: diagonal can be supported, but since the path will be simplified,
			//		diagonal voxel movements do not seem to contribute much improvement,
			//		in fact non-diagonal movements can keep away better from wall corners
			XMUINT3 neighbors[] = {
				// left-right:
				XMUINT3(coord.x - 1, coord.y, coord.z),
				XMUINT3(coord.x + 1, coord.y, coord.z),

				// left-right up:
				XMUINT3(coord.x - 1, coord.y - 1, coord.z),
				XMUINT3(coord.x + 1, coord.y - 1, coord.z),

				// left-right down:
				XMUINT3(coord.x - 1, coord.y + 1, coord.z),
				XMUINT3(coord.x + 1, coord.y + 1, coord.z),

				// up-down:
				XMUINT3(coord.x, coord.y - 1, coord.z),
				XMUINT3(coord.x, coord.y + 1, coord.z),

				// forward-back:
				XMUINT3(coord.x, coord.y, coord.z - 1),
				XMUINT3(coord.x, coord.y, coord.z + 1),

				// forward-back up:
				XMUINT3(coord.x, coord.y - 1, coord.z - 1),
				XMUINT3(coord.x, coord.y - 1, coord.z + 1),

				// forward-back down:
				XMUINT3(coord.x, coord.y + 1, coord.z - 1),
				XMUINT3(coord.x, coord.y + 1, coord.z + 1),
			};
			uint32_t neighbor_count = arraysize(neighbors);
#else
			uint32_t neighbor_count = 0;
			XMUINT3 neighbors[26];
			for (int x = -1; x <= 1; ++x)
			{
				for (int y = -1; y <= 1; ++y)
				{
					for (int z = -1; z <= 1; ++z)
					{
						if (x == 0 && y == 0 && z == 0)
						{
							continue;
						}
						neighbors[neighbor_count++] = XMUINT3(uint32_t(coord.x + x), uint32_t(coord.y + y), uint32_t(coord.z + z));
					}
				}
			}
#endif

			for (uint32_t i = 0; i < neighbor_count; ++i)
			{
				if (!is_voxel_valid(voxelgrid, neighbors[i]))
					continue;
				Node next = Node::create(neighbors[i]);
				uint16_t new_cost = cost_so_far[current] + cost_to_goal(current.coord(), next.coord());
				if (cost_so_far.find(next) == cost_so_far.end() || new_cost < cost_so_far[next])
				{
					cost_so_far[next] = new_cost;
					next.cost = new_cost + cost_to_goal(neighbors[i], goal.coord());
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
			// first waypoint will always need to be in the simplified path:
			result_path_goal_to_start_simplified.push_back(result_path_goal_to_start[0]);

			for (size_t i = 0; i < result_path_goal_to_start.size() - 1;)
			{
				Node current = Node::create(voxelgrid.world_to_coord(result_path_goal_to_start[i]));

				// If no occlusion test was successful, then the next will be inserted.
				//	We don't check occlusion for this as this is definitely traversible from previous node
				size_t next_candidate = i + 1;

				// Occlusion tests will be performed further down from next node:
				for (size_t j = next_candidate + 1; j < result_path_goal_to_start.size(); ++j)
				{
					Node next = Node::create(voxelgrid.world_to_coord(result_path_goal_to_start[j]));

					// Visibility check from current to next by drawing a line with DDA and checking validity at each step:
					if (dda(current.coord(), next.coord()))
					{
						// if visible from current, this is accepted as a good next candidate:
						next_candidate = j;
					}
					else
					{
						// if not visible from current we abandon testing anything further:
						break;
					}
				}

				// Always insert the next best candidate node to the simplified path:
				result_path_goal_to_start_simplified.push_back(result_path_goal_to_start[next_candidate]);
				i = next_candidate; // the next candidate will be the current node of the next iteration
			}
		}
	}

	bool PathQuery::search_cover(
		const XMFLOAT3& observer,
		const XMFLOAT3& subject,
		const XMFLOAT3& direction,
		float max_distance,
		const wi::VoxelGrid& voxelgrid
	)
	{
		XMFLOAT3 goal_world;
		XMStoreFloat3(&goal_world, XMLoadFloat3(&subject) + XMLoadFloat3(&direction) * max_distance);

		XMINT3 start = voxelgrid.world_to_coord_signed(observer);
		XMINT3 goal = voxelgrid.world_to_coord_signed(goal_world);
		start.y = goal.y;
		XMUINT3 observer_coord = voxelgrid.world_to_coord(observer);
		if (!flying)
		{
			// place visibility start check above ground if not flying
			observer_coord.y -= 1;
		}
		const XMFLOAT3 observer_point = voxelgrid.coord_to_world(observer_coord);

		auto is_cover = [&](const XMUINT3& coord) {
			if (!is_voxel_valid(voxelgrid, coord))
				return false;
			XMUINT3 visibility_coord = coord;
			if (visibility_coord.y > 0 && !flying)
			{
				// place visibility end check above ground if not flying
				visibility_coord.y -= 1;
			}
			XMFLOAT3 center = voxelgrid.coord_to_world(visibility_coord);
			float width = agent_width * voxelgrid.voxelSize.x * 2;
			float height = agent_height * voxelgrid.voxelSize.y * 2;
			AABB subject_aabb(XMFLOAT3(center.x - width, center.y, center.z - width), XMFLOAT3(center.x + width, center.y + height, center.z + width));
			if (voxelgrid.is_visible(observer_point, subject_aabb))
				return false;
			process(subject, voxelgrid.coord_to_world(coord), voxelgrid);
			return is_succesful();
		};

		const int dx = int(goal.x) - int(start.x);
		const int dy = int(goal.y) - int(start.y);
		const int dz = int(goal.z) - int(start.z);

		const int step = std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz)));

		const float x_incr = float(dx) / step;
		const float y_incr = float(dy) / step;
		const float z_incr = float(dz) / step;

		float x = float(start.x);
		float y = float(start.y);
		float z = float(start.z);

		for (int i = 0; i < step; i++)
		{
			XMUINT3 coord = XMUINT3(uint32_t(std::round(x)), uint32_t(std::round(y)), uint32_t(std::round(z)));
			if (coord.x == goal.x && coord.y == goal.y && coord.z == goal.z)
				return false;
			if (!voxelgrid.is_coord_valid(coord))
				return false;

			if (is_cover(coord))
			{
				return true;
			}

			x += x_incr;
			y += y_incr;
			z += z_incr;
		}
		return false;
	}

	bool PathQuery::is_succesful() const
	{
		return !result_path_goal_to_start.empty();
	}

	XMFLOAT3 PathQuery::get_next_waypoint() const
	{
		const wi::vector<XMFLOAT3>& results = result_path_goal_to_start_simplified.empty() ? result_path_goal_to_start : result_path_goal_to_start_simplified;
		if (results.size() < 2)
			return process_startpos;
		return results[results.size() - 2];
	}

	size_t PathQuery::get_waypoint_count() const
	{
		const wi::vector<XMFLOAT3>& results = result_path_goal_to_start_simplified.empty() ? result_path_goal_to_start : result_path_goal_to_start_simplified;
		return results.size();
	}

	XMFLOAT3 PathQuery::get_waypoint(size_t index) const
	{
		const wi::vector<XMFLOAT3>& results = result_path_goal_to_start_simplified.empty() ? result_path_goal_to_start : result_path_goal_to_start_simplified;
		if (results.size() <= index)
			return process_startpos;
		return results[results.size() - 1 - index]; // return results in direction: start -> goal
	}

	XMFLOAT3 PathQuery::get_goal() const
	{
		const wi::vector<XMFLOAT3>& results = result_path_goal_to_start_simplified.empty() ? result_path_goal_to_start : result_path_goal_to_start_simplified;
		if (results.empty())
			return process_startpos;
		return results.front();
	}

	bool PathQuery::is_voxel_valid(const VoxelGrid& voxelgrid, XMUINT3 coord) const
	{
		if (flying)
		{
			// Flying checks:

			// Center voxel must be within voxel grid:
			if (!voxelgrid.is_coord_valid(coord))
				return false;

			// Neighbor voxels around center must be empty according to agent width and height:
			for (int x = -agent_width; x <= agent_width; ++x)
			{
				for (int z = -agent_width; z <= agent_width; ++z)
				{
					for (int y = 0; y < agent_height; ++y)
					{
						XMUINT3 neighbor_coord = XMUINT3(uint32_t(coord.x + x), uint32_t(coord.y - y), uint32_t(coord.z + z));
						if (voxelgrid.check_voxel(neighbor_coord))
							return false;
					}
				}
			}
		}
		else
		{
			// Grounded checks:

			// Center voxel must be ground (valid):
			if (!voxelgrid.check_voxel(coord))
				return false;

			// Neighbor voxels above center must be empty according to agent width and height:
			for (int x = -agent_width; x <= agent_width; ++x)
			{
				for (int z = -agent_width; z <= agent_width; ++z)
				{
					for (int y = 0; y < 0 + agent_height; ++y)
					{
						// Note that we check above ground only (-1 on Y)!
						XMUINT3 neighbor_coord = XMUINT3(uint32_t(coord.x + x), uint32_t(coord.y - y - 1), uint32_t(coord.z + z));
						if (voxelgrid.check_voxel(neighbor_coord))
							return false;
					}
				}
			}
		}
		return true;
	}

	namespace PathQuery_internal
	{
		PipelineState pso_curve;
		PipelineState pso_waypoint;
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
			device->CreatePipelineState(&desc, &pso_curve);

			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_FRONT);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_ADDITIVE);
			desc.pt = PrimitiveTopology::TRIANGLELIST;
			device->CreatePipelineState(&desc, &pso_waypoint);
		}
	}
	using namespace PathQuery_internal;

	void PathQuery::debugdraw(const XMFLOAT4X4& ViewProjection, CommandList cmd) const
	{
		const wi::vector<XMFLOAT3>& results = result_path_goal_to_start_simplified.empty() ? result_path_goal_to_start : result_path_goal_to_start_simplified;

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

		GraphicsDevice* device = GetDevice();
		device->EventBegin("PathQuery::debugdraw", cmd);

		MiscCB sb;
		sb.g_xTransform = ViewProjection;
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);

		if (debug_voxels)
		{
			static constexpr Vertex cubeVerts[] = {
				{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
			};

			if (results.size() >= 2)
			{
				// Waypoint cubes:
				size_t numVoxels = result_path_goal_to_start_simplified.size() + result_path_goal_to_start.size();
				auto mem = device->AllocateGPU(sizeof(cubeVerts) * numVoxels, cmd);

				const XMVECTOR VOXELSIZE = XMLoadFloat3(&debugvoxelsize);

				size_t dst_offset = 0;
				for (size_t i = 0; i < numVoxels; ++i)
				{
					const wi::vector<XMFLOAT3>& srcarray = i < result_path_goal_to_start.size() ? result_path_goal_to_start : result_path_goal_to_start_simplified;
					const size_t idx = i < result_path_goal_to_start.size() ? i : (i - result_path_goal_to_start.size());
					const XMFLOAT4 color = i < result_path_goal_to_start.size() ? XMFLOAT4(0, 0, 1, 1) : XMFLOAT4(1, 0, 0, 1);

					const XMVECTOR P = XMLoadFloat3(srcarray.data() + idx);

					Vertex verts[arraysize(cubeVerts)];
					std::memcpy(verts, cubeVerts, sizeof(cubeVerts));
					for (auto& v : verts)
					{
						XMVECTOR C = XMLoadFloat4(&v.position);
						C *= VOXELSIZE;
						C += P;
						C = XMVectorSetW(C, 1);
						XMStoreFloat4(&v.position, C);
						v.color = color;
					}
					std::memcpy((uint8_t*)mem.data + dst_offset, verts, sizeof(verts));
					dst_offset += sizeof(verts);
				}

				device->BindPipelineState(&pso_waypoint, cmd);

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

				device->Draw(uint32_t(arraysize(cubeVerts) * numVoxels), 0, cmd);
			}
			else
			{
				// Start and goal positions:
				const size_t numVoxels = 2;
				auto mem = device->AllocateGPU(sizeof(cubeVerts) * numVoxels, cmd);

				const XMVECTOR VOXELSIZE = XMLoadFloat3(&debugvoxelsize);

				size_t dst_offset = 0;
				{
					XMVECTOR P = XMLoadFloat3(&debugstartnode);
					Vertex verts[arraysize(cubeVerts)];
					std::memcpy(verts, cubeVerts, sizeof(cubeVerts));
					for (auto& v : verts)
					{
						XMVECTOR C = XMLoadFloat4(&v.position);
						C *= VOXELSIZE;
						C += P;
						C = XMVectorSetW(C, 1);
						XMStoreFloat4(&v.position, C);
						v.color = XMFLOAT4(1, 0, 0, 1);
					}
					std::memcpy((uint8_t*)mem.data + dst_offset, verts, sizeof(verts));
					dst_offset += sizeof(verts);
				}
				{
					XMVECTOR P = XMLoadFloat3(&debuggoalnode);
					Vertex verts[arraysize(cubeVerts)];
					std::memcpy(verts, cubeVerts, sizeof(cubeVerts));
					for (auto& v : verts)
					{
						XMVECTOR C = XMLoadFloat4(&v.position);
						C *= VOXELSIZE;
						C += P;
						C = XMVectorSetW(C, 1);
						XMStoreFloat4(&v.position, C);
						v.color = XMFLOAT4(1, 0, 0, 1);
					}
					std::memcpy((uint8_t*)mem.data + dst_offset, verts, sizeof(verts));
					dst_offset += sizeof(verts);
				}

				device->BindPipelineState(&pso_waypoint, cmd);

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

				device->Draw(uint32_t(arraysize(cubeVerts) * numVoxels), 0, cmd);
			}
		}

		// Curve:
		if (results.size() >= 2)
		{
			static uint32_t resolution = 100;
			const float resolution_rcp = 1.0f / resolution;
			uint32_t numSegments = uint32_t(results.size()) * resolution;
			uint32_t numVertices = numSegments * 2;

			auto mem = device->AllocateGPU(sizeof(Vertex) * numVertices, cmd);

			static float width = std::max(debugvoxelsize.x, debugvoxelsize.z) * 0.2f;
			debugtimer += 0.16f;
			float segmenti = 0;
			static float gradientsize = 5.0f / resolution;
			static XMFLOAT4 color0 = wi::Color(10, 10, 20, 255);
			static XMFLOAT4 color1 = wi::Color(70, 150, 170, 255);
			static float curve_tension = 0.5f;

			const XMVECTOR topalign = XMVectorSet(0, flying ? 0 : debugvoxelsize.y, 0, 0); // align line to top of voxels (if not flying)

			numVertices = 0;
			Vertex* vertices = (Vertex*)mem.data;
			int count = (int)results.size();
			for (int i = 0; i < count; ++i)
			{
				XMVECTOR P0 = XMLoadFloat3(&results[std::max(0, std::min(i - 1, count - 1))]);
				XMVECTOR P1 = XMLoadFloat3(&results[std::max(0, std::min(i, count - 1))]);
				XMVECTOR P2 = XMLoadFloat3(&results[std::max(0, std::min(i + 1, count - 1))]);
				XMVECTOR P3 = XMLoadFloat3(&results[std::max(0, std::min(i + 2, count - 1))]);

				if (i == 0)
				{
					// when P0 == P1, centripetal catmull doesn't work, so we have to do a dummy control point
					P0 += P1 - P2;
				}
				if (i >= count - 2)
				{
					// when P2 == P3, centripetal catmull doesn't work, so we have to do a dummy control point
					P3 += P2 - P1;
				}

				bool cap = i == (count - 1);
				uint32_t segment_resolution = cap ? 1 : resolution;

				for (uint32_t j = 0; j < segment_resolution; ++j)
				{
					float t = float(j) / float(segment_resolution);

#if 1
					XMVECTOR P = cap ? XMVectorLerp(P1, P2, t) : wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t, curve_tension);
					XMVECTOR P_prev = cap ? XMVectorLerp(P0, P1, t - resolution_rcp) : wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t - resolution_rcp, curve_tension);
					XMVECTOR P_next = cap ? XMVectorLerp(P1, P2, t + resolution_rcp) : wi::math::CatmullRomCentripetal(P0, P1, P2, P3, t + resolution_rcp, curve_tension);
#else
					XMVECTOR P = XMVectorLerp(P1, P2, t);
					XMVECTOR P_prev = XMVectorLerp(P0, P1, t - resolution_rcp);
					XMVECTOR P_next = XMVectorLerp(P1, P2, t + resolution_rcp);
#endif

					XMVECTOR T = XMVector3Normalize(P_next - P_prev);
					XMVECTOR B = XMVector3Normalize(XMVector3Cross(T, XMVectorSet(0, 1, 0, 0)));
					B *= width;

					Vertex vert;
					vert.color = wi::math::Lerp(color0, color1, saturate(std::sin(segmenti + debugtimer) * 0.5f + 0.5f));

					XMStoreFloat4(&vert.position, XMVectorSetW(P - B + topalign, 1));
					std::memcpy(vertices + numVertices, &vert, sizeof(vert));
					numVertices++;

					XMStoreFloat4(&vert.position, XMVectorSetW(P + B + topalign, 1));
					std::memcpy(vertices + numVertices, &vert, sizeof(vert));
					numVertices++;

					segmenti += gradientsize;
				}
			}

			device->BindPipelineState(&pso_curve, cmd);

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

			device->Draw(numVertices, 0, cmd);
		}

		device->EventEnd(cmd);
	}
}
