#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"

PUSHCONSTANT(push, LightmapPushConstants);

struct Output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos3D : WORLDPOSITION;
	float3 normal : NORMAL;
};

Output main(uint vertexID : SV_VertexID)
{
	ShaderMeshInstance inst = load_instance(push.instanceIndex);
	float3 pos = bindless_buffers_float4[push.vb_pos_wind][vertexID].xyz;
	float3 nor = bindless_buffers_float4[push.vb_nor][vertexID].xyz;
	float2 atl = bindless_buffers_float2[push.vb_atl][vertexID];

	Output output;

	output.pos = float4(atl, 0, 1);
	output.pos.xy = output.pos.xy * 2 - 1;
	output.pos.y *= -1;
	output.pos.xy += xTracePixelOffset;

	output.uv = atl;

	output.pos3D = mul(inst.transform.GetMatrix(), float4(pos, 1)).xyz;

	output.normal = mul((float3x3)inst.transformInverseTranspose.GetMatrix(), nor);

	return output;
}
