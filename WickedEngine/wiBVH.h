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
		wi::vector<Node> nodes;
		wi::vector<uint32_t> leaf_indices;
		uint32_t node_count = 0;

		bool IsValid() const { return node_count > 0; }

		// Completely rebuilds tree from scratch
		void Build(const wi::primitive::AABB* aabbs, uint32_t aabb_count)
		{
			node_count = 0;
			nodes.clear();
			leaf_indices.clear();
			if (aabb_count == 0)
				return;

			nodes.resize(aabb_count * 2 - 1);
			leaf_indices.resize(aabb_count);

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

		// Updates the AABBs, but doesn't modify the tree structure (fast update mode) 
		void Update(const wi::primitive::AABB* aabbs, uint32_t aabb_count)
		{
			if (node_count == 0)
				return;
			if (aabb_count == 0)
				return;
			if (aabb_count != (uint32_t)leaf_indices.size())
				return;

			for (uint32_t i = node_count - 1; i > 0; --i)
			{
				Node& node = nodes[i];
				node.aabb = wi::primitive::AABB();
				if (node.isLeaf())
				{
					for (uint32_t j = 0; j < node.count; ++j)
					{
						node.aabb = wi::primitive::AABB::Merge(node.aabb, aabbs[leaf_indices[node.offset + j]]);
					}
				}
				else
				{
					node.aabb = wi::primitive::AABB::Merge(node.aabb, nodes[node.left].aabb);
					node.aabb = wi::primitive::AABB::Merge(node.aabb, nodes[node.left + 1].aabb);
				}
			}
		}

		// Intersect with a primitive shape and return the closest hit
		template <typename T>
		void Intersects(
			const T& primitive,
			uint32_t nodeIndex,
			const std::function<void(uint32_t index)>& callback
		) const
		{
			if (node_count == 0)
				return;
			const Node& node = nodes[nodeIndex];
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

		// Returning true from callback will immediately exit the whole search
		template <typename T>
		bool IntersectsFirst(
			const T& primitive,
			const std::function<bool(uint32_t index)>& callback
		) const
		{
			if (node_count == 0)
				return false;
			uint32_t stack[64];
			uint32_t count = 0;
			stack[count++] = 0; // push node 0
			while (count > 0)
			{
				const uint32_t nodeIndex = stack[--count];
				const Node& node = nodes[nodeIndex];
				if (!node.aabb.intersects(primitive))
					continue;
				if (node.isLeaf())
				{
					for (uint32_t i = 0; i < node.count; ++i)
					{
						if (callback(leaf_indices[node.offset + i]))
							return true;
					}
				}
				else
				{
					stack[count++] = node.left;
					stack[count++] = node.left + 1;
				}
			}
			return false;
		}

	private:
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
	};
}
