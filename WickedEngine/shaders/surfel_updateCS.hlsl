#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);

RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelIndexBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelDataBuffer, SurfelData, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);
RWSTRUCTUREDBUFFER(surfelPayloadBuffer, SurfelPayload, 1);
RWSTRUCTUREDBUFFER(surfelCellIndexBuffer, float, 2); // sorting written for floats

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelIndexBuffer[DTid.x];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	Surfel surfel = surfelBuffer[surfel_index];


	uint2 primitiveID = surfel_data.primitiveID;
	uint primitiveIndex = primitiveID.x;
	uint instanceID = primitiveID.y & 0xFFFFFF;
	uint subsetIndex = (primitiveID.y >> 24u) & 0xFF;
	ShaderMeshInstance inst = InstanceBuffer[instanceID];
	ShaderMesh mesh = inst.mesh;
	ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][subsetIndex];
	uint startIndex = primitiveIndex * 3 + subset.indexOffset;
	uint i0 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 0];
	uint i1 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 1];
	uint i2 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 2];

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

		float u = surfel_data.bary.x;
		float v = surfel_data.bary.y;
		float w = 1 - u - v;
		float3 N = n0 * w + n1 * u + n2 * v;
		N = mul((float3x3)worldMatrix, N);
		N = normalize(N);
		surfel.normal = pack_unitvector(N);

		surfel.position = p0 * w + p1 * u + p2 * v;
	}



	surfelBuffer[surfel_index] = surfel;
	surfelPayloadBuffer[surfel_index].color = pack_half4(float4(surfel_data.mean, saturate(surfel_data.life * 4)));

	surfelCellIndexBuffer[surfel_index] = surfel_hash(surfel_cell(surfel.position));
}
