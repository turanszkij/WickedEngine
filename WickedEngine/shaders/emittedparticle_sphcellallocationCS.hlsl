#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

StructuredBuffer<uint> aliveBuffer_CURRENT : register(t0);
StructuredBuffer<Particle> particleBuffer : register(t1);

RWStructuredBuffer<ParticleCounters> counterBuffer : register(u0);
RWStructuredBuffer<SPHGridCell> cellBuffer : register(u1);

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (cellBuffer[DTid.x].count == 0)
		return;
	InterlockedAdd(counterBuffer[0].cellAllocator, cellBuffer[DTid.x].count, cellBuffer[DTid.x].offset);
	cellBuffer[DTid.x].count = 0; // will be reused for binning
}
