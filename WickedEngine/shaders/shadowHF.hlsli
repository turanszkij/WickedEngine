#ifndef WI_SHADOW_HF
#define WI_SHADOW_HF
#include "globals.hlsli"

// Moment shadow mapping things from: https://github.com/TheRealMJP/Shadows
float Linstep(float a, float b, float v)
{
    return saturate((v - a) / (b - a));
}
float ReduceLightBleeding(float pMax, float amount)
{
  // Remove the [0, amount] tail and linearly rescale (amount, 1].
   return Linstep(amount, 1.0f, pMax);
}
float4 ConvertOptimizedMoments(in float4 optimizedMoments)
{
    optimizedMoments[0] -= 0.035955884801f;
    return mul(optimizedMoments, float4x4(0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f,
                                          0.0771972861f, 0.1394629426f, 0.2120202157f, 0.2591432266f,
                                          0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f,
                                          0.0319417555f,-0.1722823173f,-0.2758014811f,-0.3376131734f));
}
float ComputeMSMHamburger(in float4 moments, in float fragmentDepth , in float depthBias, in float momentBias)
{
    // Bias input data to avoid artifacts
    float4 b = lerp(moments, float4(0.5f, 0.5f, 0.5f, 0.5f), momentBias);
    float3 z;
    z[0] = fragmentDepth - depthBias;

    // Compute a Cholesky factorization of the Hankel matrix B storing only non-
    // trivial entries or related products
    float L32D22 = mad(-b[0], b[1], b[2]);
    float D22 = mad(-b[0], b[0], b[1]);
    float squaredDepthVariance = mad(-b[1], b[1], b[3]);
    float D33D22 = dot(float2(squaredDepthVariance, -L32D22), float2(D22, L32D22));
    float InvD22 = 1.0f / D22;
    float L32 = L32D22 * InvD22;

    // Obtain a scaled inverse image of bz = (1,z[0],z[0]*z[0])^T
    float3 c = float3(1.0f, z[0], z[0] * z[0]);

    // Forward substitution to solve L*c1=bz
    c[1] -= b.x;
    c[2] -= b.y + L32 * c[1];

    // Scaling to solve D*c2=c1
    c[1] *= InvD22;
    c[2] *= D22 / D33D22;

    // Backward substitution to solve L^T*c3=c2
    c[1] -= L32 * c[2];
    c[0] -= dot(c.yz, b.xy);

    // Solve the quadratic equation c[0]+c[1]*z+c[2]*z^2 to obtain solutions
    // z[1] and z[2]
    float p = c[1] / c[2];
    float q = c[0] / c[2];
    float D = (p * p * 0.25f) - q;
    float r = sqrt(D);
    z[1] =- p * 0.5f - r;
    z[2] =- p * 0.5f + r;

    // Compute the shadow intensity by summing the appropriate weights
    float4 switchVal = (z[2] < z[0]) ? float4(z[1], z[0], 1.0f, 1.0f) :
                      ((z[1] < z[0]) ? float4(z[0], z[1], 0.0f, 1.0f) :
                      float4(0.0f,0.0f,0.0f,0.0f));
    float quotient = (switchVal[0] * z[2] - b[0] * (switchVal[0] + z[2]) + b[1])/((z[2] - switchVal[1]) * (z[0] - z[1]));
    float shadowIntensity = switchVal[2] + switchVal[3] * quotient;
    return 1.0f - saturate(shadowIntensity);
}

static const float MSMDepthBias = 0.0;
static const float MSMMomentBias = 0.01;
static const float LightBleedingReduction = 0.16;

inline half3 sample_shadow(float2 uv, float cmp)
{
	Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
	
	float4 moments = texture_shadowatlas.SampleLevel(sampler_linear_clamp, uv, 0);
	moments = ConvertOptimizedMoments(moments); // 16-bit moments fix
	float msm = ComputeMSMHamburger(moments, 1 - cmp, MSMDepthBias * 0.001f, MSMMomentBias * 0.001f);
	half3 shadow = ReduceLightBleeding(msm, LightBleedingReduction);
	
#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	Texture2D texture_shadowatlas_transparent = bindless_textures[GetFrame().texture_shadowatlas_transparent_index];
	half4 transparent_shadow = (half4)texture_shadowatlas_transparent.SampleLevel(sampler_linear_clamp, uv, 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
	if (transparent_shadow.a > cmp)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
	{
		shadow *= transparent_shadow.rgb;
	}
#endif //DISABLE_TRANSPARENT_SHADOWMAP

	return shadow;
}

// This is used to clamp the uvs to last texel center to avoid sampling on the border and overfiltering into a different shadow
inline void shadow_border_shrink(in ShaderEntity light, inout float2 shadow_uv)
{
	const float2 shadow_resolution = light.shadowAtlasMulAdd.xy * GetFrame().shadow_atlas_resolution;
	const float border_size = 0.5;
	shadow_uv = clamp(shadow_uv * shadow_resolution, border_size, shadow_resolution - border_size) / shadow_resolution;
}

inline half3 shadow_2D(in ShaderEntity light, in float3 shadow_pos, in float2 shadow_uv, in uint cascade, in uint2 pixel = 0)
{
	shadow_border_shrink(light, shadow_uv);
	shadow_uv.x += cascade;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, shadow_pos.z);
}

inline half3 shadow_cube(in ShaderEntity light, in float3 Lunnormalized)
{
	const float remapped_distance = light.GetCubemapDepthRemapNear() + light.GetCubemapDepthRemapFar() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid artifact
	const float3 uv_slice = cubemap_to_uv(-Lunnormalized);
	float2 shadow_uv = uv_slice.xy;
	shadow_border_shrink(light, shadow_uv);
	shadow_uv.x += uv_slice.z;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, remapped_distance);
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
	if (!any(GetFrame().rain_blocker_mad))
		return false;
		
	float3 shadow_pos = mul(GetFrame().rain_blocker_matrix, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);
	if (is_saturated(shadow_uv))
	{
		shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad.xy, GetFrame().rain_blocker_mad.zw);
		float shadow = sample_shadow(shadow_uv.xy, shadow_pos.z).r;

		if(shadow < 0.5)
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
	if (!any(GetFrame().rain_blocker_mad_prev))
		return false;
		
	float3 shadow_pos = mul(GetFrame().rain_blocker_matrix_prev, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);
	if (is_saturated(shadow_uv))
	{
		shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad_prev.xy, GetFrame().rain_blocker_mad_prev.zw);
		float shadow = sample_shadow(shadow_uv.xy, shadow_pos.z).r;
		
		if(shadow < 0.5)
		{
			return true;
		}
		
	}
	return false;
}

#endif // WI_SHADOW_HF
