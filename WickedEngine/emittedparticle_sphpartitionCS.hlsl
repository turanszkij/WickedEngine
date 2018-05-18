#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

STRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 0);
RAWBUFFER(counterBuffer, 1);
STRUCTUREDBUFFER(particleBuffer, Particle, 2);

RWSTRUCTUREDBUFFER(cellIndexBuffer, uint, 0);

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);

	if (DTid.x < aliveCount)
	{
		uint particleIndex = aliveBuffer_CURRENT[DTid.x];
		Particle particle = particleBuffer[particleIndex];

		int3 cellIndex = floor(particle.position);
		uint flatCellIndex = SPH_GridHash(cellIndex);

		cellIndexBuffer[particleIndex] = flatCellIndex;
	}
}
