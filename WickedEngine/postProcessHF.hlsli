#ifndef _POSTPROCESS_HF_
#define _POSTPROCESS_HF_

#include "imageHF.hlsli"
#include "packHF.hlsli"
#include "depthConvertHF.hlsli"


float2 GetVelocity(in int2 pixel)
{
#ifdef DILATE_VELOCITY_BEST_3X3 // search best velocity in 3x3 neighborhood

	float bestDepth = 1;
	int2 bestPixel = int2(0, 0);

	[loop]
	for (int i = -1; i <= 1; ++i)
	{
		[unroll]
		for (int j = -1; j <= 1; ++j)
		{
			int2 curPixel = pixel + int2(i, j);
			float depth = texture_lineardepth[curPixel];
			[flatten]
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestPixel = curPixel;
			}
		}
	}

	return texture_gbuffer1[bestPixel].zw;

#elif defined DILATE_VELOCITY_BEST_FAR // search best velocity in a far reaching 5-tap pattern

	float bestDepth = 1;
	int2 bestPixel = int2(0, 0);

	// top-left
	int2 curPixel = pixel + int2(-2, -2);
	float depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// top-right
	curPixel = pixel + int2(2, -2);
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// bottom-right
	curPixel = pixel + int2(2, 2);
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// bottom-left
	curPixel = pixel + int2(-2, 2);
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	// center
	curPixel = pixel;
	depth = texture_lineardepth[curPixel];
	[flatten]
	if (depth < bestDepth)
	{
		bestDepth = depth;
		bestPixel = curPixel;
	}

	return texture_gbuffer1[bestPixel].zw;

#elif defined DILATE_VELOCITY_AVG_FAR

	float2 velocity_TL = texture_gbuffer1[pixel + int2(-2, -2)].zw;
	float2 velocity_TR = texture_gbuffer1[pixel + int2(2, -2)].zw;
	float2 velocity_BL = texture_gbuffer1[pixel + int2(-2, 2)].zw;
	float2 velocity_BR = texture_gbuffer1[pixel + int2(2, 2)].zw;
	float2 velocity_CE = texture_gbuffer1[pixel].zw;

	return (velocity_TL + velocity_TR + velocity_BL + velocity_BR + velocity_CE) / 5.0f;

#else

	return texture_gbuffer1[pixel].zw;

#endif // DILATE_VELOCITY
}

#endif // _POSTPROCESS_HF_
