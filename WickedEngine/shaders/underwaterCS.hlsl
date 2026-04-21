#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "oceanSurfaceHF.hlsli"
#include "lightingHF.hlsli"
#include "fogHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

// https://www.shadertoy.com/view/MlSXR3
float2 brownConradyDistortion(float2 uv)
{
	// positive values of K1 give barrel distortion, negative give pincushion
	//float barrelDistortion1 = 0.15; // K1 in text books
	//float barrelDistortion2 = 0.0; // K2 in text books
	float barrelDistortion1 = 0.25; // K1 in text books
	float barrelDistortion2 = -0.34; // K2 in text books
	float r2 = uv.x*uv.x + uv.y*uv.y;
	uv *= 1.0 + barrelDistortion1 * r2 + barrelDistortion2 * r2 * r2;
	
	// tangential distortion (due to off center lens elements)
	// is not modeled in this function, but if it was, the terms would go here
	return uv;
}


[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;

	// Unproject near plane and determine for every pixel if it's below water surface:
	float4 clipspace = float4(uv_to_clipspace(uv), 0.2, 1); // push further away from near plane
	float4 unproj = mul(GetCamera().inverse_view_projection, clipspace);
	unproj.xyz /= unproj.w;
	float3 world_pos = unproj.xyz;
	const ShaderOcean ocean = GetWeather().ocean;
	float3 ocean_pos = float3(world_pos.x, ocean.water_height, world_pos.z);
	[branch]
	if (ocean.texture_displacementmap >= 0)
	{
		const float2 ocean_uv = ocean_pos.xz * ocean.patch_size_rcp;
		Texture2D texture_displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
		const float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
		ocean_pos += displacement;
	}

	float4 original_color = input.SampleLevel(sampler_linear_clamp, uv, 0);
	float4 color = original_color;
	if (world_pos.y < ocean_pos.y) // if below water surface, apply effects
	{

		// Some lens distortion uv modulation:
		uv = clipspace_to_uv(brownConradyDistortion(uv_to_clipspace(uv) * 0.9));
		color = input.SampleLevel(sampler_linear_clamp, uv, 0);

#if 0
		// It's not realistic to apply much refraction underwater to camera, but looks cool:
		uv += sin(uv * 10 + GetFrame().time * 5) * 0.005;
		uv += sin(-uv.y * 5 + GetFrame().time * 2) * 0.005;
		color = input.SampleLevel(sampler_linear_mirror, uv, 0);
#endif

		const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
		float3 surface_position = reconstruct_position(uv, depth);

		float3 V = GetCamera().position.xyz - surface_position.xyz;
		float surface_dist = length(V);
		V /= surface_dist;

		// The ocean is not rendered into the lineardepth unfortunately, so we also trace it:
		//	Otherwise the ocean surface could be same as infinite depth and incorrectly fogged
		float3 ocean_surface_pos = intersectPlaneClampInfinite(GetCamera().position, V, float3(0, 1, 0), ocean.water_height);
		float2 ocean_surface_uv = ocean_surface_pos.xz * ocean.patch_size_rcp;
		Texture2D texture_displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
		const float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, ocean_surface_uv, 0).xzy;
		ocean_surface_pos += displacement;
		const float ocean_dist = length(ocean_surface_pos - GetCamera().position);

		[branch]
		if (ocean.texture_displacementmap >= 0)
		{
			const float2 ocean_uv = surface_position.xz * ocean.patch_size_rcp;
			Texture2D texture_displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
			const float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
			surface_position += displacement;
		}

		const float ray_dist = min(surface_dist, ocean_dist) * 0.1;
		const float water_depth = max(0, ocean_pos.y - surface_position.y);
		const float camera_depth = max(0, ocean_pos.y - world_pos.y);
		const float fog_distance = max(ray_dist, water_depth);

		float fogAmount = 1 - saturate(exp(-fog_distance * ocean.water_color.a));
		float3 transmittance = saturate(exp(-fog_distance * ocean.extinction_color.rgb * ocean.water_color.a));
		half3 fogColor = ocean.water_color.rgb;

		// Sample inscattering color:
		{
			const half3 L = GetSunDirection();
		
			half3 inscatteringColor = GetSunColor();

			// Apply atmosphere transmittance:
			if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
			{
				// 0 for position since fog is centered around world center
				inscatteringColor *= GetAtmosphericLightTransmittance(GetWeather().atmosphere, 0, L, texture_transmittancelut);
			}
		
			// Apply phase function solely for directionality:
			const half cosTheta = dot(V, L);
			inscatteringColor *= HgPhase(FOG_INSCATTERING_PHASE_G, cosTheta);

			// Apply uniform phase since this medium is constant:
			inscatteringColor *= UniformPhase();

			// My custom water fog scatter modulation:
			inscatteringColor *= (1 - ocean.extinction_color.rgb) * saturate(exp(-camera_depth * 0.25 * ocean.water_color.a));
		
			fogColor += inscatteringColor;
		}

		color.rgb *= transmittance;
		color.rgb = lerp(color.rgb, fogColor, fogAmount);

		//color = fogAmount;
		//color = float4(1, 0, 0, 1);
	}

	// Transition at intersection:
	float intersection_direction = world_pos.y - ocean_pos.y;
	float intersection_distance = abs(intersection_direction);
	float intersection_blend = saturate(exp(-intersection_distance * 100));
	float3 intersection_color = ocean.water_color.rgb;
	color.rgb = lerp(color.rgb, original_color, intersection_blend);
	
	output[DTid.xy] = color;
}
