#include "ResourceMapping.h"
#include "ShaderInterop.h"

struct Bone
{
	float4 pose0;
	float4 pose1;
	float4 pose2;
};
STRUCTUREDBUFFER(boneBuffer, Bone, SKINNINGSLOT_IN_BONEBUFFER);

RAWBUFFER(vertexBuffer_POS, SKINNINGSLOT_IN_VERTEX_POS);
RAWBUFFER(vertexBuffer_BON, SKINNINGSLOT_IN_VERTEX_BON);

RWRAWBUFFER(streamoutBuffer_POS, SKINNINGSLOT_OUT_VERTEX_POS);
RWRAWBUFFER(streamoutBuffer_PRE, SKINNINGSLOT_OUT_VERTEX_PRE);



inline void Skinning(inout float3 pos, inout float3 nor, in float4 inBon, in float4 inWei)
{
	float4 p = 0;
	float3 n = 0;
	float weisum = 0;
	bool w = any(inWei);

	// force loop to reduce register pressure
	// though this way we can not interleave TEX - ALU operations
	[loop]
	for (uint i = 0; (w && (i < 4) && (weisum<1.0f)); ++i)
	{
		float4x4 m = float4x4(
			boneBuffer[(uint)inBon[i]].pose0,
			boneBuffer[(uint)inBon[i]].pose1,
			boneBuffer[(uint)inBon[i]].pose2,
			float4(0, 0, 0, 1)
			);

		p += mul(m, float4(pos.xyz, 1))*inWei[i];
		n += mul((float3x3)m, nor.xyz)*inWei[i];

		weisum += inWei[i];
	}

	pos.xyz = w ? p.xyz : pos.xyz;
	nor.xyz = w ? normalize(n.xyz) : nor.xyz;
}


[numthreads(SKINNING_COMPUTE_THREADCOUNT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint stride_POS_NOR = 16;
	const uint stride_BON_IND = 8;
	const uint stride_BON_WEI = 8;

	const uint fetchAddress_POS_NOR = DTid.x * stride_POS_NOR;
	const uint fetchAddress_BON = DTid.x * (stride_BON_IND + stride_BON_WEI);

	// Manual type-conversion for pos:
	uint4 pos_nor_u = vertexBuffer_POS.Load4(fetchAddress_POS_NOR);
	float3 pos = asfloat(pos_nor_u.xyz);


	// Manual type-conversion for normal:
	float4 nor = 0;
	{
		nor.x = (float)((pos_nor_u.w >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor.y = (float)((pos_nor_u.w >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor.z = (float)((pos_nor_u.w >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor.w = (float)((pos_nor_u.w >> 24) & 0x000000FF) / 255.0f; // wind
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
	Skinning(pos, nor.xyz, ind, wei);



	// Manual type-conversion for pos:
	pos_nor_u.xyz = asuint(pos.xyz);


	// Manual type-conversion for normal:
	pos_nor_u.w = 0;
	{
		pos_nor_u.w |= (uint)((nor.x * 0.5f + 0.5f) * 255.0f) << 0;
		pos_nor_u.w |= (uint)((nor.y * 0.5f + 0.5f) * 255.0f) << 8;
		pos_nor_u.w |= (uint)((nor.z * 0.5f + 0.5f) * 255.0f) << 16;
		pos_nor_u.w |= (uint)(nor.w * 255.0f) << 24; // wind
	}

	// Store data:
	streamoutBuffer_PRE.Store4(fetchAddress_POS_NOR, streamoutBuffer_POS.Load4(fetchAddress_POS_NOR)); // copy prev frame current pos to current frame prev pos
	streamoutBuffer_POS.Store4(fetchAddress_POS_NOR, pos_nor_u);
}