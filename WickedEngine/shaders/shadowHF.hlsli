#ifndef WI_SHADOW_HF
#define WI_SHADOW_HF
#include "globals.hlsli"

static const float exponential_shadow_bias = 60;

inline half3 sample_shadow(float2 uv, float cmp)
{
	Texture2D<float> texture_shadowatlas = bindless_textures_float[descriptor_index(GetFrame().texture_shadowatlas_filtered_index)];
	float shadowMapValue = texture_shadowatlas.SampleLevel(sampler_linear_clamp, uv, 0);
	half3 shadow = sqr((half)saturate(shadowMapValue * exp(-exponential_shadow_bias * cmp)));
		
#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	Texture2D<half4> texture_shadowatlas_transparent = bindless_textures_half4[descriptor_index(GetFrame().texture_shadowatlas_transparent_index)];
	half4 transparent_shadow = texture_shadowatlas_transparent.SampleLevel(sampler_linear_clamp, uv, 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
	if (transparent_shadow.a < cmp)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
	{
		shadow *= transparent_shadow.rgb;
	}
#endif // DISABLE_TRANSPARENT_SHADOWMAP

	return shadow;
}

// This is used to clamp the uvs to last texel center to avoid sampling on the border and overfiltering into a different shadow
inline void shadow_border_clamp(in ShaderEntity light, in float slice, inout float2 uv)
{
	const float border_size = 0.75 * GetFrame().shadow_atlas_resolution_rcp;
	const float2 topleft = mad(float2(slice, 0), light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw) + border_size;
	const float2 bottomright = mad(float2(slice + 1, 1), light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw) - border_size;
	uv = clamp(uv, topleft, bottomright);
}

inline half3 shadow_2D(in ShaderEntity light, in float z, in float2 shadow_uv, in uint cascade)
{
	shadow_uv.x += cascade;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	shadow_border_clamp(light, cascade, shadow_uv);
	return sample_shadow(shadow_uv, z);
}

inline half3 shadow_cube(in ShaderEntity light, in float3 Lunnormalized)
{
	const float3 uv_slice = cubemap_to_uv(-Lunnormalized);
	float2 shadow_uv = uv_slice.xy;
	shadow_uv.x += uv_slice.z;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	shadow_border_clamp(light, uv_slice.z, shadow_uv);
	return sample_shadow(shadow_uv, length(Lunnormalized) / light.GetRange());
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

		Texture2D<half4> texture_volumetricclouds_shadow = bindless_textures_half4[descriptor_index(GetFrame().texture_volumetricclouds_shadow_index)];
		half3 cloudShadowData = texture_volumetricclouds_shadow.SampleLevel(sampler_linear_clamp, shadow_uv.xy, 0.0).rgb;

		half sampleDepthKm = saturate(1.0 - cloudShadowSampleZ) * GetFrame().cloudShadowFarPlaneKm;
		
		half opticalDepth = cloudShadowData.g * (max(0.0, cloudShadowData.r - sampleDepthKm) * SKY_UNIT_TO_M);
		opticalDepth = min(cloudShadowData.b, opticalDepth);

		half transmittance = saturate(exp(-opticalDepth));
		return transmittance;
	}

	return 1.0;
}

// Sample light and furthest cascade for large mediums (volumetrics)
// Used with SkyAtmosphere and Volumetric Clouds
inline bool furthest_cascade_volumetrics(inout ShaderEntity light, inout uint furthestCascade)
{
	light = load_entity(lights().first_item() + GetWeather().most_important_light_index);
	furthestCascade = light.GetShadowCascadeCount() - 1;
	
	if (!light.IsStaticLight() && light.IsCastingShadow() && furthestCascade >= 0)
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
	// Check ocean blocker:
	const ShaderOcean ocean = GetWeather().ocean;
	if (ocean.texture_displacementmap >= 0)
	{
		Texture2D displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
		float2 ocean_uv = P.xz * ocean.patch_size_rcp;
		float3 displacement = displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
		float water_height = ocean.water_height + displacement.y;
		if (P.y < water_height)
		{
			return true;
		}
	}

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
		
	Texture2D texture_shadowatlas = bindless_textures[descriptor_index(GetFrame().texture_shadowatlas_index)];
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
	// Check ocean blocker:
	const ShaderOcean ocean = GetWeather().ocean;
	if (ocean.texture_displacementmap >= 0)
	{
		Texture2D displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
		float2 ocean_uv = P.xz * ocean.patch_size_rcp;
		float3 displacement = displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
		float water_height = ocean.water_height + displacement.y;
		if (P.y < water_height)
		{
			return true;
		}
	}
	
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
		
	Texture2D texture_shadowatlas = bindless_textures[descriptor_index(GetFrame().texture_shadowatlas_index)];
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
