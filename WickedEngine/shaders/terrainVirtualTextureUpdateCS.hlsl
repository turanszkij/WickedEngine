#include "globals.hlsli"

static const uint region_count = 4;
Texture2D<float4> region_weights_texture : register(t0);

struct Terrain
{
	ShaderMaterial materials[region_count];
};
ConstantBuffer<Terrain> terrain : register(b10);

RWTexture2D<unorm float4> output_baseColorMap : register(u0);
RWTexture2D<unorm float4> output_surfaceMap : register(u1);
RWTexture2D<unorm float4> output_normalMap : register(u2);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 dim = 0;
	output_baseColorMap.GetDimensions(dim.x, dim.y);
	const float2 uv = (DTid.xy + 0.5f) / dim;

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
			float4 baseColorMap = bindless_textures[material.texture_basecolormap_index].SampleLevel(sampler_linear_clamp, uv, 0);
			baseColor *= baseColorMap;
		}
		total_baseColor += baseColor * weight;

		float4 surface = float4(1, material.roughness, material.metalness, material.reflectance);
		[branch]
		if (material.texture_surfacemap_index >= 0)
		{
			float4 surfaceMap = bindless_textures[material.texture_surfacemap_index].SampleLevel(sampler_linear_clamp, uv, 0);
			surface *= surfaceMap;
		}
		total_surface += surface * weight;

		float2 normal = float2(0.5, 0.5);
		[branch]
		if (material.texture_normalmap_index >= 0)
		{
			float2 normalMap = bindless_textures[material.texture_normalmap_index].SampleLevel(sampler_linear_clamp, uv, 0).rg;
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
