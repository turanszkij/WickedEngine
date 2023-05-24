#include "globals.hlsli"
#include "BlockCompress.hlsli"
#include "ColorSpaceUtility.hlsli"

static const uint region_count = 4;

PUSHCONSTANT(push, TerrainVirtualTexturePush);

struct Terrain
{
	ShaderMaterial materials[region_count];
};
ConstantBuffer<Terrain> terrain : register(b0);

#if !defined(UPDATE_NORMALMAP) && !defined(UPDATE_SURFACEMAP)
#define UPDATE_BASECOLORMAP
RWTexture2D<uint2> output : register(u0);
#else
RWTexture2D<uint4> output : register(u0);
#endif // UPDATE_NORMALMAP

static const uint2 block_offsets[16] = {
	uint2(0, 0), uint2(1, 0), uint2(2, 0), uint2(3, 0),
	uint2(0, 1), uint2(1, 1), uint2(2, 1), uint2(3, 1),
	uint2(0, 2), uint2(1, 2), uint2(2, 2), uint2(3, 2),
	uint2(0, 3), uint2(1, 3), uint2(2, 3), uint2(3, 3),
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= push.write_size || DTid.y >= push.write_size)
		return;

	Texture2D<float4> region_weights_texture = bindless_textures[push.region_weights_textureRO];

#ifdef UPDATE_BASECOLORMAP
	float3 block_rgb[16];
#endif // UPDATE_BASECOLORMAP

#ifdef UPDATE_NORMALMAP
	float block_x[16];
	float block_y[16];
#endif // UPDATE_NORMALMAP

#ifdef UPDATE_SURFACEMAP
	float3 block_rgb[16];
	float block_a[16];
#endif // UPDATE_SURFACEMAP

	for(uint idx = 0; idx < 16; ++idx)
	{
		const uint2 block_offset = block_offsets[idx];
		const int2 pixel = push.offset + DTid.xy * 4 + block_offset;
		const float2 uv = (pixel.xy + 0.5f) * push.resolution_rcp;

		float4 region_weights = region_weights_texture.SampleLevel(sampler_linear_clamp, uv, 0);

		float weight_sum = 0;
		float4 total_color = 0;
		for (uint i = 0; i < region_count; ++i)
		{
			float weight = region_weights[i];

			ShaderMaterial material = terrain.materials[i];

#ifdef UPDATE_BASECOLORMAP
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
			//if (DTid.x < 2 || DTid.y < 2)
			//	total_color = 0;
#endif // UPDATE_BASECOLORMAP

#ifdef UPDATE_NORMALMAP
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
#endif // UPDATE_NORMALMAP

#ifdef UPDATE_SURFACEMAP
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
#endif // UPDATE_SURFACEMAP

			weight_sum += weight;
		}
		total_color /= weight_sum;

#ifdef UPDATE_BASECOLORMAP
		block_rgb[idx] = ApplySRGBCurve_Fast(total_color.rgb);
#endif // UPDATE_BASECOLORMAP

#ifdef UPDATE_NORMALMAP
		block_x[idx] = total_color.r;
		block_y[idx] = total_color.g;
#endif // UPDATE_NORMALMAP

#ifdef UPDATE_SURFACEMAP
		block_rgb[idx] = total_color.rgb;
		block_a[idx] = total_color.a;
#endif // UPDATE_SURFACEMAP
	}

	const uint2 write_coord = push.write_offset + DTid.xy;

#ifdef UPDATE_BASECOLORMAP
	output[write_coord] = CompressBC1Block(block_rgb);
#endif // UPDATE_BASECOLORMAP

#ifdef UPDATE_NORMALMAP
	output[write_coord] = CompressBC5Block(block_x, block_y);
#endif // UPDATE_NORMALMAP

#ifdef UPDATE_SURFACEMAP
	output[write_coord] = CompressBC3Block(block_rgb, block_a);
#endif // UPDATE_SURFACEMAP
}

