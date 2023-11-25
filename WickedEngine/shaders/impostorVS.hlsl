#include "globals.hlsli"
#include "impostorHF.hlsli"

static const float2 BILLBOARD[] = {
	float2(-1, -1),
	float2(1, -1),
	float2(-1, 1),
	float2(1, 1),
};

Buffer<float4> vb_pos : register(t0);
ByteAddressBuffer impostor_data : register(t2);

VSOut main(uint vertexID : SV_VertexID)
{
	uint impostorID = vertexID / 4u;
	uint2 data = impostor_data.Load2(impostorID * sizeof(uint2));

	VSOut Out;
	Out.pos3D = vb_pos[vertexID].xyz;
	Out.clip = dot(float4(Out.pos3D, 1), GetCamera().clip_plane);
	Out.pos = mul(GetCamera().view_projection, float4(Out.pos3D, 1));
	Out.uv = float2(BILLBOARD[vertexID % 4u] * float2(0.5f, -0.5f) + 0.5f);
	Out.slice = data.x & 0xFFFFFF;
	Out.dither = float((data.x >> 24u) & 0xFF) / 255.0;
	Out.instanceColor = data.y;
	Out.primitiveID = vertexID / 2u;
	return Out;
}
