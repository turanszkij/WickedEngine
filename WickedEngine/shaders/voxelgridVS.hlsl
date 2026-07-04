#include "globals.hlsli"

void main(float4 position : POSITION, uint vertexID : SV_VertexID, out float4 pos : SV_Position, out half4 col : COLOR)
{
	pos = float4(vertexID_create_cube(vertexID) * 2 - 1, 1);
	pos = mul(g_xTransform, pos);
	pos.xyz += position.xyz;
	pos = mul(GetCamera().view_projection, pos);
	col = unpack_rgba(asuint(position.w)) * g_xColor;
}
