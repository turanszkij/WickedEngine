#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"

PUSHCONSTANT(push, LightmapPushConstants);

struct Output
{
	float4 pos : SV_POSITION;
	centroid float3 pos3D : WORLDPOSITION;
	centroid float3 normal : NORMAL;
};

Output main(uint vertexID : SV_VertexID)
{
	ShaderMeshInstance inst = load_instance(push.instanceIndex);
	float3 pos = bindless_buffers_float4[descriptor_index(push.vb_pos_wind)][vertexID].xyz;
	float3 nor = bindless_buffers_float4[descriptor_index(push.vb_nor)][vertexID].xyz;
	float2 atl = bindless_buffers_float2[descriptor_index(push.vb_atl)][vertexID];

	Output output;

	output.pos = float4(atl, 1, 1); // Note: set Z to 1 for depth is intentional, there is depth write and depth test
	output.pos.xy = output.pos.xy * 2 - 1;
	output.pos.y *= -1;
	output.pos.xy += xTracePixelOffset;

	output.pos3D = mul(inst.transform.GetMatrix(), float4(pos, 1)).xyz;

	output.normal = mul(inst.transformRaw.GetMatrixAdjoint(), nor);

	return output;
}
