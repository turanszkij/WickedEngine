#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"
#include "gammaHF.hlsli"
#include "toonHF.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	float4 baseColor = g_xMat_diffuseColor;

	PSIn.tex *= g_xMat_texMulAdd.xy;
	PSIn.tex += g_xMat_texMulAdd.zw;
	
	if(g_xMat_hasTex) {
		baseColor *= xTextureTex.Sample(sampler_aniso_wrap,PSIn.tex);
	}
	baseColor.rgb *= PSIn.instanceColor;
	
	ALPHATEST(baseColor.a)

	
	return baseColor*(1 + g_xMat_emissive);
}

