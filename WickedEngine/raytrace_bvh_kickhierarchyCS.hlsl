#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader will merely kick off the BVH hierarchy build shader by writing its dispatch arguments

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);

RWRAWBUFFER(indirectDispatchBuffer, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint clusterCount = clusterCounterBuffer.Load(0);

	indirectDispatchBuffer.Store3(0, uint3(ceil((float)(clusterCount - 1) / (float)TRACEDRENDERING_BVH_HIERARCHY_GROUPSIZE), 1, 1));
}
