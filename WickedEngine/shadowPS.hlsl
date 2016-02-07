#include "globals.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

void main(VertextoPixel PSIn)
{
	[branch]if (g_xMat_hasTex)
		ALPHATEST(texture_0.Sample(sampler_linear_wrap, PSIn.tex).a);
}