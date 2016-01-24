#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	return g_xMat_diffuseColor;
}

