#ifndef _SKY_HF_
#define _SKY_HF_
#include "globals.hlsli"
#include "lightingHF.hlsli"


float3 GetStaticSkyColor(in float3 normal)
{
	return texture_env_global.SampleLevel(sampler_linear_clamp, normal, 0).rgb;
}

float3 GetDynamicSkyColor(in float3 normal)
{
	float aboveHorizon = saturate(pow(saturate(normal.y), 1.0f / 4.0f + g_xWorld_Fog.z) / (g_xWorld_Fog.z + 1));
	float3 sky = lerp(g_xWorld_Horizon, g_xWorld_Zenith, aboveHorizon);
	float3 sun = normal.y > 0 ? max(saturate(dot(GetSunDirection(), normal) > 0.9998 ? 1 : 0)*GetSunColor() * 10000, 0) : 0;
	return sky + sun;
}


#endif // _SKY_HF_
