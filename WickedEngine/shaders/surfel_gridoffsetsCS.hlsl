#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWStructuredBuffer<SurfelGridCell> surfelGridBuffer : register(u0);
RWStructuredBuffer<uint> surfelCellBuffer : register(u1);
RWByteAddressBuffer surfelStatsBuffer : register(u2);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (surfelGridBuffer[DTid.x].count == 0)
		return;
	surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_CELLALLOCATOR, surfelGridBuffer[DTid.x].count, surfelGridBuffer[DTid.x].offset);
	surfelGridBuffer[DTid.x].count = 0;
}
