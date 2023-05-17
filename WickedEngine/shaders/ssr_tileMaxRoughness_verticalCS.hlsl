#include "globals.hlsli"
#include "brdf.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float2> tile_minmax_roughness_horizontal : register(t0);

RWStructuredBuffer<PostprocessTileStatistics> tile_tracing_statistics : register(u0);
RWStructuredBuffer<uint> tiles_tracing_earlyexit : register(u1);
RWStructuredBuffer<uint> tiles_tracing_cheap : register(u2);
RWStructuredBuffer<uint> tiles_tracing_expensive : register(u3);
RWTexture2D<float2> tile_minmax_roughness : register(u4);

static const float SSRRoughnessCheap = 0.35;
static const uint tile_tracing_replicate = sqr(SSR_TILESIZE / 2 / POSTPROCESS_BLOCKSIZE);

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
		InterlockedAdd(tile_tracing_statistics[0].dispatch_expensive.ThreadGroupCountX, tile_tracing_replicate, prevCount);
		tiles_tracing_expensive[prevCount / tile_tracing_replicate] = tile;
	}
	else if (maxRoughness > SSRRoughnessCheap && minRoughness < ssr_roughness_cutoff)
	{
		InterlockedAdd(tile_tracing_statistics[0].dispatch_cheap.ThreadGroupCountX, tile_tracing_replicate, prevCount);
		tiles_tracing_cheap[prevCount / tile_tracing_replicate] = tile;
	}
	else
	{
		InterlockedAdd(tile_tracing_statistics[0].dispatch_earlyexit.ThreadGroupCountX, tile_tracing_replicate, prevCount);
		tiles_tracing_earlyexit[prevCount / tile_tracing_replicate] = tile;
	}

	tile_minmax_roughness[DTid.xy] = float2(minRoughness, maxRoughness);
}
