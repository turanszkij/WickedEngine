#include "globals.hlsli"

PUSHCONSTANT(push, SkinningPushConstants);

#ifndef __PSSL__
#undef WICKED_ENGINE_DEFAULT_ROOTSIGNATURE // don't use auto root signature!
[RootSignature(
	"RootConstants(num32BitConstants=20, b999),"
	"DescriptorTable( "
		"SRV(t0, space = 2, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 3, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 4, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 5, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 6, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 7, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 8, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 9, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 10, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 11, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 12, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 13, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 14, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 15, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 16, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 17, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 18, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 19, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 20, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 21, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 22, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 23, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 24, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 25, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 26, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 27, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 28, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 29, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 30, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 31, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 32, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 33, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 34, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 35, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 36, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 37, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 38, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 39, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 40, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)" \
	")"
)]
#endif // __PSSL__

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
	const uint vertexID = DTid.x;

	[branch]
	if (push.vb_pos_wind < 0 || vertexID >= push.vertexCount)
		return;
	
	float4 pos_wind = bindless_buffers_float4[push.vb_pos_wind][vertexID];
	half3 nor = bindless_buffers_float4[push.vb_nor][vertexID].xyz;
	
	float3 pos = pos_wind.xyz;
	if(any(push.aabb_min) || any(push.aabb_max))
	{
		// UNORM vertex position remap:
		pos = lerp(push.aabb_min, push.aabb_max, pos);
	}
	
	half4 tan = bindless_buffers_float4[push.vb_tan][vertexID];

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
	if (push.vb_bon >= 0 && push.bone_offset != ~0u)
	{
		float4 p = 0;
		half3 n = 0;
		half3 t = 0;
		for (uint influence = 0; influence < push.influence_div4; ++influence)
		{
			uint4 ind = 0;
			half4 wei = 0;
			[branch]
			if (push.vb_bon >= 0)
			{
				// Manual type-conversion for bone props:
				uint4 ind_wei_u = bindless_buffers[push.vb_bon].Load4((vertexID * push.influence_div4 + influence) * sizeof(uint4));

				ind.x = uint(ind_wei_u.x & 0xFFFFF);
				ind.y = uint(ind_wei_u.y & 0xFFFFF);
				ind.z = uint(ind_wei_u.z & 0xFFFFF);
				ind.w = uint(ind_wei_u.w & 0xFFFFF);

				wei.x = half(float((ind_wei_u.x >> 20) & 0xFFF) / 4095.0);
				wei.y = half(float((ind_wei_u.y >> 20) & 0xFFF) / 4095.0);
				wei.z = half(float((ind_wei_u.z >> 20) & 0xFFF) / 4095.0);
				wei.w = half(float((ind_wei_u.w >> 20) & 0xFFF) / 4095.0);
			}
			if (any(wei))
			{
				half weisum = 0;
			
				for (uint i = 0; ((i < 4) && (weisum < 1.0)); ++i)
				{
					float4x4 m = skinningbuffer.Load<ShaderTransform>(push.bone_offset + ind[i] * sizeof(ShaderTransform)).GetMatrix();
					half weight = wei[i];

					p += mul(m, float4(pos.xyz, 1)) * weight;
					n += half3(mul((half3x3)m, nor.xyz)) * weight;
					t += half3(mul((half3x3)m, tan.xyz)) * weight;
					weisum += weight;
				}
			}
		}
		pos.xyz = p.xyz;
		nor.xyz = normalize(n.xyz);
		tan.xyz = normalize(t.xyz);
	}

	// Store data:
	[branch]
	if (push.so_pos >= 0)
	{
#ifdef __PSSL__
		bindless_rwbuffers[push.so_pos].TypedStore<float3>(vertexID * sizeof(float3), pos);
#else
		bindless_rwbuffers[push.so_pos].Store<float3>(vertexID * sizeof(float3), pos);
#endif // __PSSL__
	}

	[branch]
	if (push.so_nor >= 0)
	{
		bindless_rwbuffers_float4[push.so_nor][vertexID] = float4(nor, 0);
	}

	[branch]
	if (push.so_tan >= 0)
	{
		bindless_rwbuffers_float4[push.so_tan][vertexID] = tan;
	}
}
