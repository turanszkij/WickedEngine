#include "objectHF.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float3 pos3D			: POSITION3D;
	float2 tex				: TEXCOORD0;
	uint RTIndex			: SV_RenderTargetArrayIndex;
};

float main(VertextoPixel PSIn) : SV_DEPTHLESSEQUAL
{
	ALPHATEST(xBaseColorMap.Sample(sampler_linear_wrap,PSIn.tex).a);
	return distance(PSIn.pos3D.xyz, g_xColor.xyz) * g_xColor.w; // g_xColor.w = 1.0 / range
}