#include "envMapHF.hlsli"

#define DIRECTIONALLIGHT_SOFT
#include "dirLightHF.hlsli"

float4 main(PSIn input) : SV_TARGET
{
	float4 baseColor = g_xMat_diffuseColor;

	input.tex *= g_xMat_texMulAdd.xy;
	input.tex += g_xMat_texMulAdd.zw;
	
	if (g_xMat_hasTex) {
		baseColor *= texture_0.Sample(sampler_aniso_wrap, input.tex.xy);
	}
	baseColor.rgb *= input.instanceColor;

	//float lighting = dirLight(input.pos3D, input.nor, baseColor);
	//lighting = saturate(dot(input.nor.xyz, g_xDirLight_direction.xyz));
	//baseColor.rgb *= lighting;

	ALPHATEST(baseColor.a)

	return baseColor*(1 + g_xMat_emissive);
}