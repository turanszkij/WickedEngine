#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader will traverse the BVH from bottom to up, and propagate AABBs from leaves to internal nodes
//	Each thread starts at a primitive (leaf) and computes primitive AABB
//	Each thread goes to the parent node, but only if both children are complete, else terminate (bvhFlagBuffer tracks this with atomic operations)
//	Parent node will merge child AABBs and store
//	Loop until we reach the root...

RAWBUFFER(primitiveCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(primitiveIDBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(primitiveBuffer, BVHPrimitive, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(bvhParentBuffer, uint, TEXSLOT_ONDEMAND3);

RWSTRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, 0);
RWSTRUCTUREDBUFFER(bvhFlagBuffer, uint, 1);

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
		BVHPrimitive prim = primitiveBuffer[primitiveID];
		bvhNodeBuffer[nodeIndex].min = min(prim.v0(), min(prim.v1(), prim.v2()));
		bvhNodeBuffer[nodeIndex].max = max(prim.v0(), max(prim.v1(), prim.v2()));

		// Also store primitiveID in left child index of leaf node to avoid indirection 
		//	with primitiveIDBuffer when tracing later:
		bvhNodeBuffer[nodeIndex].LeftChildIndex = primitiveID;
		

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
			const uint flag_bucket_bit = 1 << (nodeIndex % 32);
			uint flag_bucket_prev;
			InterlockedOr(bvhFlagBuffer[flag_bucket_index], flag_bucket_bit, flag_bucket_prev);
			if (!(flag_bucket_prev & flag_bucket_bit))
			{
				return;
			}

			// Arrived at parent node, load up its two children's AABBs
			const uint left_child = bvhNodeBuffer[nodeIndex].LeftChildIndex;
			const uint right_child = bvhNodeBuffer[nodeIndex].RightChildIndex;
			const float3 left_min = bvhNodeBuffer[left_child].min;
			const float3 left_max = bvhNodeBuffer[left_child].max;
			const float3 right_min = bvhNodeBuffer[right_child].min;
			const float3 right_max = bvhNodeBuffer[right_child].max;

			// Merge the child AABBs:
			bvhNodeBuffer[nodeIndex].min = min(left_min, right_min);
			bvhNodeBuffer[nodeIndex].max = max(left_max, right_max);

		} while (nodeIndex != 0);
	}

}
