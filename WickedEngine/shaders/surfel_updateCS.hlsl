#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "brdf.hlsli"

StructuredBuffer<uint> surfelAliveBuffer_CURRENT : register(t1);

RWStructuredBuffer<Surfel> surfelBuffer : register(u0);
RWStructuredBuffer<SurfelGridCell> surfelGridBuffer : register(u1);
RWStructuredBuffer<uint> surfelAliveBuffer_NEXT : register(u2);
RWStructuredBuffer<uint> surfelDeadBuffer : register(u3);
RWByteAddressBuffer surfelStatsBuffer : register(u4);
RWStructuredBuffer<SurfelRayDataPacked> surfelRayBuffer : register(u5);
RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u6);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelAliveBuffer_CURRENT[DTid.x];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	Surfel surfel = surfelBuffer[surfel_index];

	float radius = SURFEL_MAX_RADIUS;

	PrimitiveID prim;
	prim.unpack2(surfel_data.primitiveID);

	Surface surface;
	surface.init();
	surface.uid_validate = surfel_data.uid;
	if (surface.load(prim, unpack_half2(surfel_data.bary)))
	{
		if(surfel_data.IsBackfaceNormal())
		{
			surface.facenormal = -surface.facenormal;
		}
		surfel.normal = pack_unitvector(surface.facenormal);
		surfel.position = surface.P;

		int3 center_cell = surfel_cell(surfel.position);
		for (uint i = 0; i < 27; ++i)
		{
			int3 gridpos = center_cell + surfel_neighbor_offsets[i];

			if(surfel_cellintersects(surfel, gridpos))
			{
				uint cellindex = surfel_cellindex(gridpos);
				InterlockedAdd(surfelGridBuffer[cellindex].count, 1);
			}
		}
	}
	else
	{
		radius = 0;
	}

	bool shortage = true;
	shortage = asint(surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_SHORTAGE)) > 0;
	if (surfel_data.GetRecycle() > SURFEL_RECYCLE_TIME && shortage)
	{
		radius = 0;
	}

	if (radius > 0)
	{
		uint aliveCount;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_NEXTCOUNT, 1, aliveCount);
		surfelAliveBuffer_NEXT[aliveCount] = surfel_index;

		// Determine ray count for surfel:
		uint rayCountRequest = saturate(surfel_data.max_inconsistency) * SURFEL_RAY_BOOST_MAX;
		const uint recycle = surfel_data.GetRecycle();
		if (recycle > 10)
		{
			rayCountRequest = 1;
		}
		if (recycle > 60)
		{
			rayCountRequest = 0;
		}
		uint rayOffset = 0;
		if (rayCountRequest > 0)
		{
			surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_RAYCOUNT, rayCountRequest, rayOffset);
		}
		uint rayCount = (rayOffset < SURFEL_RAY_BUDGET) ? rayCountRequest : 0;
		rayCount = clamp(rayCount, 0, SURFEL_RAY_BUDGET - rayOffset);
		rayCount &= 0xFF;

		surfel_data.raydata = 0;
		surfel_data.raydata |= rayOffset & 0xFFFFFF;
		surfel_data.raydata |= rayCount << 24u;

		surfelBuffer[surfel_index] = surfel;
		surfelDataBuffer[surfel_index] = surfel_data;

		SurfelRayData initialRayData = (SurfelRayData)0;
		initialRayData.surfelIndex = surfel_index;
		SurfelRayDataPacked initialRayDataPacked;
		initialRayDataPacked.store(initialRayData);
		for (uint rayIndex = 0; rayIndex < rayCount; ++rayIndex)
		{
			surfelRayBuffer[rayOffset + rayIndex] = initialRayDataPacked;
		}
	}
	else
	{
		int deadCount;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_DEADCOUNT, 1, deadCount);
		surfelDeadBuffer[deadCount] = surfel_index;
	}
}
