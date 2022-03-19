#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	float4 clipspace = float4(uv_to_clipspace(uv), 1, 1); // near plane (reversed z)
	float4 unproj = mul(GetCamera().inverse_view_projection, clipspace);
	float3 world_pos = unproj.xyz / unproj.w;
	const ShaderOcean ocean = GetWeather().ocean;
	float3 ocean_pos = float3(world_pos.x, ocean.water_height, world_pos.z);
	[branch]
	if (ocean.texture_displacementmap >= 0)
	{
		const float2 ocean_uv = ocean_pos.xz * ocean.patch_size_rcp;
		Texture2D texture_displacementmap = bindless_textures[ocean.texture_displacementmap];
		const float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
		ocean_pos += displacement;
	}
	const float4 original_color = input.SampleLevel(sampler_linear_mirror, uv, 0);
	float4 color = original_color;
	if (world_pos.y < ocean_pos.y)
	{
		uv += sin(uv * 10 + GetFrame().time * 5) * 0.01;
		uv += sin(-uv.yx * 5 + GetFrame().time * 2) * 0.01;
		color = input.SampleLevel(sampler_linear_mirror, uv, 0);
		const float lineardepth = texture_lineardepth.SampleLevel(sampler_linear_clamp, uv, 0).r * GetCamera().z_far;
		color.rgb = lerp(color.rgb, float3(0, 0, 0.2), saturate(0.1 + lineardepth * 0.025));
	}
	output[DTid.xy] = color;
}
