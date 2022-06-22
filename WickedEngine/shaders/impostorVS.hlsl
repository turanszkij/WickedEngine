#include "globals.hlsli"
#include "impostorHF.hlsli"

static const float2 BILLBOARD[] = {
	float2(-1, -1),
	float2(1, -1),
	float2(-1, 1),
	float2(1, 1),
};

ByteAddressBuffer vb_pos_nor : register(t0);
ByteAddressBuffer impostor_data : register(t2);

VSOut main(uint vertexID : SV_VertexID)
{
	uint2 data = impostor_data.Load2((vertexID / 4u) * sizeof(uint2));

	VSOut Out;
	Out.pos3D = asfloat(vb_pos_nor.Load3(vertexID * sizeof(uint4)));
	Out.pos = mul(GetCamera().view_projection, float4(Out.pos3D, 1));
	Out.uv = float2(BILLBOARD[vertexID % 4u] * float2(0.5f, -0.5f) + 0.5f);
	Out.slice = data.x & 0xFFFFFF;
	Out.dither = float((data.x >> 24u) & 0xFF) / 255.0;
	Out.instanceColor = data.y;
	return Out;
}
