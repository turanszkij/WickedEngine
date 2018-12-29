#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE2D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2D(output, MIP_OUTPUT_FORMAT, 0);

static const uint TILE_BORDER = 4;
static const uint TILE_SIZE = TILE_BORDER + GENERATEMIPCHAIN_2D_BLOCK_SIZE + TILE_BORDER;
groupshared float4 tile[TILE_SIZE * TILE_SIZE];

//#define FAKE_GAUSS // this is not completely correct, but two-pass, so faster

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	uint i;

	// First, we prewarm the tile cache, including border region:
	const int2 tile_upperleft = Gid.xy * GENERATEMIPCHAIN_2D_BLOCK_SIZE - TILE_BORDER;
	const int2 co[] = {
		uint2(0, 0), uint2(1, 0),
		uint2(0, 1), uint2(1, 1)
	};
	for (i = 0; i < 4; ++i)
	{
		const int2 coord = GTid.xy * 2 + co[i];
		const float2 uv = (tile_upperleft + coord + 0.5f) / (float2)outputResolution.xy;
		tile[flatten2D(coord, TILE_SIZE)] = input.SampleLevel(sampler_linear_clamp, uv, 0);
	}
	GroupMemoryBarrierWithGroupSync();

	const int2 thread_to_cache = GTid.xy + TILE_BORDER;

	float4 sum = 0;


#ifdef FAKE_GAUSS

	// Horizontal accumulation for each tile pixel, with help of the border region
	for (i = 0; i < 9; ++i)
	{
		const uint2 coord = thread_to_cache + int2(gaussianOffsets[i], 0);
		sum += tile[flatten2D(coord, TILE_SIZE)] * gaussianWeightsNormalized[i];
	}

	GroupMemoryBarrierWithGroupSync();

	// write out into cache (excluding border region):
	tile[flatten2D(thread_to_cache, TILE_SIZE)] = sum;

	GroupMemoryBarrierWithGroupSync();

	sum = 0;

	// Vertical accumulation for each tile pixel, with help of the border region
	for (i = 0; i < 9; ++i)
	{
		const uint2 coord = thread_to_cache + int2(0, gaussianOffsets[i]);
		sum += tile[flatten2D(coord, TILE_SIZE)] * gaussianWeightsNormalized[i];
	}

#else

	for (i = 0; i < 9; ++i)
	{
		float4 sumY = 0;
		for (uint j = 0; j < 9; ++j)
		{
			const uint2 coord = thread_to_cache + int2(gaussianOffsets[i], gaussianOffsets[j]);
			sumY += tile[flatten2D(coord, TILE_SIZE)] * gaussianWeightsNormalized[j];
		}
		sum += sumY * gaussianWeightsNormalized[i];
	}

#endif // FAKE_GAUSS


	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y)
	{
		// Each valid thread writes out one pixel:
		output[DTid.xy] = sum;

		//const int2 a = max(TILE_BORDER, Gid.xy * GENERATEMIPCHAIN_2D_BLOCK_SIZE) - TILE_BORDER;
		//const int2 b = Gid.xy * GENERATEMIPCHAIN_2D_BLOCK_SIZE - TILE_BORDER;
		//output[DTid.xy] = float4((a-b) / (float2)outputResolution, 0, 1);
	}
}