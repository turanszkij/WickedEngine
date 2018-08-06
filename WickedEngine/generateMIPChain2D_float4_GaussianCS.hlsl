#include "globals.hlsli"
#include "generateMIPChainHF.hlsli"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE2D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2D(input_output, MIP_OUTPUT_FORMAT, 0);

static const uint TILE_BORDER = 4;
static const uint TILE_SIZE = TILE_BORDER + GENERATEMIPCHAIN_2D_BLOCK_SIZE + TILE_BORDER;
groupshared float4 tile[TILE_SIZE][TILE_SIZE];

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	uint i;

	// First, we prewarm the tile cache, including border region:
	const uint2 tile_upperleft = Gid.xy * GENERATEMIPCHAIN_2D_BLOCK_SIZE - TILE_BORDER;
	const uint2 co[] = {
		uint2(0, 0), uint2(1, 0),
		uint2(0, 1), uint2(1, 1)
	};
	for (i = 0; i < 4; ++i)
	{
		const uint2 coord = GTid.xy * 2 + co[i];
		tile[coord.x][coord.y] = input.SampleLevel(sampler_linear_clamp, (tile_upperleft + coord + 1.0f) / (float2)outputResolution.xy, 0);
	}
	GroupMemoryBarrierWithGroupSync();

	const int2 thread_to_cache = GTid.xy + TILE_BORDER;

	float4 sum = 0;

	// Then each thread processes just one pixel within tile, excluding border:

	// Horizontal accumulation for each tile pixel, with help of the border region
	[unroll]
	for (i = 0; i < 9; ++i)
	{
		const uint2 coord = thread_to_cache + int2(gaussianOffsets[i], 0);
		sum += tile[coord.x][coord.y] * gaussianWeightsNormalized[i];
	}

	// write out into cache (excluding border region):
	tile[thread_to_cache.x][thread_to_cache.y] = sum;

	GroupMemoryBarrierWithGroupSync();

	sum = 0;

	// Vertical accumulation for each tile pixel, with help of the border region
	[unroll]
	for (i = 0; i < 9; ++i)
	{
		const uint2 coord = thread_to_cache + int2(0, gaussianOffsets[i]);
		sum += tile[coord.x][coord.y] * gaussianWeightsNormalized[i];
	}


	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y)
	{
		// Each valid thread writes out one pixel:
		input_output[DTid.xy] = sum;
	}
}