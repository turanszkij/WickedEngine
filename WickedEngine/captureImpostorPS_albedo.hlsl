#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float4 color = xBaseColorMap.Sample(sampler_objectshader, UV);
	color.rgb = DEGAMMA(color.rgb);
	color *= g_xMat_baseColor;
	ALPHATEST(color.a);
	color.a = 1;

	return color;
}

