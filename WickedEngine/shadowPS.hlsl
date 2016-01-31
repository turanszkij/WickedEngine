#include "globals.hlsli"

Texture2D<float4> xTextureTex:register(t0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

void main(VertextoPixel PSIn)
{
	[branch]if(g_xMat_hasTex)
		clip( xTextureTex.Sample(sampler_linear_wrap,PSIn.tex).a<0.1?-1:1 );
}