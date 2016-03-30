#include "objectHF.hlsli"



float4 main(PixelInputType input) : SV_TARGET
{
	OBJECT_PS_DITHER

	OBJECT_PS_MAKE

	OBJECT_PS_DEGAMMA
	
	return baseColor*(1 + g_xMat_emissive);
}

