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
#ifdef VERTEXBUFFER_HALFPOSITION
	const uint stride_POS = 8;
#else
	const uint stride_POS = 16;
#endif // VERTEXBUFFER_HALFPOSITION
	const uint stride_NOR = 4;
	const uint stride_BON_IND = 4;
	const uint stride_BON_WEI = 4;

	const uint fetchAddress_POS = DTid.x * stride_POS;
	const uint fetchAddress_NOR = DTid.x * stride_NOR;
	const uint fetchAddress_BON = DTid.x * (stride_BON_IND + stride_BON_WEI);

	// Manual type-conversion for pos:
#ifdef VERTEXBUFFER_HALFPOSITION
	uint2 pos_u = vertexBuffer_POS.Load2(fetchAddress_POS);
	float4 pos = 0;
	{
		pos.x = f16tof32((pos_u.x & 0x0000FFFF) >> 0);
		pos.y = f16tof32((pos_u.x & 0xFFFF0000) >> 16);
		pos.z = f16tof32((pos_u.y & 0x0000FFFF) >> 0);
		pos.w = f16tof32((pos_u.y & 0xFFFF0000) >> 16);
	}
#else
	uint4 pos_u = vertexBuffer_POS.Load4(fetchAddress_POS);
	float4 pos = asfloat(pos_u);
#endif // VERTEXBUFFER_HALFPOSITION


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
	uint2 ind_wei_u = vertexBuffer_BON.Load2(fetchAddress_BON);
	float4 ind = 0;
	float4 wei = 0;
	{
		ind.x = (float)((ind_wei_u.x >> 0) & 0x000000FF);
		ind.y = (float)((ind_wei_u.x >> 8) & 0x000000FF);
		ind.z = (float)((ind_wei_u.x >> 16) & 0x000000FF);
		ind.w = (float)((ind_wei_u.x >> 24) & 0x000000FF);

		wei.x = (float)((ind_wei_u.y >> 0) & 0x000000FF) / 255.0f;
		wei.y = (float)((ind_wei_u.y >> 8) & 0x000000FF) / 255.0f;
		wei.z = (float)((ind_wei_u.y >> 16) & 0x000000FF) / 255.0f;
		wei.w = (float)((ind_wei_u.y >> 24) & 0x000000FF) / 255.0f;
	}


	// Perform skinning:
	Skinning(pos, nor, ind, wei);



	// Manual type-conversion for pos:
#ifdef VERTEXBUFFER_HALFPOSITION
	pos_u = 0;
	{
		pos_u.x |= f32tof16(pos.x) << 0;
		pos_u.x |= f32tof16(pos.y) << 16;
		pos_u.y |= f32tof16(pos.z) << 0;
		pos_u.y |= f32tof16(pos.w) << 16;
	}
#else
	pos_u = asuint(pos);
#endif // VERTEXBUFFER_HALFPOSITION


	// Manual type-conversion for normal:
	nor_u = 0;
	{
		nor_u |= (uint)((nor.x * 0.5f + 0.5f) * 255.0f) << 0;
		nor_u |= (uint)((nor.y * 0.5f + 0.5f) * 255.0f) << 8;
		nor_u |= (uint)((nor.z * 0.5f + 0.5f) * 255.0f) << 16;
		nor_u |= (uint)(nor.w * 255.0f) << 24; // occlusion
	}

#ifdef VERTEXBUFFER_HALFPOSITION
	streamoutBuffer_PRE.Store2(fetchAddress_POS, streamoutBuffer_POS.Load2(fetchAddress_POS)); // copy prev frame current pos to current frame prev pos
	streamoutBuffer_POS.Store2(fetchAddress_POS, pos_u);
#else
	streamoutBuffer_PRE.Store4(fetchAddress_POS, streamoutBuffer_POS.Load4(fetchAddress_POS)); // copy prev frame current pos to current frame prev pos
	streamoutBuffer_POS.Store4(fetchAddress_POS, pos_u);
#endif // VERTEXBUFFER_HALFPOSITION
	streamoutBuffer_NOR.Store(fetchAddress_NOR, nor_u);
}