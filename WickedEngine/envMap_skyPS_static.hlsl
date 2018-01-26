#include "envMapHF.hlsli"
#include "skyHF.hlsli"

float4 main(PSIn_Sky input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
	return float4(DEGAMMA(GetStaticSkyColor(normal)), 1);
}