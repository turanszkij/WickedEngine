#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<uint4> input : register(t0);
Texture2D<float> lineardepth_lowres : register(t1);

RWTexture2DArray<unorm float> output : register(u0);

float load_shadow(in uint shadow_index, in uint4 shadow_mask)
{
	uint mask_shift = (shadow_index % 4) * 8;
	uint mask_bucket = shadow_index / 4;
	uint mask = (shadow_mask[mask_bucket] >> mask_shift) & 0xFF;
	return mask / 255.0;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	uint2 pixel = DTid.xy;
	const float2 uv = (pixel + 0.5f) * postprocess.resolution_rcp;

	uint2 dim;
	uint MAX_RTSHADOWS;
	output.GetDimensions(dim.x, dim.y, MAX_RTSHADOWS);
	
	const uint2 tileIndex = uint2(floor(pixel / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, GetCamera().entity_culling_tilecount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;
	
	const float2 lowres_size = postprocess.params1.xy;
	const float2 lowres_texel_size = postprocess.params1.zw;
	
	float2 sam_pixel = uv * lowres_size + (-0.5 + 1.0 / 512.0); // (1.0 / 512.0) correction is described here: https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/
	float2 sam_pixel_frac = frac(sam_pixel);

	// Gather pattern:
	uint2 pixel0 = DTid.xy / 2 + uint2(0, 1);
	uint2 pixel1 = DTid.xy / 2 + uint2(1, 1);
	uint2 pixel2 = DTid.xy / 2 + uint2(0, 0);
	uint2 pixel3 = DTid.xy / 2 + uint2(1, 0);
	uint4 shadow_mask0 = input[pixel0];
	uint4 shadow_mask1 = input[pixel1];
	uint4 shadow_mask2 = input[pixel2];
	uint4 shadow_mask3 = input[pixel3];
	float lineardepth0 = lineardepth_lowres[pixel0] * GetCamera().z_far;
	float lineardepth1 = lineardepth_lowres[pixel1] * GetCamera().z_far;
	float lineardepth2 = lineardepth_lowres[pixel2] * GetCamera().z_far;
	float lineardepth3 = lineardepth_lowres[pixel3] * GetCamera().z_far;
	float lineardepth_highres = texture_lineardepth[pixel] * GetCamera().z_far;

	float threshold = 2;
	float4 weights = max(0.001, 1 - saturate(abs(float4(lineardepth0, lineardepth1, lineardepth2, lineardepth3) - lineardepth_highres) * threshold));
	float weights_norm = rcp(bilinear(weights, sam_pixel_frac));
	
	uint shadow_index = 0;
	
	[branch]
	if (GetFrame().lightarray_count > 0)
	{
		// Loop through light buckets in the tile:
		const uint first_item = GetFrame().lightarray_offset;
		const uint last_item = first_item + GetFrame().lightarray_count - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket && shadow_index < MAX_RTSHADOWS; ++bucket)
		{
			uint bucket_bits = load_entitytile(flatTileIndex + bucket);

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0 && shadow_index < MAX_RTSHADOWS)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				// Check if it is a light and process:
				[branch]
				if (entity_index >= first_item && entity_index <= last_item)
				{
					shadow_index = entity_index - GetFrame().lightarray_offset;
					if (shadow_index >= MAX_RTSHADOWS)
						break;

					ShaderEntity light = load_entity(entity_index);

					if (!light.IsCastingShadow())
					{
						continue;
					}

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue; // static lights will be skipped (they are used in lightmap baking)
					}
					
					float shadow0 = load_shadow(shadow_index, shadow_mask0);
					float shadow1 = load_shadow(shadow_index, shadow_mask1);
					float shadow2 = load_shadow(shadow_index, shadow_mask2);
					float shadow3 = load_shadow(shadow_index, shadow_mask3);

					float shadow = bilinear(float4(shadow0,shadow1,shadow2,shadow3) * weights, sam_pixel_frac);
					shadow *= weights_norm;
		
					output[uint3(pixel, shadow_index)] = shadow;
				}
			}
		}
	}
}
