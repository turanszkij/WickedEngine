#include "globals.hlsli"

struct CubeConstants
{
	float4x4 transform;
};
PUSHCONSTANT(cube, CubeConstants);

#ifndef __PSSL__
#undef WICKED_ENGINE_DEFAULT_ROOTSIGNATURE // don't use auto root signature!
[RootSignature("RootConstants(num32BitConstants=16, b999, visibility = SHADER_VISIBILITY_VERTEX)")]
#endif // __PSSL__

float4 main(uint vID : SV_VertexID) : SV_Position
{
	// This is a 14 vertex count trianglestrip cube:
	return mul(cube.transform, float4(vertexID_create_cube(vID) * 2 - 1, 1));
}
