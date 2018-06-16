#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader will kick off the BVH hierarchy build shader by writing its dispatch arguments
//	And also do a little extra: if we have only one cluster, then the BVH will not be built. 
//	But the rays still need to enter, so we will just write the root level AABB anyway and mark the root as leaf!

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(clusterAABBBuffer, TracedRenderingAABB, TEXSLOT_ONDEMAND1);

RWRAWBUFFER(indirectDispatchBuffer, 0);
RWSTRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, 1);
RWSTRUCTUREDBUFFER(bvhAABBBuffer, TracedRenderingAABB, 2);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint clusterCount = clusterCounterBuffer.Load(0);

	indirectDispatchBuffer.Store3(0, uint3(ceil((float)(clusterCount - 1) / (float)TRACEDRENDERING_BVH_HIERARCHY_GROUPSIZE), 1, 1));

	if (clusterCount == 1)
	{
		// Initialize root as leaf node:
		bvhNodeBuffer[0].ParentIndex = 0;
		bvhNodeBuffer[0].LeftChildIndex = 0;
		bvhNodeBuffer[0].RightChildIndex = 0;

		// Write root level AABB:
		bvhAABBBuffer[0] = clusterAABBBuffer[0];

		//bvhAABBBuffer[0].min = g_xFrame_WorldBoundsMin;
		//bvhAABBBuffer[0].max = g_xFrame_WorldBoundsMax;
	}
}
