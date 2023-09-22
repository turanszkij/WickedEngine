#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
	float2 bary : TEXCOORD1;
};

VertextoPixel main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	uint vID = instanceID * 4 + vertexID;
	FontVertex vertex = bindless_buffers[font.buffer_index].Load<FontVertex>(font.buffer_offset + vID * sizeof(FontVertex));

	VertextoPixel Out;
	Out.pos = mul(font.transform, float4(asfloat(vertex.pos), 0, 1));
	Out.uv = vertex.uv;
	switch (vertexID)
	{
		default:
		case 0:
			Out.bary = float2(0, 0);
			break;
		case 1:
			Out.bary = float2(1, 0);
			break;
		case 2:
			Out.bary = float2(0, 1);
			break;
		case 3:
			Out.bary = float2(1, 1);
			break;
	}
	return Out;
}
