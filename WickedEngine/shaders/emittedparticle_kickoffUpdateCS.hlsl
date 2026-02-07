#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWByteAddressBuffer counterBuffer : register(u4);
RWStructuredBuffer<EmitterIndirectArgs> indirectBuffer : register(u5);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION);
	int deadCount = counterBuffer.Load<int>(PARTICLECOUNTER_OFFSET_DEADCOUNT);

	IndirectDispatchArgs args;
	args.ThreadGroupCountX = (aliveCount + THREADCOUNT_SIMULATION - 1) / THREADCOUNT_SIMULATION;
	args.ThreadGroupCountY = 1;
	args.ThreadGroupCountZ = 1;
	indirectBuffer[0].dispatch = args;
	
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveCount);
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, 0);
	
	counterBuffer.Store<int>(PARTICLECOUNTER_OFFSET_DEADCOUNT, max(0, deadCount));
	
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_CULLEDCOUNT, 0);
	
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_CELLALLOCATOR, 0);
}
