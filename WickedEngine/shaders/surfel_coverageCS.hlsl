#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);

//#define SURFEL_DEBUG_NORMAL
//#define SURFEL_DEBUG_COLOR
//#define SURFEL_DEBUG_POINT
//#define SURFEL_DEBUG_RANDOM
//#define SURFEL_DEBUG_DATA


static const uint nice_colors_size = 5;
static const float3 nice_colors[nice_colors_size] = {
	float3(0,0,1),
	float3(0,1,1),
	float3(0,1,0),
	float3(1,1,0),
	float3(1,0,0),
};
float3 hash_color(uint index)
{
	return nice_colors[index % nice_colors_size];
}

static const uint2 pixel_offsets[4] = {
	uint2(0, 0), uint2(1, 0),
	uint2(0, 1), uint2(1, 1),
};


STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelIndexBuffer, uint, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(surfelCellIndexBuffer, float, TEXSLOT_ONDEMAND3);
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

	const float depth = texture_depth[pixel];
	//const float4 g1 = texture_gbuffer1[pixel];

	float4 debug = 0;
	float4 color = 0;

	if (depth > 0 /*&& any(g1)*/)
	{
		const float2 uv = ((float2)pixel + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);
		const float3 V = normalize(g_xCamera_CamPos - P);

		//const float3 N = normalize(g1.rgb * 2 - 1);




		uint2 primitiveID = texture_gbuffer0[pixel];
		uint primitiveIndex = primitiveID.x;
		uint instanceID = primitiveID.y & 0xFFFFFF;
		uint subsetIndex = (primitiveID.y >> 24u) & 0xFF;
		ShaderMeshInstance inst = InstanceBuffer[instanceID];
		ShaderMesh mesh = inst.mesh;
		ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][subsetIndex];
		uint startIndex = primitiveIndex * 3 + subset.indexOffset;
		uint i0 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 0];
		uint i1 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 1];
		uint i2 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 2];

		float3 N = 0;
		[branch]
		if (mesh.vb_pos_nor_wind >= 0)
		{
			uint4 data0 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i0 * 16);
			uint4 data1 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i1 * 16);
			uint4 data2 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i2 * 16);
			float4x4 worldMatrix = float4x4(transpose(inst.transform), float4(0, 0, 0, 1));
			float3 p0 = mul(worldMatrix, float4(asfloat(data0.xyz), 1)).xyz;
			float3 p1 = mul(worldMatrix, float4(asfloat(data1.xyz), 1)).xyz;
			float3 p2 = mul(worldMatrix, float4(asfloat(data2.xyz), 1)).xyz;
			float3 n0 = unpack_unitvector(data0.w);
			float3 n1 = unpack_unitvector(data1.w);
			float3 n2 = unpack_unitvector(data2.w);

			float3 bary = compute_barycentrics(P, p0, p1, p2);
			float u = bary.x;
			float v = bary.y;
			float w = bary.z;
			N = n0 * w + n1 * u + n2 * v;
			N = mul((float3x3)worldMatrix, N);
			N = normalize(N);
		}



		uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);

		int3 cell = surfel_cell(P);

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
				//uint hash = surfelCellIndexBuffer[surfel_index];

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
							contribution = smoothstep(0, 1, contribution);
							contribution *= saturate(dotN);

							color += float4(surfel.mean, 1) * contribution;

#ifdef SURFEL_DEBUG_NORMAL
							debug.rgb += normal * contribution;
							debug.a = 1;
#endif // SURFEL_DEBUG_NORMAL

#ifdef SURFEL_DEBUG_RANDOM
							debug += float4(hash_color(surfel_index), 1) * contribution;
#endif // SURFEL_DEBUG_RANDOM

#ifdef SURFEL_DEBUG_DATA
							debug += float4(surfel.inconsistency.xxx, 1) * contribution;
#endif // SURFEL_DEBUG_DATA

						}

#ifdef SURFEL_DEBUG_POINT
						if (dist2 <= sqr(0.1))
							debug = float4(1, 0, 0, 1);
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

		uint surfel_count_at_pixel = (uint)color.a;
		surfel_count_at_pixel <<= 8;
		surfel_count_at_pixel |= (GTid.x & 0xF) << 4;
		surfel_count_at_pixel |= (GTid.y & 0xF) << 0;

		InterlockedMin(GroupMinSurfelCount, surfel_count_at_pixel);

		if (color.a > 0)
		{
			color /= color.a;
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

#if defined(SURFEL_DEBUG_RANDOM) || defined(SURFEL_DEBUG_DATA)
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

	result[DTid.xy * 2 + uint2(0, 0)] = color.rgb;
	result[DTid.xy * 2 + uint2(1, 0)] = color.rgb;
	result[DTid.xy * 2 + uint2(0, 1)] = color.rgb;
	result[DTid.xy * 2 + uint2(1, 1)] = color.rgb;

	debugUAV[DTid.xy * 2 + uint2(0, 0)] = debug;
	debugUAV[DTid.xy * 2 + uint2(1, 0)] = debug;
	debugUAV[DTid.xy * 2 + uint2(0, 1)] = debug;
	debugUAV[DTid.xy * 2 + uint2(1, 1)] = debug;
}
