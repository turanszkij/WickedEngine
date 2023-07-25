#include "globals.hlsli"

PUSHCONSTANT(push, SkinningPushConstants);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
	const uint vertexID = DTid.x;

	[branch]
	if (push.vb_pos_nor_wind < 0 || vertexID >= push.vertexCount)
		return;
	
	float4 pos_nor_wind = bindless_buffers_float4[push.vb_pos_nor_wind][vertexID];
	uint nor_wind = asuint(pos_nor_wind.w);
	
	float3 pos = pos_nor_wind.xyz;
	float4 nor = 0;
	nor.x = (float)((nor_wind >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	nor.y = (float)((nor_wind >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	nor.z = (float)((nor_wind >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	nor.w = (float)((nor_wind >> 24) & 0x000000FF) / 255.0f; // wind
	
	float4 tan = bindless_buffers_float4[push.vb_tan][vertexID];

	ByteAddressBuffer skinningbuffer = bindless_buffers[push.skinningbuffer_index];

	// Morph targets:
	[branch]
	if (push.morph_count > 0)
	{
		Buffer<float4> morphvb = bindless_buffers_float4[push.morphvb_index];
		for (uint morph_index = 0; morph_index < push.morph_count; ++morph_index)
		{
			MorphTargetGPU morph = skinningbuffer.Load<MorphTargetGPU>(push.morph_offset + morph_index * sizeof(MorphTargetGPU));
			if (morph.offset_pos != ~0u)
			{
				pos += morphvb[morph.offset_pos + vertexID].xyz * morph.weight;
			}
			if (morph.offset_nor != ~0u)
			{
				nor.xyz += morphvb[morph.offset_nor + vertexID].xyz * morph.weight;
			}
		}
	}

	// Skinning:
	[branch]
	if (push.bone_offset != ~0u)
	{
		float4 ind = 0;
		float4 wei = 0;
		[branch]
		if (push.vb_bon >= 0)
		{
			// Manual type-conversion for bone props:
			uint4 ind_wei_u = bindless_buffers[push.vb_bon].Load4(vertexID * sizeof(uint4));

			ind.x = (ind_wei_u.x >> 0) & 0xFFFF;
			ind.y = (ind_wei_u.x >> 16) & 0xFFFF;
			ind.z = (ind_wei_u.y >> 0) & 0xFFFF;
			ind.w = (ind_wei_u.y >> 16) & 0xFFFF;

			wei.x = float((ind_wei_u.z >> 0) & 0xFFFF) / 65535.0f;
			wei.y = float((ind_wei_u.z >> 16) & 0xFFFF) / 65535.0f;
			wei.z = float((ind_wei_u.w >> 0) & 0xFFFF) / 65535.0f;
			wei.w = float((ind_wei_u.w >> 16) & 0xFFFF) / 65535.0f;
		}
		if (any(wei))
		{
			float4 p = 0;
			float3 n = 0;
			float3 t = 0;
			float weisum = 0;
			
			for (uint i = 0; ((i < 4) && (weisum < 1.0f)); ++i)
			{
				float4x4 m = skinningbuffer.Load<ShaderTransform>(push.bone_offset + ind[i] * sizeof(ShaderTransform)).GetMatrix();

				p += mul(m, float4(pos.xyz, 1)) * wei[i];
				n += mul((float3x3)m, nor.xyz) * wei[i];
				t += mul((float3x3)m, tan.xyz) * wei[i];

				weisum += wei[i];
			}

			pos.xyz = p.xyz;
			nor.xyz = normalize(n.xyz);
			tan.xyz = normalize(t.xyz);
		}
	}

	// Store data:
	[branch]
	if (push.so_pos_nor_wind >= 0)
	{
		uint nor_wind = 0;
		nor_wind |= uint((nor.x * 0.5f + 0.5f) * 255.0f) << 0;
		nor_wind |= uint((nor.y * 0.5f + 0.5f) * 255.0f) << 8;
		nor_wind |= uint((nor.z * 0.5f + 0.5f) * 255.0f) << 16;
		nor_wind |= uint(nor.w * 255.0f) << 24; // wind
		pos_nor_wind = float4(pos.xyz, asfloat(nor_wind));
		bindless_rwbuffers_float4[push.so_pos_nor_wind][vertexID] = pos_nor_wind;
	}

	[branch]
	if (push.so_tan >= 0)
	{
		bindless_rwbuffers_float4[push.so_tan][vertexID] = tan;
	}
}
