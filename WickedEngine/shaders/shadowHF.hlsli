#ifndef WI_SHADOW_HF
#define WI_SHADOW_HF
#include "globals.hlsli"

static const min16uint soft_shadow_sample_count = 4;
static const half soft_shadow_sample_count_rcp = 1.0 / (half)soft_shadow_sample_count;
static const half soft_shadow_sample_count_sqrt_rcp = rsqrt((half)soft_shadow_sample_count);
static const half kGoldenAngle = 2.4;

inline half3 sample_shadow(float2 uv, float cmp, float4 uv_clamping, float spread, uint2 pixel)
{
	[branch]
	if (GetFrame().texture_shadowatlas_index < 0)
		return 1;

	Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
	
#ifdef DISABLE_SOFT_SHADOWMAP
	half3 shadow = (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, clamp(uv, uv_clamping.xy, uv_clamping.zw), cmp).r;
#else
	half3 shadow = 0;
	const float2 spread_offset = GetFrame().shadow_atlas_resolution_rcp.xy * (2 + spread * 8); // remap spread to try to match ray traced shadow result
	const half phi = dither(pixel + GetTemporalAASampleRotation()) * 2 * PI; // per pixel disk rotation
	for (min16uint i = 0; i < soft_shadow_sample_count; ++i)
	{
		// "Vogel disk" sampling pattern based on: https://github.com/corporateshark/poisson-disk-generator/blob/master/PoissonGenerator.h
		const half r = sqrt(half(i) + 0.5) * soft_shadow_sample_count_sqrt_rcp;
		const half theta = i * kGoldenAngle + phi;
		half2 theta_cos_sin;
		sincos(theta, theta_cos_sin.y, theta_cos_sin.x);
		const float2 sample_uv = uv + r * theta_cos_sin * spread_offset;
		shadow.x += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, clamp(sample_uv, uv_clamping.xy, uv_clamping.zw), cmp).r;
	}
	shadow = shadow.xxx * soft_shadow_sample_count_rcp;
#endif // DISABLE_SOFT_SHADOWMAP

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	[branch]
	if (GetFrame().options & OPTION_BIT_TRANSPARENTSHADOWS_ENABLED && GetFrame().texture_shadowatlas_transparent_index)
	{
		Texture2D texture_shadowatlas_transparent = bindless_textures[GetFrame().texture_shadowatlas_transparent_index];
		half4 transparent_shadow = (half4)texture_shadowatlas_transparent.SampleLevel(sampler_linear_clamp, uv, 0);
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

// This is used to clamp the uvs to last texel center to avoid sampling on the border and overfiltering into a different shadow
inline float4 shadow_border_clamp(in ShaderEntity light, in float slice)
{
	const float2 shadow_resolution = light.shadowAtlasMulAdd.xy * GetFrame().shadow_atlas_resolution;
	const float border_size = 0.5 * GetFrame().shadow_atlas_resolution_rcp;
	const float2 topleft = mad(float2(slice, 0), light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw) + border_size;
	const float2 bottomright = mad(float2(slice + 1, 1), light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw) - border_size;
	return float4(topleft, bottomright);
}

inline half3 shadow_2D(in ShaderEntity light, in float3 shadow_pos, in float2 shadow_uv, in uint cascade, uint2 pixel = 0)
{
	shadow_uv.x += cascade;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, shadow_pos.z, shadow_border_clamp(light, cascade), light.GetRadius(), pixel);
}

inline half3 shadow_cube(in ShaderEntity light, in float3 Lunnormalized, uint2 pixel = 0)
{
	const float remapped_distance = light.GetCubemapDepthRemapNear() + light.GetCubemapDepthRemapFar() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid artifact
	const float3 uv_slice = cubemap_to_uv(-Lunnormalized);
	float2 shadow_uv = uv_slice.xy;
	shadow_uv.x += uv_slice.z;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, remapped_distance, shadow_border_clamp(light, uv_slice.z), light.GetRadius(), pixel);
}

inline half shadow_2D_volumetricclouds(float3 P)
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
		return (half)transmittance;
	}

	return 1.0;
}

// Sample light and furthest cascade for large mediums (volumetrics)
// Used with SkyAtmosphere and Volumetric Clouds
inline bool furthest_cascade_volumetrics(inout ShaderEntity light, inout uint furthestCascade)
{
	light = load_entity(GetFrame().lightarray_offset + GetWeather().most_important_light_index);
	furthestCascade = light.GetShadowCascadeCount() - 1;
	
	if ((light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC) == 0 && light.IsCastingShadow() && furthestCascade >= 0)
	{
		// We consider this light useless if it is static, is not casting shadow and if there are no available cascades
		return true;
	}
	
	return false;
}

static const float rain_blocker_head_size = 1;
static const float rain_blocker_head_size_sq = rain_blocker_head_size * rain_blocker_head_size;

// Checks whether position is below or above rain blocker
//	true: below
//	false: above
inline bool rain_blocker_check(in float3 P)
{
	// Before checking blocker shadow map, check "head" blocker:
	if(P.y < GetCamera().position.y + rain_blocker_head_size)
	{
		float2 diff = GetCamera().position.xz - P.xz;
		float distsq = dot(diff, diff);
		if(distsq < rain_blocker_head_size_sq)
			return true;
	}

	[branch]
	if (GetFrame().texture_shadowatlas_index < 0 || !any(GetFrame().rain_blocker_mad))
		return false;
		
	Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
	float3 shadow_pos = mul(GetFrame().rain_blocker_matrix, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);
	if (is_saturated(shadow_uv))
	{
		shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad.xy, GetFrame().rain_blocker_mad.zw);
		float shadow = texture_shadowatlas.SampleLevel(sampler_point_clamp, shadow_uv.xy, 0).r;

		if(shadow > shadow_pos.z)
		{
			return true;
		}
		
	}
	return false;
}

// Same as above but using previous frame's values
inline bool rain_blocker_check_prev(in float3 P)
{
	// Before checking blocker shadow map, check "head" blocker:
	if(P.y < GetCamera().position.y + rain_blocker_head_size)
	{
		float2 diff = GetCamera().position.xz - P.xz;
		float distsq = dot(diff, diff);
		if(distsq < rain_blocker_head_size_sq)
			return true;
	}
		
	[branch]
	if (GetFrame().texture_shadowatlas_index < 0 || !any(GetFrame().rain_blocker_mad_prev))
		return false;
		
	Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
	float3 shadow_pos = mul(GetFrame().rain_blocker_matrix_prev, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);
	if (is_saturated(shadow_uv))
	{
		shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad_prev.xy, GetFrame().rain_blocker_mad_prev.zw);
		float shadow = texture_shadowatlas.SampleLevel(sampler_point_clamp, shadow_uv.xy, 0).r;

		if(shadow > shadow_pos.z)
		{
			return true;
		}
		
	}
	return false;
}

#endif // WI_SHADOW_HF
