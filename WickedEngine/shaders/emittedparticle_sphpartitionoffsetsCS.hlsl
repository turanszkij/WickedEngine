#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

StructuredBuffer<uint> aliveBuffer_CURRENT : register(t0);
ByteAddressBuffer counterBuffer : register(t1);
StructuredBuffer<float> cellIndexBuffer : register(t2);

RWStructuredBuffer<uint> cellOffsetBuffer : register(u0);

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
