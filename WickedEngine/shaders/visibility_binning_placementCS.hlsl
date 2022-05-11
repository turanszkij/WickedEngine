#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "surfaceHF.hlsli"
#include "raytracingHF.hlsli"

Texture2D<float> texture_shadertypes : register(t0);

RWStructuredBuffer<ShaderTypeBin> output_bins : register(u0);
RWStructuredBuffer<uint> output_binned_pixels : register(u1);

groupshared uint local_bin_counts[SHADERTYPE_BIN_COUNT + 1];
groupshared uint local_bin_offsets[SHADERTYPE_BIN_COUNT + 1];

[numthreads(16, 16, 1)]
void main(uint2 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	if (groupIndex < SHADERTYPE_BIN_COUNT + 1)
	{
		local_bin_counts[groupIndex] = 0;
		local_bin_offsets[groupIndex] = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint2 pixel = DTid.xy;
	const bool pixel_valid = pixel.x < GetCamera().internal_resolution.x && pixel.y < GetCamera().internal_resolution.y;

	uint bin_index = ~0u;

	if (pixel_valid)
	{
		bin_index = texture_shadertypes[pixel] * 255.0;
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
