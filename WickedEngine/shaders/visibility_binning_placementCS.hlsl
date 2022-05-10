#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "brdf.hlsli"

RWStructuredBuffer<ShaderTypeBin> output_bins : register(u14);
RWStructuredBuffer<uint> output_binned_pixels : register(u15);

groupshared uint local_bin_counts[SHADERTYPE_BIN_COUNT + 1];
groupshared uint local_bin_offsets[SHADERTYPE_BIN_COUNT + 1];

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if (groupIndex < SHADERTYPE_BIN_COUNT + 1)
	{
		local_bin_counts[groupIndex] = 0;
		local_bin_offsets[groupIndex] = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint2 GTid = remap_lane_8x8(groupIndex);
	uint2 pixel = Gid.xy * 8 + GTid;
	const bool pixel_valid = pixel.x < GetCamera().internal_resolution.x && pixel.y < GetCamera().internal_resolution.y;

	uint bin_index = ~0u;

	if (pixel_valid)
	{
		uint primitiveID = texture_primitiveID[pixel];
		if (any(primitiveID))
		{
			PrimitiveID prim;
			prim.unpack(primitiveID);

			Surface surface;
			surface.init();

			if (surface.preload_internal(prim))
			{
				bin_index = surface.material.shaderType;
			}
		}
		else
		{
			bin_index = SHADERTYPE_BIN_COUNT;
		}
	}

	uint placement = 0;
	if (bin_index != ~0u)
	{
		InterlockedAdd(local_bin_counts[bin_index], 1, placement);
	}

	GroupMemoryBarrierWithGroupSync();
	if (groupIndex < SHADERTYPE_BIN_COUNT + 1)
	{
		InterlockedAdd(output_bins[groupIndex].count, local_bin_counts[groupIndex], local_bin_offsets[groupIndex]);
		local_bin_offsets[groupIndex] += output_bins[groupIndex].offset;
	}
	GroupMemoryBarrierWithGroupSync();

	if (pixel_valid)
	{
		placement += local_bin_offsets[bin_index];
		output_binned_pixels[placement] = pack_pixel(pixel);
	}
}
