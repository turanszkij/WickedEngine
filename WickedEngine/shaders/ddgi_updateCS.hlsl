#include "globals.hlsli"
#include "ddgiHF.hlsli"
#include "bc6h.hlsli"

// This shader collects all traced rays (one probe per thread group) and integrates them
//	Rays are first gathered to shared memory
//	Then for each pixel, all traced rays will be evaluated and contributed, weighted based on pixel's own direction and ray's direction
//	This shader will run twice in DDGI, once it integrates the radiances
//	After that it will also integrate the ray depths, when DDGI_UPDATE_DEPTH is defined
//	Based on: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_probe_update.glsl

PUSHCONSTANT(push, DDGIPushConstants);

StructuredBuffer<DDGIRayDataPacked> ddgiRayBuffer : register(t0);
Buffer<uint> ddgiRayCountBuffer : register(t1);
Buffer<uint> ddgiRayBaseBuffer : register(t2);

static const float WEIGHT_EPSILON = 0.0001;

// Strength of the staggered-cascade blend-rate compensation, as an exponent on
// a cascade's update period K (cascades[c].blend_scale): the temporal blend
// factor is multiplied by pow(K, strength). 1.0 = full (coarse cascades
// converge at the same WALL-CLOCK rate as cascade 0), 0.0 = none (they converge
// K times slower = lag/darkening while moving, and cascade-boundary popping),
// 0.5 = sqrt(K).
//
// Colour and depth are tuned separately because they fail differently:
//   - COLOUR (radiance): pushing this hard lets K times more per-update
//     radiance noise through on the coarse cascades => shimmer/flicker. Keep it
//     gentle.
//   - DEPTH (visibility moments): these are smooth, so faster convergence does
//     NOT shimmer, and it makes the Chebyshev visibility test recover quickly
//     when a surface moves - which is the main cause of the transient darkening
//     on moving objects. So this can be pushed to full. Tune either and
//     recompile just the shaders (fast) to trade speed vs shimmer.
static const float DDGI_BLEND_COMPENSATION_COLOR = 0.5;
static const float DDGI_BLEND_COMPENSATION_DEPTH = 1.0;

#ifdef DDGI_UPDATE_DEPTH
static const uint THREADCOUNT = DDGI_DEPTH_RESOLUTION;
static const uint RESOLUTION = DDGI_DEPTH_RESOLUTION;
RWTexture2D<float2> output : register(u0);
groupshared uint shared_depths[DDGI_DEPTH_RESOLUTION * DDGI_DEPTH_RESOLUTION];
#else
static const uint THREADCOUNT = DDGI_COLOR_RESOLUTION;
static const uint RESOLUTION = DDGI_COLOR_RESOLUTION;
RWStructuredBuffer<DDGIVarianceDataPacked> varianceBuffer : register(u0);
groupshared half shared_r[DDGI_COLOR_RESOLUTION][DDGI_COLOR_RESOLUTION];
groupshared half shared_g[DDGI_COLOR_RESOLUTION][DDGI_COLOR_RESOLUTION];
groupshared half shared_b[DDGI_COLOR_RESOLUTION][DDGI_COLOR_RESOLUTION];
#endif // DDGI_UPDATE_DEPTH

RWStructuredBuffer<DDGIProbe> ddgiProbeBuffer : register(u1);

static const uint CACHE_SIZE = THREADCOUNT * THREADCOUNT;
groupshared DDGIRayData ray_cache[CACHE_SIZE];

static const int3 voxel_neighbors[] = {
	// First priority is direct straight neighbors:
	int3(-1, 0, 0),
	int3(1, 0, 0),
	int3(0, -1, 0),
	int3(0, 1, 0),
	int3(0, 0, -1),
	int3(0, 0, 1),

	// Second priority is diagonal neighbors:
	int3(-1, 0, -1),
	int3(1, 0, -1),
	int3(-1, 0, 1),
	int3(1, 0, 1),
	int3(-1, -1, -1),
	int3(1, -1, -1),
	int3(-1, -1, 1),
	int3(1, -1, 1),
	int3(-1, 1, -1),
	int3(1, 1, -1),
	int3(-1, 1, 1),
	int3(1, 1, 1),
};
int3 get_nearby_empty_voxel(ShaderVoxelGrid voxelgrid, uint3 coord)
{
	for(uint i = 0; i < arraysize(voxel_neighbors); ++i)
	{
		if(!voxelgrid.check_voxel(coord + voxel_neighbors[i]))
			return voxel_neighbors[i];
	}
	return 0;
}

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint2 GTid : SV_GroupThreadID, uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint probeIndex = Gid.x;
	uint cascade;
	uint3 probeCoord;
	ddgi_decode_probe(probeIndex, cascade, probeCoord);

	// Staggered updates: skip cascades not refreshed this frame so their probes
	// keep their last integrated result. The active flag is uniform across the
	// group (one probe per group), so returning here is uniform control flow
	// and precedes any groupshared use / barrier.
	if (GetScene().ddgi.cascades[cascade].active == 0)
		return;

	const float maxDistance = ddgi_max_distance(cascade);

	// Blend-rate compensation: this cascade refreshes once every update_period
	// frames (= cascades[cascade].blend_scale). The colour and depth blend
	// factors are scaled by pow(update_period, strength) with separate
	// strengths (see the constants above), each clamped to <= 1 at its use
	// site.
	const float update_period = GetScene().ddgi.cascades[cascade].blend_scale;

	// A probe is treated like frame 0 (full reset, no temporal blend) on the
	// global first frame, when it was just (re)placed by a grid scroll this
	// frame (FRESH), or when it has never been refreshed at all (INITIALIZED
	// not yet set - the zeroed creation state; coarse cascades skip frame 0 and
	// converge through this path on their first round-robin activation).
	const bool probe_fresh = (push.frameIndex == 0)
		|| (ddgiProbeBuffer[probeIndex].flags & DDGIPROBE_FLAG_FRESH) != 0
		|| (ddgiProbeBuffer[probeIndex].flags & DDGIPROBE_FLAG_INITIALIZED) == 0;

#ifdef DDGI_UPDATE_DEPTH
	[branch]
	if (groupIndex == 0 && probe_fresh)
	{
		ddgiProbeBuffer[probeIndex].offset = 0;
	}
	const float3 probe_limit = ddgi_cellsize(cascade) * 0.5;
	const float probeOffsetDistance = maxDistance * DDGI_KEEP_DISTANCE;
	// Relocation + enclosure detection are tallied by thread 0 only; it visits
	// every ray exactly once across all cache chunks, so the results are exact
	// without a groupshared atomic. surface_push nudges the probe away from any
	// nearby surface, front or back (keeps it off walls and off the underside
	// of planes); backface_count drives the enclosure (validity) test.
	float3 surface_push = 0;
	float push_magnitude_sum = 0; // sum of |per-ray push|, for squeeze damping
	uint counted_ray_count = 0;
	uint backface_count = 0;
	uint escape_count = 0; // rays that reach open space (a miss, or a hit only far away)
#endif // DDGI_UPDATE_DEPTH

#ifdef DDGI_UPDATE_DEPTH
	float2 result = 0; // this must be full precision, otherwise popping will occur!
	const uint2 pixel_topleft = ddgi_probe_depth_pixel(cascade, probeCoord);
	const uint2 pixel_current = pixel_topleft + GTid.xy;
	const uint2 copy_coord = pixel_topleft - 1;
#else
	// Full precision: this accumulates the radiance of up to DDGI_MAX_RAYCOUNT
	// (512) rays before dividing by total_weight. In half precision that
	// running sum overflows to +Inf for bright probes, which then poisons the
	// probe and spreads. (The averaged result is cast back to half afterwards.)
	float3 result = 0;

	// Buried (invalid) probes do not integrate their own backface-only rays -
	// that would drive them to black, and a probe that spawned inside geometry
	// was never lit so it has no colour history to keep either. Seed the
	// probe's radiance from its nearest valid neighbour, so the region occupied
	// by a solid shows plausible surrounding GI rather than black, both while
	// buried and the moment relocation ejects it into the open (invalid probes
	// are down-weighted when sampled, but when a whole probe cage is invalid
	// their radiance is what the surface gets - so it must not be black). Once
	// ejected, the depth pass flags DDGIPROBE_FLAG_COLOR_RESET and the probe
	// converges from its own rays via the fresh path below. Fresh and
	// currently-valid probes integrate normally. The VALID flag read here is
	// last frame's verdict (the depth pass decides this frame's after the
	// colour pass). Uniform across the group (one probe per group), so this
	// early-out is uniform control flow and precedes any groupshared barrier.
	if (!probe_fresh && (ddgiProbeBuffer[probeIndex].flags & DDGIPROBE_FLAG_VALID) == 0)
	{
		if (groupIndex == 0)
		{
			const int3 world = int3(ddgi_buffer_to_world_coord(cascade, probeCoord));
			const int3 dims = int3(GetScene().ddgi.cascades[cascade].grid_dimensions);
			// Search straight then diagonal neighbours (voxel_neighbors is
			// ordered that way) for the first valid probe and copy its
			// radiance. Deep inside a large solid no neighbour is valid; leave
			// the radiance as-is (it is not meaningfully sampled there anyway).
			for (uint n = 0; n < arraysize(voxel_neighbors); ++n)
			{
				const int3 nw = world + voxel_neighbors[n];
				if (any(nw < 0) || any(nw >= dims))
					continue;
				const uint3 nb = ddgi_world_to_buffer_coord(cascade, uint3(nw));
				const DDGIProbe neighbour = ddgiProbeBuffer[ddgi_probe_index(cascade, nb)];
				if ((neighbour.flags & DDGIPROBE_FLAG_VALID) != 0)
				{
					ddgiProbeBuffer[probeIndex].radiance = neighbour.radiance;
					break;
				}
			}
		}
		return;
	}

	// A just-ejected probe (invalid last frame, now open) has no colour
	// history, so reset it like a fresh probe: full ray budget (the allocation
	// pass reads this same flag) and take this frame's result directly instead
	// of blending up from black.
	const bool color_reset = (ddgiProbeBuffer[probeIndex].flags & DDGIPROBE_FLAG_COLOR_RESET) != 0;
#endif // DDGI_UPDATE_DEPTH

	const half3 texel_direction = decode_oct((((GTid.xy % RESOLUTION) + 0.5) / RESOLUTION) * 2 - 1);

	float total_weight = 0;

	uint remaining_rays = min(ddgiRayCountBuffer[probeIndex] * DDGI_RAY_BUCKET_COUNT, DDGI_MAX_RAYCOUNT);
	uint offset = 0;

	// This probe's rays live at a compacted per-frame offset in the shared ray
	// buffer (written by the ray allocation pass), not at probeIndex * MAX: the
	// transient ray buffers are only sized for the probes refreshed per frame.
	const uint ray_base = ddgiRayBaseBuffer[probeIndex];

	while (remaining_rays > 0)
	{
		uint num_rays = min(CACHE_SIZE, remaining_rays);

		if (groupIndex < num_rays)
		{
			ray_cache[groupIndex] = ddgiRayBuffer[ray_base + groupIndex + offset].load();
		}

		GroupMemoryBarrierWithGroupSync();

		for (uint r = 0; r < num_rays; ++r)
		{
			DDGIRayData ray = ray_cache[r];

#ifdef DDGI_UPDATE_DEPTH
			// The sign of ray.depth encodes front (+) vs back (-) face; its
			// magnitude is the hit distance (a miss stores +maxDistance). The
			// depth/visibility moment uses the magnitude, unchanged from
			// before.
			const bool is_backface = ray.depth < 0;
			const half hit_dist = min((half)maxDistance, (half)abs(ray.depth));
			const half depth = clamp(hit_dist - 0.01, 0, maxDistance);

			if (groupIndex == 0)
			{
				counted_ray_count++;
				if (is_backface)
					backface_count++;
				// A ray "escapes" when it reaches open space in that direction:
				// a true miss, or a *front* face only far away (a distant wall
				// the probe faces). A far *back* face does NOT escape - the
				// probe is still behind that surface, i.e. enclosed on that
				// side. This is what separates a large solid box (far walls are
				// backfaces -> few escapes -> buried) from a large open room
				// (far walls are frontfaces -> many escapes -> valid), even
				// though both give ~50% backfaces when a plane clips through.
				// Few escapes => boxed in.
				if (!is_backface && hit_dist >= (half)maxDistance)
					escape_count++;
				// Keep the probe at DDGI_KEEP_DISTANCE from nearby surfaces,
				// but the push direction depends on which SIDE of the surface
				// we are on:
				//   - FRONT face: we are on its outward (correct) side and
				//     merely too close, so push AWAY (-ray.direction) to hold
				//     distance.
				//   - BACK face: we are behind the surface, i.e. INSIDE the
				//     solid on the wrong side, so push TOWARD it
				//     (+ray.direction) to exit through the nearest face instead
				//     of being shoved deeper in.
				//
				// This is what rescues the probe a moving box leaves just
				// inside its TRAILING face. The old "push away from any
				// surface" rule pushed that probe away from the near (back)
				// face = further into the box, so it stayed enclosed - invalid,
				// contributing no light - and the trailing face went black.
				// (The leading face never showed this because an EXTERNAL probe
				// ahead of it is pinned by the same away-push.) Now the near
				// backface pushes the probe out through itself; once it pokes
				// through it reads as a front face and the away-push stabilizes
				// it at keep-distance outside.
				//
				// Deep inside a solid the opposite backfaces are far apart and
				// their pushes cancel, so a genuinely buried probe is not
				// yanked toward an arbitrary face; only one within reach of a
				// single near face is extracted (the offset is clamped to half
				// a cell anyway).
				if (depth < probeOffsetDistance)
				{
					const float push = probeOffsetDistance - depth;
					surface_push += ray.direction * (is_backface ? push : -push);
					push_magnitude_sum += push;
				}
			}
#else
			const half3 radiance = ray.radiance.rgb;
#endif // DDGI_UPDATE_DEPTH

			half weight = saturate(dot(texel_direction, ray.direction));
#ifdef DDGI_UPDATE_DEPTH
			weight = pow(weight, 64);
#endif // DDGI_UPDATE_DEPTH

			if (weight > WEIGHT_EPSILON)
			{
#ifdef DDGI_UPDATE_DEPTH
				result += half2(depth, sqr(depth)) * weight;
#else
				result += ray.radiance.rgb * weight;
#endif // DDGI_UPDATE_DEPTH

				total_weight += weight;
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

#ifdef DDGI_UPDATE_DEPTH
	const half2 prev_result = output[pixel_current].xy;
	// Fresh probes take the new depth directly; the previous atlas value
	// belongs to the probe's old world position (before the scroll) and would
	// leak visibility if blended.
	if (!probe_fresh)
	{
		result = lerp(prev_result, result, min(0.02 * pow(update_period, DDGI_BLEND_COMPENSATION_DEPTH), 1.0));
	}
	shared_depths[flatten2D(GTid, DDGI_DEPTH_RESOLUTION)] = pack_half2(result);
	output[pixel_current] = result;

	GroupMemoryBarrierWithGroupSync();

	// Copy depth borders:
	for (uint index = groupIndex; index < arraysize(DDGI_DEPTH_BORDER_OFFSETS); index += THREADCOUNT * THREADCOUNT)
	{
		uint src_coord = flatten2D(DDGI_DEPTH_BORDER_OFFSETS[index].xy - 1, DDGI_DEPTH_RESOLUTION);
		uint2 dst_coord = copy_coord + DDGI_DEPTH_BORDER_OFFSETS[index].zw;
		output[dst_coord] = unpack_half2(shared_depths[src_coord]);
	}

	[branch]
	if (groupIndex == 0)
	{
		uint flags = ddgiProbeBuffer[probeIndex].flags;

		// A probe is buried when it is enclosed by solid geometry. Two
		// independent per-frame signatures qualify:
		//   1. Nearly every ray hits a backface (>= 87.5%) - deep inside a
		//      solid.
		//   2. It is boxed in (few rays escape to open space) AND at least half
		//      its rays hit backfaces. Catches a probe inside a box on /
		//      clipped by a plane: the box walls give the backfaces, the
		//      plane's near frontface fills the rest, and (unlike an open water
		//      plane) little escapes. An escape is a miss or a far *front*
		//      face; a far backface does not escape.
		//
		// This per-frame decision is noisy: a boundary probe only traces ~32
		// rays, re-randomized every frame, so the raw verdict blinks. So we
		// accumulate it into a temporally smoothed confidence (EMA) and derive
		// VALID from that. A fresh probe (full ray budget) takes the verdict
		// immediately; others ease toward it, which removes the blink. A
		// dead-band around the midpoint keeps a genuinely borderline probe from
		// crossing back and forth.
		//
		// NOTE: a probe just inside a box and one under an open water surface
		// with a *nearby* floor/walls are genuinely indistinguishable from ray
		// hits alone (both ~50% backface, boxed in). Separating those needs a
		// solid/voxel oracle; the voxel-grid push below already helps when
		// present.
		const bool mostly_backfaces = backface_count * 8 >= counted_ray_count * 7; // >= 87.5%
		const bool backface_heavy = backface_count * 2 >= counted_ray_count;        // >= 50%
		const bool few_escapes = escape_count * 4 < counted_ray_count;              // < 25%
		const bool enter_buried = mostly_backfaces || (backface_heavy && few_escapes);
		const bool clearly_open =
			(backface_count * 4 < counted_ray_count)       // < 25% backfaces
			|| (escape_count * 5 >= counted_ray_count * 2); // >= 40% rays escape

		// Smooth the verdict. An ambiguous frame (neither) holds the current
		// value.
		float confidence = probe_fresh ? 0.0 : ddgiProbeBuffer[probeIndex].enclosure_confidence;
		[branch]
		if (counted_ray_count > 0)
		{
			const float target = enter_buried ? 1.0 : (clearly_open ? 0.0 : confidence);
			confidence = probe_fresh ? target : lerp(confidence, target, 0.1);
		}
		ddgiProbeBuffer[probeIndex].enclosure_confidence = confidence;

		const bool was_valid = (flags & DDGIPROBE_FLAG_VALID) != 0;
		bool probe_enclosed;
		if (confidence > 0.6)
			probe_enclosed = true;
		else if (confidence < 0.4)
			probe_enclosed = false;
		else
			probe_enclosed = !was_valid; // dead-band: hold previous state

		// Relocation only ever pushes the probe *away* from nearby surfaces (a
		// smooth field with stable equilibria), never toward/through a backface
		// to "extract" it. Steering buried probes toward their nearest exit
		// oscillated badly where the exit is a dead end - e.g. a box resting on
		// the ground: the probe is pushed down out of the box, immediately
		// sandwiched against the floor, flips valid, gets pushed back up into
		// the box, and repeats. Buried probes instead settle inward
		// (surface_push is ~symmetric inside a solid), stay enclosed, and are
		// simply hidden/down-weighted.
		// Squeeze damping: when the probe is pinched between opposing surfaces
		// - e.g. two nearby FRONT faces of a thin gap - their pushes point in
		// opposite directions and largely cancel, but the per-ray magnitudes
		// stay large. Tiny frame-to-frame changes in which face the
		// (re-randomized) rays sample then make the small net push swing sign,
		// and the probe flickers back and forth between the two faces. The
		// coherence (net push length / summed push magnitude) is ~1 when a
		// single surface dominates and ~0 when they oppose; scaling by it
		// leaves a lone push nearly intact but collapses an opposed one, so a
		// pinched probe settles at the balance point instead of chasing an
		// unreachable keep-distance from both.
		if (push_magnitude_sum > 1e-5)
		{
			const float coherence = length(surface_push) / push_magnitude_sum;
			surface_push *= coherence;
		}

		float3 probeOffsetNew = surface_push;

		[branch]
		if(GetScene().voxelgrid.IsValid())
		{
			// If there is a voxel grid, that can help offset the probes when they are stuck in closed spaces:
			ShaderVoxelGrid voxelgrid = GetScene().voxelgrid;
			uint3 coord = voxelgrid.world_to_coord(ddgi_probe_position_rest(cascade, probeCoord));
			if(voxelgrid.check_voxel(coord))
			{
				probeOffsetNew += get_nearby_empty_voxel(voxelgrid, coord) * voxelgrid.voxelSize * 2;
			}
		}

		half3 probeOffset = unpack_half3(ddgiProbeBuffer[probeIndex].offset);
		probeOffset *= probe_limit;
		probeOffset = lerp(probeOffset, probeOffsetNew, 0.05); // Increased from 0.01 for faster escape
		probeOffset = clamp(probeOffset, -probe_limit, probe_limit);
		probeOffset /= probe_limit;
		ddgiProbeBuffer[probeIndex].offset = pack_half3(probeOffset);

		// Validity: only a probe enclosed in solid geometry this frame is
		// marked invalid so ddgi_sample_irradiance down-weights it. It becomes
		// valid again automatically once relocation moves it into open space
		// and the backface ratio drops below the low threshold.
		if (probe_enclosed)
			flags &= ~DDGIPROBE_FLAG_VALID;
		else
			flags |= DDGIPROBE_FLAG_VALID;
		// Ejection: the probe was buried last frame and is now open (relocation
		// pushed it out of the solid). Flag it so its next update takes the
		// full ray budget and the fresh colour-reset path, converging in one
		// step from its new open position instead of trickling up from black -
		// a probe that spawned inside geometry has no colour history to fall
		// back on. Consumed by the colour pass next frame; does NOT reset the
		// offset (that would send it back inside).
		if (!was_valid && !probe_enclosed)
			flags |= DDGIPROBE_FLAG_COLOR_RESET;
		// This pass runs last (after the colour update), so consume the fresh
		// flag here. INITIALIZED marks that this probe has been through a full
		// update at least once; a probe without it (zeroed creation state)
		// takes the fresh path on its first refresh.
		flags &= ~DDGIPROBE_FLAG_FRESH;
		flags |= DDGIPROBE_FLAG_INITIALIZED;
		ddgiProbeBuffer[probeIndex].flags = flags;
	}
#else

	if (GTid.x < DDGI_COLOR_RESOLUTION && GTid.y < DDGI_COLOR_RESOLUTION)
	{
		const uint idx = flatten2D(GTid.xy, DDGI_COLOR_RESOLUTION);
		const uint variance_data_index = probeIndex * DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION + idx;
		DDGIVarianceData varianceData = varianceBuffer[variance_data_index].load();
		if (probe_fresh || color_reset)
		{
			// Frame 0, a probe just (re)placed by a scroll, or a probe just
			// ejected from solid geometry: discard stale/absent history and
			// take this frame's full-budget result directly.
			varianceData = (DDGIVarianceData)0;
			varianceData.mean = result;
			varianceData.shortMean = result;
			varianceData.inconsistency = 1;
		}
		MultiscaleMeanEstimator(result, varianceData, min(push.blendSpeed * pow(update_period, DDGI_BLEND_COMPENSATION_COLOR), 1.0));
		varianceBuffer[variance_data_index].store(varianceData);
		result = varianceData.mean;

		shared_r[GTid.x][GTid.y] = result.r;
		shared_g[GTid.x][GTid.y] = result.g;
		shared_b[GTid.x][GTid.y] = result.b;
	}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		SH::L1_RGB radiance = SH::L1_RGB::Zero();
		for (int x = 0; x < RESOLUTION; ++x)
		{
			for (int y = 0; y < RESOLUTION; ++y)
			{
				const half3 direction = decode_oct(((half2(x, y) + 0.5) / RESOLUTION) * 2 - 1);
				half3 value = half3(shared_r[x][y], shared_g[x][y], shared_b[x][y]);
				radiance = SH::Add(radiance, SH::ProjectOntoL1_RGB(direction, value));
			}
		}
		radiance = SH::Multiply(radiance, rcp(RESOLUTION * RESOLUTION * SPHERE_SAMPLING_PDF));
		ddgiProbeBuffer[probeIndex].radiance = radiance.Pack();

		// Consume the one-shot colour-reset flag now that the probe has
		// converged from its own rays. Safe to clear here: the depth pass
		// writes flags later this frame from a fresh read, so it sees the
		// cleared bit.
		if (color_reset)
			ddgiProbeBuffer[probeIndex].flags &= ~DDGIPROBE_FLAG_COLOR_RESET;

		//draw_line(ddgi_probe_position(probeCoord), ddgi_probe_position(probeCoord) + OptimalLinearDirection(radiance));
	}
	
#endif // DDGI_UPDATE_DEPTH
}
