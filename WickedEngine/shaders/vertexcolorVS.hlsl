#include "globals.hlsli"

void main(float4 inPos : POSITION, float4 inCol : TEXCOORD0, out float4 pos : SV_Position, out half4 col : COLOR)
{
	pos = mul(g_xTransform, inPos);
	col = inCol * g_xColor;
}
