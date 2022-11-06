#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

StructuredBuffer<uint> aliveBuffer_CURRENT : register(t0);
StructuredBuffer<Particle> particleBuffer : register(t1);

RWByteAddressBuffer counterBuffer : register(u0);
RWStructuredBuffer<SPHGridCell> cellBuffer : register(u1);
RWStructuredBuffer<uint> particleCellBuffer : register(u2);

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);
	if (DTid.x >= aliveCount)
		return;

	uint particleIndex = aliveBuffer_CURRENT[DTid.x];
	Particle particle = particleBuffer[particleIndex];

	// Grid cell is of size [SPH smoothing radius], so position is refitted into that
	float3 remappedPos = particle.position * xSPH_h_rcp;

	int3 cellIndex = floor(remappedPos);
	uint flatCellIndex = SPH_GridHash(cellIndex);

	uint prevCount;
	InterlockedAdd(cellBuffer[flatCellIndex].count, 1, prevCount);
	particleCellBuffer[cellBuffer[flatCellIndex].offset + prevCount] = particleIndex;
}
