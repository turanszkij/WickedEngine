#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader reads the cluster index buffer (sorted by morton)
//	and outputs the direct sorted morton codes

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(clusterMortonBuffer, uint, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(clusterSortedMortonBuffer, uint, 0);

[numthreads(TRACEDRENDERING_BVH_SORTEDMORTON_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x < clusterCounterBuffer.Load(0))
	{
		clusterSortedMortonBuffer[DTid.x] = clusterMortonBuffer[clusterIndexBuffer[DTid.x]];
	}
}
