#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader resets the BVH structure

RWRAWBUFFER(clusterCounterBuffer, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	clusterCounterBuffer.Store(0, 0);
}
