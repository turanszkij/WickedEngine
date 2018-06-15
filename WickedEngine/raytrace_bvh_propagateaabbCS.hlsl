#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader will traverse the BVH from bottom to up, and propagate AABBs from leaves to internal nodes
//	Cluster nodes are already computed, which correspond directly to BVH leaf node AABBs
//	Each thread starts at a cluster (leaf)
//	Each thread goes to the parent node, but only if both children are complete, else terminate (bvhFlagBuffer tracks this with atomic operations)
//	Parent node will merge child AABBs and store
//	Loop until we reach the root...

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(clusterAABBBuffer, TracedRenderingAABB, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_ONDEMAND3);

RWSTRUCTUREDBUFFER(bvhAABBBuffer, TracedRenderingAABB, 0);
RWSTRUCTUREDBUFFER(bvhFlagBuffer, uint, 1);

[numthreads(TRACEDRENDERING_BVH_PROPAGATEAABB_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint clusterCount = clusterCounterBuffer.Load(0);

	if (DTid.x < clusterCount)
	{
		const uint leafNodeOffset = clusterCount - 1;
		const uint clusterIndex = clusterIndexBuffer[DTid.x];
		uint nodeIndex = leafNodeOffset + DTid.x;

		// First, we read the current (leaf) node:
		BVHNode node = bvhNodeBuffer[nodeIndex];

		// Leaf node will receive the corresponding cluster AABB:
		TracedRenderingAABB clusterAABB = clusterAABBBuffer[clusterIndex];
		bvhAABBBuffer[nodeIndex] = clusterAABB;

		// Propagate until we reach root node:
		do
		{
			// Move up in the tree:
			nodeIndex = node.ParentIndex;

			// Atomic flag to only allow one thread to write into parent. The other thread is discarded.
			//	If the previous value was 0, that means it's the first child to arrive here, this will be discarded, because maybe the second child is not yet computed its AABB.
			//	Else, this is the second child to arrive, we can continue to parent, because there was already a child that arrived here and been discarded.
			uint flag;
			InterlockedAdd(bvhFlagBuffer[node.ParentIndex], 1, flag);
			if (flag == 0)
			{
				return;
			}

			// We arrived to the parent node:
			node = bvhNodeBuffer[nodeIndex];

			// Load up its two children's AABBs
			TracedRenderingAABB leftAABB = bvhAABBBuffer[node.LeftChildIndex];
			TracedRenderingAABB rightAABB = bvhAABBBuffer[node.RightChildIndex];

			// Merge the child AABBs:
			TracedRenderingAABB mergedAABB;
			mergedAABB.min = min(leftAABB.min, rightAABB.min);
			mergedAABB.max = max(leftAABB.max, rightAABB.max);

			// Write the merged AABB to this node:
			bvhAABBBuffer[nodeIndex] = mergedAABB;

		} while (nodeIndex != 0);
	}

}
