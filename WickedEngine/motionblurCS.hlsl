#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// Implementation based on Jorge Jimenez Siggraph 2014 Next Generation Post Processing in Call of Duty Advanced Warfare

//#define DEBUG_TILING

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(neighborhoodmax, float2, TEXSLOT_ONDEMAND1);

#if defined(MOTIONBLUR_EARLYEXIT)
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND2);
#elif defined(MOTIONBLUR_CHEAP)
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND3);
#else
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND4);
#endif // MOTIONBLUR_EARLYEXIT

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
	return dot(depthCmp, spreadCmp);
}

[numthreads(POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	const uint tile_replicate = sqr(MOTIONBLUR_TILESIZE / POSTPROCESS_BLOCKSIZE);
	const uint tile_idx = Gid.x / tile_replicate;
	const uint tile_packed = tiles[tile_idx];
	const uint2 tile = uint2(tile_packed & 0xFFFF, (tile_packed >> 16) & 0xFFFF);
	const uint subtile_idx = Gid.x % tile_replicate;
	const uint2 subtile = unflatten2D(subtile_idx, MOTIONBLUR_TILESIZE / POSTPROCESS_BLOCKSIZE);
	const uint2 subtile_upperleft = tile * MOTIONBLUR_TILESIZE + subtile * POSTPROCESS_BLOCKSIZE;
	const uint2 pixel = subtile_upperleft + unflatten2D(GTid.x, POSTPROCESS_BLOCKSIZE);

	float4 color;

#ifdef MOTIONBLUR_EARLYEXIT
	color = input[pixel];

#else

	const float2 neighborhood_velocity = neighborhoodmax[(pixel + (dither((float2)pixel) - 0.5f) * 16) / MOTIONBLUR_TILESIZE] * motionblur_strength; // dither to reduce tile artifact
	const float neighborhood_velocity_magnitude = length(neighborhood_velocity);
	const float4 center_color = input[pixel];

	const float2 center_velocity = texture_gbuffer1[pixel].zw * motionblur_strength;
	const float center_velocity_magnitude = length(center_velocity);
	const float center_depth = texture_lineardepth[pixel];

	const float2 uv = (pixel + 0.5f) * xPPResolution_rcp;

	float seed = 12345;
	const float random_direction = rand(seed, uv) * 0.5f + 0.5f;

#ifdef MOTIONBLUR_CHEAP
	const float2 sampling_direction = center_velocity * random_direction * xPPResolution_rcp;
#else
	const float2 sampling_direction = neighborhood_velocity * random_direction * xPPResolution_rcp;
#endif // MOTIONBLUR_CHEAP

	const float range = 7.5f;
	float4 sum = 0;
	float numSampling = 0;
	float2 uv2 = uv - range * sampling_direction;
	[unroll]
	for (float i = -range; i <= range; i += 2.0f)
	{
		const float depth1 = texture_lineardepth.SampleLevel(sampler_point_clamp, uv2, 0);
		const float2 velocity1 = texture_gbuffer1.SampleLevel(sampler_point_clamp, uv2, 0).zw;
		const float velocity_magnitude1 = length(velocity1);
		const float3 color1 = input.SampleLevel(sampler_point_clamp, uv2, 0).rgb;
		uv2 += sampling_direction;
		const float depth2 = texture_lineardepth.SampleLevel(sampler_point_clamp, uv2, 0);
		const float2 velocity2 = texture_gbuffer1.SampleLevel(sampler_point_clamp, uv2, 0).zw;
		const float velocity_magnitude2 = length(velocity2);
		const float3 color2 = input.SampleLevel(sampler_point_clamp, uv2, 0).rgb;
		uv2 += sampling_direction;

#ifdef MOTIONBLUR_CHEAP
		sum += float4(color1, 1);
		sum += float4(color2, 1);
#else
		float weight1 = SampleWeight(center_depth, depth1, neighborhood_velocity_magnitude, center_velocity_magnitude, velocity_magnitude1, 1000, g_xCamera_ZFarP);
		float weight2 = SampleWeight(center_depth, depth2, neighborhood_velocity_magnitude, center_velocity_magnitude, velocity_magnitude2, 1000, g_xCamera_ZFarP);
		
		bool2 mirror = bool2(depth1 > depth2, velocity_magnitude2 > velocity_magnitude1);
		weight1 = all(mirror) ? weight2 : weight1;
		weight2 = any(mirror) ? weight2 : weight1;

		sum += weight1 * float4(color1, 1);
		sum += weight2 * float4(color2, 1);
#endif // MOTIONBLUR_CHEAP

		numSampling += 2;
	}
	sum /= numSampling;

	color.rgb = sum.rgb + (1 - sum.w) * center_color.rgb;
	color.a = center_color.a;

	//color = float4(sum.www, 1);
	//color = float4(neighborhood_velocity * 10, 0, 1);

#endif // MOTIONBLUR_EARLYEXIT


#ifdef DEBUG_TILING
#if defined(MOTIONBLUR_EARLYEXIT)
	color *= float4(0, 0, 1, 1);
#elif defined(MOTIONBLUR_CHEAP)
	color *= float4(0, 1, 0, 1);
#else
	color *= float4(1, 0, 0, 1);
#endif // MOTIONBLUR_EARLYEXIT
#endif // DEBUG_TILING

	output[pixel] = color;
}