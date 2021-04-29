#include "globals.hlsli"

struct VertexToPixel 
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR;
};

VertexToPixel main(float4 inPos : POSITION, float4 inCol : TEXCOORD0)
{
	VertexToPixel Out;

	Out.pos = mul(g_xTransform, inPos);
	Out.col = inCol * g_xColor;

	return Out;
}
