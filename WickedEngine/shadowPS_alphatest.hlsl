#include "globals.hlsli"
#include "objectHF.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 uv				: UV;
};

void main(VertextoPixel PSIn)
{
	ALPHATEST(texture_basecolormap.Sample(sampler_linear_wrap, PSIn.uv).a);
}