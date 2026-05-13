#define DISABLE_DECALS
#define DISABLE_ENVMAPS
#define DISABLE_TRANSPARENT_SHADOWMAP
#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT
#define WATER
#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"
#include "objectHF.hlsli"

Texture2D<float4> texture_ocean_displacementmap : register(t0);
Texture2D<float4> texture_gradientmap : register(t1);
Texture2D<float4> texture_perlin : register(t2);

[earlydepthstencil]
float4 main(PSIn input) : SV_TARGET
{
	ShaderCamera camera = GetCameraIndexed(input.cameraIndex);
	float lineardepth = camera.IsOrtho() ? ((1 - input.pos.z) * camera.z_far) : input.pos.w;
	half4 color = xOceanWaterColor;
	float2 ScreenCoord = input.pos.xy * camera.internal_resolution_rcp;

	float4 pos2D = mul(camera.view_projection, float4(input.GetPos3D(), 1));
	pos2D.xyz /= pos2D.w;
	if (pos2D.z > OCEAN_NEARPLANE_CUTOFF)
		discard;
	
	float3 V = input.GetViewVector();
	float dist = length(V);
	V /= dist;
	
	uint2 pixel = input.pos.xy;

	half4 gradient = texture_gradientmap.Sample(sampler_aniso_wrap, input.uv);


	const float g_PerlinSize = 1;
	const float2 g_UVBase = 0;
	const float3 g_PerlinOctave = float3(1.12f, 0.59f, 0.23f);
	const float g_PerlinMovement = -GetTime() * xOceanTimeScale * 0.06f;
	const float3 g_PerlinGradient = float3(1.4f, 1.6f, 2.2f);

	float2 perlin_tc = input.uv * g_PerlinSize + g_UVBase;
	float2 perlin_tc0 = perlin_tc * g_PerlinOctave.x + g_PerlinMovement;
	float2 perlin_tc1 = perlin_tc * g_PerlinOctave.y + g_PerlinMovement;
	float2 perlin_tc2 = perlin_tc * g_PerlinOctave.z + g_PerlinMovement;

	float2 perlin_0 = texture_perlin.Sample(sampler_linear_wrap, perlin_tc0).xy;
	float2 perlin_1 = texture_perlin.Sample(sampler_linear_wrap, perlin_tc1).xy;
	float2 perlin_2 = texture_perlin.Sample(sampler_linear_wrap, perlin_tc2).xy;
	
	float2 perlin = (perlin_0 * g_PerlinGradient.x + perlin_1 * g_PerlinGradient.y + perlin_2 * g_PerlinGradient.z);
	//perlin = normalize(perlin);
	perlin = perlin * 0.000005 * xOceanWaveAmplitude;

	const half gradient_fade = smoothstep(0.08, 0.8, saturate(dist * 0.005));
	gradient.rg = lerp(gradient.rg, perlin, gradient_fade);
	//gradient.rg = perlin;
	//return float4(gradient_fade.xxx,1);
	//return float4(gradient.rg, 0, 1);


	[branch]
	if (camera.texture_waterriples_index >= 0)
	{
		gradient.rg += bindless_textures_half4[descriptor_index(camera.texture_waterriples_index)].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).rg * 0.0125;
	}

	// Add some small scale detail waves to make it look less uniform:
	const uint detail_count = 3;
	half2 gradient_detail = 0;
	for (uint i = 0; i < detail_count; ++i)
	{
		gradient_detail += texture_gradientmap.Sample(sampler_aniso_wrap, input.uv * pow(2.0, half(i + 1))).rg;
	}
	gradient_detail /= half(detail_count);
	gradient.rg += gradient_detail;

	const float bump_strength = 0.1;
	
	float4 water_plane = camera.reflection_plane;
	const bool camera_below_water = dot(float4(camera.position, 1), water_plane) < 0; 

	Surface surface;
	surface.init();
	surface.SetReceiveShadow(true);
	surface.pixel = pixel;
	float depth = input.pos.z;
	surface.albedo = color.rgb;
	surface.f0 = 0.02;
	surface.roughness = 0.1;
	surface.P = input.GetPos3D();
	surface.N = normalize(float3(gradient.x, xOceanTexelLength * 2, gradient.y));
	surface.V = V;
	surface.sss = 1;
	surface.sss_inv = 1.0f / ((1 + surface.sss) * (1 + surface.sss));
	surface.extinction = xOceanExtinctionColor.rgb;
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

#ifdef FORWARD
	ForwardLighting(surface, lighting);
#else
	TiledLighting(surface, lighting, GetFlatTileIndex(pixel));
#endif // FORWARD

	if (camera_below_water)
	{
		// Just eyeballing a nice modified transition for refraction-reflection from underwater view:
		const float3 VdotN = abs(dot(surface.V, surface.N));
		surface.F = smoothstep(0.4, 0.6, 1 - VdotN);
	}
	
	[branch]
	if (camera.texture_reflection_index >= 0)
	{
		//REFLECTION
		float4 reflectionPos = mul(camera.reflection_view_projection, float4(surface.P, 1));
		float2 reflectionUV = clipspace_to_uv(reflectionPos.xy / reflectionPos.w) + surface.N.xz * bump_strength;
		half4 reflectiveColor = bindless_textures[descriptor_index(camera.texture_reflection_index)].SampleLevel(sampler_linear_mirror, reflectionUV, 0);
		[branch]
		if(camera.texture_reflection_depth_index >=0)
		{
			float reflectiveDepth = bindless_textures[descriptor_index(camera.texture_reflection_depth_index)].SampleLevel(sampler_point_clamp, reflectionUV, 0).r;
			float3 reflectivePosition = reconstruct_position(reflectionUV, reflectiveDepth, camera.reflection_inverse_view_projection);
			float water_depth = -dot(float4(reflectivePosition, 1), water_plane);
			water_depth += texture_ocean_displacementmap.SampleLevel(sampler_linear_wrap, reflectivePosition.xz * xOceanPatchSizeRecip, 0).z; // texture contains xzy!
			reflectiveColor.rgb = lerp(color.rgb, reflectiveColor.rgb, saturate(exp(-water_depth * color.a)));
		}

		// remove planar reflection at high perturbation where it gets too inaccurate
		const float3 planar_reflection_vector_flat = reflect(V, float3(0, 1, 0));
		const float3 planar_reflection_vector = reflect(V, surface.N);
		const float planar_factor = smoothstep(camera_below_water ? 0.9 : 0.95,1.0,abs(dot(planar_reflection_vector_flat, planar_reflection_vector)));
		//return float4(planar_factor.xxx,1);

		lighting.indirect.specular += reflectiveColor.rgb * surface.F * saturate(dist * 0.1) * planar_factor; // fade out very close to camera, doesn't look good
	}
	else
	{
		lighting.indirect.specular += EnvironmentReflection_Global(surface);
	}

	float water_depth = FLT_MAX;

	[branch]
	if (camera.texture_refraction_index >= 0)
	{
		// Water refraction:
		Texture2D texture_refraction = bindless_textures[descriptor_index(camera.texture_refraction_index)];
		// First sample using full perturbation:
		float2 refraction_uv = ScreenCoord.xy + surface.N.xz * bump_strength;
		float refraction_depth = find_max_depth(refraction_uv, 2, 2);
		float3 refraction_position = reconstruct_position(refraction_uv, refraction_depth);
		water_depth = -dot(float4(refraction_position, 1), water_plane);
		water_depth += texture_ocean_displacementmap.SampleLevel(sampler_linear_wrap, refraction_position.xz * xOceanPatchSizeRecip, 0).z; // texture contains xzy!
		if (camera_below_water && V.y < 0)
			water_depth = -water_depth;
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
		water_depth += texture_ocean_displacementmap.SampleLevel(sampler_linear_wrap, refraction_position.xz * xOceanPatchSizeRecip, 0).z; // texture contains xzy!
		if (camera_below_water && V.y < 0)
			water_depth = -water_depth;
		// Water fog computation:
		float waterfog = saturate(exp(-water_depth * color.a));
		float3 transmittance = saturate(exp(-water_depth * surface.extinction * color.a));
		surface.refraction.a = waterfog;
		surface.refraction.rgb *= transmittance;
		color.a = 1;
	}
	
#if 1
	[branch]
	if (camera.texture_lineardepth_index >= 0)
	{
		// FOAM:
		float water_depth_diff = abs(texture_lineardepth[pixel] * camera.z_far - lineardepth); // Note: for the shore foam, this is more accurate than water plane distance
		float foam_shore = saturate(exp(-water_depth_diff * 2));
		float foam_wave = pow(saturate(gradient.a), 4) * saturate(exp(-water_depth * 0.1));
		float foam_combined = saturate(foam_shore + foam_wave);
		float foam_simplex = 0;
		foam_simplex += smoothstep(0, 0.8, noise_simplex_2D(surface.P.xz * 1 + GetTime()));
		foam_simplex += smoothstep(0, 0.8, noise_simplex_2D(surface.P.xz * 2 + GetTime()));
		foam_simplex += smoothstep(0, 0.8, noise_simplex_2D(surface.P.zx * 4 - GetTime() * 2));
		float foam_voronoi = 0;
		foam_voronoi += smoothstep(0.5, 0.8, noise_voronoi(surface.P.xz * 1, GetTime()).x);
		foam_voronoi += smoothstep(0.5, 0.8, noise_voronoi(surface.P.xz * 2, GetTime()).x);
		foam_voronoi += smoothstep(0.5, 0.8, noise_voronoi(surface.P.xz * 4, GetTime()).x);
		float foam = 0;
		foam += foam_voronoi * foam_simplex * foam_combined;
		foam += smoothstep(0.5, 0.6, saturate(foam_combined + 0.1));
		foam *= 2;
		foam = saturate(foam);
		surface.albedo = lerp(surface.albedo, 0.6, foam);
		surface.refraction.a *= 1 - foam;
		surface.refraction.a = saturate(surface.refraction.a + saturate(exp(-water_depth_diff * 4)));
	}
#endif

	ApplyLighting(surface, lighting, color);
	
	// Blend out at distance:
	float far_fade = saturate(1 - saturate(dist / camera.z_far - 0.8) * 5.0); // fade will be on edge and inwards 20%
	color.a = far_fade;

	ApplyAerialPerspective(ScreenCoord, surface.P, color);
	
	ApplyFog(dist, V, color);

	return saturateMediump(color);
}

