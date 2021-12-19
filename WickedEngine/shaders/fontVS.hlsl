#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

VertextoPixel main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	uint vID = instanceID * 4 + vertexID;
	FontVertex vertex = bindless_buffers[font_push.buffer_index].Load<FontVertex>(font_push.buffer_offset + vID * sizeof(FontVertex));

	VertextoPixel Out;
	Out.pos = mul(font.transform, float4(asfloat(vertex.pos), 0, 1));
	Out.uv = vertex.uv;
	return Out;
}
