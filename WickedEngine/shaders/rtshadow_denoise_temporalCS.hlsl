#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(resolve_current, uint4, TEXSLOT_ONDEMAND0);
TEXTURE2D(resolve_history, uint4, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_depth_history, float, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, uint4, 0);

static const float temporalResponseMin = 0.9;
static const float temporalResponseMax = 1.0f;
static const float temporalScale = 2.0;
static const float temporalExposure = 10.0f;

float load_shadow(in uint shadow_index, in uint4 shadow_mask)
{
	uint mask_shift = (shadow_index % 4) * 8;
	uint mask_bucket = shadow_index / 4;
	uint mask = (shadow_mask[mask_bucket] >> mask_shift) & 0xFF;
	return mask / 255.0;
}

void store_shadow(in float shadow, in uint shadow_index, inout uint4 shadow_mask)
{
	uint mask = uint(saturate(shadow) * 255); // 8 bits
	uint mask_shift = (shadow_index % 4) * 8;
	uint mask_bucket = shadow_index / 4;
	shadow_mask[mask_bucket] |= mask << mask_shift;
}

inline void ResolverAABB(in uint shadow_index, float sharpness, float exposureScale, float AABBScale, float2 uv, inout float currentMin, inout float currentMax, inout float currentAverage, inout float currentOutput)
{
	const int2 SampleOffset[9] = { int2(-1.0, -1.0), int2(0.0, -1.0), int2(1.0, -1.0), int2(-1.0, 0.0), int2(0.0, 0.0), int2(1.0, 0.0), int2(-1.0, 1.0), int2(0.0, 1.0), int2(1.0, 1.0) };

	float sampleColors[9];
	[unroll]
	for (uint i = 0; i < 9; i++)
	{
		sampleColors[i] = load_shadow(shadow_index, resolve_current[floor(uv * xPPResolution) + SampleOffset[i]]);
	}

	// Variance Clipping (AABB)

	float m1 = 0;
	float m2 = 0;
	[unroll]
	for (uint x = 0; x < 9; x++)
	{
		m1 += sampleColors[x];
		m2 += sampleColors[x] * sampleColors[x];
	}

	float mean = m1 / 9.0;
	float stddev = sqrt((m2 / 9.0) - sqr(mean));

	currentMin = mean - AABBScale * stddev;
	currentMax = mean + AABBScale * stddev;

	currentOutput = sampleColors[4];
	currentMin = min(currentMin, currentOutput);
	currentMax = max(currentMax, currentOutput);
	currentAverage = mean;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;
	if (!is_saturated(prevUV))
	{
		output[DTid.xy] = resolve_current[DTid.xy];
		return;
	}

	// Disocclusion fallback:
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1);
	float depth_current = getLinearDepth(depth);
	float depth_history = getLinearDepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 0));
	if (abs(depth_current - depth_history) > 1)
	{
		output[DTid.xy] = resolve_current[DTid.xy];
		return;
	}

	uint4 shadow_mask = 0;

	[unroll]
	for (uint shadow_index = 0; shadow_index < 16; ++shadow_index)
	{
		float previous = load_shadow(shadow_index, resolve_history[floor(prevUV * xPPResolution)]);

		float current = 0;
		float currentMin, currentMax, currentAverage;
		ResolverAABB(shadow_index, 0, temporalExposure, temporalScale, uv, currentMin, currentMax, currentAverage, current);

		float lumDifference = abs(current - previous) / max(current, max(previous, 0.2f));
		float lumWeight = sqr(1.0f - lumDifference);
		float blendFinal = lerp(temporalResponseMin, temporalResponseMax, lumWeight);

		// Reduce ghosting by refreshing the blend by velocity (Unreal)
		float2 velocityScreen = velocity * xPPResolution;
		float velocityBlend = sqrt(dot(velocityScreen, velocityScreen));
		blendFinal = lerp(blendFinal, 0.2, saturate(velocityBlend / 100.0));

		float result = lerp(current, previous, blendFinal);

		store_shadow(result, shadow_index, shadow_mask);
	}

	output[DTid.xy] = shadow_mask;
}
