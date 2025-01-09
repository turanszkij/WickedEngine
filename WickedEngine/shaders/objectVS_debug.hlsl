#include "globals.hlsli"

PUSHCONSTANT(push, DebugObjectPushConstants);

float4 main(uint vertexID : SV_VertexID) : SV_POSITION
{
	return mul(g_xTransform, float4(bindless_buffers_float4[descriptor_index(push.vb_pos_wind)][vertexID].xyz, 1));
}
