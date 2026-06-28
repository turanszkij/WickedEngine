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

RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u0);
RWStructuredBuffer<uint> surfelDeadBuffer : register(u1);
RWStructuredBuffer<uint> surfelAliveBuffer : register(u2);
RWStructuredBuffer<SurfelStats> surfelStatsBuffer : register(u3);
RWTexture2D<float3> result : register(u4);
RWTexture2D<unorm float4> debugUAV : register(u5);

void write_result(uint2 DTid, float4 color)
{
	result[DTid] = color.rgb;
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

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	if (groupIndex < COVERAGE_SUBTILE_COUNT)
	{
		GroupMinSurfelCount[groupIndex] = ~0;
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

	const float3 N = surface.N;

	float coverage = 0;

	// Accumulate coverage and GI from surfels across all cascaded grid levels
	// (near surfaces are covered by fine level-0 surfels, distant ones by a few
	// large coarse-level surfels).
	for (uint level = 0; level < SURFEL_GRID_LEVELS; ++level)
	{
	int3 gridpos = surfel_cell(surface.P, level);
	if (!surfel_cellvalid(gridpos))
		continue;

	SurfelGridCell cell = surfelGridBuffer[surfel_cellindex(gridpos, level)];
	for (uint i = 0; i < cell.count; ++i)
	{
		uint surfel_index = surfelCellBuffer[cell.offset + i];
		Surfel surfel = surfelBuffer[surfel_index];

		float3 L = surface.P - surfel.position;
		float dist2 = dot(L, L);
		if (dist2 < sqr(surfel.GetRadius()))
		{
			float3 normal = normalize(unpack_half3(surfel.normal));
			float dotN = dot(N, normal);
			if (dotN > 0)
			{
				float dist = sqrt(dist2);

				// This surfel contributes GI to a visible pixel this frame, so
				// mark it "seen": the recycler (surfel_updateCS) treats recently
				// seen surfels as relevant and ages out the rest. A plain OR is
				// safe under the race here - every writer sets the same bit and
				// no other field of properties is written during coverage.
				surfelDataBuffer[surfel_index].properties |= SURFEL_PROPERTY_SEEN_BIT;

				// Smooth radial falloff: (1 - d^2/r^2)^2 reaches zero with zero
				// slope at the surfel edge, so surfels fade in/out gently as
				// the camera moves instead of popping at a hard boundary.
				float falloff = saturate(1 - dist2 / sqr(surfel.GetRadius()));
				falloff *= falloff;
				float contribution = saturate(dotN) * falloff;
				coverage += contribution;
				
				float2 moments = surfelMomentsTexture.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, L / dist), 0);
				contribution *= surfel_moment_weight(moments, dist);

				// Defensive 1-frame fade: a surfel reaches full weight as soon
				// as it has been integrated once (life >= 1). Birth seeding
				// (surfel_integrateCS) already gives newborns a plausible
				// non-black radiance, so the old half-weighting at life 1 only
				// slowed perceived placement and left under-covered pixels
				// dark; ramping to full at life 1 fills surfaces faster without
				// reintroducing black pops.
				contribution = lerp(0, contribution, saturate((float)surfelDataBuffer[surfel_index].GetLife()));

				color += float4(SH::CalculateIrradiance(surfel.radiance.Unpack(), N), 1) * contribution;

				switch (push.debug)
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

			if (push.debug == SURFEL_DEBUG_POINT)
			{
				if (dist2 <= sqr(0.05))
					debug = float4(1, 0, 1, 1);
			}
		}

	}
	}

	// The level and cell a newly spawned surfel here would occupy - used to
	// gate spawning and for the heatmap debug (a new surfel is placed at
	// surfel_level).
	const uint spawn_level = surfel_level(surface.P);
	const int3 spawn_gridpos = surfel_cell(surface.P, spawn_level);
	const uint spawn_cell_count = surfel_cellvalid(spawn_gridpos)
		? surfelGridBuffer[surfel_cellindex(spawn_gridpos, spawn_level)].count
		: SURFEL_CELL_LIMIT;

	if (spawn_cell_count < SURFEL_CELL_LIMIT)
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

	if (spawn_cell_count < SURFEL_CELL_LIMIT)
	{
		uint surfel_coverage = GroupMinSurfelCount[subtile];
		uint2 minGTid;
		minGTid.x = (surfel_coverage >> 4) & 0xF;
		minGTid.y = (surfel_coverage >> 0) & 0xF;
		if (GTid.x == minGTid.x && GTid.y == minGTid.y && coverage < SURFEL_TARGET_COVERAGE)
		{
			// Spawn probability grows with the coverage deficit: empty areas fill
			// quickly while areas near the target slow down to converge. Per-cell
			// density is bounded by SURFEL_CELL_LIMIT.
			const float deficit = saturate((SURFEL_TARGET_COVERAGE - coverage) / SURFEL_TARGET_COVERAGE);
			if (rng.next_float() > deficit)
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
