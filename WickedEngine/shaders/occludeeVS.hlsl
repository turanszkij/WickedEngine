#include "globals.hlsli"

StructuredBuffer<float4x4> transforms : register(t0);

float4 main(uint vID : SV_VertexID, uint instanceID : SV_InstanceID, out uint occlusion_slot : OCCLUSION_SLOT) : SV_Position
{
	occlusion_slot = instanceID;
	return mul(transforms[instanceID], float4(vertexID_create_cube(vID) * 2 - 1, 1));
}
