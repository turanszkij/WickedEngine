#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D<float2> tilemax : register(t0);
Texture2D<float2> tilemin : register(t1);

RWStructuredBuffer<PostprocessTileStatistics> tile_statistics : register(u0);
RWStructuredBuffer<uint> tiles_earlyexit : register(u1);
RWStructuredBuffer<uint> tiles_cheap : register(u2);
RWStructuredBuffer<uint> tiles_expensive : register(u3);
RWTexture2D<float2> output : register(u4);

static const uint tile_replicate = sqr(MOTIONBLUR_TILESIZE / POSTPROCESS_BLOCKSIZE);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float max_magnitude = 0;
	float2 max_velocity = 0;
	float min_magnitude = 100000;
	float2 min_velocity = 100000;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float2 velocity = tilemax[DTid.xy + int2(x, y)];
			float magnitude = length(velocity);
			if (magnitude > max_magnitude)
			{
				max_magnitude = magnitude;
				max_velocity = velocity;
			}

			velocity = tilemin[DTid.xy + int2(x, y)];
			magnitude = length(velocity);
			if (magnitude < min_magnitude)
			{
				min_magnitude = magnitude;
				min_velocity = velocity;
			}
		}
	}

	const uint tile = (DTid.x & 0xFFFF) | ((DTid.y & 0xFFFF) << 16);

	uint prevCount;
	if (max_magnitude < 0.0001f)
	{
		InterlockedAdd(tile_statistics[0].dispatch_earlyexit.ThreadGroupCountX, tile_replicate, prevCount);
		tiles_earlyexit[prevCount / tile_replicate] = tile;
	}
	else if (abs(max_magnitude - min_magnitude) < 0.002f)
	{
		InterlockedAdd(tile_statistics[0].dispatch_cheap.ThreadGroupCountX, tile_replicate, prevCount);
		tiles_cheap[prevCount / tile_replicate] = tile;
		output[DTid.xy] = max_velocity;
	}
	else
	{
		InterlockedAdd(tile_statistics[0].dispatch_expensive.ThreadGroupCountX, tile_replicate, prevCount);
		tiles_expensive[prevCount / tile_replicate] = tile;
		output[DTid.xy] = max_velocity;
	}
}
