#include "globals.hlsli"

static const float3 QUAD[] = {
	float3(0, 0, 0),	// 0
	float3(1, 0, 0),	// 1
	float3(0, 1, 0),	// 2
	float3(0, 1, 0),	// 3
	float3(1, 0, 0),	// 4
	float3(1, 1, 0),	// 5
};

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

VSOut main(uint fakeIndex : SV_VERTEXID)
{
	VSOut Out;
	Out.color = float4(1, 1, 1, 1);

	uint vertexID = fakeIndex % 6;
	uint instanceID = fakeIndex / 6;
	
	Out.pos = float4(QUAD[vertexID], 1);

	float2 dim = g_xColor.xy;
	float2 invdim = g_xColor.zw;

	Out.pos.xy *= invdim;
	Out.pos.xy += (float2)unflatten2D(instanceID, dim.xy) * invdim;
	Out.pos.xy = Out.pos.xy * 2 - 1;

	return Out;
}