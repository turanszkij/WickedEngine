#include "globals.hlsli"

#define ASPM_HLSL
#include "compressonator/bcn_common_kernel.h"

static const uint region_count = 4;

PUSHCONSTANT(push, TerrainVirtualTexturePush);

struct Terrain
{
	ShaderMaterial materials[region_count];
};
ConstantBuffer<Terrain> terrain : register(b0);

RWTexture2D<uint2> output_bc1_unorm : register(u0);
RWTexture2D<uint4> output_bc3_unorm : register(u1);
RWTexture2D<uint4> output_bc5_unorm : register(u2);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID)
{
	Texture2D<float4> region_weights_texture = bindless_textures[push.region_weights_textureRO];

	float3 block_rgb[BLOCK_SIZE_4X4];
	float block_x[BLOCK_SIZE_4X4];
	float block_y[BLOCK_SIZE_4X4];

	for (uint y = 0; y < 4; ++y)
	{
		for (uint x = 0; x < 4; ++x)
		{
			const uint2 pixel = push.offset + DTid.xy * 4 + uint2(x, y);
			const float2 uv = (pixel.xy + 0.5f) * push.resolution_rcp;

			float4 region_weights = region_weights_texture.SampleLevel(sampler_linear_clamp, uv, 0);

			float weight_sum = 0;
			float4 total_color = 0;
			for (uint i = 0; i < region_count; ++i)
			{
				float weight = region_weights[i];

				ShaderMaterial material = terrain.materials[i];

				switch (push.map_type)
				{

				default:
				case 0:
				{
					float4 baseColor = material.baseColor;
					[branch]
					if (material.textures[BASECOLORMAP].IsValid())
					{
						Texture2D tex = bindless_textures[material.textures[BASECOLORMAP].texture_descriptor];
						float2 dim = 0;
						tex.GetDimensions(dim.x, dim.y);
						float2 diff = dim * push.resolution_rcp;
						float lod = log2(max(diff.x, diff.y));
						float2 overscale = lod < 0 ? diff : 1;
						float4 baseColorMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod);
						baseColor *= baseColorMap;
					}
					total_color += baseColor * weight;
				}
				break;

				case 1:
				{
					float2 normal = float2(0.5, 0.5);
					[branch]
					if (material.textures[NORMALMAP].IsValid())
					{
						Texture2D tex = bindless_textures[material.textures[NORMALMAP].texture_descriptor];
						float2 dim = 0;
						tex.GetDimensions(dim.x, dim.y);
						float2 diff = dim * push.resolution_rcp;
						float lod = log2(max(diff.x, diff.y));
						float2 overscale = lod < 0 ? diff : 1;
						float2 normalMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod).rg;
						normal = normalMap;
					}
					total_color += float4(normal.rg, 1, 1) * weight;
				}
				break;

				case 2:
				{
					float4 surface = float4(1, material.roughness, material.metalness, material.reflectance);
					[branch]
					if (material.textures[SURFACEMAP].IsValid())
					{
						Texture2D tex = bindless_textures[material.textures[SURFACEMAP].texture_descriptor];
						float2 dim = 0;
						tex.GetDimensions(dim.x, dim.y);
						float2 diff = dim * push.resolution_rcp;
						float lod = log2(max(diff.x, diff.y));
						float2 overscale = lod < 0 ? diff : 1;
						float4 surfaceMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod);
						surface *= surfaceMap;
					}
					total_color += surface * weight;
				}
				break;

				}

				weight_sum += weight;
			}
			total_color /= weight_sum;

			const uint idx = x + y * 4;
			if (push.map_type == 1)
			{
				block_x[idx] = total_color.r;
				block_y[idx] = total_color.g;
			}
			if (push.map_type == 2)
			{
				block_rgb[idx] = total_color.rgb;
				block_x[idx] = total_color.a;
			}
			else
			{
				block_rgb[idx] = total_color.rgb;
			}
		}
	}

	if (push.map_type == 1)
	{
		output_bc5_unorm[DTid.xy] = CompressBlockBC5_UNORM(block_x, block_y, CMP_QUALITY0);
	}
	if (push.map_type == 2)
	{
		output_bc3_unorm[DTid.xy] = CompressBlockBC3_UNORM(block_rgb, block_x, CMP_QUALITY2, /*isSRGB =*/ false);
	}
	else
	{
		output_bc1_unorm[DTid.xy] = CompressBlockBC1_UNORM(block_rgb, CMP_QUALITY0, /*isSRGB =*/ false);
	}
}
