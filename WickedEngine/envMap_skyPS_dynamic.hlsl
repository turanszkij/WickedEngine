#include "envMapHF.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"

float4 main(PSIn_Sky_EnvmapRendering input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
	float4 color = float4(GetDynamicSkyColor(normal), 1);
	return float4(color.rgb,1);
}