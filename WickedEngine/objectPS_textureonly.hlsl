#include "objectHF.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	float4 baseColor = g_xMat_diffuseColor;

	PSIn.tex *= g_xMat_texMulAdd.xy;
	PSIn.tex += g_xMat_texMulAdd.zw;
	
	if(g_xMat_hasTex) {
		baseColor *= xTextureMap.Sample(sampler_aniso_wrap,PSIn.tex);
	}
	baseColor.rgb *= PSIn.instanceColor;
	
	ALPHATEST(baseColor.a)

	
	return baseColor*(1 + g_xMat_emissive);
}

