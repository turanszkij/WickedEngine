#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D<float2> tile_mindepth_maxcoc : register(t0);
Texture2D<float> tile_mincoc : register(t1);

RWStructuredBuffer<PostprocessTileStatistics> tile_statistics : register(u0);
RWStructuredBuffer<uint> tiles_earlyexit : register(u1);
RWStructuredBuffer<uint> tiles_cheap : register(u2);
RWStructuredBuffer<uint> tiles_expensive : register(u3);
RWTexture2D<float2> output : register(u4);

static const uint tile_replicate = sqr(DEPTHOFFIELD_TILESIZE / 2 / POSTPROCESS_BLOCKSIZE);

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
		InterlockedAdd(tile_statistics[0].dispatch_earlyexit.ThreadGroupCountX, tile_replicate, prevCount);
		tiles_earlyexit[prevCount / tile_replicate] = tile;
	}
	else if (abs(max_coc - min_coc) < 1)
	{
		InterlockedAdd(tile_statistics[0].dispatch_cheap.ThreadGroupCountX, tile_replicate, prevCount);
		tiles_cheap[prevCount / tile_replicate] = tile;
	}
	else
	{
		InterlockedAdd(tile_statistics[0].dispatch_expensive.ThreadGroupCountX, tile_replicate, prevCount);
		tiles_expensive[prevCount / tile_replicate] = tile;
	}
	output[DTid.xy] = float2(min_depth, max_coc); 
}
