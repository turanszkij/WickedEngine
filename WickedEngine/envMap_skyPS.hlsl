#include "envMapHF.hlsli"
#include "skyHF.hlsli"

float4 main(PSIn_Sky input) : SV_TARGET
{
	return float4(GetSkyColor(input.nor),0);
}