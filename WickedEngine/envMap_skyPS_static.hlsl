#include "envMapHF.hlsli"
#include "skyHF.hlsli"

TEXTURECUBE(texture_sky, float4, TEXSLOT_ONDEMAND0);

float4 main(PSIn_Sky_EnvmapRendering input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
	float3 sky = DEGAMMA(texture_sky.SampleLevel(sampler_linear_clamp, normal, 0).rgb);
	CalculateClouds(sky, normal, false);
	return float4(sky, 1);
}