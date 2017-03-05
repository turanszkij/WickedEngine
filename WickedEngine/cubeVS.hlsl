#include "globals.hlsli"
#include "cube.hlsli"

float4 main(uint vID : SV_VERTEXID) : SV_Position
{
	// This is a 36 vertexcount variant:
	//return mul(CUBE[vID], g_xTransform);

	// This is a 14 vertex count trianglestrip variant:
	return mul(float4(CreateCube(vID) * 2 - 1,1), g_xTransform);
}