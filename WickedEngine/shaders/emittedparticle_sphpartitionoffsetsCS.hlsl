#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

STRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 0);
RAWBUFFER(counterBuffer, 1);
STRUCTUREDBUFFER(cellIndexBuffer, float, 2);

RWSTRUCTUREDBUFFER(cellOffsetBuffer, uint, 0);

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);

	if (DTid.x < aliveCount)
	{
		uint particleIndex = aliveBuffer_CURRENT[DTid.x];
		uint cellIndex = (uint)cellIndexBuffer[particleIndex];

		InterlockedMin(cellOffsetBuffer[cellIndex], DTid.x);
	}
}
