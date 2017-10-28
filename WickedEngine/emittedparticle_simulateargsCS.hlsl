#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(counterBuffer, uint3, 4);	// alive count, dead count, new alive count
RWRAWBUFFER(indirectSimulateBuffer, 5);			// indirectdispatch args

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint aliveCount_OLD = counterBuffer[0][0];

	// Fill dispatch argument buffer for simulation:
	indirectSimulateBuffer.Store3(0, uint3(ceil((float)aliveCount_OLD / (float)THREADCOUNT_SIMULATION), 1, 1));
}