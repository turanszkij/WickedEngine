#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> cloud_current : register(t0);
Texture2D<float2> cloud_depth_current : register(t1);

static const int UPSAMPLE_SAMPLE_RADIUS = 1;

#define GAUSSIAN_UPSAMPLE
#define GAUSSIAN_SIGMA_SPATIAL 0.5
#define GAUSSIAN_SIGMA_RANGE 100.0

#define UPSAMPLE_TOLERANCE 0.15

float Gaussian(float x, float sigma)
{
	return exp(-x * x / (2.0 * sigma * sigma));
}

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
	const uint2 pixel = pos.xy;
	const float depth = texture_depth[pixel];
	
	const uint2 reprojectionCoord = pixel / 2;
	
	const float2 reprojectionResolution = postprocess.params0.xy;
	const float2 reprojectionTexelSize = postprocess.params0.zw;

	float3 depthWorldPosition = reconstruct_position(uv, depth);
	float tToDepthBuffer = length(depthWorldPosition - GetCamera().position);
	
	// If sky, set distance to infinite
	tToDepthBuffer = depth == 0.0 ? FLT_MAX : tToDepthBuffer;

	// Adapted from upsample_bilateral_float4CS:
	
	const float2 uv00 = uv - reprojectionTexelSize * 0.5;
	const float2 uv10 = float2(uv00.x + reprojectionTexelSize.x, uv00.y);
	const float2 uv01 = float2(uv00.x, uv00.y + reprojectionTexelSize.y);
	const float2 uv11 = float2(uv00.x + reprojectionTexelSize.x, uv00.y + reprojectionTexelSize.y);

	const float4 lineardepth_lowres = float4(
		cloud_depth_current.SampleLevel(sampler_point_clamp, uv00, 0).g,
		cloud_depth_current.SampleLevel(sampler_point_clamp, uv10, 0).g,
		cloud_depth_current.SampleLevel(sampler_point_clamp, uv01, 0).g,
		cloud_depth_current.SampleLevel(sampler_point_clamp, uv11, 0).g
	);

	const float4 depthDiff = abs(tToDepthBuffer - lineardepth_lowres);	
	float depthDiffMax = max(max(depthDiff.x, depthDiff.y), max(depthDiff.z, depthDiff.w));
	
	bool validResult = false;
	float4 result = 0;
	
	[branch]
	if (depthDiffMax < tToDepthBuffer * 0.2)
	{
		// small error, take bilinear sample:

		validResult = true;
		result = cloud_current.SampleLevel(sampler_linear_clamp, uv, 0);
	}
	else
	{
		// large error, calculate weight and color depending on depth difference with gaussian configuration

		float4 color = 0;
		float weightSum = 0;
				
		[unroll]
		for (int y = -UPSAMPLE_SAMPLE_RADIUS; y <= UPSAMPLE_SAMPLE_RADIUS; y++)
		{
			[unroll]
			for (int x = -UPSAMPLE_SAMPLE_RADIUS; x <= UPSAMPLE_SAMPLE_RADIUS; x++)
			{
				int2 offset = int2(x, y);
			
				int2 neighborReprojectionCoord = reprojectionCoord + offset;
				float2 neighborReprojectionUV = (neighborReprojectionCoord + 0.5) / reprojectionResolution;
			
				float4 cloudResult = cloud_current.SampleLevel(sampler_linear_clamp, neighborReprojectionUV, 0);
				float cloudDepth = cloud_depth_current[neighborReprojectionCoord].g;

#ifdef GAUSSIAN_UPSAMPLE
				float spatialWeight = Gaussian(length(float2(offset)), GAUSSIAN_SIGMA_SPATIAL);
				float rangeWeight = Gaussian(abs(tToDepthBuffer - cloudDepth), GAUSSIAN_SIGMA_RANGE);
				
				float weight = spatialWeight * rangeWeight;
#else
				float currentDepthDiff = abs(tToDepthBuffer - cloudDepth);
				float weight = 1.0f / (currentDepthDiff * UPSAMPLE_TOLERANCE + 1.0f);
#endif
				
				color += cloudResult * weight;
				weightSum += weight;
			}
		}

		validResult = weightSum > 0.0f;
		result = color / weightSum;
	}

	float4 color = validResult ? result : 0;

	return color;
}
