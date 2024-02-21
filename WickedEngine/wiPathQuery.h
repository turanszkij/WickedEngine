#pragma once
#include "CommonInclude.h"
#include "wiUnorderedMap.h"
#include "wiVector.h"
#include "wiVoxelGrid.h"
#include "wiGraphicsDevice.h"

#include <queue>

namespace wi
{
	struct PathQuery
	{
		struct Node
		{
			uint16_t x = 0;
			uint16_t y = 0;
			uint16_t z = 0;
			uint16_t cost = 0;
			constexpr XMUINT3 coord() const
			{
				return XMUINT3(x, y, z);
			}
			static constexpr Node create(XMUINT3 coord)
			{
				Node node = {};
				node.x = coord.x;
				node.y = coord.y;
				node.z = coord.z;
				return node;
			}
			static constexpr Node invalid()
			{
				return {};
			}
			constexpr bool operator>(const Node& other) const { return cost > other.cost; } // for priority_queue
			constexpr bool operator<(const Node& other) const { return cost < other.cost; } // for priority_queue
			constexpr operator uint64_t() const { return uint64_t(uint64_t(x) | (uint64_t(y) << 16ull) | (uint64_t(z) << 32ull)); } // for unordered_map
		};

		std::priority_queue<Node, wi::vector<Node>, std::greater<Node>> frontier;
		wi::unordered_map<uint64_t, Node> came_from;
		wi::unordered_map<uint64_t, uint16_t> cost_so_far;
		wi::vector<XMFLOAT3> result_path_goal_to_start;
		wi::vector<XMFLOAT3> result_path_goal_to_start_simplified;
		XMFLOAT3 process_startpos = XMFLOAT3(0, 0, 0);

		// Find the path between startpos and goalpos in the voxel grid:
		void process(
			const XMFLOAT3& startpos,
			const XMFLOAT3& goalpos,
			const wi::VoxelGrid& voxelgrid,
			wi::VoxelGrid* debug_voxelgrid = nullptr
		);

		// Gets the first waypoint between start and goal that was used in process():
		XMFLOAT3 get_first_waypoint() const;

		mutable float debugtimer = 0;
		XMFLOAT3 debugvoxelsize = XMFLOAT3(0, 0, 0);
		void debugdraw(const XMFLOAT4X4& ViewProjection, wi::graphics::CommandList cmd) const;
	};
}
