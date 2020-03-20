#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifndef BLUR_FORMAT
#define BLUR_FORMAT float4
#endif // BLUR_FORMAT

TEXTURE2D(input, BLUR_FORMAT, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, BLUR_FORMAT, 0);

static const int TILE_BORDER = 4;
static const int CACHE_SIZE = TILE_BORDER + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT + TILE_BORDER;
groupshared BLUR_FORMAT color_cache[CACHE_SIZE];

#ifdef BILATERAL
groupshared float depth_cache[CACHE_SIZE];
#endif // BILATERAL

static const float gaussWeight0 = 1.0f;
static const float gaussWeight1 = 0.9f;
static const float gaussWeight2 = 0.55f;
static const float gaussWeight3 = 0.18f;
static const float gaussWeight4 = 0.1f;
static const float gaussNormalization = 1.0f / (gaussWeight0 + 2.0f * (gaussWeight1 + gaussWeight2 + gaussWeight3 + gaussWeight4));
static const float gaussianWeightsNormalized[9] = {
	gaussWeight4 * gaussNormalization,
	gaussWeight3 * gaussNormalization,
	gaussWeight2 * gaussNormalization,
	gaussWeight1 * gaussNormalization,
	gaussWeight0 * gaussNormalization,
	gaussWeight1 * gaussNormalization,
	gaussWeight2 * gaussNormalization,
	gaussWeight3 * gaussNormalization,
	gaussWeight4 * gaussNormalization,
};
static const int gaussianOffsets[9] = {
	-4, -3, -2, -1, 0, 1, 2, 3, 4
};

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
	for (i = 0; i < 9; ++i)
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
