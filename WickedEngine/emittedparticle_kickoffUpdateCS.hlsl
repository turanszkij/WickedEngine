#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(counterBuffer, uint4, 4);	// alive count, dead count, new alive count, realEmitCount
RWRAWBUFFER(indirectDispatchBuffer, 5);			// indirect kickoff args

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint deadCount = counterBuffer[0][1];

	// we can not emit more than there are free slots in the dead list:
	uint realEmitCount = min(deadCount, xEmitCount);

	// Fill dispatch argument buffer for simulation:
	indirectDispatchBuffer.Store3(0, uint3(ceil((float)realEmitCount / (float)THREADCOUNT_EMIT), 1, 1));

	// copy new alivelistcount to oldalivelistcount:
	counterBuffer[0][0] = counterBuffer[0][2];

	// also reset the new alive list count:
	counterBuffer[0][2] = 0;

	// and write real emit count:
	counterBuffer[0][3] = realEmitCount;
}