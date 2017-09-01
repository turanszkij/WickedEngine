#ifndef _POSTPROCESS_HF_
#define _POSTPROCESS_HF_

#include "imageHF.hlsli"
#include "packHF.hlsli"
#include "depthConvertHF.hlsli"


float2 GetVelocity(in int2 pixel)
{
#ifdef DILATE_VELOCITY
	float bestDepth = g_xFrame_MainCamera_ZFarP;
	int2 bestPixel = int2(0, 0);

	[unroll]
	for (int i = -1; i <= 1; ++i)
	{
		[unroll]
		for (int j = -1; j <= 1; ++j)
		{
			float depth = texture_lineardepth[pixel];
			[flatten]
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestPixel = pixel + int2(i, j);
			}
		}
	}

	return texture_gbuffer1[bestPixel].zw;
#else
	return texture_gbuffer1[pixel].zw;
#endif // DILATE_VELOCITY
}

float loadDepth(float2 texCoord)
{
	float2 dim;
	texture_lineardepth.GetDimensions(dim.x, dim.y);
	return texture_lineardepth.Load(int3(dim*texCoord, 0)).r;
}
float4 loadMask(float2 texCoord)
{
	float2 dim;
	texture_1.GetDimensions(dim.x, dim.y);
	return texture_1.Load(int3(dim*texCoord, 0));
}
float4 loadScene(float2 texCoord)
{
	float2 dim;
	texture_0.GetDimensions(dim.x, dim.y);
	return texture_0.Load(int3(dim*texCoord, 0));
}

#endif // _POSTPROCESS_HF_
