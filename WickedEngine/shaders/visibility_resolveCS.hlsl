#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);

RWTEXTURE2D(output_normal_roughness, unorm float4, 0);
RWTEXTURE2D(output_velocity, float2, 1);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	uint2 pixel = DTid.xy;

	const float depth = texture_depth[pixel];
	float2 velocity = 0;

	if (depth > 0)
	{
		const float2 uv = ((float2)pixel + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);


		PrimitiveID prim;
		prim.unpack(texture_gbuffer0[pixel]);
		ShaderMeshInstance inst = InstanceBuffer[prim.instanceIndex];
		ShaderMesh mesh = inst.mesh;
		ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][prim.subsetIndex];
		uint startIndex = prim.primitiveIndex * 3 + subset.indexOffset;
		uint i0 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 0];
		uint i1 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 1];
		uint i2 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 2];

		float3 N = 0;
		[branch]
		if (mesh.vb_pos_nor_wind >= 0)
		{
			uint4 data0 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i0 * 16);
			uint4 data1 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i1 * 16);
			uint4 data2 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i2 * 16);
			float4x4 worldMatrix = float4x4(transpose(inst.transform), float4(0, 0, 0, 1));
			float3 p0 = mul(worldMatrix, float4(asfloat(data0.xyz), 1)).xyz;
			float3 p1 = mul(worldMatrix, float4(asfloat(data1.xyz), 1)).xyz;
			float3 p2 = mul(worldMatrix, float4(asfloat(data2.xyz), 1)).xyz;
			float3 n0 = unpack_unitvector(data0.w);
			float3 n1 = unpack_unitvector(data1.w);
			float3 n2 = unpack_unitvector(data2.w);

			float3 bary = compute_barycentrics(P, p0, p1, p2);
			float u = bary.x;
			float v = bary.y;
			float w = bary.z;
			N = n0 * w + n1 * u + n2 * v;
			N = mul((float3x3)worldMatrix, N);
			N = normalize(N);



			[branch]
			if (mesh.vb_pre >= 0)
			{
				p0 = asfloat(bindless_buffers[mesh.vb_pre].Load3(i0 * 16));
				p1 = asfloat(bindless_buffers[mesh.vb_pre].Load3(i1 * 16));
				p2 = asfloat(bindless_buffers[mesh.vb_pre].Load3(i2 * 16));
			}
			else
			{
				p0 = asfloat(bindless_buffers[mesh.vb_pos_nor_wind].Load3(i0 * 16));
				p1 = asfloat(bindless_buffers[mesh.vb_pos_nor_wind].Load3(i1 * 16));
				p2 = asfloat(bindless_buffers[mesh.vb_pos_nor_wind].Load3(i2 * 16));
			}
			float3 pre = p0 * w + p1 * u + p2 * v;

			float4x4 worldMatrixPrev = float4x4(transpose(inst.transformPrev), float4(0, 0, 0, 1));
			pre = mul(worldMatrixPrev, float4(pre, 1)).xyz;

			float4 pos2DPrev = mul(g_xCamera_PrevVP, float4(pre, 1));
			pos2DPrev.xy /= pos2DPrev.w;

			float2 pos2D = uv * 2 - 1;
			pos2D.y *= -1;

			velocity = ((pos2DPrev.xy - g_xFrame_TemporalAAJitterPrev) - (pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5, -0.5);
		}


		output_normal_roughness[pixel] = float4(N * 0.5 + 0.5, 1);
	}

	output_velocity[pixel] = velocity;
}
