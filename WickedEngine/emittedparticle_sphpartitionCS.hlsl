#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

STRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 0);
RAWBUFFER(counterBuffer, 1);
STRUCTUREDBUFFER(particleBuffer, Particle, 2);

RWSTRUCTUREDBUFFER(cellIndexBuffer, float, 0);

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);

	if (DTid.x < aliveCount)
	{
		uint particleIndex = aliveBuffer_CURRENT[DTid.x];
		Particle particle = particleBuffer[particleIndex];

		// Grid cell is of size [SPH smoothing radius], so position is refitted into that
		float3 remappedPos = particle.position * xSPH_h_rcp;

		int3 cellIndex = floor(remappedPos);
		uint flatCellIndex = SPH_GridHash(cellIndex);

		cellIndexBuffer[particleIndex] = (float)flatCellIndex;
	}
}
