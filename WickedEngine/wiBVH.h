#pragma once
#include "CommonInclude.h"
#include "wiPrimitive.h"

namespace wi
{
	// Simple fast update BVH
	//	https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
	struct BVH
	{
		struct Node
		{
			wi::primitive::AABB aabb;
			uint32_t left = 0;
			uint32_t offset = 0;
			uint32_t count = 0;
			constexpr bool isLeaf() const { return count > 0; }
		};
		wi::vector<uint8_t> allocation;
		Node* nodes = nullptr;
		uint32_t node_count = 0;
		uint32_t* leaf_indices = nullptr;
		uint32_t leaf_count = 0;

		constexpr bool IsValid() const { return nodes != nullptr; }

		void Build(const wi::primitive::AABB* aabbs, uint32_t aabb_count)
		{
			node_count = 0;
			if (aabb_count == 0)
				return;

			const uint32_t node_capacity = aabb_count * 2 - 1;
			allocation.reserve(
				sizeof(Node) * node_capacity +
				sizeof(uint32_t) * aabb_count
			);
			nodes = (Node*)allocation.data();
			leaf_indices = (uint32_t*)(nodes + node_capacity);
			leaf_count = aabb_count;

			Node& node = nodes[node_count++];
			node = {};
			node.count = aabb_count;
			for (uint32_t i = 0; i < aabb_count; ++i)
			{
				node.aabb = wi::primitive::AABB::Merge(node.aabb, aabbs[i]);
				leaf_indices[i] = i;
			}
			Subdivide(0, aabbs);
		}

		void Subdivide(uint32_t nodeIndex, const wi::primitive::AABB* leaf_aabb_data)
		{
			Node& node = nodes[nodeIndex];
			if (node.count <= 2)
				return;

			XMFLOAT3 extent = node.aabb.getHalfWidth();
			XMFLOAT3 min = node.aabb.getMin();
			int axis = 0;
			if (extent.y > extent.x) axis = 1;
			if (extent.z > ((float*)&extent)[axis]) axis = 2;
			float splitPos = ((float*)&min)[axis] + ((float*)&extent)[axis] * 0.5f;

			// in-place partition
			int i = node.offset;
			int j = i + node.count - 1;
			while (i <= j)
			{
				XMFLOAT3 center = leaf_aabb_data[leaf_indices[i]].getCenter();
				float value = ((float*)&center)[axis];

				if (value < splitPos)
				{
					i++;
				}
				else
				{
					std::swap(leaf_indices[i], leaf_indices[j--]);
				}
			}

			// abort split if one of the sides is empty
			int leftCount = i - node.offset;
			if (leftCount == 0 || leftCount == node.count)
				return;

			// create child nodes
			uint32_t left_child_index = node_count++;
			uint32_t right_child_index = node_count++;
			node.left = left_child_index;
			nodes[left_child_index] = {};
			nodes[left_child_index].offset = node.offset;
			nodes[left_child_index].count = leftCount;
			nodes[right_child_index] = {};
			nodes[right_child_index].offset = i;
			nodes[right_child_index].count = node.count - leftCount;
			node.count = 0;
			UpdateNodeBounds(left_child_index, leaf_aabb_data);
			UpdateNodeBounds(right_child_index, leaf_aabb_data);

			// recurse
			Subdivide(left_child_index, leaf_aabb_data);
			Subdivide(right_child_index, leaf_aabb_data);
		}

		void UpdateNodeBounds(uint32_t nodeIndex, const wi::primitive::AABB* leaf_aabb_data)
		{
			Node& node = nodes[nodeIndex];
			node.aabb = {};
			for (uint32_t i = 0; i < node.count; ++i)
			{
				uint32_t offset = node.offset + i;
				uint32_t index = leaf_indices[offset];
				node.aabb = wi::primitive::AABB::Merge(node.aabb, leaf_aabb_data[index]);
			}
		}

		template <typename T>
		void Intersects(
			const T& primitive,
			uint32_t nodeIndex,
			const std::function<void(uint32_t index)>& callback
		) const
		{
			Node& node = nodes[nodeIndex];
			if (!node.aabb.intersects(primitive))
				return;
			if (node.isLeaf())
			{
				for (uint32_t i = 0; i < node.count; ++i)
				{
					callback(leaf_indices[node.offset + i]);
				}
			}
			else
			{
				Intersects(primitive, node.left, callback);
				Intersects(primitive, node.left + 1, callback);
			}
		}
	};
}
