#include "globals.hlsli"

struct VertexToPixel{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR;
};

VertexToPixel main( float3 inPos : POSITION )
{
	VertexToPixel Out = (VertexToPixel)0;
	Out.pos = mul( float4(inPos,1), g_xTransform);
	Out.col = g_xColor;
	return Out;
}