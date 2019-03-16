#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	const float2 UV_baseColorMap = g_xMat_uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
	float4 color = xBaseColorMap.Sample(sampler_objectshader, UV_baseColorMap);
	color.rgb = DEGAMMA(color.rgb);
	color *= input.color;
	ALPHATEST(color.a);
	color.a = 1;

	return color;
}

