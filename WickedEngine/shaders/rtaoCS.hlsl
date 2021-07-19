#define RTAPI
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

RWTEXTURE2D(output, unorm float, 0);
RWTEXTURE2D(output_normals, float3, 1);
RWSTRUCTUREDBUFFER(output_tiles, uint, 2);

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared float2 tile_XY[TILE_SIZE * TILE_SIZE];
groupshared float tile_Z[TILE_SIZE * TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	// ray traced ao works better with GBUFFER normal:
	//	Reprojection issues are mostly solved by denoiser anyway
	const float2 uv = ((float2)DTid.xy + 0.5) * xPPResolution_rcp;
	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;
	const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, prevUV, 0);
	const float3 N = normalize(g1.rgb * 2 - 1);

	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	const float3 P = reconstructPosition(uv, depth);

	uint flatTileIdx = 0;
	if (GTid.y < 4)
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 0), (xPPResolution + uint2(7, 3)) / uint2(8, 4));
	}
	else
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 1), (xPPResolution + uint2(7, 3)) / uint2(8, 4));
	}
	output_tiles[flatTileIdx] = 0;

	RayDesc ray;
	ray.TMin = 0.01;
	ray.TMax = rtao_range;
	ray.Origin = P;

	const float2 bluenoise = blue_noise(DTid.xy).xy;

	ray.Direction = normalize(mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), GetTangentSpace(N)));

	float shadow = 0;

#ifdef RTAPI
	RayQuery<
		RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
	> q;
	q.TraceRayInline(
		scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
		0,								// uint RayFlags
		0xFF,							// uint InstanceInclusionMask
		ray								// RayDesc Ray
	);
	while (q.Proceed())
	{
		ShaderMesh mesh = bindless_buffers[NonUniformResourceIndex(q.CandidateInstanceID())].Load<ShaderMesh>(0);
		ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][q.CandidateGeometryIndex()];
		ShaderMaterial material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);
		[branch]
		if (!material.IsCastingShadow())
		{
			continue;
		}
		[branch]
		if (material.texture_basecolormap_index < 0)
		{
			q.CommitNonOpaqueTriangleHit();
			break;
		}

		Surface surface;
		EvaluateObjectSurface(
			mesh,
			subset,
			material,
			q.CandidatePrimitiveIndex(),
			q.CandidateTriangleBarycentrics(),
			q.CandidateObjectToWorld3x4(),
			surface
		);

		[branch]
		if (surface.opacity >= material.alphaTest)
		{
			q.CommitNonOpaqueTriangleHit();
			break;
		}
	}
	shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : 1;
#else
	shadow = TraceRay_Any(newRay, groupIndex) ? 0 : 1;
#endif // RTAPI

	output[DTid.xy] = pow(saturate(shadow), rtao_power);
	output_normals[DTid.xy] = saturate(N * 0.5 + 0.5);

	uint2 pixel_pos = DTid.xy;
	int lane_index = (pixel_pos.y % 4) * 8 + (pixel_pos.x % 8);
	uint bit = (shadow > 0) ? (1u << lane_index) : 0;
	InterlockedOr(output_tiles[flatTileIdx], bit);
}
