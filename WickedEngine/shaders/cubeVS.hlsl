#include "globals.hlsli"
#include "cube.hlsli"

struct CubePushConstants
{
	float4x4 transform;
};
PUSHCONSTANT(push, CubePushConstants);

float4 main(uint vID : SV_VERTEXID) : SV_Position
{
	// This is a 36 vertexcount variant:
	//return mul(g_xTransform, CUBE[vID]);

	// This is a 14 vertex count trianglestrip variant:
	return mul(push.transform, float4(vertexID_create_cube(vID) * 2 - 1,1));
}
