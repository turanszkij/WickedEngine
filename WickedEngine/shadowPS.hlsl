#include "globals.hlsli"
#include "objectHF_PS.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

void main(VertextoPixel PSIn)
{
	ALPHATEST(xBaseColorMap.Sample(sampler_linear_wrap, PSIn.tex).a);
}