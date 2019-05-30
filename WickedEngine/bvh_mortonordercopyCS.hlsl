#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader iterates through all primitives and copies the morton codes to the sorted index buffer order. Morton codes can be used after this to assemble the BVH.

RAWBUFFER(primitiveCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(primitiveIDBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(primitiveMortonBuffer, float, TEXSLOT_ONDEMAND2); // morton buffer is float because sorting is written for floats!

RWSTRUCTUREDBUFFER(primitiveSortedMortonBuffer, uint, 0);

[numthreads(BVH_BUILDER_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x < primitiveCounterBuffer.Load(0))
	{
		const uint primitiveID = primitiveIDBuffer[DTid.x];
		primitiveSortedMortonBuffer[DTid.x] = (uint)primitiveMortonBuffer[primitiveID];
	}
}
