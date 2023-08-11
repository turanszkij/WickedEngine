#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWByteAddressBuffer surfelStatsBuffer : register(u0);
RWByteAddressBuffer surfelIndirectBuffer : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_NEXTCOUNT);
	surfel_count = clamp(surfel_count, 0, SURFEL_CAPACITY);

	int dead_count = asint(surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_DEADCOUNT));
	int shortage = max(0, -dead_count); // if deadcount was negative, there was shortage
	dead_count = clamp(dead_count, 0, (int)SURFEL_CAPACITY);

	uint ray_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_RAYCOUNT);

	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_COUNT, surfel_count);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_NEXTCOUNT, 0);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_DEADCOUNT, dead_count);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_CELLALLOCATOR, 0);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_RAYCOUNT, 0);
	surfelStatsBuffer.Store(SURFEL_STATS_OFFSET_SHORTAGE, shortage);

	surfelIndirectBuffer.Store3(SURFEL_INDIRECT_OFFSET_ITERATE, uint3((surfel_count + SURFEL_INDIRECT_NUMTHREADS - 1) / SURFEL_INDIRECT_NUMTHREADS, 1, 1));
	surfelIndirectBuffer.Store3(SURFEL_INDIRECT_OFFSET_RAYTRACE, uint3((ray_count + SURFEL_INDIRECT_NUMTHREADS - 1) / SURFEL_INDIRECT_NUMTHREADS, 1, 1));
	surfelIndirectBuffer.Store3(SURFEL_INDIRECT_OFFSET_INTEGRATE, uint3(surfel_count, 1, 1));
}
