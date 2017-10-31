#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);
RWRAWBUFFER(indirectDispatchBuffer, 5);			// indirect simulation args

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Fill dispatch argument buffer for simulation:
	indirectDispatchBuffer.Store3(12, uint3(ceil((float)counterBuffer[0].aliveCount_CURRENT / (float)THREADCOUNT_SIMULATION), 1, 1));
}