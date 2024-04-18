#include "globals.hlsli"
#include "BlockCompress.hlsli"
#include "ColorSpaceUtility.hlsli"

PUSHCONSTANT(push, TerrainVirtualTexturePush);

Texture2DArray<float> blendmap : register(t0);
ByteAddressBuffer blendmap_buffer : register(t1);

#if !defined(UPDATE_NORMALMAP) && !defined(UPDATE_SURFACEMAP) && !defined(UPDATE_EMISSIVEMAP)
#define UPDATE_BASECOLORMAP
#endif

#if defined(UPDATE_BASECOLORMAP) || defined(UPDATE_EMISSIVEMAP)
RWTexture2D<uint2> output : register(u0);
#else
RWTexture2D<uint4> output : register(u0);
#endif

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

	uint2 dim;
	uint array_size;
	blendmap.GetDimensions(dim.x, dim.y, array_size);
		
#ifdef UPDATE_BASECOLORMAP
	float3 block_rgb[16];
#endif // UPDATE_BASECOLORMAP
		
#ifdef UPDATE_EMISSIVEMAP
	float3 block_rgb[16];
#endif // UPDATE_EMISSIVEMAP

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
		
		float4 total_color = 0;
		float accumulation = 0;

		// Note: blending is front-to back with early exit like decals
		for(int blendmap_index = array_size - 1; blendmap_index >= 0; blendmap_index--)
		{
			float weight = blendmap.SampleLevel(sampler_linear_clamp, float3(uv, blendmap_index), 0);
			if(weight == 0)
				continue;

			uint materialIndex = blendmap_buffer.Load(push.blendmap_buffer_offset + blendmap_index * sizeof(uint));
			ShaderMaterial material = load_material(materialIndex);

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
			total_color = mad(1 - accumulation, weight * baseColor, total_color);
			accumulation = mad(1 - weight, accumulation, weight);
			if(accumulation >= 1)
				break;
#endif // UPDATE_BASECOLORMAP

#ifdef UPDATE_NORMALMAP
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
				total_color.rg = mad(1 - accumulation, weight * normalMap, total_color.rg);
				accumulation = mad(1 - weight, accumulation, weight);
				if(accumulation >= 1)
					break;
			}
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
			total_color = mad(1 - accumulation, weight * surface, total_color);
			accumulation = mad(1 - weight, accumulation, weight);
			if(accumulation >= 1)
				break;
#endif // UPDATE_SURFACEMAP

#ifdef UPDATE_EMISSIVEMAP
			float4 emissiveColor = 0;
			[branch]
			if (material.textures[EMISSIVEMAP].IsValid())
			{
				Texture2D tex = bindless_textures[material.textures[EMISSIVEMAP].texture_descriptor];
				float2 dim = 0;
				tex.GetDimensions(dim.x, dim.y);
				float2 diff = dim * push.resolution_rcp;
				float lod = log2(max(diff.x, diff.y));
				float2 overscale = lod < 0 ? diff : 1;
				float4 emissiveMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod);
				emissiveColor.rgb = emissiveMap.rgb * emissiveMap.a;
				emissiveColor.a = emissiveMap.a;
			}
			total_color = mad(1 - accumulation, weight * emissiveColor, total_color);
			accumulation = mad(1 - weight, accumulation, weight);
			if(accumulation >= 1)
				break;
#endif // UPDATE_EMISSIVEMAP
		}

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

#ifdef UPDATE_EMISSIVEMAP
		block_rgb[idx] = ApplySRGBCurve_Fast(total_color.rgb);
#endif // UPDATE_EMISSIVEMAP
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

#ifdef UPDATE_EMISSIVEMAP
	output[write_coord] = CompressBC1Block(block_rgb);
#endif // UPDATE_EMISSIVEMAP
}

