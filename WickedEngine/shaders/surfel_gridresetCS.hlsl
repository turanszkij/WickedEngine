#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWSTRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, 0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	surfelGridBuffer[DTid.x].count = 0;
}
