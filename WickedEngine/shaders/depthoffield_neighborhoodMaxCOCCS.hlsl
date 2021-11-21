#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D<float2> tile_mindepth_maxcoc : register(t0);
Texture2D<float> tile_mincoc : register(t1);

RWByteAddressBuffer tile_statistics : register(u0);
RWStructuredBuffer<uint> tiles_earlyexit : register(u1);
RWStructuredBuffer<uint> tiles_cheap : register(u2);
RWStructuredBuffer<uint> tiles_expensive : register(u3);
RWTexture2D<float2> output : register(u4);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float min_depth = 1;
	float max_coc = 0;
	float min_coc = 1000000;

	int2 dim;
	tile_mindepth_maxcoc.GetDimensions(dim.x, dim.y);

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const int2 pixel = DTid.xy + int2(x, y);
			if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < dim.x && pixel.y < dim.y)
			{
				const float2 mindepth_maxcoc = tile_mindepth_maxcoc[pixel];
				min_depth = min(min_depth, mindepth_maxcoc.x);
				max_coc = max(max_coc, mindepth_maxcoc.y);
				min_coc = min(min_coc, tile_mincoc[pixel]);
			}
		}
	}

	const uint tile = (DTid.x & 0xFFFF) | ((DTid.y & 0xFFFF) << 16);

	uint prevCount;
	if (max_coc < 1)
	{
		tile_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_EARLYEXIT, 1, prevCount);
		tiles_earlyexit[prevCount] = tile;
	}
	else if (abs(max_coc - min_coc) < 1)
	{
		tile_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_CHEAP, 1, prevCount);
		tiles_cheap[prevCount] = tile;
	}
	else
	{
		tile_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_EXPENSIVE, 1, prevCount);
		tiles_expensive[prevCount] = tile;
	}
	output[DTid.xy] = float2(min_depth, max_coc); 
}
