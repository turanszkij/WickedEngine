#include "ResourceMapping.h"
#include "ShaderInterop.h"

struct Bone
{
	float4x4 pose;
};
STRUCTUREDBUFFER(boneBuffer, Bone, SKINNINGSLOT_IN_BONEBUFFER);

RAWBUFFER(vertexBuffer_POS, SKINNINGSLOT_IN_VERTEX_POS);
RAWBUFFER(vertexBuffer_NOR, SKINNINGSLOT_IN_VERTEX_NOR);
RAWBUFFER(vertexBuffer_BON, SKINNINGSLOT_IN_VERTEX_BON);

RWRAWBUFFER(streamoutBuffer_POS, SKINNINGSLOT_OUT_VERTEX_POS);
RWRAWBUFFER(streamoutBuffer_NOR, SKINNINGSLOT_OUT_VERTEX_NOR);
RWRAWBUFFER(streamoutBuffer_PRE, SKINNINGSLOT_OUT_VERTEX_PRE);



inline void Skinning(inout float4 pos, inout float4 nor, in float4 inBon, in float4 inWei)
{
	float4 p = 0, pp = 0;
	float3 n = 0;
	float4x4 m;
	float3x3 m3;
	float weisum = 0;

	// force loop to reduce register pressure
	// though this way we can not interleave TEX - ALU operations
	[loop]
	for (uint i = 0; ((i < 4) && (weisum<1.0f)); ++i)
	{
		m = boneBuffer[(uint)inBon[i]].pose;
		m3 = (float3x3)m;

		p += mul(float4(pos.xyz, 1), m)*inWei[i];
		n += mul(nor.xyz, m3)*inWei[i];

		weisum += inWei[i];
	}

	bool w = any(inWei);
	pos.xyz = w ? p.xyz : pos.xyz;
	nor.xyz = w ? normalize(n.xyz) : nor.xyz;
}


[numthreads(SKINNING_COMPUTE_THREADCOUNT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint stride_POS = 16;
	const uint stride_NOR = 4;
	const uint stride_BON_IND = 8;
	const uint stride_BON_WEI = 8;

	const uint fetchAddress_POS = DTid.x * stride_POS;
	const uint fetchAddress_NOR = DTid.x * stride_NOR;
	const uint fetchAddress_BON = DTid.x * (stride_BON_IND + stride_BON_WEI);

	// Manual type-conversion for pos:
	uint4 pos_u = vertexBuffer_POS.Load4(fetchAddress_POS);
	float4 pos = asfloat(pos_u);


	// Manual type-conversion for normal:
	uint nor_u = vertexBuffer_NOR.Load(fetchAddress_NOR);
	float4 nor = 0;
	{
		nor.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor.w = (float)((nor_u >> 24) & 0x000000FF) / 255.0f; // occlusion
	}


	// Manual type-conversion for bone props:
	uint4 ind_wei_u = vertexBuffer_BON.Load4(fetchAddress_BON);
	float4 ind = 0;
	float4 wei = 0;
	{
		ind.x = (float)((ind_wei_u.x >> 0) & 0x0000FFFF);
		ind.y = (float)((ind_wei_u.x >> 16) & 0x0000FFFF);
		ind.z = (float)((ind_wei_u.y >> 0) & 0x0000FFFF);
		ind.w = (float)((ind_wei_u.y >> 16) & 0x0000FFFF);

		wei.x = (float)((ind_wei_u.z >> 0) & 0x0000FFFF) / 65535.0f;
		wei.y = (float)((ind_wei_u.z >> 16) & 0x0000FFFF) / 65535.0f;
		wei.z = (float)((ind_wei_u.w >> 0) & 0x0000FFFF) / 65535.0f;
		wei.w = (float)((ind_wei_u.w >> 16) & 0x0000FFFF) / 65535.0f;
	}


	// Perform skinning:
	Skinning(pos, nor, ind, wei);



	// Manual type-conversion for pos:
	pos_u = asuint(pos);


	// Manual type-conversion for normal:
	nor_u = 0;
	{
		nor_u |= (uint)((nor.x * 0.5f + 0.5f) * 255.0f) << 0;
		nor_u |= (uint)((nor.y * 0.5f + 0.5f) * 255.0f) << 8;
		nor_u |= (uint)((nor.z * 0.5f + 0.5f) * 255.0f) << 16;
		nor_u |= (uint)(nor.w * 255.0f) << 24; // occlusion
	}

	// Store data:
	streamoutBuffer_PRE.Store4(fetchAddress_POS, streamoutBuffer_POS.Load4(fetchAddress_POS)); // copy prev frame current pos to current frame prev pos
	streamoutBuffer_POS.Store4(fetchAddress_POS, pos_u);
	streamoutBuffer_NOR.Store(fetchAddress_NOR, nor_u);
}