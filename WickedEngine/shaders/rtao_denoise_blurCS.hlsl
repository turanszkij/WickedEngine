#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// This is a bilateral blur, that also operates on normal buffer, not just depth

#define BLUR_FORMAT unorm float

TEXTURE2D(input, BLUR_FORMAT, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_normals, float3, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, BLUR_FORMAT, 0);

// Calculate gaussian weights online: http://dev.theomader.com/gaussian-kernel-calculator/
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

static const int TILE_BORDER = GAUSS_KERNEL / 2;
static const int CACHE_SIZE = TILE_BORDER + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT + TILE_BORDER;
groupshared BLUR_FORMAT color_cache[CACHE_SIZE];

groupshared float depth_cache[CACHE_SIZE];
groupshared float3 normal_cache[CACHE_SIZE];

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
		depth_cache[i] = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0);
		normal_cache[i] = texture_normals.SampleLevel(sampler_point_clamp, uv, 0) * 2 - 1;
	}
	GroupMemoryBarrierWithGroupSync();

	const uint2 pixel = tile_start + groupIndex * direction;
	if (pixel.x >= xPPResolution.x || pixel.y >= xPPResolution.y)
	{
		return;
	}

	const uint center = TILE_BORDER + groupIndex;

	const float depth_threshold = xPPParams0.w;
	const float center_depth = depth_cache[center];
	const float3 center_normal = normal_cache[center];
	const BLUR_FORMAT center_color = color_cache[center];

	BLUR_FORMAT color = 0;
	for (i = 0; i < GAUSS_KERNEL; ++i)
	{
		const uint sam = center + gaussianOffsets[i];
		const BLUR_FORMAT color2 = color_cache[sam];
		const float depth = depth_cache[sam];
		const float3 normal = normal_cache[sam];
		const float weight1 = saturate(abs(depth - center_depth) * g_xCamera_ZFarP * depth_threshold);
		const float weight2 = 1 - saturate(dot(normal, center_normal));
		color += lerp(color2, center_color, max(weight1, weight2)) * gaussianWeightsNormalized[i];
	}

	output[pixel] = color;
}
