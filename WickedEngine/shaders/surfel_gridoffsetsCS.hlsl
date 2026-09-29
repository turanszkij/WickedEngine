#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWStructuredBuffer<SurfelGridCell> surfelGridBuffer : register(u0);
RWStructuredBuffer<uint> surfelCellBuffer : register(u1);
RWStructuredBuffer<SurfelStats> surfelStatsBuffer : register(u2);

[numthreads(SURFEL_GRIDOFFSETS_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= SURFEL_TOTAL_TABLE_SIZE)
		return;
	if (surfelGridBuffer[DTid.x].count == 0)
		return;
	InterlockedAdd(surfelStatsBuffer[0].cellAllocator, surfelGridBuffer[DTid.x].count, surfelGridBuffer[DTid.x].offset);
	surfelGridBuffer[DTid.x].count = 0;
}
