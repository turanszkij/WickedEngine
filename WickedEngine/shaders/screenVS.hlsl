#include "globals.hlsli"

void main(uint vertexID : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD)
{
	vertexID_create_fullscreen_triangle(vertexID, pos, uv);
}
