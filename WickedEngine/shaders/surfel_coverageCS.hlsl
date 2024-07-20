#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "raytracingHF.hlsli"
#include "brdf.hlsli"

PUSHCONSTANT(push, SurfelDebugPushConstants);

static const uint random_colors_size = 11;
static const float3 random_colors[random_colors_size] = {
	float3(0,0,1),
	float3(0,1,1),
	float3(0,1,0),
	float3(1,1,0),
	float3(1,0,0),
	float3(1,0,1),
	float3(0.5,1,1),
	float3(0.5,1,0.5),
	float3(1,1,0.5),
	float3(1,0.5,0.5),
	float3(1,0.5,1),
};
float3 random_color(uint index)
{
	return random_colors[index % random_colors_size];
}

StructuredBuffer<Surfel> surfelBuffer : register(t0);
StructuredBuffer<SurfelGridCell> surfelGridBuffer : register(t1);
StructuredBuffer<uint> surfelCellBuffer : register(t2);
Texture2D<float2> surfelMomentsTexture : register(t3);
Texture2D<float4> surfelIrradianceTexture : register(t4);

RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u0);
RWStructuredBuffer<uint> surfelDeadBuffer : register(u1);
RWStructuredBuffer<uint> surfelAliveBuffer : register(u2);
RWByteAddressBuffer surfelStatsBuffer : register(u3);
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

groupshared uint GroupMinSurfelCount;

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	if (groupIndex == 0)
	{
		GroupMinSurfelCount = ~0;
	}
	GroupMemoryBarrierWithGroupSync();
	
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
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();
	if (!surface.load(prim, ray.Origin, ray.Direction))
	{
		return;
	}

	const float3 N = surface.N;

	float coverage = 0;

	int3 gridpos = surfel_cell(surface.P);
	if (!surfel_cellvalid(gridpos))
	{
		write_debug(DTid.xy, 0);
		return;
	}

	uint cellindex = surfel_cellindex(gridpos);
	SurfelGridCell cell = surfelGridBuffer[cellindex];
	for (uint i = 0; i < cell.count; ++i)
	{
		uint surfel_index = surfelCellBuffer[cell.offset + i];
		Surfel surfel = surfelBuffer[surfel_index];

		float3 L = surface.P - surfel.position;
		float dist2 = dot(L, L);
		if (dist2 < sqr(surfel.GetRadius()))
		{
			float3 normal = normalize(unpack_unitvector(surfel.normal));
			float dotN = dot(N, normal);
			if (dotN > 0)
			{
				float dist = sqrt(dist2);
				float contribution = 1;

				contribution *= saturate(dotN);
				contribution *= saturate(1 - dist / surfel.GetRadius());
				contribution = smoothstep(0, 1, contribution);
				coverage += contribution;
				
				float2 moments = surfelMomentsTexture.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, L / dist), 0);
				contribution *= surfel_moment_weight(moments, dist);

				// contribution based on life can eliminate black popping surfels, but the surfel_data must be accessed...
				contribution = lerp(0, contribution, saturate(surfelDataBuffer[surfel_index].GetLife() / 2.0f));

				color += surfelIrradianceTexture.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, N), 0) * contribution;
				//color += float4(surfel.color, 1) * contribution;

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

	if (cell.count < SURFEL_CELL_LIMIT)
	{
		uint surfel_count_at_pixel = 0;
		surfel_count_at_pixel |= (uint(coverage) & 0xFF) << 24; // the upper bits matter most for min selection
		surfel_count_at_pixel |= (uint(rng.next_float() * 65535) & 0xFFFF) << 8; // shuffle pixels randomly
		surfel_count_at_pixel |= (GTid.x & 0xF) << 4;
		surfel_count_at_pixel |= (GTid.y & 0xF) << 0;
		InterlockedMin(GroupMinSurfelCount, surfel_count_at_pixel);
	}

	if (color.a > 0)
	{
		color.rgb /= color.a;
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
			float l = saturate((float)cell.count / maxHeat) * mapTexLen;
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

	if (cell.count < SURFEL_CELL_LIMIT)
	{
		uint surfel_coverage = GroupMinSurfelCount;
		uint2 minGTid;
		minGTid.x = (surfel_coverage >> 4) & 0xF;
		minGTid.y = (surfel_coverage >> 0) & 0xF;
		uint coverage_amount = surfel_coverage >> 24;
		if (GTid.x == minGTid.x && GTid.y == minGTid.y && coverage < SURFEL_TARGET_COVERAGE)
		{
			// Slow down the propagation by chance
			//	Closer surfaces have less chance to avoid excessive clumping of surfels
			const float lineardepth = compute_lineardepth(depth) * GetCamera().z_far_rcp;
			const float chance = pow(1 - lineardepth, 8);

			//if (blue_noise(Gid.xy).x < chance)
			//	return;

			if (rng.next_float() < chance)
				return;

			// new particle index retrieved from dead list (pop):
			int deadCount;
			surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_DEADCOUNT, -1, deadCount);
			if (deadCount <= 0 || deadCount > SURFEL_CAPACITY)
				return;
			uint newSurfelIndex = surfelDeadBuffer[deadCount - 1];

			// and add index to the alive list (push):
			uint aliveCount;
			surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_NEXTCOUNT, 1, aliveCount);
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

	write_result(DTid.xy, color);
	write_debug(DTid.xy, debug);
}
