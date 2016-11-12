#include "globals.hlsli"
#include "cube.hlsli"

float4 main(uint vID : SV_VERTEXID) : SV_Position
{
	return mul(CUBE[vID], g_xTransform);
}