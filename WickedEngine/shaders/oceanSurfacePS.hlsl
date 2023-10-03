#define DISABLE_DECALS
#define DISABLE_ENVMAPS
#define DISABLE_TRANSPARENT_SHADOWMAP
#define TRANSPARENT
#define WATER
#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"
#include "objectHF.hlsli"

Texture2D<float4> texture_ocean_displacementmap : register(t0);
Texture2D<float4> texture_gradientmap : register(t1);

[earlydepthstencil]
float4 main(PSIn input) : SV_TARGET
{
	float lineardepth = input.pos.w;
	float4 color = xOceanWaterColor;
	float3 V = GetCamera().position - input.pos3D;
	float dist = length(V);
	V /= dist;
	float emissive = 0;
	uint2 pixel = input.pos.xy;

	float ocean_level_at_camera_pos = xOceanWaterHeight;
	ocean_level_at_camera_pos += texture_ocean_displacementmap.SampleLevel(sampler_linear_wrap, GetCamera().position.xz * xOceanPatchSizeRecip, 0).z; // texture contains xzy!
	const bool camera_above_water = GetCamera().position.y > ocean_level_at_camera_pos;

	const float gradient_fade = saturate(dist * 0.001);
	const float4 gradientNear = texture_gradientmap.Sample(sampler_aniso_wrap, input.uv);
	const float4 gradientFar = texture_gradientmap.Sample(sampler_aniso_wrap, input.uv * 0.125);
	const float2 gradient = lerp(gradientNear.rg, gradientFar.rg, gradient_fade);
	const float sss_amount = lerp(gradientNear.a, gradientFar.a, gradient_fade);

	Surface surface;
	surface.init();
	surface.flags |= SURFACE_FLAG_RECEIVE_SHADOW;
	surface.pixel = pixel;
	float depth = input.pos.z;
	surface.albedo = color.rgb;
	surface.f0 = camera_above_water ? 0.02 : 0.1;
	surface.roughness = 0.1;
	surface.P = input.pos3D;
	surface.N = normalize(float3(gradient.x, xOceanTexelLength * 2, gradient.y));
	surface.V = V;
	//surface.sss = color * sss_amount;
	//surface.sss_inv = 1.0f / ((1 + surface.sss) * (1 + surface.sss));
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	TiledLighting(surface, lighting, GetFlatTileIndex(pixel));

	float2 ScreenCoord = surface.pixel * GetCamera().internal_resolution_rcp;
	const float bump_strength = camera_above_water ? 0.04 : 0.1;

	[branch]
	if (GetCamera().texture_reflection_index >= 0)
	{
		//REFLECTION
		float4 reflectionPos = mul(GetCamera().reflection_view_projection, float4(input.pos3D, 1));
		float2 reflectionUV = clipspace_to_uv(reflectionPos.xy / reflectionPos.w);
		float4 reflectiveColor = bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_mirror, reflectionUV + surface.N.xz * bump_strength, 0);
		lighting.indirect.specular = reflectiveColor.rgb * surface.F;
	}

	[branch]
	if (GetCamera().texture_refraction_index >= 0)
	{
		if (camera_above_water)
		{
			// Water refraction:
			Texture2D texture_refraction = bindless_textures[GetCamera().texture_refraction_index];
			float4 water_plane = GetCamera().reflection_clip_plane;
			// First sample using full perturbation:
			float2 refraction_uv = ScreenCoord.xy + surface.N.xz * bump_strength;
			float refraction_depth = find_max_depth(refraction_uv, 2, 2);
			float3 refraction_position = reconstruct_position(refraction_uv, refraction_depth);
			float water_depth = -dot(float4(refraction_position, 1), water_plane);
			if (water_depth <= 0)
			{
			// Above water, fill holes by taking unperturbed sample:
				refraction_uv = ScreenCoord.xy;
			}
			else
			{
				// Below water, compute perturbation according to first sample water depth:
				refraction_uv = ScreenCoord.xy + surface.N.xz * bump_strength * saturate(1 - exp(-water_depth));
			}
			surface.refraction.rgb = texture_refraction.SampleLevel(sampler_linear_mirror, refraction_uv, 0).rgb;
			// Recompute depth params again with actual perturbation:
			refraction_depth = texture_depth.SampleLevel(sampler_point_clamp, refraction_uv, 0);
			refraction_position = reconstruct_position(refraction_uv, refraction_depth);
			water_depth = max(water_depth, -dot(float4(refraction_position, 1), water_plane));
			// Water fog computation:
			surface.refraction.a = saturate(exp(-water_depth * color.a));
			color.a = 1;
		}
		else
		{
			surface.refraction.a = 1; // no refraction fog when looking from under water to above water
		}
	}

	// Blend out at distance:
	color.a = saturate(1 - saturate(dist / GetCamera().z_far - 0.8) * 5.0); // fade will be on edge and inwards 20%
	color.a = lerp(0, color.a, saturate(pow(lineardepth, 2))); // fade when very close to camera

	ApplyLighting(surface, lighting, color);

	ApplyAerialPerspective(ScreenCoord, surface.P, color);
	
	ApplyFog(dist, V, color);

	return color;
}

