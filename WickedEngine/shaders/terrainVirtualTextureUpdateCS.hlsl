#include "globals.hlsli"

static const uint region_count = 4;
Texture2D<float4> region_weights_texture : register(t0);

struct Terrain
{
	ShaderMaterial materials[region_count];
};
ConstantBuffer<Terrain> terrain : register(b10);

RWTexture2D<unorm float4> output_baseColor : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 dim = 0;
	output_baseColor.GetDimensions(dim.x, dim.y);
	const float2 uv = (DTid.xy + 0.5f) / dim;

	float4 region_weights = region_weights_texture.SampleLevel(sampler_linear_clamp, uv, 0);

	float weight_sum = 0;
	float4 total_baseColor = 0;
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

		weight_sum += weight;
	}
	total_baseColor /= weight_sum;

	output_baseColor[DTid.xy] = total_baseColor;
}
