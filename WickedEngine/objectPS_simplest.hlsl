#include "objectHF.hlsli"




float4 main(PixelInputType PSIn) : SV_TARGET
{
	return g_xMat_baseColor * float4(PSIn.instanceColor,1);
}

