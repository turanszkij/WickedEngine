#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	float4 baseColor = float4(0.5,0.5,0.5,1);

	baseColor = g_xMat_diffuseColor;
	
	return baseColor;
}

