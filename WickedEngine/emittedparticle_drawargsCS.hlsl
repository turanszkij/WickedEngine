#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);
RWRAWBUFFER(indirectDrawBuffer, 5);				// indirectdrawinstanced args

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Fill Draw arguments for drawing:
	//		VertexCountPerInstance;
	//		InstanceCount;
	//		StartVertexLocation;
	//		StartInstanceLocation;
	indirectDrawBuffer.Store4(24, uint4(counterBuffer[0].aliveCount_NEW * 6, 1, 0, 0));
}