#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

//#define SURFEL_DEBUG_NORMAL
//#define SURFEL_DEBUG_COLOR
#define SURFEL_DEBUG_POINT
//#define SURFEL_DEBUG_RANDOM


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

static const uint2 pixel_offsets[4] = {
	uint2(0, 0), uint2(1, 0),
	uint2(0, 1), uint2(1, 1),
};


STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelPayloadBuffer, SurfelPayload, TEXSLOT_ONDEMAND1);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(surfelIndexBuffer, uint, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(surfelCellOffsetBuffer, uint, TEXSLOT_ONDEMAND4);

RWTEXTURE2D(coverage, uint, 0);
RWTEXTURE2D(result, float3, 1);
RWTEXTURE2D(debugUAV, unorm float4, 2);

groupshared uint GroupMinSurfelCount;

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	if (groupIndex == 0)
	{
		GroupMinSurfelCount = ~0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint2 pixel = DTid.xy * 2 + pixel_offsets[g_xFrame_FrameCount % 4];
	//uint2 pixel = DTid.xy;

	const float depth = texture_depth[pixel];
	const float4 g1 = texture_gbuffer1[pixel];

	float4 debug = 0;
	float4 color = 0;

	if (depth > 0 && any(g1))
	{
		const float2 uv = ((float2)pixel + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);

		const float3 N = normalize(g1.rgb * 2 - 1);

		uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);

		int3 cell = surfel_cell(P);
		float coverage = 0;

		// iterate through all [27] neighbor cells:
		[loop]
		for (uint i = 0; i < 27; ++i)
		{
			uint surfel_hash_target = surfel_hash(cell + surfel_neighbor_offsets[i]);

			uint surfel_list_offset = surfelCellOffsetBuffer[surfel_hash_target];
			while (surfel_list_offset != ~0u && surfel_list_offset < surfel_count)
			{
				uint surfel_index = surfelIndexBuffer[surfel_list_offset];
				Surfel surfel = surfelBuffer[surfel_index];
				uint hash = surfel_hash(surfel_cell(surfel.position));

				if (hash == surfel_hash_target)
				{
					float3 L = surfel.position - P;
					float dist2 = dot(L, L);
					if (dist2 <= SURFEL_RADIUS2)
					{
						float3 normal = normalize(unpack_unitvector(surfel.normal));
						float dotN = dot(N, normal);
						if (dotN > 0)
						{
							float dist = sqrt(dist2);
							float contribution = 1;
							contribution *= saturate(1 - dist / SURFEL_RADIUS);
							contribution *= pow(saturate(dotN), SURFEL_NORMAL_TOLERANCE);
							contribution = smoothstep(0, 1, contribution);
							coverage += contribution;

							SurfelPayload surfel_payload = surfelPayloadBuffer[surfel_index];
							color += unpack_half4(surfel_payload.color) * contribution;

#ifdef SURFEL_DEBUG_NORMAL
							debug.rgb += normal * contribution;
							debug.a = 1;
#endif // SURFEL_DEBUG_NORMAL

#ifdef SURFEL_DEBUG_RANDOM
							debug += float4(random_color(surfel_index), 1) * contribution;
#endif // SURFEL_DEBUG_RANDOM

						}

#ifdef SURFEL_DEBUG_POINT
						if (dist2 <= sqr(0.05))
							debug = float4(1, 0, 1, 1);
#endif // SURFEL_DEBUG_POINT
					}
				}
				else
				{
					// in this case we stepped out of the surfel list of the cell
					break;
				}

				surfel_list_offset++;
			}

		}

		uint surfel_count_at_pixel = (uint)coverage;
		surfel_count_at_pixel <<= 8;
		surfel_count_at_pixel |= (GTid.x & 0xF) << 4;
		surfel_count_at_pixel |= (GTid.y & 0xF) << 0;

		// Randomizing here too like in surfel placement can give better distribution,
		// as it can select a different initial pixel even while tile is very empty
		// So randomizing in both shaders is not a mistake
		const float lineardepth = getLinearDepth(depth) * g_xCamera_ZFarP_rcp;
		const float chance = pow(1 - lineardepth, 4);
		if (blue_noise(DTid.xy).x >= chance)
			InterlockedMin(GroupMinSurfelCount, surfel_count_at_pixel);

		if (color.a > 0)
		{
			color.rgb /= color.a;
			color.a = saturate(color.a);
		}
		else
		{
			color = 0;
		}

#ifdef SURFEL_DEBUG_NORMAL
		debug.rgb = normalize(debug.rgb) * 0.5 + 0.5;
#endif // SURFEL_DEBUG_NORMAL

#ifdef SURFEL_DEBUG_COLOR
		debug = color;
		debug.rgb = tonemap(debug.rgb);
#endif // SURFEL_DEBUG_COLOR

#if defined(SURFEL_DEBUG_RANDOM)
		if (debug.a > 0)
		{
			debug /= debug.a;
		}
		else
		{
			debug = 0;
		}
#endif // SURFEL_DEBUG_RANDOM
	}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		coverage[Gid.xy] = GroupMinSurfelCount;
	}

	// Todo: temporal accumulation
	result[DTid.xy * 2 + uint2(0, 0)] = color.rgb;
	result[DTid.xy * 2 + uint2(1, 0)] = color.rgb;
	result[DTid.xy * 2 + uint2(0, 1)] = color.rgb;
	result[DTid.xy * 2 + uint2(1, 1)] = color.rgb;
	
	debugUAV[DTid.xy * 2 + uint2(0, 0)] = debug;
	debugUAV[DTid.xy * 2 + uint2(1, 0)] = debug;
	debugUAV[DTid.xy * 2 + uint2(0, 1)] = debug;
	debugUAV[DTid.xy * 2 + uint2(1, 1)] = debug;


	//result[DTid.xy] = color;
	//debugUAV[DTid.xy] = debug;
}
