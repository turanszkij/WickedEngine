#include "globals.hlsli"
#include "cube.hlsli"

struct CubeConstants
{
	float4x4 transform;
};
ConstantBuffer<CubeConstants> cube : register(b0);

[RootSignature("CBV(b0)")]
float4 main(uint vID : SV_VertexID) : SV_Position
{
	// This is a 14 vertex count trianglestrip cube:
	return mul(cube.transform, float4(vertexID_create_cube(vID) * 2 - 1, 1));
}
