#include "globals.hlsli"
#include "cube.hlsli"

float4 main(uint vID : SV_VERTEXID) : SV_Position
{
	return mul(mul(float4(CUBE[vID], 1), g_xTransform), g_xCamera_VP);
}