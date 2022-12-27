#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> texture_color_current : register(t0);
Texture2D<float4> texture_color_history : register(t1);
Texture2D<float> texture_variance_current : register(t2);
Texture2D<float> texture_variance_history : register(t3);

RWTexture2D<float4> output_color : register(u0);
RWTexture2D<float> output_variance : register(u1);

static const float temporalResponse = 0.98;
static const float temporalScale = 0.9;
static const float disocclusionDepthWeight = 1.0f;
static const float disocclusionThreshold = 0.89f;
static const float varianceTemporalResponse = 0.6f;

float2 CalculateReprojectionBuffer(float2 uv, float depth)
{
	float2 screenPosition = uv_to_clipspace(uv);

	float4 thisClip = float4(screenPosition, depth, 1);

	float4 prevClip = mul(GetCamera().inverse_view_projection, thisClip);
	prevClip = mul(GetCamera().previous_view_projection, prevClip);

	float2 prevScreen = prevClip.xy / prevClip.w;

	float2 screenVelocity = screenPosition - prevScreen;
	float2 prevScreenPosition = screenPosition - screenVelocity;

	return clipspace_to_uv(prevScreenPosition);
}

float GetDisocclusion(float depth, float depthHistory)
{
	float lineardepthCurrent = compute_lineardepth(depth);
	float lineardepthHistory = compute_lineardepth(depthHistory);

	float disocclusion = 1.0
		//* exp(-abs(1.0 - max(0.0, dot(normal, normalHistory))) * disocclusionNormalWeight) // Potential normal check if necessary
		* exp(-abs(lineardepthHistory - lineardepthCurrent) / lineardepthCurrent * disocclusionDepthWeight);

	return disocclusion;
}

float4 SamplePreviousColor(float2 prevUV, float2 size, float depth, out float disocclusion, out float2 prevUVSample)
{
	prevUVSample = prevUV;

	float4 previousColor = texture_color_history.SampleLevel(sampler_linear_clamp, prevUVSample, 0);
	float previousDepth = texture_depth_history.SampleLevel(sampler_point_clamp, prevUVSample, 0);

	disocclusion = GetDisocclusion(depth, previousDepth);
	if (disocclusion > disocclusionThreshold) // Good enough
	{
		return previousColor;
	}

	// Try to find the closest sample in the vicinity if we are not convinced of a disocclusion
	if (disocclusion < disocclusionThreshold)
	{
		float2 closestUV = prevUVSample;
		float2 dudv = rcp(size);

		const int searchRadius = 1;
		for (int y = -searchRadius; y <= searchRadius; y++)
		{
			for (int x = -searchRadius; x <= searchRadius; x++)
			{
				int2 offset = int2(x, y);
				float2 sampleUV = prevUVSample + offset * dudv;

				float samplePreviousDepth = texture_depth_history.SampleLevel(sampler_point_clamp, sampleUV, 0);

				float weight = GetDisocclusion(depth, samplePreviousDepth);
				if (weight > disocclusion)
				{
					disocclusion = weight;
					closestUV = sampleUV;
					prevUVSample = closestUV;
				}
			}
		}

		previousColor = texture_color_history.SampleLevel(sampler_linear_clamp, prevUVSample, 0);
	}

	// Bilinear interpolation on fallback - near edges
	if (disocclusion < disocclusionThreshold)
	{
		float2 weight = frac(prevUVSample * size + 0.5);

		// Bilinear weights
		float weights[4] =
		{
			(1 - weight.x) * (1 - weight.y),
			weight.x * (1 - weight.y),
			(1 - weight.x) * weight.y,
			weight.x * weight.y
		};

		float4 previousColorResult = 0;
		float previousDepthResult = 0;
		float weightSum = 0;

		uint2 prevCoord = uint2(size * prevUVSample - 0.5);
		uint2 offsets[4] = { uint2(0, 0), uint2(1, 0), uint2(0, 1), uint2(1, 1) };

		for (uint i = 0; i < 4; i++)
		{
			uint2 sampleCoord = prevCoord + offsets[i];

			previousColorResult += weights[i] * texture_color_history[sampleCoord];
			previousDepthResult += weights[i] * texture_depth_history[sampleCoord];

			weightSum += weights[i];
		}

		previousColorResult /= max(weightSum, 0.00001);
		previousDepthResult /= max(weightSum, 0.00001);

		previousColor = previousColorResult;
		disocclusion = GetDisocclusion(depth, previousDepthResult);
	}

	disocclusion = disocclusion < disocclusionThreshold ? 0.0 : disocclusion;
	return previousColor;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
	if ((uint) rtdiffuse_frame == 0)
	{
		float4 color = texture_color_current[DTid.xy];
		output_color[DTid.xy] = color;
		return;
	}

	const float depth = texture_depth[DTid.xy * 2];

	// Welford's online algorithm:
	//  https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance

	float4 m1 = 0.0;
	float4 m2 = 0.0;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			int2 offset = int2(x, y);
			int2 coord = DTid.xy + offset;

			float4 sampleColor = texture_color_current[coord];

			m1 += sampleColor;
			m2 += sampleColor * sampleColor;
		}
	}

	float4 mean = m1 / 9.0;
	float4 variance = (m2 / 9.0) - (mean * mean);
	float4 stddev = sqrt(max(variance, 0.0f));

	float2 velocity = texture_velocity[DTid.xy * 2];

	float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;

	float2 prevUVVelocity = uv + velocity;
	float2 prevUVReflectionHit = CalculateReprojectionBuffer(uv, depth);

	float4 previousColorVelocity = texture_color_history.SampleLevel(sampler_linear_clamp, prevUVVelocity, 0);
	float4 previousColorReflectionHit = texture_color_history.SampleLevel(sampler_linear_clamp, prevUVReflectionHit, 0);

	float previousDistanceVelocity = abs(Luminance(previousColorVelocity.rgb) - Luminance(mean.rgb));
	float previousDistanceReflectionHit = abs(Luminance(previousColorReflectionHit.rgb) - Luminance(mean.rgb));

	float2 prevUV = previousDistanceVelocity < previousDistanceReflectionHit ? prevUVVelocity : prevUVReflectionHit;

	float disocclusion = 0.0;
	float2 prevUVSample = 0.0;
	float4 previousColor = SamplePreviousColor(prevUV, postprocess.resolution, depth, disocclusion, prevUVSample);

	float4 currentColor = texture_color_current[DTid.xy];
	float4 resultColor = currentColor;

	// Disocclusion fallback: color
	if (disocclusion > disocclusionThreshold && is_saturated(prevUVSample))
	{
		// Color box clamp
		float4 colorMin = mean - temporalScale * stddev;
		float4 colorMax = mean + temporalScale * stddev;
		previousColor = clamp(previousColor, colorMin, colorMax);

		resultColor = lerp(currentColor, previousColor, temporalResponse);
	}
#if 0 // Debug
	else
	{
		resultColor = float4(1, 0, 0, 1);
	}
#endif

	float currentVariance = texture_variance_current[DTid.xy];
	float varianceResponse = varianceTemporalResponse;

	// Disocclusion fallback: variance
	if (disocclusion < disocclusionThreshold || !is_saturated(prevUVSample))
	{
		// Apply white for variance on occlusion. This helps to hide artifacts from temporal
		varianceResponse = 0.0f;
		currentVariance = 1.0f;
	}

	float previousVariance = texture_variance_history.SampleLevel(sampler_linear_clamp, prevUVSample, 0);
	float resultVariance = lerp(currentVariance, previousVariance, varianceResponse);

	output_color[DTid.xy] = max(0, resultColor);
	output_variance[DTid.xy] = max(0, resultVariance);
}
