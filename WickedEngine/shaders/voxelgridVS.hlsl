#include "globals.hlsli"

struct VertexToPixel
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

VertexToPixel main(float4 position : POSITION, uint vertexID : SV_VertexID)
{
	VertexToPixel Out;
	
	Out.pos = float4(vertexID_create_cube(vertexID) * 2 - 1, 1);
	Out.pos = mul(g_xTransform, Out.pos);
	Out.pos.xyz += position.xyz;
	Out.pos = mul(GetCamera().view_projection, Out.pos);
	Out.col = unpack_rgba(asuint(position.w)) * g_xColor;

	return Out;
}
