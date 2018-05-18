#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

STRUCTUREDBUFFER(counterBuffer, ParticleCounters, 0);

RWRAWBUFFER(indirectBuffers, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint particleCount = counterBuffer[0].aliveCount_afterSimulation;
	
	// Create draw argument buffer (VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation):
	indirectBuffers.Store4(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, uint4(particleCount * 6, 1, 0, 0));
}
