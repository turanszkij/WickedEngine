#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWStructuredBuffer<ParticleCounters> counterBuffer : register(u4);
RWStructuredBuffer<EmitterIndirectArgs> indirectBuffer : register(u5);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint aliveCount = counterBuffer[0].aliveCount_afterSimulation;
	int deadCount = counterBuffer[0].deadCount;

	IndirectDispatchArgs args;
	args.ThreadGroupCountX = (aliveCount + THREADCOUNT_SIMULATION - 1) / THREADCOUNT_SIMULATION;
	args.ThreadGroupCountY = 1;
	args.ThreadGroupCountZ = 1;
	indirectBuffer[0].dispatch = args;
	
	counterBuffer[0].aliveCount = aliveCount;
	counterBuffer[0].aliveCount_afterSimulation = 0;
	
	counterBuffer[0].deadCount = max(0, deadCount);
	
	counterBuffer[0].culledCount = 0;
	
	counterBuffer[0].cellAllocator = 0;
}
