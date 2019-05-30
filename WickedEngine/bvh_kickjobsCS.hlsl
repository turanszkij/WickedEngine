#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader will kick off the BVH builder GPU jobs such as:
//	- BVH hierarchy, AABB propagation
//	- BVH bottom level builder

RAWBUFFER(primitiveCounterBuffer, TEXSLOT_ONDEMAND0);

RWRAWBUFFER(indirectDispatchBuffer, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint primitiveCount = primitiveCounterBuffer.Load(0);

	// Write dispatch argument buffer:
	indirectDispatchBuffer.Store3(0, uint3(ceil((float)(primitiveCount) / (float)BVH_BUILDER_GROUPSIZE), 1, 1));
}
