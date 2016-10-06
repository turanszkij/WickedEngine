#ifndef _LIGHTING_HF_
#define _LIGHTING_HF_
#include "globals.hlsli"
#include "brdf.hlsli"

struct LightArrayType
{
	float3 positionVS; // View Space!
	float range;
	// --
	float4 color;
	// --
	float3 positionWS;
	float energy;
	// --
	float3 directionVS;
	float shadowKernel;
	// --
	float3 directionWS;
	uint type;
	// --
	float shadowBias;
	int shadowMap_index;
	float coneAngle;
	float coneAngleCos;
	// --
	float4x4 shadowMat[3];
};

TEXTURE2D(LightGrid, uint2, TEXSLOT_LIGHTGRID);
STRUCTUREDBUFFER(LightIndexList, uint, SBSLOT_LIGHTINDEXLIST);
STRUCTUREDBUFFER(LightArray, LightArrayType, SBSLOT_LIGHTARRAY);

inline float3 GetSunColor()
{
	return LightArray[g_xFrame_SunLightArrayIndex].color.rgb;
}
inline float3 GetSunDirection()
{
	return LightArray[g_xFrame_SunLightArrayIndex].directionWS;
}

struct LightingResult
{
	float3 diffuse;
	float3 specular;
};

inline float shadowCascade(float4 shadowPos, float2 ShTex, float shadowKernel, float bias, float slice) 
{
	float realDistance = shadowPos.z - bias;
	float sum = 0;
	float retVal = 1; 
#ifdef DIRECTIONALLIGHT_SOFT
	float samples = 0.0f;
	static const float range = 1.5f;
	for (float y = -range; y <= range; y += 1.0f)
	{
		for (float x = -range; x <= range; x += 1.0f)
		{
			sum += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(ShTex + float2(x, y) * shadowKernel, slice), realDistance).r;
			samples++;
		}
	}
	retVal *= sum / samples;
#else
	retVal *= texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(ShTex, slice), realDistance).r;
#endif
	return retVal;
}

inline LightingResult DirectionalLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result;
	float3 lightColor = light.color.rgb*light.energy;

	float3 L = light.directionWS.xyz;
	BRDF_MAKE(N, L, V);
	result.specular = lightColor * BRDF_SPECULAR(roughness, f0);
	result.diffuse = lightColor * BRDF_DIFFUSE(roughness);

	float sh = max(NdotL, 0);

	[branch]
	if (light.shadowMap_index >= 0)
	{
		float4 ShPos[3];
		ShPos[0] = mul(float4(P, 1), light.shadowMat[0]);
		ShPos[1] = mul(float4(P, 1), light.shadowMat[1]);
		ShPos[2] = mul(float4(P, 1), light.shadowMat[2]);
		ShPos[0].xyz /= ShPos[0].w;
		ShPos[1].xyz /= ShPos[1].w;
		ShPos[2].xyz /= ShPos[2].w;
		float3 ShTex[3];
		ShTex[0] = ShPos[0].xyz*float3(1, -1, 1) / 2.0f + 0.5f;
		ShTex[1] = ShPos[1].xyz*float3(1, -1, 1) / 2.0f + 0.5f;
		ShTex[2] = ShPos[2].xyz*float3(1, -1, 1) / 2.0f + 0.5f;
		[branch]if ((saturate(ShTex[2].x) == ShTex[2].x) && (saturate(ShTex[2].y) == ShTex[2].y) && (saturate(ShTex[2].z) == ShTex[2].z))
		{
			const float shadows[] = {
				shadowCascade(ShPos[1],ShTex[1].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 1),
				shadowCascade(ShPos[2],ShTex[2].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 2)
			};
			const float2 lerpVal = abs(ShTex[2].xy * 2 - 1);
			sh *= lerp(shadows[1], shadows[0], pow(max(lerpVal.x, lerpVal.y), 4));
		}
		else[branch]if ((saturate(ShTex[1].x) == ShTex[1].x) && (saturate(ShTex[1].y) == ShTex[1].y) && (saturate(ShTex[1].z) == ShTex[1].z))
		{
			const float shadows[] = {
				shadowCascade(ShPos[0],ShTex[0].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 0),
				shadowCascade(ShPos[1],ShTex[1].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 1),
			};
			const float2 lerpVal = abs(ShTex[1].xy * 2 - 1);
			sh *= lerp(shadows[1], shadows[0], pow(max(lerpVal.x, lerpVal.y), 4));
		}
		else[branch]if ((saturate(ShTex[0].x) == ShTex[0].x) && (saturate(ShTex[0].y) == ShTex[0].y) && (saturate(ShTex[0].z) == ShTex[0].z))
		{
			sh *= shadowCascade(ShPos[0], ShTex[0].xy, light.shadowKernel, light.shadowBias, light.shadowMap_index + 0);
		}
	}

	result.diffuse *= sh;
	result.specular *= sh;

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);
	return result;
}
inline LightingResult PointLight(in LightArrayType light, in float3 L, in float distance, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;
	float3 lightColor = light.color.rgb*light.energy;

	BRDF_MAKE(N, L, V);
	result.specular = lightColor * BRDF_SPECULAR(roughness, f0);
	result.diffuse = lightColor * BRDF_DIFFUSE(roughness);

	float att = (light.energy * (light.range / (light.range + 1 + distance)));
	float attenuation = (att * (light.range - distance) / light.range);
	result.diffuse *= attenuation;
	result.specular *= attenuation;

	float sh = max(NdotL, 0);
	[branch]
	if (light.shadowMap_index >= 0) {
		static const float bias = 0.025;
		sh *= texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.shadowMap_index), distance / light.range - bias).r;
	}
	result.diffuse *= sh;
	result.specular *= sh;

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);
	return result;
}
inline LightingResult SpotLight(in LightArrayType light, in float3 L, in float distance, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;
	float3 lightColor = light.color.rgb*light.energy;

	float SpotFactor = dot(L, light.directionWS);
	float spotCutOff = light.coneAngleCos;

	[branch]
	if (SpotFactor > spotCutOff)
	{

		BRDF_MAKE(N, L, V);
		result.specular = lightColor * BRDF_SPECULAR(roughness, f0);
		result.diffuse = lightColor * BRDF_DIFFUSE(roughness);

		float att = (light.energy * (light.range / (light.range + 1 + distance)));
		float attenuation = (att * (light.range - distance) / light.range);
		attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));
		result.diffuse *= attenuation;
		result.specular *= attenuation;

		float sh = max(NdotL, 0);
		[branch]
		if (light.shadowMap_index >= 0)
		{
			float4 ShPos = mul(float4(P, 1), light.shadowMat[0]);
			ShPos.xyz /= ShPos.w;
			float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
			[branch]
			if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
			{
				sh *= shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.shadowMap_index);
			}
		}
		result.diffuse *= sh;
		result.specular *= sh;

		result.diffuse = max(result.diffuse, 0);
		result.specular = max(result.specular, 0);
	}

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);
	return result;
}

#endif // _LIGHTING_HF_