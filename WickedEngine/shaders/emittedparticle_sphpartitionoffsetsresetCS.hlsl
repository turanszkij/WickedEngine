#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(cellOffsetBuffer, uint, 0);

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	cellOffsetBuffer[DTid.x] = 0xFFFFFFFF; // invalid offset
}
