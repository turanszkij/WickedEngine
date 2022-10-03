#include "globals.hlsli"

static const uint region_count = 4;
Texture2D<float4> region_weights_texture : register(t0);

struct Terrain
{
	ShaderMaterial materials[region_count];
};
ConstantBuffer<Terrain> terrain : register(b0);

// These are expected to be in the same bind slots as corresponding MaterialComponent::TEXTURESLOT enums
RWTexture2D<unorm float4> output_baseColorMap : register(u0);
RWTexture2D<unorm float4> output_normalMap : register(u1);
RWTexture2D<unorm float4> output_surfaceMap : register(u2);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 output_dim = 0;
	output_baseColorMap.GetDimensions(output_dim.x, output_dim.y);
	const float2 uv = (DTid.xy + 0.5f) / output_dim;

	float4 region_weights = region_weights_texture.SampleLevel(sampler_linear_clamp, uv, 0);

	float weight_sum = 0;
	float4 total_baseColor = 0;
	float4 total_surface = 0;
	float2 total_normal = 0;
	for (uint i = 0; i < region_count; ++i)
	{
		float weight = region_weights[i];

		ShaderMaterial material = terrain.materials[i];

		float4 baseColor = material.baseColor;
		[branch]
		if (material.texture_basecolormap_index >= 0)
		{
			Texture2D tex = bindless_textures[material.texture_basecolormap_index];
			float2 dim = 0;
			tex.GetDimensions(dim.x, dim.y);
			float2 diff = dim / output_dim;
			float lod = log2(max(diff.x, diff.y));
			float2 overscale = lod < 0 ? diff : 1;
			float4 baseColorMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod);
			baseColor *= baseColorMap;
		}
		total_baseColor += baseColor * weight;

		float4 surface = float4(1, material.roughness, material.metalness, material.reflectance);
		[branch]
		if (material.texture_surfacemap_index >= 0)
		{
			Texture2D tex = bindless_textures[material.texture_surfacemap_index];
			float2 dim = 0;
			tex.GetDimensions(dim.x, dim.y);
			float2 diff = dim / output_dim;
			float lod = log2(max(diff.x, diff.y));
			float2 overscale = lod < 0 ? diff : 1;
			float4 surfaceMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod);
			surface *= surfaceMap;
		}
		total_surface += surface * weight;
		
		float2 normal = float2(0.5, 0.5);
		[branch]
		if (material.texture_normalmap_index >= 0)
		{
			Texture2D tex = bindless_textures[material.texture_normalmap_index];
			float2 dim = 0;
			tex.GetDimensions(dim.x, dim.y);
			float2 diff = dim / output_dim;
			float lod = log2(max(diff.x, diff.y));
			float2 overscale = lod < 0 ? diff : 1;
			float2 normalMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod).rg;
			normal = normalMap;
		}
		total_normal += normal * weight;

		weight_sum += weight;
	}
	total_baseColor /= weight_sum;
	total_surface /= weight_sum;
	total_normal /= weight_sum;

	output_baseColorMap[DTid.xy] = total_baseColor;
	output_surfaceMap[DTid.xy] = total_surface;
	output_normalMap[DTid.xy] = float4(total_normal, 1, 1);
}
