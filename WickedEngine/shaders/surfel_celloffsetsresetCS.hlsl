#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

RWSTRUCTUREDBUFFER(surfelCellOffsetBuffer, uint, 0);

[numthreads(64, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	surfelCellOffsetBuffer[DTid.x] = ~0;
}
