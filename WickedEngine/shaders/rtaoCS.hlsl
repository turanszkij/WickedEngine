#define TEXTURE_SLOT_NONUNIFORM
#define RTAPI
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float3> output_normals : register(u0);
RWStructuredBuffer<uint> output_tiles : register(u1);

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared float2 tile_XY[TILE_SIZE * TILE_SIZE];
groupshared float tile_Z[TILE_SIZE * TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	uint primitiveID = texture_primitiveID[DTid.xy * 2];
	if (!any(primitiveID))
		return;

	uint flatTileIdx = 0;
	if (GTid.y < 4)
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 0), (postprocess.resolution + uint2(7, 3)) / uint2(8, 4));
	}
	else
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 1), (postprocess.resolution + uint2(7, 3)) / uint2(8, 4));
	}
	output_tiles[flatTileIdx] = 0;

	const float2 uv = ((float2)DTid.xy + 0.5) * postprocess.resolution_rcp;
	float2 clipspace = uv * 2 - 1;
	clipspace.y *= -1;
	RayDesc ray = CreateCameraRay(clipspace);

	PrimitiveID prim;
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();
	if (!surface.load(prim, ray.Origin, ray.Direction))
		return;

	float3 P = surface.P;
	float3 N = surface.facenormal;

	ray.TMin = 0.01;
	ray.TMax = rtao_range;
	ray.Origin = P;

	const float2 bluenoise = blue_noise(DTid.xy).xy;

	ray.Direction = normalize(mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), get_tangentspace(N)));

	float shadow = 0;

#ifdef RTAPI
	wiRayQuery q;
	q.TraceRayInline(
		scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
		RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, // uint RayFlags
		asuint(postprocess.params1.x),	// uint InstanceInclusionMask
		ray								// RayDesc Ray
	);
	while (q.Proceed())
	{
		PrimitiveID prim;
		prim.primitiveIndex = q.CandidatePrimitiveIndex();
		prim.instanceIndex = q.CandidateInstanceID();
		prim.subsetIndex = q.CandidateGeometryIndex();

		Surface surface;
		surface.init();
		if (!surface.load(prim, q.CandidateTriangleBarycentrics()))
			break;

		float alphatest = clamp(blue_noise(DTid.xy, q.CandidateTriangleRayT()).r, 0, 0.99);

		[branch]
		if (surface.opacity - alphatest >= 0)
		{
			q.CommitNonOpaqueTriangleHit();
		}
	}
	shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : 1;
#else
	shadow = TraceRay_Any(newRay, asuint(postprocess.params1.x), groupIndex) ? 0 : 1;
#endif // RTAPI

	output_normals[DTid.xy] = saturate(N * 0.5 + 0.5);

	uint2 pixel_pos = DTid.xy;
	int lane_index = (pixel_pos.y % 4) * 8 + (pixel_pos.x % 8);
	uint bit = (shadow > 0) ? (1u << lane_index) : 0;
	InterlockedOr(output_tiles[flatTileIdx], bit);
}
