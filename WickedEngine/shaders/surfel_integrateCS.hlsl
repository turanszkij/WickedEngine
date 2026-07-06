#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "bc6h.hlsli"

static const float WEIGHT_EPSILON = 0.0001;

ByteAddressBuffer surfelStatsBuffer : register(t0);
StructuredBuffer<SurfelGridCell> surfelGridBuffer : register(t1);
StructuredBuffer<uint> surfelCellBuffer : register(t2);
StructuredBuffer<uint> surfelAliveBuffer : register(t3);
StructuredBuffer<SurfelRayDataPacked> surfelRayBuffer : register(t4);

RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u0);
RWStructuredBuffer<SurfelVarianceDataPacked> surfelVarianceBuffer : register(u1);
RWTexture2D<float2> surfelMomentsTexture : register(u2);
RWStructuredBuffer<Surfel> surfelBuffer : register(u3);

static const uint THREADCOUNT = 8;
static const uint CACHE_SIZE = THREADCOUNT * THREADCOUNT;
groupshared SurfelRayData ray_cache[CACHE_SIZE];
groupshared float3 shared_texels[SURFEL_MOMENT_RESOLUTION * SURFEL_MOMENT_RESOLUTION];
groupshared float shared_inconsistency[CACHE_SIZE];
// Birth seeding (life == 0): neighbor-cache radiance + total weight, gathered
// once per group and read by every texel thread to seed a newborn surfel.
groupshared SH::L1_RGB shared_seed_sh;
groupshared float shared_seed_weight;
groupshared float shared_seed_max_lum; // brightest neighbor luminance; clamps the seed so a newborn can never exceed its surroundings (keeps birth seeding strictly non-amplifying)

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint surfel_index = surfelAliveBuffer[Gid.x];
	Surfel surfel = surfelBuffer[surfel_index];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	uint life = surfel_data.GetLife();
	uint recycle = surfel_data.GetRecycle();
	bool backface = surfel_data.IsBackfaceNormal();
	float maxDistance = surfel.GetRadius();

	const float3 P = surfel.position;
	const float3 N = normalize(unpack_half3(surfel.normal));

	float3 texel_direction = decode_hemioct(((GTid.xy + 0.5) / (float2)SURFEL_MOMENT_RESOLUTION) * 2 - 1);
	texel_direction = mul(texel_direction, get_tangentspace(N));
	texel_direction = normalize(texel_direction);

	// Birth seeding: a newborn surfel (life == 0) seeds its radiance from the
	// surrounding surfel cache so it never starts black - a ray-starved newborn
	// would otherwise fold a zero sample and pop black until it converges.
	// Gather once per group (life is uniform across the group) into shared
	// memory; the texel threads below evaluate it in their own direction. Reads
	// last-frame neighbor radiance (this frame's writes happen later in this
	// pass), normal-gated so we only seed from similarly oriented surfaces.
	//
	// Searches the surfel's own level AND the +/-1 neighbour levels - the same
	// span the coverage gather uses. A newborn placed at a level boundary
	// (common while the camera moves, or on a large plane crossing near->far)
	// is surrounded by differently-SIZED surfels binned into adjacent levels;
	// reading only its own level would find nothing and it would fall back to
	// its raw rays and pop. Each neighbour is gated by ITS OWN radius, so
	// seeding across sizes is correct. If there are genuinely no neighbours the
	// weight stays 0 and the texels fall back to this frame's rays.
	if (life == 0)
	{
		if (groupIndex == 0)
		{
			SH::L1_RGB seed_sh = SH::L1_RGB::Zero();
			float seed_weight = 0;
			float seed_max_lum = 0;

			const uint base_level = surfel_level_from_radius(surfel.GetRadius());
			const uint level_lo = (base_level > 0) ? (base_level - 1) : 0;
			const uint level_hi = min(base_level + 1, SURFEL_GRID_LEVELS - 1);
			for (uint lvl = level_lo; lvl <= level_hi; ++lvl)
			{
				const int3 seed_cell = surfel_cell(P, lvl);
				if (!surfel_cellvalid(seed_cell))
					continue;

				SurfelGridCell cell =
					surfelGridBuffer[surfel_cellindex(seed_cell, lvl)];
				for (uint i = 0; i < cell.count; ++i)
				{
					const uint nbr_index = surfelCellBuffer[cell.offset + i];
					if (nbr_index == surfel_index)
						continue;

					Surfel nbr = surfelBuffer[nbr_index];
					const float3 nbr_normal = normalize(unpack_half3(nbr.normal));
					const float dotN = dot(N, nbr_normal);
					if (dotN <= 0)
						continue;

					const float3 to_self = P - nbr.position;
					const float dist2 = dot(to_self, to_self);
					if (dist2 >= sqr(nbr.GetRadius()))
						continue;

					// Weight the seed by the SAME geometry weight the coverage
					// gather uses (radial^2 * normal^k * tangent-plane), not a
					// flat dotN. Irradiance is linear in the SH, so a
					// gather-weighted average of neighbour SH makes the
					// newborn's irradiance equal what the surface already shows
					// at this point - the spawn adds (nearly) nothing new to
					// the pixel, so it can't pop.
					const float w = surfel_geometry_weight(
						to_self, nbr_normal, nbr.GetRadius(), dist2, dotN);
					if (w <= 0)
						continue;

					const SH::L1_RGB nbr_sh = nbr.radiance.Unpack();
					seed_sh = SH::Add(seed_sh, SH::Multiply(nbr_sh, w));
					seed_weight += w;

					// Track the brightest neighbor (radiance toward this
					// surfel's normal) so the seed can be clamped below it.
					const float3 nbr_rad = max(0, SH::Evaluate(nbr_sh, N));
					seed_max_lum = max(seed_max_lum,
						dot(float3(0.299, 0.587, 0.114), nbr_rad));
				}
			}

			if (seed_weight > 0)
				seed_sh = SH::Multiply(seed_sh, rcp(seed_weight));

			shared_seed_sh = seed_sh;
			shared_seed_weight = seed_weight;
			shared_seed_max_lum = seed_max_lum;
		}
		GroupMemoryBarrierWithGroupSync();
	}

	float3 result = 0;
	float2 result_depth = 0;
	float total_weight = 0;
	float total_depth_weight = 0;

	uint remaining_rays = surfel_data.GetRayCount();
	uint offset = surfel_data.GetRayOffset();
	while (remaining_rays > 0)
	{
		uint num_rays = min(CACHE_SIZE, remaining_rays);

		if (groupIndex < num_rays)
		{
			ray_cache[groupIndex] = surfelRayBuffer[offset + groupIndex].load();
		}

		GroupMemoryBarrierWithGroupSync();

		for (uint r = 0; r < num_rays; ++r)
		{
			SurfelRayData ray = ray_cache[r];

			float depth;
			if (ray.depth > 0)
			{
				depth = clamp(ray.depth, 0, maxDistance);
			}
			else
			{
				depth = maxDistance;
			}

			float weight = saturate(dot(texel_direction, ray.direction) + 0.01);
			if (weight > WEIGHT_EPSILON)
			{
				result += ray.radiance.rgb * weight;
				total_weight += weight;
			}
			
			float depth_weight = pow(weight, 32);
			if (depth_weight > WEIGHT_EPSILON)
			{
				result_depth += float2(depth, sqr(depth)) * depth_weight;
				total_depth_weight += depth_weight;
			}
		}

		GroupMemoryBarrierWithGroupSync();

		remaining_rays -= num_rays;
		offset += num_rays;
	}

	if (total_weight > WEIGHT_EPSILON)
	{
		result /= total_weight;
	}
	
	if (total_depth_weight > WEIGHT_EPSILON)
	{
		result_depth /= total_depth_weight;
	}
	
	const uint2 moments_topleft = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_RESOLUTION;

	float inconsistency = 0;
	
	if (GTid.x < SURFEL_MOMENT_RESOLUTION && GTid.y < SURFEL_MOMENT_RESOLUTION)
	{
		uint2 moments_pixel = moments_topleft + GTid.xy;

		const uint idx = flatten2D(GTid.xy, SURFEL_MOMENT_RESOLUTION);
		const uint variance_data_index = surfel_index * SURFEL_MOMENT_RESOLUTION * SURFEL_MOMENT_RESOLUTION + idx;
		SurfelVarianceData varianceData = surfelVarianceBuffer[variance_data_index].load();
		if (life == 0)
		{
			varianceData = (SurfelVarianceData)0;
			// Seed from the neighbor cache (evaluated in this texel's
			// direction) when we have neighbors, else fall back to this frame's
			// rays.
			float3 seed = result;
			if (shared_seed_weight > 0)
			{
				seed = max(0, SH::Evaluate(shared_seed_sh, texel_direction));

				// Clamp to the brightest neighbor so a newborn can never be
				// brighter than its surroundings. This makes birth seeding
				// strictly non-amplifying. Clamped, the regional max can only
				// decrease.
				const float seed_lum = dot(float3(0.299, 0.587, 0.114), seed);
				if (seed_lum > shared_seed_max_lum)
					seed *= shared_seed_max_lum / max(seed_lum, 1e-5);
			}
			varianceData.mean = seed;
			varianceData.shortMean = seed;
			varianceData.inconsistency = 1;
		}

		// Fold this frame's sample into the estimator only when this texel
		// actually received rays. Under ray-budget pressure a surfel can get
		// few or no rays; folding the resulting zero would drag its mean toward
		// black, and because the starved set rotates frame to frame that shows
		// up as flicker. A newborn (life == 0) still stores once to commit its
		// seed above, but only folds a real sample when it has rays - so a
		// ray-starved newborn keeps its seed instead of going black.
		const bool has_rays = total_weight > WEIGHT_EPSILON;
		if (has_rays || life == 0)
		{
			if (has_rays)
				MultiscaleMeanEstimator(result, varianceData, 0.1);
			surfelVarianceBuffer[variance_data_index].store(varianceData);
		}

		// Depth (occlusion) moments. Only updated when this texel was sampled -
		// except a newborn (life == 0) always commits once, to seed them.
		//
		// A newborn texel that got no depth sample would otherwise store 0, and
		// surfel_moment_weight() reads 0 moments as "fully occluded" -> weight
		// 0, which CANCELS the radiance seed above (the surfel contributes
		// nothing until its own rays fill every direction's moments, and that
		// fill-in is what pops in - very visible on a large flat face where the
		// radiance seed is otherwise perfect). Seed those unsampled newborn
		// texels to a fully-OPEN distance (maxDistance) instead: any shading
		// point is within the surfel radius (the gather gates dist < radius ==
		// maxDistance), so dist <= mean and the moment weight is 1, letting the
		// seeded radiance show at once. Texels that DID get a depth sample keep
		// their real moments (accurate occlusion from birth); real samples then
		// blend in over frames.
		if (total_depth_weight > WEIGHT_EPSILON || life == 0)
		{
			if (life == 0 && total_depth_weight <= WEIGHT_EPSILON)
			{
				result_depth = float2(maxDistance, sqr(maxDistance));
			}
			else if (life > 0)
			{
				const float2 prev_moment = surfelMomentsTexture[moments_pixel];
				result_depth = lerp(prev_moment, result_depth, 0.02);
			}
			surfelMomentsTexture[moments_pixel] = result_depth;
		}

		result = varianceData.mean;
		inconsistency = varianceData.inconsistency;

		shared_texels[idx] = result;
	}

	const uint lane_count_per_wave = WaveGetLaneCount();
	if(WaveIsFirstLane())
	{
		float wave_max_inconsistency = WaveActiveMax(inconsistency);
		shared_inconsistency[groupIndex / lane_count_per_wave] = wave_max_inconsistency;
	}
	
	GroupMemoryBarrierWithGroupSync();

	// below this, we switch to bigger per-surfel tasks done by just a few select threads:

	if (groupIndex == 0)
	{
		float3x3 TBN = get_tangentspace(N);
		SH::L1_RGB radiance = SH::L1_RGB::Zero();
		for (int x = 0; x < SURFEL_MOMENT_RESOLUTION; ++x)
		{
			for (int y = 0; y < SURFEL_MOMENT_RESOLUTION; ++y)
			{
				const float3 direction = normalize(mul(decode_hemioct(((float2(x, y) + 0.5) / (float2)SURFEL_MOMENT_RESOLUTION) * 2 - 1), TBN));
				float3 value = shared_texels[flatten2D(int2(x, y), SURFEL_MOMENT_RESOLUTION)];
				radiance = SH::Add(radiance, SH::ProjectOntoL1_RGB(direction, value));
			}
		}
		radiance = SH::Multiply(radiance, rcp(SURFEL_MOMENT_RESOLUTION * SURFEL_MOMENT_RESOLUTION * HEMISPHERE_SAMPLING_PDF));
		surfelBuffer[surfel_index].radiance = radiance.Pack();
	}
	else if(groupIndex == 1)
	{
		life++;

		// Recency for the recycler: frames since this surfel last contributed
		// to a visible pixel. surfel_coverageCS sets the "seen" bit when it
		// gathers this surfel for a visible surface - a truer "still useful"
		// signal than a frustum test, since it accounts for occlusion.
		// properties is rebuilt from 0 below, clearing the bit for next frame;
		// coverage re-sets it if the surfel contributes again.
		if (surfel_data.IsSeen())
		{
			recycle = 0;
		}
		else
		{
			recycle++;
		}

		life = clamp(life, 0, 255);
		recycle = clamp(recycle, 0, 255);

		// Thinning ("discard on recede"): mark this surfel redundant when a
		// same- orientation, LOWER-INDEX neighbour already sits within its
		// CURRENT spacing. As surfels grow while the camera backs away, a
		// region packed dense up close becomes over-dense; surfel_update
		// recycles the flagged ones so far-away density relaxes back to the
		// spacing target. Lower-index- wins is a stable tiebreak (the lowest
		// index of an over-close cluster survives). Threshold is below the
		// spawn spacing (SURFEL_THIN_HYSTERESIS) so a freshly placed surfel is
		// not instantly re-thinned. Skipped for newborns (life <= 1), which
		// just passed the spawn spacing test anyway.
		bool is_redundant = false;
		if (life > 1)
		{
			const uint self_level = surfel_level_from_radius(surfel.GetRadius());
			const float thin_dist2 = sqr(surfel.GetRadius()
				* surfel_spawn_spacing(self_level) * SURFEL_THIN_HYSTERESIS);
			const int3 self_cell = surfel_cell(P, self_level);
			if (surfel_cellvalid(self_cell))
			{
				SurfelGridCell gc =
					surfelGridBuffer[surfel_cellindex(self_cell, self_level)];
				for (uint i = 0; i < gc.count && !is_redundant; ++i)
				{
					const uint nbr = surfelCellBuffer[gc.offset + i];
					if (nbr >= surfel_index)
						continue; // only lower indices - tiebreak so one survives
					Surfel nbr_surfel = surfelBuffer[nbr];
					const float3 to_nbr = P - nbr_surfel.position;
					if (dot(to_nbr, to_nbr) >= thin_dist2)
						continue;
					const float3 nbr_normal =
						normalize(unpack_half3(nbr_surfel.normal));
					if (dot(N, nbr_normal) <= 0.5)
						continue; // different orientation: not a duplicate
					is_redundant = true;
				}
			}
		}

		surfel_data.properties = 0;
		surfel_data.SetLife(life);
		surfel_data.SetRecycle(recycle);
		surfel_data.SetBackfaceNormal(backface);
		surfel_data.SetRedundant(is_redundant);
		
		const uint wave_count_per_group = THREADCOUNT * THREADCOUNT / lane_count_per_wave;
		surfel_data.max_inconsistency = inconsistency;
		for(uint i = 0; i < wave_count_per_group; ++i)
		{
			surfel_data.max_inconsistency = max(surfel_data.max_inconsistency, shared_inconsistency[i]);
		}

		surfelDataBuffer[surfel_index] = surfel_data;
	}
}
