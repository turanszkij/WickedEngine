#define TEXTURE_SLOT_NONUNIFORM
#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "raytracingHF.hlsli"
#include "brdf.hlsli"

PUSHCONSTANT(push, SurfelDebugPushConstants);

StructuredBuffer<Surfel> surfelBuffer : register(t0);
StructuredBuffer<SurfelGridCell> surfelGridBuffer : register(t1);
StructuredBuffer<uint> surfelCellBuffer : register(t2);
Texture2D<float2> surfelMomentsTexture : register(t3);
Texture2D<float3> surfelHistoryTexture : register(t4); // previous frame's half-res GI (temporal history)

RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u0);
RWStructuredBuffer<uint> surfelDeadBuffer : register(u1);
RWStructuredBuffer<uint> surfelAliveBuffer : register(u2);
RWStructuredBuffer<SurfelStats> surfelStatsBuffer : register(u3);
RWTexture2D<float3> result : register(u4);
RWTexture2D<unorm float4> debugUAV : register(u5);

// Reproject last frame's half-res GI to this pixel via the motion vector.
// Returns the history sample and whether it landed on-screen with a finite
// value (valid). Recomputes the full-res pixel/uv from DTid (half-res
// dispatch).
float3 reproject_history(uint2 DTid, out bool valid)
{
	const uint2 pixel = DTid * 2;
	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 prev_uv = uv + texture_velocity[pixel];
	valid = is_saturated(prev_uv);
	if (!valid)
		return 0;
	// Disocclusion reject (camera-motion aware). Reproject THIS pixel's world
	// position into the PREVIOUS camera and compare its expected depth there to
	// the depth actually stored at prev_uv last frame. If they differ, this
	// pixel was occluded last frame (a behind-object reveal), so the
	// reprojected history is a DIFFERENT surface - reject it (forces a group
	// refresh for skipped groups). Doing the compare in the previous camera
	// (rather than a raw this-vs-last linear-depth diff) removes the
	// camera-motion component, so a dolly/pan no longer false-triggers and the
	// threshold can be strict enough to kill reveal ghosting without forcing
	// needless refreshes.
	const float3 P = reconstruct_position(uv, texture_depth[pixel]);
	const float4 prev_clip = mul(GetCamera().previous_view_projection, float4(P, 1.0));
	if (prev_clip.w <= 0.0)
	{
		valid = false; // behind the previous camera
		return 0;
	}
	const float expected_prev_z = compute_lineardepth(prev_clip.z / prev_clip.w);
	const float stored_prev_z = compute_lineardepth(
		texture_depth_history.SampleLevel(sampler_point_clamp, prev_uv, 0));
	if (abs(expected_prev_z - stored_prev_z) >
		expected_prev_z * SURFEL_COVERAGE_DISOCCLUSION)
	{
		valid = false;
		return 0;
	}
	const float3 h = surfelHistoryTexture.SampleLevel(
		sampler_linear_clamp, prev_uv, 0);
	// Guard the uninitialised history texture on the first frames (and any
	// stray non-finite value).
	if (any(isnan(h)) || any(isinf(h)))
	{
		valid = false;
		return 0;
	}
	return h;
}
void write_result(uint2 DTid, float4 color)
{
	const float3 fresh = color.rgb;

	// Temporal reprojection + accumulation. Blend the reprojected history in
	// where it still agrees with this frame's freshly gathered value; fall back
	// to fresh when the reprojection lands off-screen or the history disagrees
	// (disocclusion, lighting change, reprojection error), so a stable view
	// denoises without ghosting under camera motion.
	bool valid;
	const float3 history = reproject_history(DTid, valid);
	float alpha = 1.0; // 1 == take the fresh value (no history reuse)
	if (valid)
	{
		const float3 luma = float3(0.299, 0.587, 0.114);
		const float fresh_luma = dot(fresh, luma);
		const float rel = abs(dot(history, luma) - fresh_luma) /
			(fresh_luma + 0.01);
		alpha = lerp(SURFEL_COVERAGE_TEMPORAL_BLEND, 1.0,
			saturate(rel * SURFEL_COVERAGE_TEMPORAL_REJECT));
	}
	result[DTid] = lerp(valid ? history : fresh, fresh, alpha);
}
void write_debug(uint2 DTid, float4 debug)
{
	debugUAV[DTid * 2 + uint2(0, 0)] = debug;
	debugUAV[DTid * 2 + uint2(1, 0)] = debug;
	debugUAV[DTid * 2 + uint2(0, 1)] = debug;
	debugUAV[DTid * 2 + uint2(1, 1)] = debug;
}

// The 16x16 thread group is split into sub-tiles; each sub-tile independently
// elects one spawn candidate per frame, so a group can place several surfels
// where coverage is low instead of just one. This makes surfaces populate much
// faster while still spreading spawns out spatially.
static const uint COVERAGE_GROUP_SIZE = 16;
static const uint COVERAGE_SUBTILE_SIZE = 8; // group_size / subtile_size per axis = spawns per group axis
static const uint COVERAGE_SUBTILES_1D = COVERAGE_GROUP_SIZE / COVERAGE_SUBTILE_SIZE;
static const uint COVERAGE_SUBTILE_COUNT = COVERAGE_SUBTILES_1D * COVERAGE_SUBTILES_1D;
groupshared uint GroupMinSurfelCount[COVERAGE_SUBTILE_COUNT];

// Cooperative LDS cache of the group's shared base-level grid cell.
//
// The cascaded grid cell size tracks screen size (see surfel_level), so a flat
// tile maps to ~one base-level cell at ANY distance. When every active pixel in
// the 16x16 group agrees on its base-level cell, the group loads that cell's
// surfels into groupshared ONCE and every pixel gathers the base level from LDS
// instead of re-fetching each surfel from global memory 256 times. Cells larger
// than the cache spill their tail to the global path; non-uniform tiles (edges
// / steep angles) and the +/-1 neighbour levels use the global path. This is a
// pure load-sharing optimisation - each pixel still tests exactly its own
// cell's surfel list once through gather_surfel(), so GI results are identical.
static const uint COVERAGE_LDS_SURFELS = 128; // per-group base-cell cache capacity
groupshared Surfel GroupSurfels[COVERAGE_LDS_SURFELS];
groupshared uint GroupSurfelIndices[COVERAGE_LDS_SURFELS];
groupshared uint GroupBaseCellMin;    // min base-cell index over active pixels
groupshared uint GroupBaseCellMax;    // max (== min => the group shares one cell)
groupshared uint GroupBaseCellOffset; // cellBuffer offset of the shared base cell
groupshared uint GroupLdsCount;       // surfels actually cached = min(count, CAP)
groupshared uint GroupLoadNext;       // atomic cursor for the cooperative fill
// Per-subtile refresh vote. The gather-skip decision is made per 8x8 SUBTILE
// (COVERAGE_SUBTILE_COUNT of them per group), not per whole 16x16 group, so a
// stable region can skip while only the subtiles that need it (disocclusion,
// near geometry, this frame's rotating set) pay for a full gather. Any pixel in
// a subtile that reprojects off-screen / near forces that subtile (only) to
// refresh.
groupshared uint GroupSubtileRefresh[COVERAGE_SUBTILE_COUNT];

// True if this SUBTILE does a full gather this frame. A rotating 1/PERIOD of
// the subtiles refresh each frame, scheduled by a per-subtile hash so the
// refresh set sweeps the whole screen over SURFEL_COVERAGE_REFRESH_PERIOD
// frames. Keyed on the subtile's global id (group id * subtiles-per-axis +
// local subtile), so neighbouring subtiles refresh on different frames rather
// than in lockstep.
bool surfel_coverage_subtile_refresh(uint2 subtile_gid, uint frame)
{
	const uint h = (subtile_gid.x * 73856093u) ^ (subtile_gid.y * 19349663u);
	return ((h + frame) % SURFEL_COVERAGE_REFRESH_PERIOD) == 0u;
}

// Accumulates one surfel's GI/coverage contribution at a shading point. Shared
// by the LDS and global gather paths so both apply identical weighting; only
// the moment sample and the seen-bit write still touch global memory (both are
// inherently per-pixel). Matches the original inlined gather exactly.
void gather_surfel(
	float3 P,
	float3 N,
	Surfel surfel,
	uint surfel_index,
	SURFEL_DEBUG debug_mode,
	inout float coverage,
	inout float nearest_dist2,
	inout float4 color,
	inout float4 debug)
{
	float3 L = P - surfel.position;
	float dist2 = dot(L, L);
	if (dist2 >= sqr(surfel.GetRadius()))
		return;

	// Point debug marks any surfel centre near the shading point, independent of
	// orientation, so it runs before the dotN gate.
	if (debug_mode == SURFEL_DEBUG_POINT && dist2 <= sqr(0.05))
		debug = float4(1, 0, 1, 1);

	float3 normal = normalize(unpack_half3(surfel.normal));
	float dotN = dot(N, normal);
	if (dotN <= 0)
		return;

	float dist = sqrt(dist2);

	// This surfel contributes GI to a visible pixel this frame, so mark it
	// "seen": the recycler (surfel_updateCS) treats recently seen surfels as
	// relevant and ages out the rest. A plain OR is safe under the race here -
	// every writer sets the same bit and no other field of properties is
	// written during coverage.
	surfelDataBuffer[surfel_index].properties |= SURFEL_PROPERTY_SEEN_BIT;

	// Coverage (the spawn metric) uses a loose radial * dotN weight,
	// deliberately NOT the sharpened anti-leak weight (sharpening collapses it
	// on curved / foliage / normal-mapped surfaces, which over-spawns and
	// churns the pool).
	float radial = saturate(1 - dist2 / sqr(surfel.GetRadius()));
	radial *= radial;
	coverage += saturate(dotN) * radial;

	// Track the nearest same-orientation neighbour (dotN > 0.5 so a
	// perpendicular surface in the same cell doesn't count as "too close") for
	// the spacing reject.
	if (dotN > 0.5)
		nearest_dist2 = min(nearest_dist2, dist2);

	// GI weight: full anti-leak (sharp normal + tangent-plane). Only affects
	// the (normalized) GI value, never the spawn decision above.
	float contribution = surfel_geometry_weight(L, normal, surfel.GetRadius(), dist2, dotN);
	float2 moments = surfelMomentsTexture.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, L / dist), 0);
	contribution *= surfel_moment_weight(moments, dist);

	// Temporal fade-in: ramp a newborn's contribution WEIGHT from 0 to full
	// over SURFEL_SPAWN_FADE_FRAMES frames (by its life counter) instead of
	// snapping to full weight at life 1. A just-spawned surfel can start
	// off-colour (dark from ray-starved first rays, or bright from a stray ray)
	// before the estimator converges; snapping it to full weight makes that
	// error pop. Ramping lets it blend in and converge to its neighbours first,
	// so spawns aren't visible.
	contribution *= saturate(
		(float)surfelDataBuffer[surfel_index].GetLife() / SURFEL_SPAWN_FADE_FRAMES);

	// Clamp irradiance to non-negative. An L1 SH radiance evaluated toward a
	// direction away from its dominant lobe can ring NEGATIVE (per channel),
	// and negative irradiance is unphysical: added here it SUBTRACTS light,
	// painting a dark (often dark-red, where only some channels go negative)
	// semi-transparent blob the size of the surfel's footprint - visible for a
	// frame as N and the surfel SH shift while the camera moves. max(0) removes
	// the overshoot.
	color += float4(max(0, SH::CalculateIrradiance(surfel.radiance.Unpack(), N)), 1) * contribution;

	switch (debug_mode)
	{
	case SURFEL_DEBUG_NORMAL:
		debug.rgb += normal * contribution;
		debug.a = 1;
		break;
	case SURFEL_DEBUG_RANDOM:
		debug += float4(random_color(surfel_index), 1) * contribution;
		break;
	case SURFEL_DEBUG_INCONSISTENCY:
		debug += float4(surfelDataBuffer[surfel_index].max_inconsistency.xxx, 1) * contribution;
		break;
	default:
		break;
	}
}

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	if (groupIndex < COVERAGE_SUBTILE_COUNT)
	{
		GroupMinSurfelCount[groupIndex] = ~0;
		GroupSubtileRefresh[groupIndex] = 0;
	}
	if (groupIndex == 0)
	{
		GroupBaseCellMin = ~0u; // sentinel: no active pixel has voted yet
		GroupBaseCellMax = 0;
		GroupLdsCount = 0;
		GroupLoadNext = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	const uint subtile =
		(GTid.y / COVERAGE_SUBTILE_SIZE) * COVERAGE_SUBTILES_1D +
		(GTid.x / COVERAGE_SUBTILE_SIZE);
	
	uint2 pixel = DTid.xy * 2;

	const float depth = texture_depth[pixel];
	if (depth == 0)
	{
		write_debug(DTid.xy, 0);
		return;
	}

	// Temporal gather skip (the coverage speedup). Reproject last frame's GI; a
	// SUBTILE whose reprojection is fully on-screen AND isn't in this frame's
	// rotating refresh set reuses that history and skips the expensive gather +
	// spawn entirely. The decision is per 8x8 subtile and subtile-UNIFORM
	// (every thread in a subtile reads the same schedule + the same groupshared
	// vote slot), so a skipped subtile's threads all terminate here TOGETHER -
	// the same early-return the sky/depth==0 case above already does, so the
	// cooperative base-cell vote and its barriers below (reached only by the
	// surviving, refreshing threads) don't gain any new divergence. Skipped
	// subtiles return BEFORE the base-cell vote, so only refreshing pixels vote
	// and gather. Any pixel reprojecting off-screen forces its OWN subtile
	// (only) to refresh (screen-edge disocclusion); debug views always refresh
	// so they stay correct. Skipped subtiles don't mark their surfels seen, but
	// the refresh period is under the recycler's unseen window, so a surfel is
	// re-seen on its subtile's cadence frames before it could be evicted.
	bool reproj_valid;
	const float3 reproj = reproject_history(DTid.xy, reproj_valid);
	// Adaptive skip: near geometry (high screen parallax) always refreshes -
	// the reuse smears visibly there and it's the cheap case anyway; only far
	// geometry (low parallax, the expensive case) is allowed to skip.
	const bool near_pixel =
		compute_lineardepth(depth) < SURFEL_COVERAGE_SKIP_MIN_DEPTH;
	if (!reproj_valid || near_pixel)
		InterlockedOr(GroupSubtileRefresh[subtile], 1u);
	GroupMemoryBarrierWithGroupSync();
	// Subtile's global id (group id * subtiles-per-axis + local subtile coord),
	// so the rotating schedule staggers neighbouring subtiles across frames.
	const uint2 subtile_gid = Gid.xy * COVERAGE_SUBTILES_1D +
		uint2(GTid.x / COVERAGE_SUBTILE_SIZE, GTid.y / COVERAGE_SUBTILE_SIZE);
	const bool refresh =
		push.debug != SURFEL_DEBUG_NONE ||
		surfel_coverage_subtile_refresh(subtile_gid, GetFrame().frame_count) ||
		GroupSubtileRefresh[subtile] != 0u;
	if (!refresh)
	{
		result[DTid.xy] = reproj; // reuse reprojected history, no gather this frame
		write_debug(DTid.xy, 0);
		return;
	}

	float4 debug = 0;
	float4 color = 0;

	float seed = GetFrame().time;
	RNG rng;
	rng.init(pixel, GetFrame().frame_count);

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);

	uint primitiveID = texture_primitiveID[pixel];

	PrimitiveID prim;
	prim.init();
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();
	if (!surface.load(prim, ray.Origin, ray.Direction))
	{
		return;
	}

	// Guard the SHADED pixel's surface normal. surface.load does N =
	// normalize(N) with no zero-guard (surfaceHF.hlsli), so foliage cards /
	// billboards / impostors with missing or cancelling vertex normals give a
	// NaN surface.N. Every SH::CalculateIrradiance(surfel.radiance, N) in the
	// gather below would then be NaN - per-pixel NaN GI on exactly the foliage
	// pixels, which is why a coverage tile straddling a leaf edge goes NaN
	// (black/white) on the leaf and stays clean on the background behind it.
	// Fall back to a camera-facing normal so the gather stays finite
	// (approximate ambient GI) instead of poisoning the pixel and its temporal
	// history.
	float3 N = surface.N;
	if (any(isnan(N)) || any(isinf(N)) || dot(N, N) < 0.5)
		N = normalize(-ray.Direction);

	// Skip spawning on strongly emissive surfaces: their appearance is
	// dominated by emission, so cached diffuse GI on them is wasted. They still
	// light other surfels (emission is gathered at ray-trace hit points, not
	// from surfels on them), so this only saves placement/gather cost. Gates
	// the spawn paths below; the GI gather/render still runs on these pixels.
	const bool skip_emissive_spawn =
		max3(surface.emissiveColor) >= SURFEL_EMISSIVE_SPAWN_SKIP;

	// Skip spawning on fully-metallic (or fully-black) surfaces: metals have no
	// diffuse response, so cached diffuse GI is invisible on them. Surface
	// stores no metalness, but full metalness zeroes albedo, so ~zero albedo is
	// exactly the metalness>=1 case. They still reflect via specular
	// (RT/screen-space reflections), not surfels, so skipping placement costs
	// nothing here.
	const bool skip_metallic_spawn =
		max3(surface.albedo) <= SURFEL_METALLIC_ALBEDO_SKIP;

	// Skip spawning on transparent surfaces: glass/transmissive materials
	// transmit and refract light along ray paths rather than reflecting it
	// diffusely, so cached diffuse GI on them is meaningless.
	// surface.transmission already folds in cloak (transmission =
	// lerp(GetTransmission(), 1, GetCloak())), so this one test covers both
	// transmissive and cloaked materials.
	const bool skip_transparent_spawn =
		surface.transmission > SURFEL_TRANSMISSION_SPAWN_SKIP;

	// Combined spawn skip for surfaces where cached diffuse GI is wasted.
	const bool skip_spawn =
		skip_emissive_spawn || skip_metallic_spawn || skip_transparent_spawn;

	float coverage = 0; // spawn metric (loose radial * dotN)
	// Distance (squared) to the nearest same-orientation surfel, for the
	// Poisson-disk minimum-spacing spawn reject below. Found for free during
	// the gather since it already visits every nearby surfel.
	float nearest_dist2 = 1e30;

	// Cascade levels around this point's own level. surfel_update keeps every
	// surfel at surfel_level(its position), so a surface point and the surfels
	// on it share a level; +/-1 covers level-boundary pixels. Bounds it to <=3
	// levels.
	const uint base_level = surfel_level(surface.P);
	const uint level_lo = (base_level > 0) ? (base_level - 1) : 0;
	const uint level_hi = min(base_level + 1, SURFEL_GRID_LEVELS - 1);

	// Vote the group's base-level cell: every active pixel offers its base-cell
	// index; if they all agree (min == max) the group shares one cell and
	// caches it in LDS (see the note above the groupshared declarations).
	const int3 base_gridpos = surfel_cell(surface.P, base_level);
	const uint base_cellindex = surfel_cellvalid(base_gridpos)
		? surfel_cellindex(base_gridpos, base_level) : ~0u;
	InterlockedMin(GroupBaseCellMin, base_cellindex);
	InterlockedMax(GroupBaseCellMax, base_cellindex);
	GroupMemoryBarrierWithGroupSync();

	// use_lds is group-uniform (both operands are groupshared, read after the
	// barrier), so the cooperative blocks below never diverge within the group.
	const bool use_lds =
		(GroupBaseCellMin == GroupBaseCellMax) && (GroupBaseCellMin != ~0u);
	if (use_lds)
	{
		// Every active thread reads the identical shared-cell header (a benign
		// same-value race on the groupshared scalars).
		SurfelGridCell shared_cell = surfelGridBuffer[GroupBaseCellMin];
		GroupBaseCellOffset = shared_cell.offset;
		GroupLdsCount = min(shared_cell.count, COVERAGE_LDS_SURFELS);
	}
	GroupMemoryBarrierWithGroupSync();

	// Cooperative fill: active threads pull cache slots via an atomic cursor,
	// so the cache is fully populated regardless of which lanes are active this
	// group.
	if (use_lds)
	{
		uint slot = 0;
		InterlockedAdd(GroupLoadNext, 1, slot);
		[loop] while (slot < GroupLdsCount)
		{
			uint idx = surfelCellBuffer[GroupBaseCellOffset + slot];
			GroupSurfels[slot] = surfelBuffer[idx];
			GroupSurfelIndices[slot] = idx;
			InterlockedAdd(GroupLoadNext, 1, slot);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	// Gather. The base level reads the shared cell from LDS when the group
	// agrees on it (with any >CAP tail from global); the +/-1 levels and
	// non-shared tiles use the global path. Both paths route through
	// gather_surfel(), so identical.
	for (uint level = level_lo; level <= level_hi; ++level)
	{
		int3 gridpos = surfel_cell(surface.P, level);
		if (!surfel_cellvalid(gridpos))
			continue;

		const uint cellindex = surfel_cellindex(gridpos, level);
		SurfelGridCell cell = surfelGridBuffer[cellindex];

		if (use_lds && level == base_level && cellindex == GroupBaseCellMin)
		{
			// Cached prefix from LDS.
			for (uint i = 0; i < GroupLdsCount; ++i)
				gather_surfel(surface.P, N, GroupSurfels[i], GroupSurfelIndices[i],
					push.debug, coverage, nearest_dist2, color, debug);

			// Tail beyond the cache (cell larger than COVERAGE_LDS_SURFELS)
			// from global memory.
			for (uint i = GroupLdsCount; i < cell.count; ++i)
			{
				uint surfel_index = surfelCellBuffer[cell.offset + i];
				gather_surfel(surface.P, N, surfelBuffer[surfel_index], surfel_index,
					push.debug, coverage, nearest_dist2, color, debug);
			}
		}
		else
		{
			for (uint i = 0; i < cell.count; ++i)
			{
				uint surfel_index = surfelCellBuffer[cell.offset + i];
				gather_surfel(surface.P, N, surfelBuffer[surfel_index], surfel_index,
					push.debug, coverage, nearest_dist2, color, debug);
			}
		}
	}

	// The level and cell a newly spawned surfel here would occupy (placed at
	// surfel_level); gates spawning and drives the heatmap debug.
	const uint spawn_level = base_level;
	const int3 spawn_gridpos = surfel_cell(surface.P, spawn_level);
	const uint spawn_cell_count = surfel_cellvalid(spawn_gridpos)
		? surfelGridBuffer[surfel_cellindex(spawn_gridpos, spawn_level)].count
		: SURFEL_CELL_LIMIT;

	if (!skip_spawn && spawn_cell_count < SURFEL_CELL_LIMIT)
	{
		uint surfel_count_at_pixel = 0;
		surfel_count_at_pixel |= (uint(coverage) & 0xFF) << 24; // the upper bits matter most for min selection
		surfel_count_at_pixel |= (uint(rng.next_float() * 65535) & 0xFFFF) << 8; // shuffle pixels randomly
		surfel_count_at_pixel |= (GTid.x & 0xF) << 4;
		surfel_count_at_pixel |= (GTid.y & 0xF) << 0;
		InterlockedMin(GroupMinSurfelCount[subtile], surfel_count_at_pixel);
	}

	if (color.a > 0)
	{
		color.rgb /= color.a;
		color.rgb /= PI;
		color.a = saturate(color.a);
	}

	switch (push.debug)
	{
	case SURFEL_DEBUG_NORMAL:
		debug.rgb = normalize(debug.rgb) * 0.5 + 0.5;
		break;
	case SURFEL_DEBUG_COLOR:
		debug = color;
		debug.rgb = tonemap(debug.rgb);
		debug.a = 1;
		break;
	case SURFEL_DEBUG_RANDOM:
		if (debug.a > 0)
		{
			debug /= debug.a;
		}
		else
		{
			debug = 0;
		}
		break;
	case SURFEL_DEBUG_HEATMAP:
		{
			const float3 mapTex[] = {
				float3(0,0,0),
				float3(0,0,1),
				float3(0,1,1),
				float3(0,1,0),
				float3(1,1,0),
				float3(1,0,0),
			};
			const uint mapTexLen = 5;
			const uint maxHeat = 100;
			float l = saturate((float)spawn_cell_count / maxHeat) * mapTexLen;
			float3 a = mapTex[floor(l)];
			float3 b = mapTex[ceil(l)];
			float4 heatmap = float4(lerp(a, b, l - floor(l)), 0.8);
			debug = heatmap;
		}
		break;
	case SURFEL_DEBUG_INCONSISTENCY:
		if (debug.a > 0)
		{
			debug /= debug.a;
		}
		else
		{
			debug = 0;
		}
		break;
	default:
		break;
	}

	GroupMemoryBarrierWithGroupSync();

	// Write the GI result up front, before the spawn logic. The spawn block
	// below returns early on the elected candidate pixels; if the writes lived
	// after it, those pixels would keep their cleared (zero) value, and since
	// the candidate is re-chosen randomly every frame a different pixel would
	// flash to black each frame, producing visible flicker.
	write_result(DTid.xy, color);
	write_debug(DTid.xy, debug);

	if (!skip_spawn && spawn_cell_count < SURFEL_CELL_LIMIT)
	{
		uint surfel_coverage = GroupMinSurfelCount[subtile];
		uint2 minGTid;
		minGTid.x = (surfel_coverage >> 4) & 0xF;
		minGTid.y = (surfel_coverage >> 0) & 0xF;
		if (GTid.x == minGTid.x && GTid.y == minGTid.y)
		{
			// Poisson-disk fill - the DENSITY DRIVER. Spawn only where the
			// nearest same-orientation surfel is FARTHER than radius*SPACING,
			// i.e. wherever there is still a gap; reject (return) when one is
			// already within spacing. This fills the surface to a uniform
			// spacing of radius*SPACING and stops, so SURFEL_SPAWN_MIN_SPACING
			// sets the density directly: smaller = denser/more overlap.
			// Deliberately NOT gated on coverage - big surfels make coverage
			// saturate while still sparse, which stalled the fill (and the old
			// deficit probability went to 0 as coverage approached target,
			// stalling it asymptotically).
			const float spawn_radius = surfel_cellsize(spawn_level);
			const float desired_spacing =
				spawn_radius * surfel_spawn_spacing(spawn_level);

			if (nearest_dist2 < sqr(desired_spacing))
				return;

			// Distance-compensated per-cell spawn rate. The spawner is
			// screen-space (one candidate per COVERAGE_SUBTILE_SIZE tile), so
			// up close MANY tiles overlap one world cell and, seeing coverage 0
			// on a fresh frame, would all spawn into it at once (the
			// burst/clump). Scale the per-tile spawn probability by
			// (tile_world_size / cell_size)^2 so the EXPECTED new surfels per
			// cell per frame stays ~SURFEL_SPAWN_PER_CELL at any distance:
			// close => tiny per-tile prob (many tiles share the cell), far =>
			// ~1. tile_world_size = subtile size in full-res px *
			// world-per-pixel.
			const float dist_to_cam = distance(surface.P, GetCamera().position);
			const float world_per_pixel = 2.0 * dist_to_cam /
				(GetCamera().projection[1][1] * (float)GetCamera().internal_resolution.y);
			const float subtile_world =
				(float)COVERAGE_SUBTILE_SIZE * 2.0 * world_per_pixel;
			const float distance_prob =
				saturate(SURFEL_SPAWN_PER_CELL * sqr(subtile_world / spawn_radius));

			// Rate-limit only (anti-burst); density is set by the spacing gap
			// above, not by coverage. The fill stops on its own once every gap
			// is closed (no neighbour farther than the spacing), so no coverage
			// gate is needed here.
			if (rng.next_float() > distance_prob)
				return;

			// Respect the per-frame spawn budget to bound placement cost:
			uint spawn_index;
			InterlockedAdd(surfelStatsBuffer[0].spawnCount, 1, spawn_index);
			if (spawn_index >= SURFEL_SPAWN_BUDGET)
				return;

			// new particle index retrieved from dead list (pop):
			int deadCount;
			InterlockedAdd(surfelStatsBuffer[0].deadCount, -1, deadCount);
			if (deadCount <= 0 || deadCount > SURFEL_CAPACITY)
				return;
			uint newSurfelIndex = surfelDeadBuffer[deadCount - 1];

			// and add index to the alive list (push):
			uint aliveCount;
			InterlockedAdd(surfelStatsBuffer[0].nextCount, 1, aliveCount);
			if (aliveCount < SURFEL_CAPACITY)
			{
				surfelAliveBuffer[aliveCount] = newSurfelIndex;

				SurfelData surfel_data = (SurfelData)0;
				surfel_data.primitiveID = prim.pack2();
				surfel_data.bary = pack_half2(surface.bary.xy);
				surfel_data.uid = surface.inst.uid;
				surfel_data.SetBackfaceNormal(surface.IsBackface());
				surfel_data.max_inconsistency = 1;
				surfelDataBuffer[newSurfelIndex] = surfel_data;
			}
		}
	}
}
