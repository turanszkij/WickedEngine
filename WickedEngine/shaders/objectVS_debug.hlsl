#include "globals.hlsli"

PUSHCONSTANT(push, DebugObjectPushConstants);

void main(uint vertexID : SV_VertexID, out float4 pos : SV_Position, out half4 col : COLOR)
{
	pos = mul(g_xTransform, float4(bindless_buffers_float4[descriptor_index(push.vb_pos_wind)][vertexID].xyz, 1));
	col = g_xColor;
}
