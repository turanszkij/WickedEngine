#define DISABLE_DECALS
#define DISABLE_ENVMAPS
#define DISABLE_TRANSPARENT_SHADOWMAP
#define TRANSPARENT
#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"
#include "objectHF.hlsli"

Texture2D<float4> texture_gradientmap : register(t1);

[earlydepthstencil]
float4 main(PSIn input) : SV_TARGET
{
	float4 color = xOceanWaterColor;
	float3 V = GetCamera().position - input.pos3D;
	float dist = length(V);
	V /= dist;
	float emissive = 0;

	const float gradient_fade = saturate(dist * 0.001);
	const float4 gradientNear = texture_gradientmap.Sample(sampler_aniso_wrap, input.uv);
	const float4 gradientFar = texture_gradientmap.Sample(sampler_aniso_wrap, input.uv * 0.125);
	const float2 gradient = lerp(gradientNear.rg, gradientFar.rg, gradient_fade);
	const float sss_amount = lerp(gradientNear.a, gradientFar.a, gradient_fade);

	Surface surface;
	surface.init();
	surface.albedo = color.rgb;
	surface.f0 = 0.02;
	surface.roughness = 0.1;
	surface.P = input.pos3D;
	surface.N = normalize(float3(gradient.x, xOceanTexelLength * 2, gradient.y));
	surface.V = V;
	surface.sss = color * sss_amount;
	surface.sss_inv = 1.0f / ((1 + surface.sss) * (1 + surface.sss));
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	surface.pixel = input.pos.xy;
	float depth = input.pos.z;

	TiledLighting(surface, lighting);

	float2 ScreenCoord = surface.pixel * GetCamera().internal_resolution_rcp;

	[branch]
	if (GetCamera().texture_reflection_index >= 0)
	{
		//REFLECTION
		float4 reflectionPos = mul(GetCamera().reflection_view_projection, float4(input.pos3D, 1));
		float2 reflectionUV = clipspace_to_uv(reflectionPos.xy / reflectionPos.w);
		float4 reflectiveColor = bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_mirror, reflectionUV + surface.N.xz * 0.04f, 0);
		lighting.indirect.specular = reflectiveColor.rgb * surface.F;
	}

	[branch]
	if (GetCamera().texture_refraction_index >= 0)
	{
		// WATER REFRACTION 
		Texture2D texture_refraction = bindless_textures[GetCamera().texture_refraction_index];
		float lineardepth = input.pos.w;
		float sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy + surface.N.xz * 0.04f, 0) * GetCamera().z_far;
		float depth_difference = sampled_lineardepth - lineardepth;
		surface.refraction.rgb = texture_refraction.SampleLevel(sampler_linear_mirror, ScreenCoord.xy + surface.N.xz * 0.04f * saturate(0.5 * depth_difference), 0).rgb;
		if (depth_difference < 0)
		{
			// Fix cutoff by taking unperturbed depth diff to fill the holes with fog:
			sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy, 0) * GetCamera().z_far;
			depth_difference = sampled_lineardepth - lineardepth;
		}
		// WATER FOG:
		surface.refraction.a = 1 - saturate(color.a * 0.1f * depth_difference);
	}

	// Blend out at distance:
	color.a = pow(saturate(1 - saturate(dist / GetCamera().z_far)), 2);

	ApplyLighting(surface, lighting, color);

	ApplyFog(dist, GetCamera().position, V, color);

	return color;
}

