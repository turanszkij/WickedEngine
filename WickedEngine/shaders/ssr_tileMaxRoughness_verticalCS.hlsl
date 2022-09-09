#include "globals.hlsli"
#include "brdf.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float2> tile_minmax_roughness_horizontal : register(t0);

RWByteAddressBuffer tile_tracing_statistics : register(u0);
RWStructuredBuffer<uint> tiles_tracing_earlyexit : register(u1);
RWStructuredBuffer<uint> tiles_tracing_cheap : register(u2);
RWStructuredBuffer<uint> tiles_tracing_expensive : register(u3);
RWTexture2D<float2> tile_minmax_roughness : register(u4);

static const float SSRRoughnessCheap = 0.35;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x, DTid.y * SSR_TILESIZE);
	float minRoughness = 1.0;
	float maxRoughness = 0.0;

	int2 dim;
	tile_minmax_roughness_horizontal.GetDimensions(dim.x, dim.y);

	[loop]
	for (uint i = 0; i < SSR_TILESIZE; ++i)
	{
		const uint2 pixel = uint2(tile_upperleft.x, tile_upperleft.y + i);
		if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < dim.x && pixel.y < dim.y)
		{
			float2 minmax_roughness = tile_minmax_roughness_horizontal[pixel];
			minRoughness = min(minRoughness, minmax_roughness.r);
			maxRoughness = max(maxRoughness, minmax_roughness.g);
		}
	}

	const uint tile = (DTid.x & 0xFFFF) | ((DTid.y & 0xFFFF) << 16);

	uint prevCount;
	if (minRoughness < SSRRoughnessCheap)
	{
		tile_tracing_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_EXPENSIVE, 1, prevCount);
		tiles_tracing_expensive[prevCount] = tile;
	}
	else if (maxRoughness > SSRRoughnessCheap && minRoughness < ssr_roughness_cutoff)
	{
		tile_tracing_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_CHEAP, 1, prevCount);
		tiles_tracing_cheap[prevCount] = tile;
	}
	else
	{
		tile_tracing_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_EARLYEXIT, 1, prevCount);
		tiles_tracing_earlyexit[prevCount] = tile;
	}

	tile_minmax_roughness[DTid.xy] = float2(minRoughness, maxRoughness);
}
