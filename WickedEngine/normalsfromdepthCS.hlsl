#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// This shader takes depth buffer and converts it to a flat normals buffer (world space)
//	Output is unsigned float3 in [0, 1] range

RWTEXTURE2D(output, float3, 0);

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared float2 tile_XY[TILE_SIZE * TILE_SIZE];
groupshared float tile_Z[TILE_SIZE * TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const int2 tile_upperleft = Gid.xy * POSTPROCESS_BLOCKSIZE - TILE_BORDER;
	for (uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		const float2 uv = (pixel + 0.5f) * xPPResolution_rcp;
		const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
		const float3 position = reconstructPosition(uv, depth);
		tile_XY[t] = position.xy;
		tile_Z[t] = position.z;
	}
	GroupMemoryBarrierWithGroupSync();

	// reconstruct flat normals from depth buffer:
	//	Explanation: There are two main ways to reconstruct flat normals from depth buffer:
	//		1: use ddx() and ddy() on the reconstructed positions to compute triangle, this has artifacts on depth discontinuities and doesn't work in compute shader
	//		2: Take 3 taps from the depth buffer, reconstruct positions and compute triangle. This can still produce artifacts
	//			on discontinuities, but a little less. To fix the remaining artifacts, we can take 4 taps around the center, and find the "best triangle"
	//			by only computing the positions from those depths that have the least amount of discontinuity

	const uint cross_idx[5] = {
		flatten2D(TILE_BORDER + GTid.xy, TILE_SIZE),				// 0: center
		flatten2D(TILE_BORDER + GTid.xy + int2(1, 0), TILE_SIZE),	// 1: right
		flatten2D(TILE_BORDER + GTid.xy + int2(-1, 0), TILE_SIZE),	// 2: left
		flatten2D(TILE_BORDER + GTid.xy + int2(0, 1), TILE_SIZE),	// 3: down
		flatten2D(TILE_BORDER + GTid.xy + int2(0, -1), TILE_SIZE),	// 4: up
	};

	const float center_Z = tile_Z[cross_idx[0]];

	[branch]
	if (center_Z >= g_xCamera_ZFarP)
		return;

	const uint best_Z_horizontal = abs(tile_Z[cross_idx[1]] - center_Z) < abs(tile_Z[cross_idx[2]] - center_Z) ? 1 : 2;
	const uint best_Z_vertical = abs(tile_Z[cross_idx[3]] - center_Z) < abs(tile_Z[cross_idx[4]] - center_Z) ? 3 : 4;

	float3 p1 = 0, p2 = 0;
	if (best_Z_horizontal == 1 && best_Z_vertical == 4)
	{
		p1 = float3(tile_XY[cross_idx[1]], tile_Z[cross_idx[1]]);
		p2 = float3(tile_XY[cross_idx[4]], tile_Z[cross_idx[4]]);
	}
	else if (best_Z_horizontal == 1 && best_Z_vertical == 3)
	{
		p1 = float3(tile_XY[cross_idx[3]], tile_Z[cross_idx[3]]);
		p2 = float3(tile_XY[cross_idx[1]], tile_Z[cross_idx[1]]);
	}
	else if (best_Z_horizontal == 2 && best_Z_vertical == 4)
	{
		p1 = float3(tile_XY[cross_idx[4]], tile_Z[cross_idx[4]]);
		p2 = float3(tile_XY[cross_idx[2]], tile_Z[cross_idx[2]]);
	}
	else if (best_Z_horizontal == 2 && best_Z_vertical == 3)
	{
		p1 = float3(tile_XY[cross_idx[2]], tile_Z[cross_idx[2]]);
		p2 = float3(tile_XY[cross_idx[3]], tile_Z[cross_idx[3]]);
	}

	const float3 P = float3(tile_XY[cross_idx[0]], tile_Z[cross_idx[0]]);
	const float3 normal = normalize(cross(p2 - P, p1 - P));

	output[DTid.xy] = saturate(normal * 0.5f + 0.5f);
}
