#include "globals.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	return texture_1.Sample(sampler_linear_clamp, PSIn.tex) * g_xColor;
}