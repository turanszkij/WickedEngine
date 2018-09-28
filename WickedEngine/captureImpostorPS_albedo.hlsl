#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float4 color = g_xMat_baseColor * xBaseColorMap.Sample(sampler_objectshader, UV);
	ALPHATEST(color.a);
	color.a = 1;
	color.rgb = DEGAMMA(color.rgb);

	return color;
}

