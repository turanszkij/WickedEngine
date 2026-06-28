#define TEXTURE_SLOT_NONUNIFORM
#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "brdf.hlsli"

StructuredBuffer<uint> surfelAliveBuffer_CURRENT : register(t1);

RWStructuredBuffer<Surfel> surfelBuffer : register(u0);
RWStructuredBuffer<SurfelGridCell> surfelGridBuffer : register(u1);
RWStructuredBuffer<uint> surfelAliveBuffer_NEXT : register(u2);
RWStructuredBuffer<uint> surfelDeadBuffer : register(u3);
RWStructuredBuffer<SurfelStats> surfelStatsBuffer : register(u4);
RWStructuredBuffer<SurfelRayDataPacked> surfelRayBuffer : register(u5);
RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u6);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer[0].count;
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelAliveBuffer_CURRENT[DTid.x];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	Surfel surfel = surfelBuffer[surfel_index];

	float radius = SURFEL_MAX_RADIUS;

	PrimitiveID prim;
	prim.init();
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
		surfel.normal = pack_half3(surface.facenormal);
		surfel.position = surface.P;

		// Pick the cascaded grid level from the surfel's distance and set its
		// radius to that level's cell size. Near surfaces stay at level 0
		// (solid base radius, so they never break into sub-cell dust); distant
		// ones get larger surfels at coarser levels. Set before grid insertion
		// since surfel_cellintersects() reads GetRadius().
		const uint level = surfel_level(surfel.position);
		surfel.SetRadius(surfel_cellsize(level));

		int3 center_cell = surfel_cell(surfel.position, level);
		for (uint i = 0; i < 27; ++i)
		{
			int3 gridpos = center_cell + surfel_neighbor_offsets[i];

			if(surfel_cellintersects(surfel, gridpos, level))
			{
				uint cellindex = surfel_cellindex(gridpos, level);
				InterlockedAdd(surfelGridBuffer[cellindex].count, 1);
			}
		}

		// Write the surfel (with this frame's position/normal/level radius)
		// now, so surfel_binningCS bins it into exactly the cells counted
		// above. A surfel that loads but is then recycled below still gets
		// binned this frame, so its stored radius must already match the
		// counted level.
		surfelBuffer[surfel_index] = surfel;
	}
	else
	{
		radius = 0;
	}

	// Relevance-based recycling bounds the live working set, and with it the
	// cost of every later pass plus the per-pixel gather. Each surfel is
	// evicted with a probability that rises as it becomes less relevant: how
	// long since it last contributed to a visible pixel (recency, from the seen
	// bit), distance from the camera, and the pool filling past a soft target
	// (pressure). Eviction is probabilistic and spread across frames so it
	// never mass-evicts a cluster at once. Surfels still contributing keep
	// recycle==0 (recency 0) and are effectively immortal.
	if (radius > 0)
	{
		const uint recycle = surfel_data.GetRecycle();
		const float recency = smoothstep(
			(float)SURFEL_RECYCLE_RECENCY_MIN,
			(float)SURFEL_RECYCLE_RECENCY_MAX,
			(float)recycle);
		const float dist = distance(surfel.position, GetCamera().position);
		const float distance_term = saturate(dist / SURFEL_RECYCLE_DISTANCE_FAR);

		// How disposable this surfel is in [0,1]: distant and/or long-unseen.
		const float disposability = saturate(0.5 * distance_term + 0.5 * recency);

		// Pool pressure relative to the soft target (1.0 == at target).
		const float pressure_ratio = (float)surfel_count / (float)SURFEL_LIVE_TARGET;
		const float over_frac = saturate(pressure_ratio - 1.0);

		// Under target: only a trickle (scaled by disposability) keeps the cache
		// fresh without discarding useful surfels. Over target: shed a real
		// fraction every frame - a baseline even for near/recent surfels, more
		// for disposable ones - so an all-visible view cannot grow the set
		// without bound.
		float p_recycle = disposability * SURFEL_RECYCLE_PRESSURE_FLOOR;
		p_recycle = max(p_recycle,
			over_frac * lerp(SURFEL_RECYCLE_OVERFLOW_MIN, 1.0, disposability));

		RNG rng;
		rng.init(uint2(surfel_index, 0), GetFrame().frame_count);
		if (rng.next_float() < p_recycle)
		{
			radius = 0;
		}
	}

	if (radius > 0)
	{
		uint aliveCount;
		InterlockedAdd(surfelStatsBuffer[0].nextCount, 1, aliveCount);
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
			InterlockedAdd(surfelStatsBuffer[0].rayCount, rayCountRequest, rayOffset);
		}
		uint rayCount = (rayOffset < SURFEL_RAY_BUDGET) ? rayCountRequest : 0;
		rayCount = clamp(rayCount, 0, SURFEL_RAY_BUDGET - rayOffset);
		rayCount &= 0xFF;

		surfel_data.raydata = 0;
		surfel_data.raydata |= rayOffset & 0xFFFFFF;
		surfel_data.raydata |= rayCount << 24u;

		// surfel (Surfel) was already written in the load block above; only the
		// per-surfel SurfelData (ray allocation) needs writing here.
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
		InterlockedAdd(surfelStatsBuffer[0].deadCount, 1, deadCount);
		surfelDeadBuffer[deadCount] = surfel_index;
	}
}
