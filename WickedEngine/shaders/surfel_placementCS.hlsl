#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);

static const uint2 pixel_offsets[4] = {
	uint2(0, 0), uint2(1, 0),
	uint2(0, 1), uint2(1, 1),
};

TEXTURE2D(coverage, uint, TEXSLOT_ONDEMAND0);

RWSTRUCTUREDBUFFER(surfelDataBuffer, SurfelData, 0);
RWRAWBUFFER(surfelStatsBuffer, 1);
RWSTRUCTUREDBUFFER(surfelIndexBuffer, uint, 2);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_coverage = coverage[DTid.xy];

	// Early exit: coverage not applicable (eg. sky)
	if (surfel_coverage == ~0)
		return;

	uint2 pixel = DTid.xy * 16;
	pixel.x += (surfel_coverage >> 4) & 0xF;
	pixel.y += (surfel_coverage >> 0) & 0xF;
	pixel = pixel * 2 + pixel_offsets[g_xFrame_FrameCount % 4];

	uint coverage_amount = surfel_coverage >> 8;

	const float depth = texture_depth[pixel.xy];

	// Early exit: slow down the propagation by chance
	//	Closer surfaces have less chance to avoid excessive clumping of surfels
	const float lineardepth = getLinearDepth(depth) * g_xCamera_ZFarP_rcp;
	const float chance = pow(1 - lineardepth, 4);
	if (blue_noise(DTid.xy).x < chance)
		return;

	if (depth > 0 && coverage_amount < SURFEL_TARGET_COVERAGE)
	{
		const float2 uv = ((float2)pixel.xy + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);


		uint2 g0 = texture_gbuffer0[pixel];
		PrimitiveID prim;
		prim.unpack(g0);
		ShaderMeshInstance inst = InstanceBuffer[prim.instanceIndex];
		ShaderMesh mesh = inst.mesh;
		ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][prim.subsetIndex];
		uint startIndex = prim.primitiveIndex * 3 + subset.indexOffset;
		uint i0 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 0];
		uint i1 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 1];
		uint i2 = bindless_ib[NonUniformResourceIndex(mesh.ib)][startIndex + 2];

		float3 bary = 0;
		[branch]
		if (mesh.vb_pos_nor_wind >= 0)
		{
			uint4 data0 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i0 * 16);
			uint4 data1 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i1 * 16);
			uint4 data2 = bindless_buffers[NonUniformResourceIndex(mesh.vb_pos_nor_wind)].Load4(i2 * 16);
			float4x4 worldMatrix = inst.GetTransform();
			float3 p0 = mul(worldMatrix, float4(asfloat(data0.xyz), 1)).xyz;
			float3 p1 = mul(worldMatrix, float4(asfloat(data1.xyz), 1)).xyz;
			float3 p2 = mul(worldMatrix, float4(asfloat(data2.xyz), 1)).xyz;

			bary = compute_barycentrics(P, p0, p1, p2);
		}




		uint surfel_alloc;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_COUNT, 1, surfel_alloc);
		if (surfel_alloc < SURFEL_CAPACITY)
		{
			SurfelData surfel_data = (SurfelData)0;
			surfel_data.primitiveID = g0;
			surfel_data.bary = bary.xy;
			surfel_data.inconsistency = 1;
			surfelDataBuffer[surfel_alloc] = surfel_data;

			surfelIndexBuffer[surfel_alloc] = surfel_alloc;
		}
	}
}
