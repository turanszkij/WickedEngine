#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VertextoPixel main(uint vertexID : SV_VERTEXID, uint instanceID : SV_InstanceID)
{
	VertextoPixel Out;

	uint vID = instanceID * 4 + vertexID;
	uint3 raw = bindless_buffers[push.buffer_index].Load3(push.buffer_offset + vID * 12);
	Out.pos = mul(push.transform, float4(asfloat(raw.xy), 0, 1));
	Out.tex = unpack_half2(raw.z);

	return Out;
}
