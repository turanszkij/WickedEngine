#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "oceanSurfaceHF.hlsli"
#include "lightingHF.hlsli"
#include "fogHF.hlsli"

#define INTERSECTION_DISTORT
#define LENS_DISTORT
//#define ANIMATED_DISTORT

#define WATER_INSCATTERING_PHASE_G 0.25

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

// Modified version of: https://www.shadertoy.com/view/XdyfR1
float GodRays(in float2 lightscreen, in float2 ndc, in float2 uv, in float iTime, in float GOD_RAY_LENGTH = 0.1, in float GOD_RAY_FREQUENCY = 48.0)
{
	float2 godRayOrigin = ndc - lightscreen;
	//float rayInputFunc = atan2(godRayOrigin.y, godRayOrigin.x) * 0.63661977236; // that's 2/pi
	float rayInputFunc = atan2(godRayOrigin.y, godRayOrigin.x);
	float light = (sin(rayInputFunc * GOD_RAY_FREQUENCY + iTime * -2.25) * 0.5 + 0.5);
	light = 0.5 * (light + (sin(rayInputFunc * 13.0 + iTime) * 0.5 + 0.5));
	//light *= (sin(rayUVFunc * 8.0 + -iTime * 0.25) * 0.5 + 0.5);
	//light *= pow(clamp(dot(normalize(-godRayOrigin), normalize(ndc - godRayOrigin)), 0.0, 1.0), 2.5);
	light *= pow(uv.y, GOD_RAY_LENGTH);
	light = pow(light, 1.75);
	//light *= pow(length(godRayOrigin), 4);
	return light;
}


// Approximate the displaced waterline, then integrate only the part of the view
// segment that is in ocean. A dry viewer can see fogged water outside a pocket;
// an underwater viewer looking into a pocket gets fog only up to the boundary.
float ocean_fog_with_exclusions(ShaderOcean ocean, float3 O, float3 P, out float camera_depth)
{
	const float start_height = ocean_height_at(ocean, O.xz);
	const float end_height = ocean_height_at(ocean, P.xz);
	const float start_depth = start_height - O.y;
	const float end_depth = end_height - P.y;
	const bool start_wet_level = start_depth > 0;
	const bool end_wet_level = end_depth > 0;
	camera_depth = 0;
	if (!start_wet_level && !end_wet_level)
	{
		return 0;
	}

	if (start_wet_level != end_wet_level)
	{
		const float3 waterline = lerp(O, P, start_depth / (start_depth - end_depth));
		if (start_wet_level)
		{
			P = waterline;
		}
		else
		{
			// A normally visible surface already applies refraction and extinction.
			// Only continue from above if there is no surface on this ray.
			if (!ocean_exclusion_check(waterline))
			{
				return 0;
			}
			O = waterline;
		}
	}

	const float fraction = ocean_unexcluded_fraction(O, P);
	const float ray_length = length(P - O);
	camera_depth = max(0, start_depth) * fraction;
	return min(ray_length, max(ray_length * 0.1, end_depth)) * fraction;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (any(DTid.xy >= postprocess.resolution))
	{
		return;
	}
	const float2 original_uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	float2 uv = original_uv;
	const ShaderOcean ocean = GetWeather().ocean;
	if (!ocean.IsValid())
	{
		output[DTid.xy] = input.SampleLevel(sampler_linear_clamp, original_uv, 0);
		return;
	}

	// The rasterized ocean uses this near cutoff for the waterline. Lens
	// distortion belongs to the viewing medium, not to a distant wet surface.
	const ShaderCamera camera = GetCamera();
	const bool has_exclusions = ocean_has_exclusions(ocean);
	float3 world_pos = reconstruct_position(uv, OCEAN_NEARPLANE_CUTOFF);
	float water_height = ocean_height_at(ocean, world_pos.xz);
	float waterline_blend = 1 - smoothstep(0.0, 0.025, saturate(world_pos.y - water_height - 0.01));
	bool distort_view = true;
	[branch]
	if (has_exclusions)
	{
		const float3 eye = camera.IsOrtho() ? reconstruct_position(uv, 1.0) : camera.position;
		distort_view = waterline_blend > 0 && !ocean_exclusion_check(eye) && !ocean_exclusion_check(world_pos);
	}

#ifdef INTERSECTION_DISTORT
	if (distort_view)
	{
		const float intersection_blend = saturate(exp(-abs(world_pos.y - water_height) * 50));
		uv = clipspace_to_uv(uv_to_clipspace(uv) * lerp(1, 0.5, intersection_blend));
		world_pos = reconstruct_position(uv, OCEAN_NEARPLANE_CUTOFF);
		water_height = ocean_height_at(ocean, world_pos.xz);
		waterline_blend = 1 - smoothstep(0.0, 0.025, saturate(world_pos.y - water_height - 0.01));
	}
#endif // INTERSECTION_DISTORT

	const float4 original_color = input.SampleLevel(sampler_linear_clamp, has_exclusions ? original_uv : uv, 0);
	if (!has_exclusions && waterline_blend <= 0)
	{
		output[DTid.xy] = original_color;
		return;
	}

	float4 color;
	{
		if (distort_view)
		{
#ifdef LENS_DISTORT
			uv = clipspace_to_uv(brownConradyDistortion(uv_to_clipspace(uv) * 0.9));
#endif // LENS_DISTORT
#ifdef ANIMATED_DISTORT
			uv += sin(uv * 10 + GetFrame().time * 5) * 0.005;
			uv += sin(-uv.y * 5 + GetFrame().time * 2) * 0.005;
#endif // ANIMATED_DISTORT
		}
		uv = saturate(uv);
		const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
		const float3 surface_position = reconstruct_position(uv, depth);
		const float4 clipspace2 = float4(uv_to_clipspace(uv), 1, 1);
		const float3 near_pos = reconstruct_position(uv, 1.0);
		const float3 origin = has_exclusions && !camera.IsOrtho() ? camera.position : near_pos;
		const float3 ray = origin - surface_position;
		const float surface_dist = length(ray);
		const float3 V = ray / max(surface_dist, 1e-5);

		float camera_depth;
		float fog_distance;
		[branch]
		if (has_exclusions)
		{
			fog_distance = ocean_fog_with_exclusions(ocean, origin, surface_position, camera_depth);
			if (fog_distance <= 0)
			{
				output[DTid.xy] = original_color;
				return;
			}
		}
		else
		{
			// Legacy waterline and fog approximation for scenes without cut-outs.
			float3 ocean_surface_pos = intersectPlaneClampInfinite(near_pos, -V, float3(0, 1, 0), ocean.water_height);
			Texture2D displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
			ocean_surface_pos += displacementmap.SampleLevel(sampler_linear_wrap, ocean_surface_pos.xz * ocean.patch_size_rcp, 0).xzy;
			const float ray_dist = min(surface_dist, length(ocean_surface_pos - near_pos)) * 0.1;
			fog_distance = max(ray_dist, water_height - surface_position.y);
			camera_depth = max(0, water_height - world_pos.y);
		}
		color = input.SampleLevel(sampler_linear_mirror, uv, 0);

		const float fogAmount = 1 - saturate(exp(-fog_distance * ocean.water_color.a));
		const float3 transmittance = saturate(exp(-fog_distance * ocean.extinction_color.rgb * ocean.water_color.a));
		half3 fogColor = ocean.water_color.rgb;

		// Sample inscattering color:
		{
			const half3 L = GetSunDirection();
			const half3 refractedLightDir = refract(-L, float3(0, 1, 0), 1.0 / 1.333);
		
			half3 inscatteringColor = GetSunColor();

			// Apply atmosphere transmittance:
			if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
			{
				// 0 for position since fog is centered around world center
				inscatteringColor *= GetAtmosphericLightTransmittance(GetWeather().atmosphere, 0, L, texture_transmittancelut);
			}
		
			// Apply phase function solely for directionality:
			const half cosTheta = dot(V, -refractedLightDir);
			inscatteringColor *= HgPhase(WATER_INSCATTERING_PHASE_G, cosTheta);

			// Apply uniform phase since this medium is constant:
			inscatteringColor *= UniformPhase();

			// My custom water fog scatter modulation:
			inscatteringColor *= (1 - ocean.extinction_color.rgb) * saturate(exp(-camera_depth * 0.25 * ocean.water_color.a));

			// Add some fake godray modulation to scatter:
			float4 lightScreen = mul(GetCamera().view_projection, float4(GetCamera().position + refractedLightDir * 10000, 1));
			lightScreen.xy /= lightScreen.w;
			//if (lightScreen.z > 0)
			{
				float godray = GodRays(lightScreen, clipspace2, uv, GetTime(), 0.1, 32.0) + GodRays(lightScreen, clipspace2, uv, -GetTime() * 0.5, 0.1, 20.0);
				godray *= 1 - pow(abs(dot(refractedLightDir, V)), 2); // blend out at light center
				godray *= pow(1 - saturate(-dot(refractedLightDir, V)), 1); // blend out at other side
				inscatteringColor *= 1 - godray;
			}
		
			fogColor += inscatteringColor;
		}

		color.rgb *= transmittance;
		color.rgb = lerp(color.rgb, fogColor, fogAmount);

		//color = fogAmount;
		//color = float4(1, 0, 0, 1);
	}

	// Preserve the legacy near-plane waterline blend when no cut-outs are active.
	output[DTid.xy] = has_exclusions ? color : lerp(original_color, color, waterline_blend);
}
