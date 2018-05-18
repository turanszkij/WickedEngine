#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWRAWBUFFER(counterBuffer, 4);
RWRAWBUFFER(indirectBuffers, 5);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Load dead particle count:
	uint deadCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_DEADCOUNT);

	// Load alive particle count:
	uint aliveCount_NEW = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION);

	// we can not emit more than there are free slots in the dead list:
	uint realEmitCount = min(deadCount, xEmitCount);

	// Fill dispatch argument buffer for emitting (ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ):
	indirectBuffers.Store3(ARGUMENTBUFFER_OFFSET_DISPATCHEMIT, uint3(ceil((float)realEmitCount / (float)THREADCOUNT_EMIT), 1, 1));

	// Fill dispatch argument buffer for simulation (ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ):
	indirectBuffers.Store3(ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, uint3(ceil((float)(aliveCount_NEW + realEmitCount) / (float)THREADCOUNT_SIMULATION), 1, 1));

	// copy new alivelistcount to current alivelistcount:
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveCount_NEW);

	// reset new alivecount:
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, 0);

	// and write real emit count:
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_REALEMITCOUNT, realEmitCount);
}
