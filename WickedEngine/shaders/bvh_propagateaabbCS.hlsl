#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader will traverse the BVH from bottom to up, and propagate AABBs from leaves to internal nodes
//	Each thread starts at a primitive (leaf) and computes primitive AABB
//	Each thread goes to the parent node, but only if both children are complete, else terminate (bvhFlagBuffer tracks this with atomic operations)
//	Parent node will merge child AABBs and store
//	Loop until we reach the root...

ByteAddressBuffer primitiveCounterBuffer : register(t0);
StructuredBuffer<uint> primitiveIDBuffer : register(t1);
ByteAddressBuffer primitiveBuffer : register(t2);
StructuredBuffer<uint> bvhParentBuffer : register(t3);

RWByteAddressBuffer bvhNodeBuffer : register(u0);
RWStructuredBuffer<uint> bvhFlagBuffer : register(u1);

[numthreads(BVH_BUILDER_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint primitiveCount = primitiveCounterBuffer.Load(0);

	if (DTid.x < primitiveCount)
	{
		const uint leafNodeOffset = primitiveCount - 1;
		const uint primitiveID = primitiveIDBuffer[DTid.x];
		uint nodeIndex = leafNodeOffset + DTid.x;

		// Leaf node will receive the corresponding primitive AABB:
		BVHPrimitive prim = primitiveBuffer.Load<BVHPrimitive>(primitiveID * sizeof(BVHPrimitive));
		BVHNode node;
		node.min = min(prim.v0(), min(prim.v1(), prim.v2()));
		node.max = max(prim.v0(), max(prim.v1(), prim.v2()));

		// Also store primitiveID in left child index of leaf node to avoid indirection 
		//	with primitiveIDBuffer when tracing later:
		node.LeftChildIndex = primitiveID;
		node.RightChildIndex = ~0u;

#ifdef __PSSL__
		bvhNodeBuffer.TypedStore<BVHNode>(nodeIndex * sizeof(BVHNode), node);
#else
		bvhNodeBuffer.Store<BVHNode>(nodeIndex * sizeof(BVHNode), node);
#endif // __PSSL__
		

		// Propagate until we reach root node:
		do
		{
			// Move up in the tree:
			nodeIndex = bvhParentBuffer[nodeIndex];

			// Atomic flag to only allow one thread to write into parent. The other thread is discarded.
			//	The flag buffer is a compacted bitmask where each bit represents one internal node's propagation status
			//	If the previous bucket bit was 0, that means it's the first child to arrive here, this will be discarded, because maybe the second child is not yet computed its AABB.
			//	Else, this is the second child to arrive, we can continue to parent, because there was already a child that arrived here and been discarded.
			const uint flag_bucket_index = nodeIndex / 32;
			const uint flag_bucket_bit = 1u << (nodeIndex % 32);
			uint flag_bucket_prev;
			InterlockedOr(bvhFlagBuffer[flag_bucket_index], flag_bucket_bit, flag_bucket_prev);
			if (!(flag_bucket_prev & flag_bucket_bit))
			{
				return;
			}

			// Arrived at parent node, load up its two children's AABBs
			BVHNode parent_node = bvhNodeBuffer.Load<BVHNode>(nodeIndex * sizeof(BVHNode));
			const uint left_child = parent_node.LeftChildIndex;
			const uint right_child = parent_node.RightChildIndex;
			BVHNode left_node = bvhNodeBuffer.Load<BVHNode>(left_child * sizeof(BVHNode));
			BVHNode right_node = bvhNodeBuffer.Load<BVHNode>(right_child * sizeof(BVHNode));
			const float3 left_min = left_node.min;
			const float3 left_max = left_node.max;
			const float3 right_min = right_node.min;
			const float3 right_max = right_node.max;

			// Merge the child AABBs:
			parent_node.min = min(left_min, right_min);
			parent_node.max = max(left_max, right_max);
#ifdef __PSSL__
			bvhNodeBuffer.TypedStore<BVHNode>(nodeIndex * sizeof(BVHNode), parent_node);
#else
			bvhNodeBuffer.Store<BVHNode>(nodeIndex * sizeof(BVHNode), parent_node);
#endif // __PSSL__

		} while (nodeIndex != 0);
	}

}
