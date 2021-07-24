#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelIndexBuffer, uint, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(surfelCellIndexBuffer, float, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(surfelCellOffsetBuffer, uint, TEXSLOT_ONDEMAND4);

RWTEXTURE2D(coverage, uint, 0);
RWTEXTURE2D(debugUAV, unorm float4, 1);

groupshared uint GroupMinSurfelCount;

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	if (groupIndex == 0)
	{
		GroupMinSurfelCount = ~0;
	}
	GroupMemoryBarrierWithGroupSync();

	const float depth = texture_depth[DTid.xy];

	float4 debug = 0;

	if (depth > 0)
	{
		const float2 uv = ((float2)DTid.xy + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);

		uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
		uint surfel_count_at_pixel = 0;


		// iterate through all [27] neighbor cells:
		[loop]
		for (int i = -1; i <= 1; ++i)
		{
			[loop]
			for (int j = -1; j <= 1; ++j)
			{
				[loop]
				for (int k = -1; k <= 1; ++k)
				{

					uint surfel_hash_target = surfel_hash(P + float3(i,j,k) * SURFEL_RADIUS);
					uint surfel_list_offset = surfelCellOffsetBuffer[surfel_hash_target];
					while (surfel_list_offset != ~0u && surfel_list_offset < surfel_count)
					{
						uint surfel_index = surfelIndexBuffer[surfel_list_offset];
						uint surfel_hash = surfelCellIndexBuffer[surfel_index];

						if (surfel_hash == surfel_hash_target)
						{
							Surfel surfel = surfelBuffer[surfel_index];
							float dist = length(P - surfel.position);
							if (dist <= SURFEL_RADIUS)
							{
								surfel_count_at_pixel++;
								debug.rgb += unpack_unitvector(surfel.normal) * (1 - dist / SURFEL_RADIUS);
								debug.a = 1;
							}
						}
						else
							break;

						surfel_list_offset++;
					}

				}
			}
		}

		surfel_count_at_pixel <<= 8;
		surfel_count_at_pixel |= (GTid.x & 0xF) << 4;
		surfel_count_at_pixel |= (GTid.y & 0xF) << 0;

		InterlockedMin(GroupMinSurfelCount, surfel_count_at_pixel);

		debug.rgb = normalize(debug.rgb) * 0.5 + 0.5;
	}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		coverage[Gid.xy] = GroupMinSurfelCount;
	}

	debugUAV[DTid.xy] = debug;
}
