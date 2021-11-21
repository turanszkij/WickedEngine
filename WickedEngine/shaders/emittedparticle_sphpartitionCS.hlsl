#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

StructuredBuffer<uint> aliveBuffer_CURRENT : register(t0);
ByteAddressBuffer counterBuffer : register(t1);
StructuredBuffer<Particle> particleBuffer : register(t2);

RWStructuredBuffer<float> cellIndexBuffer : register(u0);

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
