#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(shadingrate, ShadingRateClassification);

static const uint THREAD_COUNT = 64;

RWTexture2D<uint> output : register(u0);

#ifdef DEBUG_SHADINGRATECLASSIFICATION
RWTexture2D<float4> output_debug : register(u1);
#endif // DEBUG_SHADINGRATECLASSIFICATION

groupshared uint tile_rate;

[numthreads(THREAD_COUNT, 1, 1)]
void main( uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex )
{
	if (groupIndex == 0)
	{
		tile_rate = 0xFF;
	}
	GroupMemoryBarrierWithGroupSync();

	uint2 dim;
	texture_gbuffer1.GetDimensions(dim.x, dim.y);

	const uint2 tile = Gid.xy;

	const uint sqrtile = shadingrate.TileSize * shadingrate.TileSize;

	uint i;
	for (i = groupIndex; i < sqrtile; i += THREAD_COUNT)
	{
		const uint2 tile_pixel = unflatten2D(i, shadingrate.TileSize);
		const uint2 pixel = min(tile * shadingrate.TileSize + tile_pixel, dim - 1);

		const float2 velocity = abs(texture_gbuffer1[pixel].xy);
		const float magnitude = max(velocity.x, velocity.y);

		uint rate = 0;
		if (magnitude > 0.1f)
		{
			// least detailed
			rate = shadingrate.SHADING_RATE_4X4;
		}
		else if (magnitude > 0.01f)
		{
			rate = shadingrate.SHADING_RATE_4X2;
		}
		else if (magnitude > 0.005f)
		{
			rate = shadingrate.SHADING_RATE_2X2;
		}
		else if (magnitude > 0.001f)
		{
			rate = shadingrate.SHADING_RATE_2X1;
		}
		else
		{
			// most detailed
			rate = shadingrate.SHADING_RATE_1X1;
		}
		InterlockedMin(tile_rate, rate);
	}

	GroupMemoryBarrierWithGroupSync();

	uint rate = tile_rate;
	if (groupIndex == 0)
	{
		output[tile] = rate;
	}

#ifdef DEBUG_SHADINGRATECLASSIFICATION
	for (i = groupIndex; i < sqrtile; i += THREAD_COUNT)
	{
		const uint2 tile_pixel = unflatten2D(i, shadingrate.TileSize);
		const uint2 pixel = min(tile * shadingrate.TileSize + tile_pixel, dim - 1);

		float4 debugcolor = float4(0, 0, 0, 0);
		float debugalpha = 0.6f;

		if (rate == shadingrate.SHADING_RATE_4X4)
			debugcolor = float4(1, 0, 0, debugalpha);
		else if (rate == shadingrate.SHADING_RATE_2X4 || rate == shadingrate.SHADING_RATE_4X2)
			debugcolor = float4(1, 1, 0, debugalpha);
		else if (rate == shadingrate.SHADING_RATE_2X2)
			debugcolor = float4(0, 1, 0, debugalpha);
		else if (rate == shadingrate.SHADING_RATE_1X2 || rate == shadingrate.SHADING_RATE_2X1)
			debugcolor = float4(0, 0, 1, debugalpha);

		if (tile_pixel.x == 0 || tile_pixel.x == shadingrate.TileSize - 1 || 
			tile_pixel.y == 0 || tile_pixel.y == shadingrate.TileSize - 1)
			debugcolor = float4(0, 0, 0, debugalpha);

		output_debug[pixel] = debugcolor;
	}
#endif // DEBUG_SHADINGRATECLASSIFICATION
}
