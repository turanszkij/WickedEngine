#include "globals.hlsli"

static const uint region_count = 4;

PUSHCONSTANT(push, TerrainVirtualTexturePush);

struct Terrain
{
	ShaderMaterial materials[region_count];
};
ConstantBuffer<Terrain> terrain : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID)
{
	Texture2D<float4> region_weights_texture = bindless_textures[push.region_weights_textureRO];
	RWTexture2D<unorm float4> output = bindless_rwtextures[push.output_textureRW];

	const uint2 pixel = DTid.xy + push.offset;

	float2 output_dim;
	output.GetDimensions(output_dim.x, output_dim.y);
	const float2 uv = (pixel.xy + 0.5f) / output_dim;

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
				float2 diff = dim / output_dim;
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
				float2 diff = dim / output_dim;
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
				float2 diff = dim / output_dim;
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

	output[pixel] = total_color;
}
