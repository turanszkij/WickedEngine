#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "oceanSurfaceHF.hlsli"
#include "lightingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;

	// Unproject near plane and determine for every pixel if it's below water surface:
	float4 clipspace = float4(uv_to_clipspace(uv), 1, 1);
	float4 unproj = mul(GetCamera().inverse_view_projection, clipspace);
	unproj.xyz /= unproj.w;
	float3 world_pos = unproj.xyz;
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
	if (world_pos.y < ocean_pos.y) // if below water surface, apply effects
	{
#if 0
		// It's not realistic to apply much refraction underwater to camera, but looks cool:
		uv += sin(uv * 10 + GetFrame().time * 5) * 0.005;
		uv += sin(-uv.y * 5 + GetFrame().time * 2) * 0.005;
		color = input.SampleLevel(sampler_linear_mirror, uv, 0);
#endif

		const float depth = texture_depth[DTid.xy];
		const float lineardepth = compute_lineardepth(depth);

		// The ocean is not rendered into the lineardepth unfortunately, so we also trace it:
		//	Otherwise the ocean surface could be same as infinite depth and incorrectly fogged
		const float3 o = GetCamera().position;
		const float3 d = normalize(o.xyz - unproj.xyz);
		float3 ocean_surface_pos = intersectPlaneClampInfinite(o, d, float3(0, 1, 0), ocean.water_height);
		float2 ocean_surface_uv = ocean_surface_pos.xz * ocean.patch_size_rcp;
		Texture2D texture_displacementmap = bindless_textures[ocean.texture_displacementmap];
		const float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, ocean_surface_uv, 0).xzy;
		ocean_surface_pos += displacement;
		const float ocean_dist = length(ocean_surface_pos - o);

		float3 surface_position = reconstruct_position(uv, depth);
		[branch]
		if (ocean.texture_displacementmap >= 0)
		{
			const float2 ocean_uv = surface_position.xz * ocean.patch_size_rcp;
			Texture2D texture_displacementmap = bindless_textures[ocean.texture_displacementmap];
			const float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
			surface_position += displacement;
		}
		float water_depth = ocean_pos.y - surface_position.y;
		water_depth = max(min(lineardepth, ocean_dist), water_depth);
		
		color.rgb = lerp(ocean.water_color.rgb, color.rgb, saturate(exp(-water_depth * ocean.water_color.a)));
		//color = float4(1, 0, 0, 1);
	}

	// Transition at intersection:
	float intersection_direction = world_pos.y - ocean_pos.y;
	float intersection_distance = abs(intersection_direction);
	float intersection_blend = saturate(exp(-intersection_distance * 100));
	float3 intersection_color = ocean.water_color.rgb;
	color.rgb = lerp(color.rgb, intersection_color, intersection_blend);
	
	output[DTid.xy] = color;
}
