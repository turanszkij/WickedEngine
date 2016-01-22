#include "globals.hlsli"

struct VertexToPixel{
	float4 pos						: SV_POSITION;
	float2 tex						: TEXCOORD0;
	float4 col						: TEXCOORD1;
	float4 dis						: TEXCOORD2;
};

VertexToPixel main( float3 inPos : POSITION, float2 inTex : TEXCOORD0, float4 inCol : TEXCOORD1 )
{
	VertexToPixel Out = (VertexToPixel)0;

	Out.pos = Out.dis = mul( float4(inPos,1),g_xCamera_VP );
	Out.col = inCol;
	Out.tex = inTex;

	return Out;
}