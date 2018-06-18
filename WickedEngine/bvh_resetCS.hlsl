#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader resets the BVH structure

RWRAWBUFFER(clusterCounterBuffer, 0);
RWSTRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, 1);
RWSTRUCTUREDBUFFER(bvhAABBBuffer, TracedRenderingAABB, 2);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Initialize cluster array to empty:
	clusterCounterBuffer.Store(0, 0);

	// Initialize root as leaf node (safety):
	bvhNodeBuffer[0].ParentIndex = 0;
	bvhNodeBuffer[0].LeftChildIndex = 0;
	bvhNodeBuffer[0].RightChildIndex = 0;

	// Write root level AABB:
	bvhAABBBuffer[0].min = g_xFrame_WorldBoundsMin;
	bvhAABBBuffer[0].max = g_xFrame_WorldBoundsMax;
}
