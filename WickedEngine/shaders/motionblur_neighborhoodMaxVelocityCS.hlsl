#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(tilemax, float2, TEXSLOT_ONDEMAND0);
TEXTURE2D(tilemin, float2, TEXSLOT_ONDEMAND1);

RWRAWBUFFER(tile_statistics, 0);
RWSTRUCTUREDBUFFER(tiles_earlyexit, uint, 1);
RWSTRUCTUREDBUFFER(tiles_cheap, uint, 2);
RWSTRUCTUREDBUFFER(tiles_expensive, uint, 3);
RWTEXTURE2D(output, float2, 4);

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
		tile_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_EARLYEXIT, 1, prevCount);
		tiles_earlyexit[prevCount] = tile;
	}
	else if (abs(max_magnitude - min_magnitude) < 0.002f)
	{
		tile_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_CHEAP, 1, prevCount);
		tiles_cheap[prevCount] = tile;
		output[DTid.xy] = max_velocity;
	}
	else
	{
		tile_statistics.InterlockedAdd(TILE_STATISTICS_OFFSET_EXPENSIVE, 1, prevCount);
		tiles_expensive[prevCount] = tile;
		output[DTid.xy] = max_velocity;
	}
}