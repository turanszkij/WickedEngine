#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"
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

static const float WEIGHT_EPSILON = 0.0001;

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
	const uint3 probeCoord = ddgi_probe_coord(probeIndex);
	const float maxDistance = ddgi_max_distance();

	// A probe is treated like frame 0 (full reset, no temporal blend) either on
	// the global first frame or when it was just (re)placed by a grid scroll
	// this frame.
	const bool probe_fresh = (push.frameIndex == 0)
		|| (ddgiProbeBuffer[probeIndex].flags & DDGIPROBE_FLAG_FRESH) != 0;

#ifdef DDGI_UPDATE_DEPTH
	[branch]
	if (groupIndex == 0 && probe_fresh)
	{
		ddgiProbeBuffer[probeIndex].offset = 0;
	}
	const float3 probe_limit = ddgi_cellsize() * 0.5;
	const float probeOffsetDistance = maxDistance * DDGI_KEEP_DISTANCE;
	// Relocation + enclosure detection are tallied by thread 0 only; it visits
	// every ray exactly once across all cache chunks, so the results are exact
	// without a groupshared atomic. surface_push nudges the probe away from any
	// nearby surface, front or back (keeps it off walls and off the underside
	// of planes); backface_count drives the enclosure (validity) test.
	float3 surface_push = 0;
	uint counted_ray_count = 0;
	uint backface_count = 0;
	uint escape_count = 0; // rays that reach open space (a miss, or a hit only far away)
#endif // DDGI_UPDATE_DEPTH

#ifdef DDGI_UPDATE_DEPTH
	float2 result = 0; // this must be full precision, otherwise popping will occur!
	const uint2 pixel_topleft = ddgi_probe_depth_pixel(probeCoord);
	const uint2 pixel_current = pixel_topleft + GTid.xy;
	const uint2 copy_coord = pixel_topleft - 1;
#else
	// Full precision: this accumulates the radiance of up to DDGI_MAX_RAYCOUNT
	// (512) rays before dividing by total_weight. In half precision that
	// running sum overflows to +Inf for bright probes, which then poisons the
	// probe and spreads. (The averaged result is cast back to half afterwards.)
	float3 result = 0;
#endif // DDGI_UPDATE_DEPTH

	const half3 texel_direction = decode_oct((((GTid.xy % RESOLUTION) + 0.5) / RESOLUTION) * 2 - 1);

	float total_weight = 0;

	uint remaining_rays = min(ddgiRayCountBuffer[probeIndex] * DDGI_RAY_BUCKET_COUNT, DDGI_MAX_RAYCOUNT);
	uint offset = 0;

	while (remaining_rays > 0)
	{
		uint num_rays = min(CACHE_SIZE, remaining_rays);

		if (groupIndex < num_rays)
		{
			ray_cache[groupIndex] = ddgiRayBuffer[probeIndex * DDGI_MAX_RAYCOUNT + groupIndex + offset].load();
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
				// Push away from any near surface, front or back. Including
				// backfaces keeps a probe that legitimately sits behind a
				// single-sided face (e.g. under a water plane) at keep-distance
				// on its own side instead of drifting into the surface.
				if (depth < probeOffsetDistance)
				{
					surface_push -= ray.direction * (probeOffsetDistance - depth);
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
		result = lerp(prev_result, result, 0.02);
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
		// independent signatures qualify:
		//   1. Nearly every ray hits a backface (>= 87.5%) - deep inside a
		//      solid, even one whose far walls read as "open" by distance.
		//   2. It is boxed in (almost no rays escape to open space) AND at
		//      least half its rays hit backfaces. This catches a probe just
		//      inside the bottom of a box resting on a plane: the box walls
		//      give ~50% backfaces while the plane's frontface fills the rest,
		//      so rule 1 misses it, but nothing escapes so rule 2 flags it. A
		//      probe merely behind a single-sided face in the open (under a
		//      water plane, an overhang) fails both: ~50% backfaces but plenty
		//      of rays escape.
		//
		// NOTE: a probe just inside a box bottom and one just under an open
		// water surface with a *nearby* floor/walls are genuinely
		// indistinguishable from ray hits alone (both are ~50% backface, boxed
		// in). Separating those needs a solid/voxel oracle; the voxel-grid push
		// below already helps when present.
		//
		// The decision is hysteretic (enter buried on the rules above, leave
		// only when clearly open) so a probe whose counts hover near a boundary
		// does not flicker as the re-randomized rays jitter the tallies frame
		// to frame.
		const bool was_valid = (flags & DDGIPROBE_FLAG_VALID) != 0;
		const bool mostly_backfaces = backface_count * 8 >= counted_ray_count * 7; // >= 87.5%
		const bool backface_heavy = backface_count * 2 >= counted_ray_count;        // >= 50%
		const bool few_escapes = escape_count * 8 < counted_ray_count;              // < 12.5%
		const bool enter_buried = mostly_backfaces || (backface_heavy && few_escapes);
		const bool clearly_open =
			(backface_count * 4 < counted_ray_count)   // < 25% backfaces
			|| (escape_count * 4 >= counted_ray_count); // >= 25% rays escape
		bool probe_enclosed;
		if (counted_ray_count == 0)
			probe_enclosed = false;
		else if (enter_buried)
			probe_enclosed = true;
		else if (clearly_open)
			probe_enclosed = false;
		else
			probe_enclosed = !was_valid; // in-between: hold previous state

		// Relocation only ever pushes the probe *away* from nearby surfaces (a
		// smooth field with stable equilibria), never toward/through a backface
		// to "extract" it. Steering buried probes toward their nearest exit
		// oscillated badly where the exit is a dead end - e.g. a box resting on
		// the ground: the probe is pushed down out of the box, immediately
		// sandwiched against the floor, flips valid, gets pushed back up into
		// the box, and repeats. Buried probes instead settle inward
		// (surface_push is ~symmetric inside a solid), stay enclosed, and are
		// simply hidden/down-weighted.
		float3 probeOffsetNew = surface_push;

		[branch]
		if(GetScene().voxelgrid.IsValid())
		{
			// If there is a voxel grid, that can help offset the probes when they are stuck in closed spaces:
			ShaderVoxelGrid voxelgrid = GetScene().voxelgrid;
			uint3 coord = voxelgrid.world_to_coord(ddgi_probe_position_rest(probeCoord));
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
		// This pass runs last (after the colour update), so consume the fresh flag here.
		flags &= ~DDGIPROBE_FLAG_FRESH;
		ddgiProbeBuffer[probeIndex].flags = flags;
	}
#else

	if (GTid.x < DDGI_COLOR_RESOLUTION && GTid.y < DDGI_COLOR_RESOLUTION)
	{
		const uint idx = flatten2D(GTid.xy, DDGI_COLOR_RESOLUTION);
		const uint variance_data_index = probeIndex * DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION + idx;
		DDGIVarianceData varianceData = varianceBuffer[variance_data_index].load();
		if (probe_fresh)
		{
			// Frame 0, or a probe just (re)placed by a scroll: discard stale
			// history.
			varianceData = (DDGIVarianceData)0;
			varianceData.mean = result;
			varianceData.shortMean = result;
			varianceData.inconsistency = 1;
		}
		MultiscaleMeanEstimator(result, varianceData, push.blendSpeed);
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
		
		//draw_line(ddgi_probe_position(probeCoord), ddgi_probe_position(probeCoord) + OptimalLinearDirection(radiance));
	}
	
#endif // DDGI_UPDATE_DEPTH
}
