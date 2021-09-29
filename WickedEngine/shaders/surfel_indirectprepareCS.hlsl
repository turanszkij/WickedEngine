#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWRAWBUFFER(surfelStatsBuffer, 0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_NEXTCOUNT);
	surfel_count = clamp(surfel_count, 0, SURFEL_CAPACITY);

	int dead_count = asint(surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_DEADCOUNT));
	dead_count = clamp(dead_count, 0, SURFEL_CAPACITY);

	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_COUNT, surfel_count);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_NEXTCOUNT, 0);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_DEADCOUNT, dead_count);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_CELLALLOCATOR, 0);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_RAYCOUNT, 0);

	surfelStatsBuffer.Store3(SURFEL_STATS_OFFSET_INDIRECT, uint3((surfel_count + SURFEL_INDIRECT_NUMTHREADS - 1) / SURFEL_INDIRECT_NUMTHREADS, 1, 1));
}
