#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWStructuredBuffer<SurfelGridCell> surfelGridBuffer : register(u0);
RWStructuredBuffer<uint> surfelCellBuffer : register(u1);
RWByteAddressBuffer surfelStatsBuffer : register(u2);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	SurfelGridCell cell = surfelGridBuffer[DTid.x];
	surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_CELLALLOCATOR, cell.count, cell.offset);
	cell.count = 0;
	surfelGridBuffer[DTid.x] = cell;
}
