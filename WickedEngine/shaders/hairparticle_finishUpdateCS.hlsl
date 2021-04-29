#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

RWRAWBUFFER(counterBuffer, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint particleCount = counterBuffer.Load(0);

	AllMemoryBarrier();

	// Reset counter for next frame:
	counterBuffer.Store(0, 0);

	// Create draw argument buffer (VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation):
	counterBuffer.Store4(4, uint4(4, particleCount, 0, 0));
}
