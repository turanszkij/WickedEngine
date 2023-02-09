#ifndef WI_SHADOW_HF
#define WI_SHADOW_HF
#include "globals.hlsli"

inline float3 sample_shadow(float2 uv, float cmp)
{
	[branch]
	if (GetFrame().texture_shadowatlas_index < 0)
		return 0;

	Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
	float3 shadow = texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp).r;

#ifndef DISABLE_SOFT_SHADOWMAP
	// sample along a rectangle pattern around center:
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(-1, -1)).r;
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(-1, 0)).r;
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(-1, 1)).r;
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(0, -1)).r;
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(0, 1)).r;
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(1, -1)).r;
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(1, 0)).r;
	shadow.x += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(1, 1)).r;
	shadow = shadow.xxx / 9.0;
#endif // DISABLE_SOFT_SHADOWMAP

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	[branch]
	if (GetFrame().options & OPTION_BIT_TRANSPARENTSHADOWS_ENABLED && GetFrame().texture_shadowatlas_transparent_index)
	{
		Texture2D texture_shadowatlas_transparent = bindless_textures[GetFrame().texture_shadowatlas_transparent_index];
		float4 transparent_shadow = texture_shadowatlas_transparent.SampleLevel(sampler_linear_clamp, uv, 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		if (transparent_shadow.a > cmp)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		{
			shadow *= transparent_shadow.rgb;
		}
	}
#endif //DISABLE_TRANSPARENT_SHADOWMAP

	return shadow;
}

// This is used to pull the uvs to the center to avoid sampling on the border and overfiltering into a different shadow
inline void shadow_border_shrink(in ShaderEntity light, inout float2 shadow_uv)
{
	const float2 shadow_resolution = light.shadowAtlasMulAdd.xy * GetFrame().shadow_atlas_resolution;
#ifdef DISABLE_SOFT_SHADOWMAP
	const float border_size = 0.5;
#else
	const float border_size = 1.5;
#endif // DISABLE_SOFT_SHADOWMAP
	shadow_uv *= shadow_resolution - border_size * 2;
	shadow_uv += border_size;
	shadow_uv /= shadow_resolution;
}

inline float3 shadow_2D(in ShaderEntity light, in float3 shadow_pos, in float2 shadow_uv, in uint cascade)
{
	shadow_border_shrink(light, shadow_uv);
	shadow_uv.x += cascade;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, shadow_pos.z);
}

inline float3 shadow_cube(in ShaderEntity light, in float3 Lunnormalized)
{
	const float remapped_distance = light.GetCubemapDepthRemapNear() + light.GetCubemapDepthRemapFar() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid artifact
	const float3 uv_slice = cubemap_to_uv(-Lunnormalized);
	float2 shadow_uv = uv_slice.xy;
	shadow_border_shrink(light, shadow_uv);
	shadow_uv.x += uv_slice.z;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, remapped_distance);
}

inline float shadow_2D_volumetricclouds(float3 P)
{
	// Project into shadow map space (no need to divide by .w because ortho projection!):
	float3 shadow_pos = mul(GetFrame().cloudShadowLightSpaceMatrix, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);

	[branch]
	if (shadow_uv.z < 0.5)
	{
		return 1.0;
	}
	
	[branch]
	if (is_saturated(shadow_uv))
	{
		float cloudShadowSampleZ = shadow_pos.z;

		Texture2D texture_volumetricclouds_shadow = bindless_textures[GetFrame().texture_volumetricclouds_shadow_index];
		float3 cloudShadowData = texture_volumetricclouds_shadow.SampleLevel(sampler_linear_clamp, shadow_uv.xy, 0.0f).rgb;

		float sampleDepthKm = saturate(1.0 - cloudShadowSampleZ) * GetFrame().cloudShadowFarPlaneKm;
		
		float opticalDepth = cloudShadowData.g * (max(0.0f, cloudShadowData.r - sampleDepthKm) * SKY_UNIT_TO_M);
		opticalDepth = min(cloudShadowData.b, opticalDepth);

		float transmittance = saturate(exp(-opticalDepth));
		return transmittance;
	}

	return 1.0;
}

// Sample light and furthest cascade for large mediums (volumetrics)
// Used with SkyAtmosphere and Volumetric Clouds
inline void furthest_cascade_volumetrics(inout ShaderEntity light, inout uint furthestCascade)
{
	for (uint iterator = 0; iterator < GetFrame().lightarray_count; iterator++)
	{
		light = load_entity(GetFrame().lightarray_offset + iterator);
					
		if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC || !light.IsCastingShadow())
		{
			// static lights will be skipped (they are used in lightmap baking)
			// useless if light doesn't cast shadow
			continue;
		}
		
		if (light.GetType() == ENTITY_TYPE_DIRECTIONALLIGHT)
		{
			furthestCascade = light.GetShadowCascadeCount() - 1;
			
			// We've found the first/primary directional light, quit
			break;
		}
	}
}

#endif // WI_SHADOW_HF
