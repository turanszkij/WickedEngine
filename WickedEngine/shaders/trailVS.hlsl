#include "globals.hlsli"

struct VertexOut
{
	float4 pos : SV_Position;
	float4 uv : TEXCOORD;
	float4 color : COLOR;
};

VertexOut main(float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR)
{
	VertexOut Out;
	Out.pos = mul(g_xTrailTransform, float4(pos, 1));
	Out.uv.xy = mad(uv, g_xTrailTexMulAdd.xy, g_xTrailTexMulAdd.zw);
	Out.uv.zw = mad(uv, g_xTrailTexMulAdd2.xy, g_xTrailTexMulAdd2.zw);
	Out.color = color * g_xTrailColor;
	return Out;
}
