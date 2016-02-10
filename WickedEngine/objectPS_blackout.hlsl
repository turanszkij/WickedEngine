#include "objectHF.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	float4 baseColor = float4(0,0,0,0);

	if(g_xMat_hasTex) {
		baseColor = xTextureMap.Sample(sampler_aniso_wrap,PSIn.tex);
	}
	
	ALPHATEST(baseColor.a)

	return float4(0, 0, 0, 1);
}