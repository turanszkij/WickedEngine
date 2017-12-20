#include "envMapHF.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"

float4 main(PSIn_Sky input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
	return float4(GAMMA(GetDynamicSkyColor(normal)),1);
}