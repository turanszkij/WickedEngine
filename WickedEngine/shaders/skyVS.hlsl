#include "globals.hlsli"
#include "icosphere.hlsli"
#include "skyHF.hlsli"

void main(uint vI : SV_VertexID, out float4 pos : SV_Position)
{
	vertexID_create_fullscreen_triangle(vI, pos);
}
