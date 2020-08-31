#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

static const uint THREAD_COUNT = 64;

RWTEXTURE2D(output, uint, 0);

#ifdef DEBUG_SHADINGRATECLASSIFICATION
RWTEXTURE2D(output_debug, float4, 1);
#endif // DEBUG_SHADINGRATECLASSIFICATION

groupshared uint tile_rate;

[numthreads(THREAD_COUNT, 1, 1)]
void main( uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex )
{
	if (groupIndex == 0)
	{
		tile_rate = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint2 dim;
	texture_gbuffer1.GetDimensions(dim.x, dim.y);

	const uint2 tile = Gid.xy;

	const uint sqrtile = xShadingRateTileSize * xShadingRateTileSize;

	uint i;
	for (i = groupIndex; i < sqrtile; i += THREAD_COUNT)
	{
		const uint2 tile_pixel = unflatten2D(i, xShadingRateTileSize);
		const uint2 pixel = min(tile * xShadingRateTileSize + tile_pixel, dim - 1);

		const float2 velocity = abs(texture_gbuffer1[pixel].zw);
		const float magnitude = max(velocity.x, velocity.y);

		uint bit_rate = 0;
		if (magnitude > 0.1f)
		{
			// least detailed
			bit_rate = 1 << 0;
		}
		else if (magnitude > 0.01f)
		{
			bit_rate = 1 << 1;
		}
		else if (magnitude > 0.005f)
		{
			bit_rate = 1 << 2;
		}
		else if (magnitude > 0.001f)
		{
			bit_rate = 1 << 3;
		}
		else
		{
			// most detailed
			bit_rate = 1 << 4;
		}
		InterlockedOr(tile_rate, bit_rate);
	}

	GroupMemoryBarrierWithGroupSync();

	uint firstbit = firstbithigh(tile_rate);
	uint rate = 0;
	switch (firstbit)
	{
	case 0:
		rate = SHADING_RATE_4X4;
		break;
	case 1:
		rate = SHADING_RATE_2X4;
		break;
	case 2:
		rate = SHADING_RATE_2X2;
		break;
	case 3:
		rate = SHADING_RATE_1X2;
		break;
	case 4:
	default:
		rate = SHADING_RATE_1X1;
		break;
	}

	if (groupIndex == 0)
	{
		output[tile] = rate;
	}

#ifdef DEBUG_SHADINGRATECLASSIFICATION
	for (i = groupIndex; i < sqrtile; i += THREAD_COUNT)
	{
		const uint2 tile_pixel = unflatten2D(i, xShadingRateTileSize);
		const uint2 pixel = min(tile * xShadingRateTileSize + tile_pixel, dim - 1);

		float4 debugcolor = float4(0, 0, 0, 0);
		float debugalpha = 0.6f;

		if (rate == SHADING_RATE_4X4)
			debugcolor = float4(1, 0, 0, debugalpha);
		else if (rate == SHADING_RATE_2X4 || rate == SHADING_RATE_4X2)
			debugcolor = float4(1, 1, 0, debugalpha);
		else if (rate == SHADING_RATE_2X2)
			debugcolor = float4(0, 1, 0, debugalpha);
		else if (rate == SHADING_RATE_1X2 || rate == SHADING_RATE_2X1)
			debugcolor = float4(0, 0, 1, debugalpha);

		if (tile_pixel.x == 0 || tile_pixel.x == xShadingRateTileSize - 1 || 
			tile_pixel.y == 0 || tile_pixel.y == xShadingRateTileSize - 1)
			debugcolor = float4(0, 0, 0, debugalpha);

		output_debug[pixel] = debugcolor;
	}
#endif // DEBUG_SHADINGRATECLASSIFICATION
}
