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

	uint ray_count = surfelStatsBuffer[0].rayCount;

	surfelStatsBuffer[0].count = surfel_count;
	surfelStatsBuffer[0].nextCount = 0;
	surfelStatsBuffer[0].deadCount = dead_count;
	surfelStatsBuffer[0].cellAllocator = 0;
	surfelStatsBuffer[0].rayCount = 0;
	surfelStatsBuffer[0].shortage = shortage;

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
