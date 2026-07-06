#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWStructuredBuffer<SurfelStats> surfelStatsBuffer : register(u0);
RWStructuredBuffer<SurfelIndirectArgs> surfelIndirectBuffer : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer[0].nextCount;
	surfel_count = clamp(surfel_count, 0, SURFEL_CAPACITY);

	int dead_count = surfelStatsBuffer[0].deadCount;
	int shortage = max(0, -dead_count); // if deadcount was negative, there was shortage
	dead_count = clamp(dead_count, 0, (int)SURFEL_CAPACITY);

	// rayCount is the uncapped SUM of every surfel's ray request, which can
	// exceed SURFEL_RAY_BUDGET (e.g. a huge far view). Only the first
	// budget-worth of ray slots are actually written, so bound the dispatch to
	// the budget - otherwise the raytrace dispatches threads for rays that were
	// never written (stale slots) and the budget knob has no effect on cost.
	uint ray_count = min(surfelStatsBuffer[0].rayCount, SURFEL_RAY_BUDGET);

	surfelStatsBuffer[0].count = surfel_count;
	surfelStatsBuffer[0].nextCount = 0;
	surfelStatsBuffer[0].deadCount = dead_count;
	surfelStatsBuffer[0].cellAllocator = 0;
	surfelStatsBuffer[0].rayCount = 0;
	surfelStatsBuffer[0].shortage = shortage;
	surfelStatsBuffer[0].spawnCount = 0;
	surfelStatsBuffer[0].raySortCount = 0;

	SurfelIndirectArgs args;
	
	args.iterate.ThreadGroupCountX = (surfel_count + SURFEL_INDIRECT_NUMTHREADS - 1) / SURFEL_INDIRECT_NUMTHREADS;
	args.iterate.ThreadGroupCountY = 1;
	args.iterate.ThreadGroupCountZ = 1;
	
	args.raytrace.ThreadGroupCountX = (ray_count + SURFEL_INDIRECT_NUMTHREADS - 1) / SURFEL_INDIRECT_NUMTHREADS;
	args.raytrace.ThreadGroupCountY = 1;
	args.raytrace.ThreadGroupCountZ = 1;
	
	args.integrate.ThreadGroupCountX = surfel_count;
	args.integrate.ThreadGroupCountY = 1;
	args.integrate.ThreadGroupCountZ = 1;

	surfelIndirectBuffer[0] = args;
}
