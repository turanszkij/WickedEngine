#ifndef WI_LIGHTING_HF
#define WI_LIGHTING_HF
#include "globals.hlsli"
#include "shadowHF.hlsli"
#include "brdf.hlsli"
#include "voxelConeTracingHF.hlsli"
#include "skyHF.hlsli"

#ifdef CARTOON
#define DISABLE_SOFT_SHADOWMAP
#endif // CARTOON

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

inline void ApplyLighting(in Surface surface, in Lighting lighting, inout float4 color)
{
	float3 diffuse = lighting.direct.diffuse / PI + lighting.indirect.diffuse * GetFrame().gi_boost * (1 - surface.F) * surface.occlusion;
	float3 specular = lighting.direct.specular + lighting.indirect.specular * surface.occlusion; // reminder: cannot apply surface.F for whole indirect specular, because multiple layers have separate fresnels (sheen, clearcoat)
	color.rgb = lerp(surface.albedo * diffuse, surface.refraction.rgb, surface.refraction.a);
	color.rgb += specular;
	color.rgb += surface.emissiveColor;
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
			if (GetFrame().options & OPTION_BIT_VOLUMETRICCLOUDS_CAST_SHADOW)
			{
				shadow *= shadow_2D_volumetricclouds(surface.P);
			}

#if defined(SHADOW_MASK_ENABLED) && !defined(TRANSPARENT)
			[branch]
			if ((GetFrame().options & OPTION_BIT_RAYTRACED_SHADOWS) == 0 || GetCamera().texture_rtshadow_index < 0)
#endif // SHADOW_MASK_ENABLED
			{
				// Loop through cascades from closest (smallest) to furthest (largest)
				[loop]
				for (uint cascade = 0; cascade < light.GetShadowCascadeCount(); ++cascade)
				{
					// Project into shadow map space (no need to divide by .w because ortho projection!):
					float3 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(surface.P, 1)).xyz;
					float3 shadow_uv = clipspace_to_uv(shadow_pos);

					// Determine if pixel is inside current cascade bounds and compute shadow if it is:
					[branch]
					if (is_saturated(shadow_uv))
					{
						const float3 shadow_main = shadow_2D(light, shadow_pos, shadow_uv.xy, cascade);
						const float3 cascade_edgefactor = saturate(saturate(abs(shadow_pos)) - 0.8) * 5.0; // fade will be on edge and inwards 20%
						const float cascade_fade = max(cascade_edgefactor.x, max(cascade_edgefactor.y, cascade_edgefactor.z));

						// If we are on cascade edge threshold and not the last cascade, then fallback to a larger cascade:
						[branch]
						if (cascade_fade > 0 && cascade < light.GetShadowCascadeCount() - 1)
						{
							// Project into next shadow cascade (no need to divide by .w because ortho projection!):
							cascade += 1;
							shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(surface.P, 1)).xyz;
							shadow_uv = clipspace_to_uv(shadow_pos);
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
			float3 light_color = light.GetColor().rgb * shadow;

			[branch]
			if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
			{
				light_color *= GetAtmosphericLightTransmittance(GetWeather().atmosphere, surface.P, L, texture_transmittancelut);
			}

			lighting.direct.diffuse = mad(light_color, BRDF_GetDiffuse(surface, surface_to_light), lighting.direct.diffuse);
			lighting.direct.specular = mad(light_color, BRDF_GetSpecular(surface, surface_to_light), lighting.direct.specular);

#ifndef WATER
			const ShaderOcean ocean = GetWeather().ocean;
			if (ocean.texture_displacementmap >= 0)
			{
				Texture2D displacementmap = bindless_textures[ocean.texture_displacementmap];
				float2 ocean_uv = surface.P.xz * ocean.patch_size_rcp;
				float3 displacement = displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
				float water_height = ocean.water_height + displacement.y;
				if (surface.P.y < water_height)
				{
					float3 caustic = caustic_pattern(ocean_uv * 20, GetFrame().time);
					caustic *= sqr(saturate((water_height - surface.P.y) * 0.5)); // fade out at shoreline
					caustic *= light_color;
					lighting.indirect.diffuse += caustic;
				}
			}
#endif // WATER

		}
	}
}

inline float attenuation_pointlight(in float dist, in float dist2, in float range, in float range2)
{
	// GLTF recommendation: https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#range-property
	//return saturate(1 - pow(dist / range, 4)) / dist2;

	// Removed pow(x, 4), and avoid zero divisions:
	float dist_per_range = dist2 / max(0.0001, range2); // pow2
	dist_per_range *= dist_per_range; // pow4
	return saturate(1 - dist_per_range) / max(0.0001, dist2);
}
inline void light_point(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1)
{
	float3 L = light.position - surface.P;

#ifndef DISABLE_AREA_LIGHTS
	if (light.GetLength() > 0)
	{
		// Diffuse representative point on line:
		const float3 line_point = closest_point_on_segment(
			light.position - light.GetDirection() * light.GetLength() * 0.5,
			light.position + light.GetDirection() * light.GetLength() * 0.5,
			surface.P
		);
		L = line_point - surface.P;
	}
#endif // DISABLE_AREA_LIGHTS

	const float dist2 = dot(L, L);
	const float range = light.GetRange();
	const float range2 = range * range;

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
			float3 shadow = shadow_mask;

			[branch]
			if (light.IsCastingShadow() && surface.IsReceiveShadow())
			{
#if defined(SHADOW_MASK_ENABLED) && !defined(TRANSPARENT)
				[branch]
				if ((GetFrame().options & OPTION_BIT_RAYTRACED_SHADOWS) == 0 || GetCamera().texture_rtshadow_index < 0)
#endif // SHADOW_MASK_ENABLED
				{
					shadow *= shadow_cube(light, light.position - surface.P);
				}
			}

			[branch]
			if (any(shadow))
			{
				float3 light_color = light.GetColor().rgb * shadow;
				light_color *= attenuation_pointlight(dist, dist2, range, range2);

				lighting.direct.diffuse = mad(light_color, BRDF_GetDiffuse(surface, surface_to_light), lighting.direct.diffuse);

#ifndef DISABLE_AREA_LIGHTS
				if (light.GetLength() > 0)
				{
					// Specular representative point on line:
					float3 P0 = light.position - light.GetDirection() * light.GetLength() * 0.5;
					float3 P1 = light.position + light.GetDirection() * light.GetLength() * 0.5;
					float3 L0 = P0 - surface.P;
					float3 L1 = P1 - surface.P;
					float3 Ld = L1 - L0;
					float RdotLd = dot(surface.R, Ld);
					float t = dot(surface.R, L0) * RdotLd - dot(L0, Ld);
					t /= dot(Ld, Ld) - RdotLd * RdotLd;
					L = (L0 + saturate(t) * Ld);
				}
				else
				{
					L = light.position - surface.P;
				}
				if(light.GetRadius() > 0)
				{
					// Specular representative point on sphere:
					float3 centerToRay = mad(dot(L, surface.R), surface.R, -L);
					L = mad(centerToRay, saturate(light.GetRadius() / length(centerToRay)), L);
					// Energy conservation for radius:
					light_color /= max(1, sphere_volume(light.GetRadius()));
				}
				if (light.GetLength() > 0 || light.GetRadius() > 0)
				{
					L = normalize(L);
					surface_to_light.create(surface, L); // recompute all surface-light vectors
				}
#endif // DISABLE_AREA_LIGHTS

				lighting.direct.specular = mad(light_color, BRDF_GetSpecular(surface, surface_to_light), lighting.direct.specular);

			}
		}
	}
}

inline float attenuation_spotlight(in float dist, in float dist2, in float range, in float range2, in float spot_factor, in float angle_scale, in float angle_offset)
{
	float attenuation = attenuation_pointlight(dist, dist2, range, range2);
	float angularAttenuation = saturate(mad(spot_factor, angle_scale, angle_offset));
	angularAttenuation *= angularAttenuation;
	attenuation *= angularAttenuation;
	return attenuation;
}
inline void light_spot(in ShaderEntity light, in Surface surface, inout Lighting lighting, in float shadow_mask = 1)
{
	float3 L = light.position - surface.P;
	const float dist2 = dot(L, L);
	const float range = light.GetRange();
	const float range2 = range * range;

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
#if defined(SHADOW_MASK_ENABLED) && !defined(TRANSPARENT)
					[branch]
					if ((GetFrame().options & OPTION_BIT_RAYTRACED_SHADOWS) == 0 || GetCamera().texture_rtshadow_index < 0)
#endif // SHADOW_MASK_ENABLED
					{
						float4 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + 0), float4(surface.P, 1));
						shadow_pos.xyz /= shadow_pos.w;
						float2 shadow_uv = clipspace_to_uv(shadow_pos.xy);
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
					float3 light_color = light.GetColor().rgb * shadow;
					light_color *= attenuation_spotlight(dist, dist2, range, range2, spot_factor, light.GetAngleScale(), light.GetAngleOffset());

					lighting.direct.diffuse = mad(light_color, BRDF_GetDiffuse(surface, surface_to_light), lighting.direct.diffuse);

#ifndef DISABLE_AREA_LIGHTS
					if (light.GetRadius() > 0)
					{
						// Specular representative point on sphere:
						L = light.position - surface.P;
						float3 centerToRay = mad(dot(L, surface.R), surface.R, -L);
						L = mad(centerToRay, saturate(light.GetRadius() / length(centerToRay)), L);
						L = normalize(L);
						surface_to_light.create(surface, L); // recompute all surface-light vectors
						// Energy conservation for radius:
						light_color /= max(1, sphere_volume(light.GetRadius()));
					}
#endif // DISABLE_AREA_LIGHTS

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
		GetDynamicSkyColor(float3(0, -1, 0), false, false, true),
		GetDynamicSkyColor(float3(0, 1, 0), false, false, true),
		saturate(N.y * 0.5 + 0.5));

#else

	[branch]
	if (GetScene().globalprobe >= 0)
	{
		TextureCube cubemap = bindless_cubemaps[GetScene().globalprobe];
		uint2 dim;
		uint mips;
		cubemap.GetDimensions(0, dim.x, dim.y, mips);
		ambient = cubemap.SampleLevel(sampler_linear_clamp, N, mips).rgb;
	}
	
#endif // ENVMAPRENDERING

#ifndef NO_FLAT_AMBIENT
	// This is not entirely correct if we have probes, because it shouldn't be added twice.
	//	However, it is not correct if we leave it out from probes, because if we render a scene
	//	with dark sky but ambient, we still want some visible result.
	ambient += GetAmbientColor();
#endif // NO_FLAT_AMBIENT

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
	float3 skycolor_real = GetDynamicSkyColor(surface.R, false, false, true); // false: disable sun disk and clouds
	float3 skycolor_rough = lerp(
		GetDynamicSkyColor(float3(0, -1, 0), false, false, true),
		GetDynamicSkyColor(float3(0, 1, 0), false, false, true),
		saturate(surface.R.y * 0.5 + 0.5));

	envColor = lerp(skycolor_real, skycolor_rough, surface.roughness) * surface.F;

#else
	
	[branch]
	if (GetScene().globalprobe < 0)
		return 0;
	
	TextureCube cubemap = bindless_cubemaps[GetScene().globalprobe];
	uint2 dim;
	uint mips;
	cubemap.GetDimensions(0, dim.x, dim.y, mips);

	float MIP = surface.roughness * mips;
	envColor = cubemap.SampleLevel(sampler_linear_clamp, surface.R, MIP).rgb * surface.F;

#ifdef SHEEN
	envColor *= surface.sheen.albedoScaling;
	MIP = surface.sheen.roughness * mips;
	envColor += cubemap.SampleLevel(sampler_linear_clamp, surface.R, MIP).rgb * surface.sheen.color * surface.sheen.DFG;
#endif // SHEEN

#ifdef CLEARCOAT
	envColor *= 1 - surface.clearcoat.F;
	MIP = surface.clearcoat.roughness * mips;
	envColor += cubemap.SampleLevel(sampler_linear_clamp, surface.clearcoat.R, MIP).rgb * surface.clearcoat.F;
#endif // CLEARCOAT

#endif // ENVMAPRENDERING

	return envColor;
}

// surface:				surface descriptor
// probe :				the shader entity holding properties
// probeProjection:		the inverse OBB transform matrix
// clipSpacePos:		world space pixel position transformed into OBB space by probeProjection matrix
// MIP:					mip level to sample
// return:				color of the environment map (rgb), blend factor of the environment map (a)
inline float4 EnvironmentReflection_Local(int textureIndex, in Surface surface, in ShaderEntity probe, in float4x4 probeProjection, in float3 clipSpacePos)
{
	[branch]
	if (GetScene().globalprobe < 0)
		return 0;
	
	// Perform parallax correction of reflection ray (R) into OBB:
	float3 RayLS = mul((float3x3)probeProjection, surface.R);
	float3 FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	float3 IntersectPositionWS = surface.P + surface.R * Distance;
	float3 R_parallaxCorrected = IntersectPositionWS - probe.position;

	TextureCube cubemap = bindless_cubemaps[NonUniformResourceIndex(textureIndex)];
	uint2 dim;
	uint mips;
	cubemap.GetDimensions(0, dim.x, dim.y, mips);

	// Sample cubemap texture:
	float MIP = surface.roughness * mips;
	float3 envColor = cubemap.SampleLevel(sampler_linear_clamp, R_parallaxCorrected, MIP).rgb * surface.F;

#ifdef SHEEN
	envColor *= surface.sheen.albedoScaling;
	MIP = surface.sheen.roughness * mips;
	envColor += cubemap.SampleLevel(sampler_linear_clamp, R_parallaxCorrected, MIP).rgb * surface.sheen.color * surface.sheen.DFG;
#endif // SHEEN

#ifdef CLEARCOAT
	RayLS = mul((float3x3)probeProjection, surface.clearcoat.R);
	FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	IntersectPositionWS = surface.P + surface.clearcoat.R * Distance;
	R_parallaxCorrected = IntersectPositionWS - probe.position;

	envColor *= 1 - surface.clearcoat.F;
	MIP = surface.clearcoat.roughness * mips;
	envColor += cubemap.SampleLevel(sampler_linear_clamp, R_parallaxCorrected, MIP).rgb * surface.clearcoat.F;
#endif // CLEARCOAT

	// blend out if close to any cube edge:
	float edgeBlend = 1 - pow(saturate(max(abs(clipSpacePos.x), max(abs(clipSpacePos.y), abs(clipSpacePos.z)))), 8);

	return float4(envColor, edgeBlend);
}



// VOXEL RADIANCE

inline void VoxelGI(inout Surface surface, inout Lighting lighting)
{
	[branch]
	if (GetFrame().vxgi.resolution != 0 && GetFrame().vxgi.texture_radiance >= 0)
	{
		Texture3D<float4> voxels = bindless_textures3D[GetFrame().vxgi.texture_radiance];

		// diffuse:
		float4 trace = ConeTraceDiffuse(voxels, surface.P, surface.N);
		lighting.indirect.diffuse = mad(lighting.indirect.diffuse, 1 - trace.a, trace.rgb);

		// specular:
		[branch]
		if (GetFrame().options & OPTION_BIT_VXGI_REFLECTIONS_ENABLED)
		{
			float roughnessBRDF = sqr(clamp(surface.roughness, 0.045, 1));
			float4 trace = ConeTraceSpecular(voxels, surface.P, surface.N, surface.V, roughnessBRDF, surface.pixel);
			lighting.indirect.specular = mad(lighting.indirect.specular, 1 - trace.a, trace.rgb * surface.F);
		}
	}
}

#endif // WI_LIGHTING_HF
