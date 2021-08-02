#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);

static const uint SURFEL_TARGET_COVERAGE = 1;

static const uint2 pixel_offsets[4] = {
	uint2(0, 0), uint2(1, 0),
	uint2(0, 1), uint2(1, 1),
};

TEXTURE2D(coverage, uint, TEXSLOT_ONDEMAND0);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);
RWRAWBUFFER(surfelStatsBuffer, 1);
RWSTRUCTUREDBUFFER(surfelIndexBuffer, uint, 2);
RWSTRUCTUREDBUFFER(surfelCellIndexBuffer, float, 3); // sorting written for floats

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
	//pixel = pixel * 2 + pixel_offsets[g_xFrame_FrameCount % 4];

	uint coverage_amount = surfel_coverage >> 8;

	const float depth = texture_depth[pixel.xy];

	// Early exit: slow down the propagation by chance
	//	Closer surfaces have less chance to avoid excessive clumping of surfels
	const float lineardepth = getLinearDepth(depth) * g_xCamera_ZFarP_rcp;
	const float chance = pow(1 - lineardepth, 4);
	if (blue_noise(DTid.xy).x < chance)
		return;

	const float4 g1 = texture_gbuffer1[pixel];

	if (depth > 0 && any(g1) && coverage_amount < SURFEL_TARGET_COVERAGE)
	{
		const float2 uv = ((float2)pixel.xy + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);
		const float3 N = normalize(g1.rgb * 2 - 1);




		uint2 primitiveID = texture_gbuffer0[pixel];
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

		//float3 N = 0;
		float3 bary = 0;
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
			//float3 n0 = unpack_unitvector(data0.w);
			//float3 n1 = unpack_unitvector(data1.w);
			//float3 n2 = unpack_unitvector(data2.w);

			bary = compute_barycentrics(P, p0, p1, p2);
			//float u = bary.x;
			//float v = bary.y;
			//float w = bary.z;
			//N = n0 * w + n1 * u + n2 * v;
			//N = mul((float3x3)worldMatrix, N);
			//N = normalize(N);
		}




		uint surfel_alloc;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_COUNT, 1, surfel_alloc);
		if (surfel_alloc < SURFEL_CAPACITY)
		{
			Surfel surfel = (Surfel)0;
			surfel.position = P;
			surfel.normal = pack_unitvector(N);
			surfel.primitiveID = primitiveID;
			surfel.bary = bary.xy;
			surfel.inconsistency = 1;
			surfelBuffer[surfel_alloc] = surfel;

			surfelIndexBuffer[surfel_alloc] = surfel_alloc;
			surfelCellIndexBuffer[surfel_alloc] = surfel_hash(surfel_cell(P));
		}
	}
}
