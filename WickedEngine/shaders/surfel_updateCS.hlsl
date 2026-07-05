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
#ifdef SURFEL_RAY_SORTING
RWStructuredBuffer<uint> surfelRaySortKeyBuffer : register(u7);     // Morton key per ray slot
RWStructuredBuffer<uint> surfelRaySortPayloadBuffer : register(u8); // ray slot index (identity, sorted by key)
#endif // SURFEL_RAY_SORTING

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
	bool valid_surface = surface.load(prim, unpack_half2(surfel_data.bary));
	if (valid_surface && surfel_data.IsBackfaceNormal())
	{
		surface.facenormal = -surface.facenormal;
	}
	// Reject a degenerate / non-finite face normal. surface.load
	// (load_internal) does N = normalize(N) with NO zero-guard
	// (surfaceHF.hlsli), so geometry with missing / degenerate / cancelling
	// vertex normals - common on foliage cards, billboards and impostors -
	// yields a NaN facenormal. Stored here and normalize()-d again in the
	// coverage gather, that NaN becomes NaN GI that flashes the whole grid
	// cell's screen quad black/white (the surfel cache broadcasts one bad
	// normal over the cell's entire footprint). Such a surfel is useless
	// anyway, so treat it exactly like a load failure below (radius 0, not
	// counted, not binned, retired to the dead list) - keeping NaN out of the
	// cache at the source. A valid normalized normal has length^2 ~ 1; NaN/zero
	// fail this test.
	if (valid_surface)
	{
		valid_surface =
			!any(isnan(surface.facenormal)) &&
			!any(isinf(surface.facenormal)) &&
			dot(surface.facenormal, surface.facenormal) > 0.5;
	}
	if (valid_surface)
	{
		surfel.normal = pack_half3(surface.facenormal);
		surfel.position = surface.P;

		// Pick the cascaded grid level from the surfel's distance and set its
		// radius to that level's cell size. Near surfaces stay at level 0
		// (solid base radius, so they never break into sub-cell dust); distant
		// ones get larger surfels at coarser levels. Set before grid insertion
		// since surfel_cellintersects() reads GetRadius(). surfel_stable_level
		// adds hysteresis + a per-surfel dithered threshold so a moving camera
		// doesn't flip a whole region's level on one frame (which pops); a
		// newborn (life 0) snaps straight to its distance level as it has no
		// current one.
		const uint level = surfel_stable_level(
			surfel.position, surfel.GetRadius(), surfel_index,
			surfel_data.GetLife() == 0);
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

		// surface.load failed (stale / streamed-out / LOD-swapped geometry).
		// The grid counting above is INSIDE the load-success block, so this
		// surfel was NOT counted into any cell this frame. But surfelBuffer
		// still holds last frame's positive radius, and surfel_binningCS bins
		// every surfel with GetRadius() > 0 - so it would bin this surfel into
		// cells that were never counted, overflowing their allocation into the
		// neighbouring cell's list. A gather on that corrupted cell then reads
		// a garbage/uninitialised surfel index; an uninitialised surfel has a
		// zero normal, and normalize(0) = NaN, which the dotN <= 0 reject does
		// not catch (NaN <= 0 is false) - so the whole cell's screen quad
		// flashes black/white for the frame. Zero the stored radius so binning
		// skips this surfel too, keeping the "counted" and "binned" sets
		// identical.
		surfel.SetRadius(0);
		surfelBuffer[surfel_index] = surfel;
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

		// Recency in [0,1]: 0 while the surfel keeps contributing to visible
		// pixels (seen this frame => recycle 0), ramping to 1 only after it has
		// gone unseen for RECENCY_MIN..MAX frames. This MUST gate eviction: a
		// surfel that is still seen has to be immortal, otherwise an in-view
		// cube/Cornell scene churns (points popping in and out).
		const float recency = smoothstep(
			(float)SURFEL_RECYCLE_RECENCY_MIN,
			(float)SURFEL_RECYCLE_RECENCY_MAX,
			(float)recycle);

		// Distance only biases which already-evictable surfels go first (1x
		// near .. BOOST far); it never by itself recycles a surfel that is
		// still seen.
		const float dist = distance(surfel.position, GetCamera().position);
		const float distance_term = saturate(dist / SURFEL_RECYCLE_DISTANCE_FAR);
		const float distance_bias = lerp(1.0, SURFEL_RECYCLE_DISTANCE_BOOST, distance_term);

		// Pool pressure relative to the soft target (1.0 == at target).
		const float pressure_ratio = (float)surfel_count / (float)SURFEL_LIVE_TARGET;
		const float over_frac = saturate(pressure_ratio - 1.0);

		// (a) Staleness trickle, recency- AND fill-gated: long-unseen surfels
		//     are reclaimed only as the pool fills toward the target (from
		//     SURFEL_RECYCLE_TRICKLE_START of it upward), distant ones first.
		//     Below that fill the pool has headroom, so off-screen surfels are
		//     KEPT - pan a full surface off screen and back and its cached GI
		//     is still there, instead of the whole cache draining just because
		//     it left the view. recency still gates it (a surfel seen this
		//     frame is never touched); this only decides WHEN the unseen ones
		//     age out.
		const float trickle_pressure = saturate(
			(pressure_ratio - SURFEL_RECYCLE_TRICKLE_START) /
			max(1e-3, 1.0 - SURFEL_RECYCLE_TRICKLE_START));
		float p_recycle = recency * distance_bias
			* SURFEL_RECYCLE_PRESSURE_FLOOR * trickle_pressure;

		// (b) Overflow shed, only past the soft target and there regardless of
		//     recency (an all-visible view has recency 0 everywhere), still
		//     biased toward distant surfels - bounds the set when the whole
		//     scene is visible at once (sky looking down, dense foliage).
		p_recycle = max(p_recycle,
			over_frac * distance_bias * SURFEL_RECYCLE_OVERFLOW_GAIN);

		// (c) Thinning ("discard on recede"): surfel_integrate flagged this
		//     surfel redundant because a same-orientation neighbour already
		//     covers its spot at the current (grown) spacing. Recycle it at
		//     SURFEL_THIN_RATE regardless of recency - this is what makes
		//     over-dense far-away regions relax back to the spacing target as
		//     surfels grow.
		if (surfel_data.IsRedundant())
			p_recycle = max(p_recycle, SURFEL_THIN_RATE);

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

		// Determine ray count for surfel. Scale the per-surfel ray boost down
		// with cascade level: near/fine surfels get the full
		// SURFEL_RAY_BOOST_MAX for detail, coarse far ones ramp toward
		// SURFEL_RAY_BOOST_MIN. Far surfels are low-frequency and long-lived,
		// so fewer rays per frame still converge via the temporal estimator,
		// but a huge far view (looking down from altitude) no longer saturates
		// the ray budget on rays the far field doesn't need.
		const uint ray_level = surfel_level_from_radius(surfel.GetRadius());
		const float level_frac =
			(float)ray_level / (float)(SURFEL_GRID_LEVELS - 1);
		const uint ray_boost = (uint)lerp(
			SURFEL_RAY_BOOST_MAX, SURFEL_RAY_BOOST_MIN, level_frac);

		// Temporal ray amortization: a surfel doesn't need fresh rays every
		// frame. Give it a re-trace PERIOD that grows with distance and only
		// allocate rays on its turn; the temporal estimator holds the cached
		// radiance in between. A per-surfel phase (stable hash) staggers the
		// turns so a region's surfels don't all re-trace on the same frame
		// (which pulses). This is what cuts the per-frame ray count when a big
		// far area is in view - only ~1/period of the far field traces each
		// frame - while the far surfels' long life + fade-in hide the slower
		// convergence.
		//
		// The period is driven by the CONTINUOUS (unclamped) distance level,
		// not the clamped cascade level, so it keeps growing for surfels far
		// beyond the coarsest level: 1 (every frame) near, ramping to
		// SURFEL_RAY_UPDATE_PERIOD_MAX at the coarsest cascade level, then
		// doubling per extra level of distance past it up to
		// SURFEL_RAY_UPDATE_PERIOD_CAP. A bird's-eye view saturates every
		// surfel at the top cascade level by radius but not by distance, and
		// the truly distant ones there need refreshing least - this extends the
		// amortization into exactly that worst case (flying very high).
		const float cont_level = surfel_level_continuous(surfel.position);
		const float top_level = (float)(SURFEL_GRID_LEVELS - 1);
		float ray_period_f = lerp(1.0,
			(float)SURFEL_RAY_UPDATE_PERIOD_MAX, saturate(cont_level / top_level));
		ray_period_f *= exp2(max(0.0, cont_level - top_level)); // extend past top
		const uint ray_period = (uint)clamp(round(ray_period_f),
			1.0, (float)SURFEL_RAY_UPDATE_PERIOD_CAP);
		const uint ray_phase =
			(uint)(surfel_hash01(surfel_index) * (float)ray_period);
		const bool ray_update =
			((GetFrame().frame_count + ray_phase) % ray_period) == 0;

		uint rayCountRequest = saturate(surfel_data.max_inconsistency) * ray_boost;
		const uint recycle = surfel_data.GetRecycle();
		if (recycle > 10)
		{
			rayCountRequest = 1;
		}
		if (recycle > 60)
		{
			rayCountRequest = 0;
		}
		// Amortization gate LAST, so it can only skip a frame's rays (drop to 0
		// off-turn), never add rays on top of the recycle maintenance above.
		if (!ray_update)
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
#ifdef SURFEL_RAY_SORTING
		// All of this surfel's rays share its origin, so they share a sort key
		// - the Morton code of the surfel position quantised to
		// SURFEL_RAY_SORT_CELL. Payload is the ray's own slot (identity); the
		// sort permutes it so the raytrace can remap thread -> original slot
		// and write results back there.
		const uint ray_key = morton3D(frac(
			surfel.position / (SURFEL_RAY_SORT_CELL * 1024.0)));
		if (rayCount > 0)
			InterlockedAdd(surfelStatsBuffer[0].raySortCount, rayCount);
#endif // SURFEL_RAY_SORTING
		for (uint rayIndex = 0; rayIndex < rayCount; ++rayIndex)
		{
			const uint ray_slot = rayOffset + rayIndex;
			surfelRayBuffer[ray_slot] = initialRayDataPacked;
#ifdef SURFEL_RAY_SORTING
			surfelRaySortKeyBuffer[ray_slot] = ray_key;
			surfelRaySortPayloadBuffer[ray_slot] = ray_slot;
#endif // SURFEL_RAY_SORTING
		}
	}
	else
	{
		int deadCount;
		InterlockedAdd(surfelStatsBuffer[0].deadCount, 1, deadCount);
		surfelDeadBuffer[deadCount] = surfel_index;
	}
}
