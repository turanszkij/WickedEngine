#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE2D(output, uint, 0);

#ifdef DEBUG_SHADINGRATECLASSIFICATION
RWTEXTURE2D(output_debug, float4, 1);
#endif // DEBUG_SHADINGRATECLASSIFICATION

// TODO optimize this shader by removing loops
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint2 tile = DTid.xy;
	const uint2 pixel_upperleft = tile * xShadingRateTileSize;

	uint2 dim;
	texture_gbuffer1.GetDimensions(dim.x, dim.y);

	float magnitude = 1000000;
	uint i, j;
	[loop]
	for (i = 0; i < xShadingRateTileSize; ++i)
	{
		[loop]
		for (j = 0; j < xShadingRateTileSize; ++j)
		{
			const uint2 pixel = min(pixel_upperleft + uint2(i, j), dim - 1);
			const float2 velocity = abs(texture_gbuffer1[pixel].zw);
			magnitude = min(magnitude, max(velocity.x, velocity.y));
		}
	}

	uint rate = 0;
	if (magnitude > 0.1f)
	{
		rate = SHADING_RATE_4X4; // least detailed
	}
	else if (magnitude > 0.01f)
	{
		rate = SHADING_RATE_2X4;
	}
	else if (magnitude > 0.005f)
	{
		rate = SHADING_RATE_2X2;
	}
	else if (magnitude > 0.001f)
	{
		rate = SHADING_RATE_1X2;
	}
	else
	{
		rate = SHADING_RATE_1X1; // most detailed
	}

	output[tile] = rate;

#ifdef DEBUG_SHADINGRATECLASSIFICATION
	[loop]
	for (i = 0; i < xShadingRateTileSize; ++i)
	{
		[loop]
		for (j = 0; j < xShadingRateTileSize; ++j)
		{
			float4 debugcolor = float4(0, 0, 0, 0);
			float debugalpha = 0.6f;

			if(rate == SHADING_RATE_4X4)
				debugcolor = float4(1, 0, 0, debugalpha);
			else if(rate == SHADING_RATE_2X4 || rate == SHADING_RATE_4X2)
				debugcolor = float4(1, 1, 0, debugalpha);
			else if (rate == SHADING_RATE_2X2)
				debugcolor = float4(0, 1, 0, debugalpha);
			else if (rate == SHADING_RATE_1X2 || rate == SHADING_RATE_2X1)
				debugcolor = float4(0, 0, 1, debugalpha);

			if (i == 0 || i == xShadingRateTileSize - 1 || j == 0 || j == xShadingRateTileSize - 1)
				debugcolor = float4(0, 0, 0, debugalpha);

			const uint2 pixel = pixel_upperleft + uint2(i, j);
			output_debug[pixel] = debugcolor;
		}
	}
#endif // DEBUG_SHADINGRATECLASSIFICATION
}
