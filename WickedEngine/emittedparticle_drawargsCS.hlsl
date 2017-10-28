#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(counterBuffer, uint3, 4);	// alive count, dead count, new alive count
RWRAWBUFFER(indirectDrawBuffer, 6);				// indirectdrawinstanced args

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint aliveCount_NEW = counterBuffer[0][2];

	// Fill Draw arguments for drawing:
	//		VertexCountPerInstance;
	//		InstanceCount;
	//		StartVertexLocation;
	//		StartInstanceLocation;
	indirectDrawBuffer.Store4(0, uint4(aliveCount_NEW * 6, 1, 0, 0));
}