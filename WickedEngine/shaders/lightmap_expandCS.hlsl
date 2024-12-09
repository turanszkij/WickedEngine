#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D lightmap_input : register(t0);

RWTexture2D<float4> lightmap_output : register(u0);

static const int TILE_BORDER = 4;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared uint2 tile_cache[TILE_SIZE*TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const int2 tile_upperleft = Gid.xy * POSTPROCESS_BLOCKSIZE - TILE_BORDER;
	for (uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		tile_cache[t] = pack_half4(lightmap_input[pixel]);
	}
	GroupMemoryBarrierWithGroupSync();

	float4 color = unpack_half4(tile_cache[flatten2D(GTid.xy + TILE_BORDER, TILE_SIZE)]);

	if (color.a < 1)
	{
		// spin outwards from center in spiral pattern and take the first sample which has valid opacity:
		int generation = TILE_BORDER;
		for (int growth = 0; (growth < generation) && (color.a < 1); ++growth)
		{
			const int side = 2 * (growth + 1);
			int x = -growth - 1;
			int y = -growth - 1;
			for (int i = 0; (i < side) && (color.a < 1); ++i)
			{
				color = unpack_half4(tile_cache[flatten2D(GTid.xy + TILE_BORDER + int2(x, y), TILE_SIZE)]);
				x++;
			}
			for (int i = 0; (i < side) && (color.a < 1); ++i)
			{
				color = unpack_half4(tile_cache[flatten2D(GTid.xy + TILE_BORDER + int2(x, y), TILE_SIZE)]);
				y++;
			}
			for (int i = 0; (i < side) && (color.a < 1); ++i)
			{
				color = unpack_half4(tile_cache[flatten2D(GTid.xy + TILE_BORDER + int2(x, y), TILE_SIZE)]);
				x--;
			}
			for (int i = 0; (i < side) && (color.a < 1); ++i)
			{
				color = unpack_half4(tile_cache[flatten2D(GTid.xy + TILE_BORDER + int2(x, y), TILE_SIZE)]);
				y--;
			}
		}
	}
	
	lightmap_output[DTid.xy] = color;
}
