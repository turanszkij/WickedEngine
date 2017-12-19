#ifndef _SKY_HF_
#define _SKY_HF_
#include "globals.hlsli"
#include "lightingHF.hlsli"

inline float3 GetSkyColor(in float3 normal)
{
	normal = normalize(normal);

	float3 col = DEGAMMA(texture_env_global.SampleLevel(sampler_linear_clamp, normal, 0).rgb);
	float3 sun = max((saturate(dot(GetSunDirection(), normal)) > 0.9998 ? 1 : 0)*GetSunColor(), 0);

	return col + sun;
}


#endif // _SKY_HF_
