#ifndef WI_LIGHTING_HF
#define WI_LIGHTING_HF
#include "globals.hlsli"
#include "brdf.hlsli"
#include "voxelConeTracingHF.hlsli"
#include "skyHF.hlsli"

#ifdef BRDF_CARTOON
#define DISABLE_SOFT_SHADOWMAP
#endif // BRDF_CARTOON

struct LightingPart
{
	float3 diffuse;
	float3 specular;
};
struct Lighting
{
	LightingPart direct;
	LightingPart indirect;

	inline void create(
		in float3 diffuse_direct,
		in float3 specular_direct,
		in float3 diffuse_indirect,
		in float3 specular_indirect
	)
	{
		direct.diffuse = diffuse_direct;
		direct.specular = specular_direct;
		indirect.diffuse = diffuse_indirect;
		indirect.specular = specular_indirect;
	}
};

// Combine the direct and indirect lighting into final contribution
inline LightingPart CombineLighting(in Surface surface, in Lighting lighting)
{
	LightingPart result;
	result.diffuse = lighting.direct.diffuse + lighting.indirect.diffuse * surface.occlusion;
	result.specular = lighting.direct.specular + lighting.indirect.specular * surface.occlusion;

	return result;
}

inline float3 shadow_2D(in ShaderEntity light, in float3 shadow_pos, in float2 shadow_uv, in uint cascade)
{
	const float slice = light.GetTextureIndex() + cascade;
	const float realDistance = shadow_pos.z; // bias was already applied when shadow map was rendered
	float3 shadow = 0;
#ifndef DISABLE_SOFT_SHADOWMAP
	// sample along a rectangle pattern around center:
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(-1, -1), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(-1, 0), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(-1, 1), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(0, -1), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadow_uv, slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(0, 1), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(1, -1), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(1, 0), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(mad(float2(1, 1), GetFrame().shadow_kernel_2D, shadow_uv), slice), realDistance).r;
	shadow = shadow.xxx / 9.0;
#else
	shadow = texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadow_uv, slice), realDistance).r;
#endif // DISABLE_SOFT_SHADOWMAP

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	if (GetFrame().options & OPTION_BIT_TRANSPARENTSHADOWS_ENABLED)
	{
		float4 transparent_shadow = texture_shadowarray_transparent_2d.SampleLevel(sampler_linear_clamp, float3(shadow_uv, slice), 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		if (transparent_shadow.a > realDistance)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		{
			shadow *= transparent_shadow.rgb;
		}
	}
#endif //DISABLE_TRANSPARENT_SHADOWMAP

	return shadow;
}

inline float3 shadow_cube(in ShaderEntity light, in float3 L, in float3 Lunnormalized)
{
	const float slice = light.GetTextureIndex();
	float remapped_distance = light.GetCubemapDepthRemapNear() + light.GetCubemapDepthRemapFar() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid border sampling artifact
	float3 shadow = 0;
#ifndef DISABLE_SOFT_SHADOWMAP
	// sample along a cube pattern around center:
	L = -L;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(-1, -1, -1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(1, -1, -1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(-1, 1, -1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(1, 1, -1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L, slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(-1, -1, 1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(1, -1, 1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(-1, 1, 1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(mad(float3(1, 1, 1), GetFrame().shadow_kernel_cube, L), slice), remapped_distance).r;
	shadow /= 9.0;
#else
	shadow = texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-Lunnormalized, slice), remapped_distance).r;
#endif // DISABLE_SOFT_SHADOWMAP

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	if (GetFrame().options & OPTION_BIT_TRANSPARENTSHADOWS_ENABLED)
	{
		float4 transparent_shadow = texture_shadowarray_transparent_cube.SampleLevel(sampler_linear_clamp, float4(-Lunnormalized, slice), 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		if (transparent_shadow.a > remapped_distance)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		{
			shadow *= transparent_shadow.rgb;
		}
	}
#endif //DISABLE_TRANSPARENT_SHADOWMAP

	return shadow;
}


inline void light_directional(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1)
{
	float3 L = light.GetDirection();

	SurfaceToLight surface_to_light;
	surface_to_light.create(surface, L);

	[branch]
	if (any(surface_to_light.NdotL_sss))
	{
		float3 shadow = shadow_mask;

		[branch]
		if (light.IsCastingShadow() && surface.IsReceiveShadow())
		{
#ifdef SHADOW_MASK_ENABLED
			[branch]
			if ((GetFrame().options & OPTION_BIT_RAYTRACED_SHADOWS) == 0)
#endif // SHADOW_MASK_ENABLED
			{
				// Loop through cascades from closest (smallest) to furthest (largest)
				[loop]
				for (uint cascade = 0; cascade < GetFrame().shadow_cascade_count; ++cascade)
				{
					// Project into shadow map space (no need to divide by .w because ortho projection!):
					float3 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(surface.P, 1)).xyz;
					float3 shadow_uv = shadow_pos * float3(0.5, -0.5, 0.5) + 0.5;

					// Determine if pixel is inside current cascade bounds and compute shadow if it is:
					[branch]
					if (is_saturated(shadow_uv))
					{
						const float3 shadow_main = shadow_2D(light, shadow_pos, shadow_uv.xy, cascade);
						const float3 cascade_edgefactor = saturate(saturate(abs(shadow_pos)) - 0.8) * 5.0; // fade will be on edge and inwards 20%
						const float cascade_fade = max(cascade_edgefactor.x, max(cascade_edgefactor.y, cascade_edgefactor.z));

						// If we are on cascade edge threshold and not the last cascade, then fallback to a larger cascade:
						[branch]
						if (cascade_fade > 0 && cascade < GetFrame().shadow_cascade_count - 1)
						{
							// Project into next shadow cascade (no need to divide by .w because ortho projection!):
							cascade += 1;
							shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(surface.P, 1)).xyz;
							shadow_uv = shadow_pos * float3(0.5, -0.5, 0.5) + 0.5;
							const float3 shadow_fallback = shadow_2D(light, shadow_pos, shadow_uv.xy, cascade);

							shadow *= lerp(shadow_main, shadow_fallback, cascade_fade);
						}
						else
						{
							shadow *= shadow_main;
						}
						break;
					}
				}
			}
		}

		[branch]
		if (any(shadow))
		{
			float3 light_color = light.GetColor().rgb * light.GetEnergy() * shadow;

			[branch]
			if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
			{
				light_color *= GetAtmosphericLightTransmittance(GetWeather().atmosphere, surface.P, L, texture_transmittancelut);
			}

			lighting.direct.diffuse = mad(light_color, BRDF_GetDiffuse(surface, surface_to_light), lighting.direct.diffuse);
			lighting.direct.specular = mad(light_color, BRDF_GetSpecular(surface, surface_to_light), lighting.direct.specular);
		}
	}
}
inline void light_point(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1)
{
	float3 L = light.position - surface.P;
	const float dist2 = dot(L, L);
	const float range2 = light.GetRange() * light.GetRange();

	[branch]
	if (dist2 < range2)
	{
		const float3 Lunnormalized = L;
		const float dist = sqrt(dist2);
		L /= dist;

		SurfaceToLight surface_to_light;
		surface_to_light.create(surface, L);

		[branch]
		if (any(surface_to_light.NdotL_sss))
		{
			float3 shadow = shadow_mask;

			[branch]
			if (light.IsCastingShadow() && surface.IsReceiveShadow())
			{
#ifdef SHADOW_MASK_ENABLED
				[branch]
				if ((GetFrame().options & OPTION_BIT_RAYTRACED_SHADOWS) == 0)
#endif // SHADOW_MASK_ENABLED
				{
					shadow *= shadow_cube(light, L, Lunnormalized);
				}
			}

			[branch]
			if (any(shadow))
			{
				float3 light_color = light.GetColor().rgb * light.GetEnergy() * shadow;

				const float att = saturate(1 - (dist2 / range2));
				const float attenuation = att * att;
				light_color *= attenuation;

				lighting.direct.diffuse = mad(light_color, BRDF_GetDiffuse(surface, surface_to_light), lighting.direct.diffuse);
				lighting.direct.specular = mad(light_color, BRDF_GetSpecular(surface, surface_to_light), lighting.direct.specular);
			}
		}
	}
}
inline void light_spot(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1)
{
	float3 L = light.position - surface.P;
	const float dist2 = dot(L, L);
	const float range2 = light.GetRange() * light.GetRange();

	[branch]
	if (dist2 < range2)
	{
		const float dist = sqrt(dist2);
		L /= dist;

		SurfaceToLight surface_to_light;
		surface_to_light.create(surface, L);

		[branch]
		if (any(surface_to_light.NdotL_sss))
		{
			const float spot_factor = dot(L, light.GetDirection());
			const float spot_cutoff = light.GetConeAngleCos();

			[branch]
			if (spot_factor > spot_cutoff)
			{
				float3 shadow = shadow_mask;

				[branch]
				if (light.IsCastingShadow() && surface.IsReceiveShadow())
				{
#ifdef SHADOW_MASK_ENABLED
					[branch]
					if ((GetFrame().options & OPTION_BIT_RAYTRACED_SHADOWS) == 0)
#endif // SHADOW_MASK_ENABLED
					{
						float4 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + 0), float4(surface.P, 1));
						shadow_pos.xyz /= shadow_pos.w;
						float2 shadow_uv = shadow_pos.xy * float2(0.5, -0.5) + 0.5;
						[branch]
						if (is_saturated(shadow_uv))
						{
							shadow *= shadow_2D(light, shadow_pos.xyz, shadow_uv.xy, 0);
						}
					}
				}

				[branch]
				if (any(shadow))
				{
					float3 light_color = light.GetColor().rgb * light.GetEnergy() * shadow;

					const float att = saturate(1 - (dist2 / range2));
					float attenuation = att * att;
					attenuation *= saturate((1 - (1 - spot_factor) * 1 / (1 - spot_cutoff)));
					light_color *= attenuation;

					lighting.direct.diffuse = mad(light_color, BRDF_GetDiffuse(surface, surface_to_light), lighting.direct.diffuse);
					lighting.direct.specular = mad(light_color, BRDF_GetSpecular(surface, surface_to_light), lighting.direct.specular);
				}
			}
		}
	}
}


// ENVIRONMENT MAPS


inline float3 GetAmbient(in float3 N)
{
	float3 ambient;

#ifdef ENVMAPRENDERING

	// Set realistic_sky_stationary to true so we capture ambient at float3(0.0, 0.0, 0.0), similar to the standard sky to avoid flickering and weird behavior
	ambient = lerp(
		GetDynamicSkyColor(float3(0, -1, 0), false, false, false, true),
		GetDynamicSkyColor(float3(0, 1, 0), false, false, false, true),
		saturate(N.y * 0.5 + 0.5));

#else

	ambient = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(N, 0), GetFrame().envprobe_mipcount).rgb;

#endif // ENVMAPRENDERING

	// This is not entirely correct if we have probes, because it shouldn't be added twice.
	//	However, it is not correct if we leave it out from probes, because if we render a scene
	//	with dark sky but ambient, we still want some visible result.
	ambient += GetAmbientColor();

	return ambient;
}

// surface:				surface descriptor
// MIP:					mip level to sample
// return:				color of the environment color (rgb)
inline float3 EnvironmentReflection_Global(in Surface surface)
{
	float3 envColor;

#ifdef ENVMAPRENDERING

	// There is no access to envmaps, so approximate sky color:
	// Set realistic_sky_stationary to true so we capture environment at float3(0.0, 0.0, 0.0), similar to the standard sky to avoid flickering and weird behavior
	float3 skycolor_real = GetDynamicSkyColor(surface.R, false, false, false, true); // false: disable sun disk and clouds
	float3 skycolor_rough = lerp(
		GetDynamicSkyColor(float3(0, -1, 0), false, false, false, true),
		GetDynamicSkyColor(float3(0, 1, 0), false, false, false, true),
		saturate(surface.R.y * 0.5 + 0.5));

	envColor = lerp(skycolor_real, skycolor_rough, saturate(surface.roughness)) * surface.F;

#else

	float MIP = surface.roughness * GetFrame().envprobe_mipcount;
	envColor = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(surface.R, 0), MIP).rgb * surface.F;

#ifdef BRDF_SHEEN
	envColor *= surface.sheen.albedoScaling;
	MIP = surface.sheen.roughness * GetFrame().envprobe_mipcount;
	envColor += texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(surface.R, 0), MIP).rgb * surface.sheen.color * surface.sheen.DFG;
#endif // BRDF_SHEEN

#ifdef BRDF_CLEARCOAT
	envColor *= 1 - surface.clearcoat.F;
	MIP = surface.clearcoat.roughness * GetFrame().envprobe_mipcount;
	envColor += texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(surface.clearcoat.R, 0), MIP).rgb * surface.clearcoat.F;
#endif // BRDF_CLEARCOAT

#endif // ENVMAPRENDERING

	return envColor;
}

// surface:				surface descriptor
// probe :				the shader entity holding properties
// probeProjection:		the inverse OBB transform matrix
// clipSpacePos:		world space pixel position transformed into OBB space by probeProjection matrix
// MIP:					mip level to sample
// return:				color of the environment map (rgb), blend factor of the environment map (a)
inline float4 EnvironmentReflection_Local(in Surface surface, in ShaderEntity probe, in float4x4 probeProjection, in float3 clipSpacePos)
{
	// Perform parallax correction of reflection ray (R) into OBB:
	float3 RayLS = mul((float3x3)probeProjection, surface.R);
	float3 FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	float3 IntersectPositionWS = surface.P + surface.R * Distance;
	float3 R_parallaxCorrected = IntersectPositionWS - probe.position;

	// Sample cubemap texture:
	float MIP = surface.roughness * GetFrame().envprobe_mipcount;
	float3 envColor = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(R_parallaxCorrected, probe.GetTextureIndex()), MIP).rgb * surface.F;

#ifdef BRDF_SHEEN
	envColor *= surface.sheen.albedoScaling;
	MIP = surface.sheen.roughness * GetFrame().envprobe_mipcount;
	envColor += texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(R_parallaxCorrected, probe.GetTextureIndex()), MIP).rgb * surface.sheen.color * surface.sheen.DFG;
#endif // BRDF_SHEEN

#ifdef BRDF_CLEARCOAT
	RayLS = mul((float3x3)probeProjection, surface.clearcoat.R);
	FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	IntersectPositionWS = surface.P + surface.clearcoat.R * Distance;
	R_parallaxCorrected = IntersectPositionWS - probe.position;

	envColor *= 1 - surface.clearcoat.F;
	MIP = surface.clearcoat.roughness * GetFrame().envprobe_mipcount;
	envColor += texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(R_parallaxCorrected, probe.GetTextureIndex()), MIP).rgb * surface.clearcoat.F;
#endif // BRDF_CLEARCOAT

	// blend out if close to any cube edge:
	float edgeBlend = 1 - pow(saturate(max(abs(clipSpacePos.x), max(abs(clipSpacePos.y), abs(clipSpacePos.z)))), 8);

	return float4(envColor, edgeBlend);
}



// VOXEL RADIANCE

inline void VoxelGI(in Surface surface, inout Lighting lighting)
{
	[branch] if (GetFrame().voxelradiance_resolution != 0)
	{
		// determine blending factor (we will blend out voxel GI on grid edges):
		float3 voxelSpacePos = surface.P - GetFrame().voxelradiance_center;
		voxelSpacePos *= GetFrame().voxelradiance_size_rcp;
		voxelSpacePos *= GetFrame().voxelradiance_resolution_rcp;
		voxelSpacePos = saturate(abs(voxelSpacePos));
		float blend = 1 - pow(max(voxelSpacePos.x, max(voxelSpacePos.y, voxelSpacePos.z)), 4);

		// diffuse:
		{
			float4 trace = ConeTraceDiffuse(texture_voxelgi, surface.P, surface.N);
			lighting.indirect.diffuse = lerp(lighting.indirect.diffuse, trace.rgb, trace.a * blend);
		}

		// specular:
		[branch]
		if (GetFrame().options & OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED)
		{
			float4 trace = ConeTraceSpecular(texture_voxelgi, surface.P, surface.N, surface.V, surface.roughness);
			lighting.indirect.specular = lerp(lighting.indirect.specular, trace.rgb * surface.F, trace.a * blend);
		}
	}
}

#endif // WI_LIGHTING_HF
