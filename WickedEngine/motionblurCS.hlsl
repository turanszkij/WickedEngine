#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(neighborhoodmax, float2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

inline float2 DepthCmp(in float centerDepth, in float sampleDepth, in float depthScale)
{
	return saturate(0.5f + float2(depthScale, -depthScale) * (sampleDepth - centerDepth));
}

inline float2 SpreadCmp(in float offsetLen, in float2 spreadLen, in float pixelToSampleUnitsScale)
{
	return saturate(pixelToSampleUnitsScale * spreadLen - offsetLen + 1.0f);
}

inline float SampleWeight(
	in float centerDepth, in float sampleDepth, 
	in float offsetLen, in float centerSpreadLen, in float sampleSpreadLen, 
	in float pixelToSampleUnitsScale, in float depthScale
)
{
	const float2 depthCmp = DepthCmp(centerDepth, sampleDepth, depthScale);
	const float2 spreadCmp = SpreadCmp(offsetLen, float2(centerSpreadLen, sampleSpreadLen), pixelToSampleUnitsScale);
	//return spreadCmp.x;
	return dot(depthCmp, spreadCmp);
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 neighborhood_velocity = neighborhoodmax[(DTid.xy + (dither((float2)DTid.xy) - 0.5f) * 16) / MOTIONBLUR_TILESIZE]; // dither to reduce tile artifact
	const float neighborhood_velocity_magnitude = length(neighborhood_velocity);
	const float4 center_color = input[DTid.xy];
	if (neighborhood_velocity_magnitude < 0.000001f)
	{
		output[DTid.xy] = center_color;
		return;
	}

	const float2 center_velocity = texture_gbuffer1[DTid.xy].zw;
	const float center_velocity_magnitude = length(center_velocity);
	const float center_depth = texture_lineardepth[DTid.xy];

	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float seed = 12345;
	const float2 random_direction = (float2(rand(seed, uv), rand(seed, uv)) - 0.5f) * xPPResolution_rcp * 0.35f;

	const float strength = 0.025f;
	const float2 sampling_direction = neighborhood_velocity * strength + random_direction;

	float4 sum = 0;
	float numSampling = 0;
	for (float i = -7.5f; i <= 7.5f; i += 1.0f)
	{
		const float2 uv2 = saturate(uv + sampling_direction * i);
		const float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv2, 0);
		const float2 velocity = texture_gbuffer1.SampleLevel(sampler_linear_clamp, uv2, 0).zw;
		const float velocity_magnitude = length(velocity);
		const float weight = SampleWeight(center_depth, depth, neighborhood_velocity_magnitude * 10, center_velocity_magnitude, velocity_magnitude, 10, g_xCamera_ZFarP);
		sum.rgb += input.SampleLevel(sampler_linear_clamp, uv2, 0).rgb * weight;
		sum.w += weight;
		numSampling++;
	}
	sum /= numSampling;

	float4 color;
	color.rgb = sum.rgb + (1 - sum.w) * center_color.rgb;
	color.a = center_color.a;

	output[DTid.xy] = color;
	//output[DTid.xy] = float4(sum.www, 1);
	//output[DTid.xy] = float4(neighborhood_velocity * 10, 0, 1);
}