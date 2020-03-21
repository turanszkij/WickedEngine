#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifndef BLUR_FORMAT
#define BLUR_FORMAT float4
#endif // BLUR_FORMAT

TEXTURE2D(input, BLUR_FORMAT, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, BLUR_FORMAT, 0);

// Calculate gaussian weights online: http://dev.theomader.com/gaussian-kernel-calculator/
#ifdef BLUR_WIDE
static const int GAUSS_KERNEL = 33;
static const float gaussianWeightsNormalized[GAUSS_KERNEL] = {
	0.004013,
	0.005554,
	0.007527,
	0.00999,
	0.012984,
	0.016524,
	0.020594,
	0.025133,
	0.030036,
	0.035151,
	0.040283,
	0.045207,
	0.049681,
	0.053463,
	0.056341,
	0.058141,
	0.058754,
	0.058141,
	0.056341,
	0.053463,
	0.049681,
	0.045207,
	0.040283,
	0.035151,
	0.030036,
	0.025133,
	0.020594,
	0.016524,
	0.012984,
	0.00999,
	0.007527,
	0.005554,
	0.004013
};
static const int gaussianOffsets[GAUSS_KERNEL] = {
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
};
#else
static const int GAUSS_KERNEL = 9;
static const float gaussianWeightsNormalized[GAUSS_KERNEL] = {
	0.004112,
	0.026563,
	0.100519,
	0.223215,
	0.29118,
	0.223215,
	0.100519,
	0.026563,
	0.004112
};
static const int gaussianOffsets[GAUSS_KERNEL] = {
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4
};
#endif // BLUR_WIDE

static const int TILE_BORDER = GAUSS_KERNEL / 2;
static const int CACHE_SIZE = TILE_BORDER + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT + TILE_BORDER;
groupshared BLUR_FORMAT color_cache[CACHE_SIZE];

#ifdef BILATERAL
groupshared float depth_cache[CACHE_SIZE];
#endif // BILATERAL

[numthreads(POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	float2 direction = xPPParams0.xy;
	const bool horizontal = direction.y == 0;
	
	uint2 tile_start = Gid.xy;
	[flatten]
	if (horizontal)
	{
		tile_start.x *= POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT;
	}
	else
	{
		tile_start.y *= POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT;
	}

	int i;
	for (i = groupIndex; i < CACHE_SIZE; i += POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT)
	{
		const float2 uv = (tile_start + 0.5f + direction * (i - TILE_BORDER)) * xPPResolution_rcp;
		color_cache[i] = input.SampleLevel(sampler_linear_clamp, uv, 0);
#ifdef BILATERAL
		depth_cache[i] = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0);
#endif // BILATERAL
	}
	GroupMemoryBarrierWithGroupSync();

	const uint2 pixel = tile_start + groupIndex * direction;
	if (pixel.x >= xPPResolution.x || pixel.y >= xPPResolution.y)
	{
		return;
	}
	
	const uint center = TILE_BORDER + groupIndex;

#ifdef BILATERAL
	const float depth_threshold = xPPParams0.w;
	const float center_depth = depth_cache[center];
	const BLUR_FORMAT center_color = color_cache[center];
#endif // BILATERAL

	BLUR_FORMAT color = 0;
	for (i = 0; i < GAUSS_KERNEL; ++i)
	{
		const uint sam = center + gaussianOffsets[i];
		const BLUR_FORMAT color2 = color_cache[sam];
#ifdef BILATERAL
		const float depth = depth_cache[sam];
		const float weight = saturate(abs(depth - center_depth) * g_xCamera_ZFarP * depth_threshold);
		color += lerp(color2, center_color, weight) * gaussianWeightsNormalized[i];
#else
		color += color2 * gaussianWeightsNormalized[i];
#endif // BILATERAL
	}

	output[pixel] = color;
}
