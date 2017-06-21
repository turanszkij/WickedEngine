#include "ResourceMapping.h"
#include "ShaderInterop.h"

struct Bone
{
	float4x4 pose, prev;
};
STRUCTUREDBUFFER(boneBuffer, Bone, SKINNINGSLOT_IN_BONEBUFFER);

RAWBUFFER(vertexBuffer_POS, SKINNINGSLOT_IN_VERTEX_POS);
RAWBUFFER(vertexBuffer_NOR, SKINNINGSLOT_IN_VERTEX_NOR);
RAWBUFFER(vertexBuffer_WEI, SKINNINGSLOT_IN_VERTEX_WEI);
RAWBUFFER(vertexBuffer_BON, SKINNINGSLOT_IN_VERTEX_BON);

RWRAWBUFFER(streamoutBuffer_POS, SKINNINGSLOT_OUT_VERTEX_POS);
RWRAWBUFFER(streamoutBuffer_NOR, SKINNINGSLOT_OUT_VERTEX_NOR);
RWRAWBUFFER(streamoutBuffer_PRE, SKINNINGSLOT_OUT_VERTEX_PRE);



inline void Skinning(inout float4 pos, inout float4 posPrev, inout float4 nor, in float4 inBon, in float4 inWei)
{
	float4 p = 0, pp = 0;
	float3 n = 0;
	float4x4 m, mp;
	float3x3 m3;
	float weisum = 0;

	// force loop to reduce register pressure
	// though this way we can not interleave TEX - ALU operations
	[loop]
	for (uint i = 0; ((i < 4) && (weisum<1.0f)); ++i)
	{
		m = boneBuffer[(uint)inBon[i]].pose;
		mp = boneBuffer[(uint)inBon[i]].prev;
		m3 = (float3x3)m;

		p += mul(float4(pos.xyz, 1), m)*inWei[i];
		pp += mul(float4(posPrev.xyz, 1), mp)*inWei[i];
		n += mul(nor.xyz, m3)*inWei[i];

		weisum += inWei[i];
	}

	bool w = any(inWei);
	pos.xyz = w ? p.xyz : pos.xyz;
	posPrev.xyz = w ? pp.xyz : posPrev.xyz;
	nor.xyz = w ? n : nor.xyz;
}


[numthreads(SKINNING_COMPUTE_THREADCOUNT, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint fetchAddress = DTid.x * 16;

	uint4 pos_u = vertexBuffer_POS.Load4(fetchAddress);
	uint4 nor_u = vertexBuffer_NOR.Load4(fetchAddress);
	uint4 wei_u = vertexBuffer_WEI.Load4(fetchAddress);
	uint4 bon_u = vertexBuffer_BON.Load4(fetchAddress);
	uint4 pre_u;

	float4 pos = asfloat(pos_u);
	float4 nor = asfloat(nor_u);
	float4 wei = asfloat(wei_u);
	float4 bon = asfloat(bon_u);
	float4 pre = pos;

	Skinning(pos, pre, nor, bon, wei);

	pos_u =	asuint(pos);
	nor_u = asuint(nor);
	pre_u =	asuint(pre);

	streamoutBuffer_POS.Store4(fetchAddress, pos_u);
	streamoutBuffer_NOR.Store4(fetchAddress, nor_u);
	streamoutBuffer_PRE.Store4(fetchAddress, pre_u);
}