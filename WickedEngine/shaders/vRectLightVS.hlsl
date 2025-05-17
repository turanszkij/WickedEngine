#include "globals.hlsli"

static const float3 RECT[] = {
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(1, 1, 0),
};

void main(uint vertexID : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD)
{
	pos = float4(RECT[vertexID], 1);
	pos = mul(xLightWorld, pos);
	pos = mul(GetCamera().view_projection, pos);

	uv = clipspace_to_uv(RECT[vertexID].xy);
}
