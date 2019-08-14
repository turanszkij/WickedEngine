#include "globals.hlsli"
#include "cube.hlsli"

float4 main(uint vID : SV_VERTEXID) : SV_Position
{
	// This is a 36 vertexcount variant:
	//return mul(g_xTransform, CUBE[vID]);

	// This is a 14 vertex count trianglestrip variant:
	return mul(g_xTransform, float4(CreateCube(vID) * 2 - 1,1));
}