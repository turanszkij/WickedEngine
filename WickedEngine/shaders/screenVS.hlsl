#include "globals.hlsli"

float4 main(uint vertexID : SV_VertexID) : SV_Position
{
	float4 pos;
	vertexID_create_fullscreen_triangle(vertexID, pos);
	return pos;
}
