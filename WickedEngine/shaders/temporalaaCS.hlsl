#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float3> input_current : register(t0);
Texture2D<float3> input_history : register(t1);

RWTexture2D<float3> output : register(u0);

/* 
 * References:
 *
 * Temporal Reprojection Anti-Aliasing by PlayDead:
 * https://github.com/playdeadgames/temporal
 *
 * High Quality Temporal Supersampling by Epic Games:
 * https://advances.realtimerendering.com/s2014/
 *
 * Filmic SMAA: Sharp Morphological and Temporal Antialiasing by Activision:
 * https://advances.realtimerendering.com/s2016/
 *
 * ---------------------------------------------------------------------
 *
 * Another thing to consider when doing TAA is to do upsampling since this has proven to be quite effective with history buffers.
 * But since this pass just focuses on anti-aliasing and we already do upsampling with FSR, this seems unnecessary here.
 *
 * According to references, we can also filter the current color with a Blackman-Harris filter that makes edges more anti-aliased.
 * The problem I've observed is that it contrasts jittered screen effects, like high quality atmosphere, and make it more like "saw teeth".
 * I didn't see huge difference regardless, and bright areas gets "blown up" in size, but this could be an optional path to endeavor.
 * 
 */

// Neighborhood load optimization:
#define USE_LDS

// This hack can improve bright areas:
#define HDR_CORRECTION

// It has its benefits, but with disocclusion checking, contrast blending and clipping, this seems unnecessary in most cases
// YCoCg configuration to reduce ghosting:
//#define YCOCG

// Shrink YCoCg box to reduce more ghosting, but may result in tinted colors
//#define YCOCG_CHROMA_SHRINK

// Variance seem to be good for flickering pixels and varying effects e.g. alpha mask blend (grass) and high quality atmosphere
// 1: Simple min-max color, 2: Variance
#define MINMAX_METHOD 2

// This will make basic min-max appear filtered instead of blocks of 3x3 pixels:
#define MINMAX_SIMPLE_ROUND

// Bicubic essentially eliminates the blur that happens in motion, but is more expensive due to 5 samples
// 1: Linear filter, 2: bicubic resampling
#define HISTORY_SAMPLING_METHOD 2

// 1: Simple min-max clamping, 2: Direct clip, 3: Line to box
#define HISTORY_CLIPPING_METHOD 2

// Blend factor based on history contrast and previous feedback
// 1: Biased, 2: Unbiased
#define BLEND_FACTOR_METHOD 2

// Lower samples can improve performance, but can lack information (flickering)
// 5: Cross, 9: 3x3 tap
#define NEIGHBORHOOD_SAMPLES 9

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared uint tile_color[TILE_SIZE*TILE_SIZE];
groupshared float tile_depth[TILE_SIZE*TILE_SIZE];

static const int2 NeighborOffsets[9] =
{
	int2(0.0f, 1.0f),
    int2(1.0f, 0.0f),
    int2(-1.0f, 0.0f),
    int2(0.0f, -1.0f),
	int2(0.0f, 0.0f),
    int2(1.0f, 1.0f),
    int2(1.0f, -1.0f),
    int2(-1.0f, 1.0f),
    int2(-1.0f, -1.0f),
};

// https://en.wikipedia.org/wiki/YCoCg
float3 RGB_YCoCg(float3 c)
{
	// Added 0.5 to scale from range [-0.5; 0.5] to [0.0; 1.0]
	// Y = R/4 + G/2 + B/4
	// Co = R/2 - B/2 + 0.5
	// Cg = -R/4 + G/2 - B/4 + 0.5
	return float3(
		 c.x * 0.25 + c.y * 0.5 + c.z * 0.25,
		 c.x * 0.5 - c.z * 0.5 + 0.5,
		-c.x * 0.25 + c.y * 0.5 - c.z * 0.25 + 0.5
	);
}

// https://en.wikipedia.org/wiki/YCoCg
float3 YCoCg_RGB(float3 c)
{
	// Added 0.5 to scale from range [-0.5; 0.5] to [0.0; 1.0]
	// Y = Y
	// Co = Co - 0.5
	// Cg = Cg - 0.5
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	
	float Y = c.x;
	float Co = c.y - 0.5;
	float Cg = c.z - 0.5;
	
	return float3(
		Y + Co - Cg,
		Y + Cg,
		Y - Co - Cg
	);
}

// It's important that we convert every screen color when using YCoCg
float3 TransformToWorkingSpace(float3 color)
{
#ifdef YCOCG
	return RGB_YCoCg(color);
#else
	return color;
#endif
}

float3 TransformToOutputSpace(float3 color)
{
#ifdef YCOCG
	return YCoCg_RGB(color);
#else
	return color;
#endif
}

float Luminance(float3 color)
{
#if 1
	return dot(color, float3(0.2126, 0.7152, 0.0722)); // Better? the judge is out...
#else
	return dot(color, float3(0.299, 0.587, 0.114));
#endif
}

float GetLuma(float3 color)
{
	// Luminance is stored in the first channel when working in YCoCg
#ifdef YCOCG
    return color.x;
#else
	return Luminance(color.xyz);
#endif
}

float3 ReinhardToneMap(float3 color)
{
	// Different from globals.hlsli, since we need luma component
	return color / (GetLuma(color) + 1.0);
}

float3 ReinhardToneMapInv(float3 color)
{
	return color / (1.0 - GetLuma(color));
}

void CalculateNeighborhoodCorners(float3 current, float3 neighborhood[NEIGHBORHOOD_SAMPLES], inout float3 neighborhoodMin, inout float3 neighborhoodMax)
{
#if MINMAX_METHOD == 1
	
	// Simple Min Max operation:

	neighborhoodMin = 100000;
	neighborhoodMax = -100000;
	
	float3 neighborhoodMinAdd = 0;
	float3 neighborhoodMaxAdd = 0;
	
	for (int i = 0; i < NEIGHBORHOOD_SAMPLES; ++i)
	{
		const float3 neighbor = neighborhood[i];
		neighborhoodMin = min(neighborhoodMin, neighbor);
		neighborhoodMax = max(neighborhoodMax, neighbor);

		if (i == 4)
		{
			// Capture cross:
			neighborhoodMinAdd = neighborhoodMin;
			neighborhoodMaxAdd = neighborhoodMax;
		}
	}

#if NEIGHBORHOOD_SAMPLES == 9 && defined(MINMAX_SIMPLE_ROUND)
	neighborhoodMin = neighborhoodMin * 0.5 + neighborhoodMinAdd * 0.5;
	neighborhoodMax = neighborhoodMax * 0.5 + neighborhoodMaxAdd * 0.5;
#endif
	
#elif MINMAX_METHOD == 2
	
	// Based on Welford's online algorithm:
	//  https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
	
	float3 m1 = 0.0;
	float3 m2 = 0.0;
	for (int i = 0; i < NEIGHBORHOOD_SAMPLES; ++i)
	{
		const float3 neighbor = neighborhood[i];
		
		m1 += neighbor;
		m2 += neighbor * neighbor;
	}

	float3 mean = m1 / NEIGHBORHOOD_SAMPLES;
	float3 variance = (m2 / NEIGHBORHOOD_SAMPLES) - (mean * mean);
	float3 stddev = sqrt(max(variance, 0.0f));

	// Too much will not necessarily smooth out results
	const float temporalScale = 1.25;
	
	// Color box clamp
	neighborhoodMin = mean - temporalScale * stddev;
	neighborhoodMax = mean + temporalScale * stddev;

	neighborhoodMin = min(neighborhoodMin, current);
	neighborhoodMax = max(neighborhoodMax, current);
	
#endif // MINMAX_METHOD
	
#if defined(YCOCG) && defined(YCOCG_CHROMA_SHRINK)
	// Shrink chroma on neighborhood corners:
	float2 chromaExtent = 0.25 * 0.5 * (neighborhoodMax.r - neighborhoodMin.r);
	float2 chromaCenter = current.gb;
	neighborhoodMin.yz = chromaCenter - chromaExtent;
	neighborhoodMax.yz = chromaCenter + chromaExtent;
#endif
}

float3 CalculateClippedHistory(float3 current, float3 history, float3 neighborhoodMin, float3 neighborhoodMax)
{
#if HISTORY_CLIPPING_METHOD == 1
	
	// Simple clamp:

	return clamp(history, neighborhoodMin, neighborhoodMax);
	
#elif HISTORY_CLIPPING_METHOD == 2
	
	// note: only clips towards aabb center (but fast!)
	float3 center = 0.5 * (neighborhoodMax + neighborhoodMin);
	float3 extents = 0.5 * (neighborhoodMax - neighborhoodMin);

	float3 offset = history - center;
	float3 v_unit = offset / (extents + 0.0000001);
	
	float3 absUnit = abs(v_unit);
	float maxUnit = max(absUnit.x, max(absUnit.y, absUnit.z));

	if (maxUnit > 1.0)
	{
		return center + offset / maxUnit;
	}
	else
	{
		return history; // point inside aabb
	}

#elif HISTORY_CLIPPING_METHOD == 3

	// Switch to current when pixel is out of neighbor bounds:
	
	float3 center = 0.5 * (neighborhoodMax + neighborhoodMin);
	float3 extents = 0.5 * (neighborhoodMax - neighborhoodMin);

	float3 rayOrigin = history - center;
	float3 rayDirection = current - history;

	float3 minIntersect = (extents - rayOrigin) * rcp(rayDirection);
	float3 maxIntersect = -(extents + rayOrigin) * rcp(rayDirection);

	float3 enterIntersection = min(minIntersect, maxIntersect);
	float clip = max(enterIntersection.x, max(enterIntersection.y, enterIntersection.z));

	return lerp(history, current, saturate(clip));
	
#endif // HISTORY_CLIPPING_METHOD
}

float CalculateBlendFactor(float3 current, float3 history, float3 neighborhoodMin, float3 neighborhoodMax)
{
	// Feedback weight (t.lottes):
	// Idea is to decrease response when near clamping
	
	float lumaCurrent = GetLuma(current.rgb);
	float lumaHistory = GetLuma(history.rgb);

	const float2 response = float2(0.02f, 0.05f);
	
#if BLEND_FACTOR_METHOD == 1

	// Biased luminance difference:
	
	float biasedWeight = 1 / (1 + abs(lumaCurrent - lumaHistory) * 20.0);
	return lerp(response.x, response.y, biasedWeight);
	
#elif BLEND_FACTOR_METHOD == 2

	// Unbiased luminance difference:
	
	float unbiasedDiff = abs(lumaCurrent - lumaHistory) / max(lumaCurrent, max(lumaHistory, 0.2));
	float unbiasedWeight = sqr(1.0 - unbiasedDiff); // sqr for thicker lines?
	return lerp(response.x, response.y, unbiasedWeight);

#endif // BLEND_FACTOR_METHOD
}

float3 SampleHistory(float2 prevUV)
{
	float3 history = 0;
	
#if HISTORY_SAMPLING_METHOD == 1

	// Linear filter:
	
	history = input_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
	
#elif HISTORY_SAMPLING_METHOD == 2

	// Filmic SMAA bicubic history resampling:
	
	float2 position = postprocess.resolution * prevUV;
	float2 centerPosition = floor(position - 0.5) + 0.5;
	float2 f = position - centerPosition;
	float2 f2 = f * f;
	float2 f3 = f * f2;

	const float sharpness = 0.75f; // [0.0; 1.0]
	
	float c = sharpness;
	float2 w0 = -c * f3 + 2.0 * c * f2 - c * f;
	float2 w1 = (2.0 - c) * f3 - (3.0 - c) * f2 + 1.0;
	float2 w2 = -(2.0 - c) * f3 + (3.0 - 2.0 * c) * f2 + c * f;
	float2 w3 = c * f3 - c * f2;
	
	float2 w12 = w1 + w2;
	float2 tc12 = postprocess.resolution_rcp * (centerPosition + w2 / w12);
	float3 centerColor = input_history.SampleLevel(sampler_linear_clamp, float2(tc12.x, tc12.y), 0).rgb;
	
	float2 tc0 = postprocess.resolution_rcp * (centerPosition - 1.0);
	float2 tc3 = postprocess.resolution_rcp * (centerPosition + 2.0);
	float4 color =
		float4(input_history.SampleLevel(sampler_linear_clamp, float2(tc12.x, tc0.y), 0).rgb, 1.0) * (w12.x * w0.y) +
        float4(input_history.SampleLevel(sampler_linear_clamp, float2(tc0.x, tc12.y), 0).rgb, 1.0) * (w0.x * w12.y) +
        float4(centerColor, 1.0) * (w12.x * w12.y) +
        float4(input_history.SampleLevel(sampler_linear_clamp, float2(tc3.x, tc12.y), 0).rgb, 1.0) * (w3.x * w12.y) +
        float4(input_history.SampleLevel(sampler_linear_clamp, float2(tc12.x, tc3.y), 0).rgb, 1.0) * (w12.x * w3.y);
	
	history = color.rgb * rcp(color.a);
	
#endif // HISTORY_SAMPLING_METHOD

	return TransformToWorkingSpace(history);
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	float3 current;
	float3 neighborhood[NEIGHBORHOOD_SAMPLES];
	float bestDepth = 1;
	
#ifdef USE_LDS
	const int2 tile_upperleft = Gid.xy * POSTPROCESS_BLOCKSIZE - TILE_BORDER;
	for (uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		const float depth = texture_lineardepth[pixel];
		const float3 color = input_current[pixel].rgb;
		tile_color[t] = Pack_R11G11B10_FLOAT(color);
		tile_depth[t] = depth;
	}
	GroupMemoryBarrierWithGroupSync();

	// Search for best velocity and capture neighborhood corners:
	int2 bestOffset = 0;
	for (int i = 0; i < NEIGHBORHOOD_SAMPLES; ++i)
	{
		const int2 offset = NeighborOffsets[i];
		const uint idx = flatten2D(GTid.xy + TILE_BORDER + offset, TILE_SIZE);

		float3 neighbor = Unpack_R11G11B10_FLOAT(tile_color[idx]);
		neighbor = TransformToWorkingSpace(neighbor); // To YCoCg
		
		neighborhood[i] = neighbor;
		
		if (i == 4) // Center
		{
			current = neighbor;
		}

		const float depth = tile_depth[idx];
		if (depth < bestDepth)
		{
			bestDepth = depth;
			bestOffset = offset;
		}
	}
	const float2 velocity = texture_velocity[DTid.xy + bestOffset].xy;

#else

	// Search for best velocity and capture neighborhood corners:
	int2 bestPixel = int2(0, 0);
	for (int i = 0; i < NEIGHBORHOOD_SAMPLES; ++i)
	{
		const int2 offset = NeighborOffsets[i];
		const int2 curPixel = DTid.xy + offset;
		
		float3 neighbor = input_current[curPixel].rgb;
		neighbor = TransformToWorkingSpace(neighbor); // To YCoCg
		
		neighborhood[i] = neighbor;
		
		if (i == 4) // Center
		{
			current = neighbor;
		}
		
		const float depth = texture_lineardepth[curPixel];
		if (depth < bestDepth)
		{
			bestDepth = depth;
			bestPixel = curPixel;
		}
	}
	const float2 velocity = texture_velocity[bestPixel].xy;

#endif // USE_LDS

	const float2 prevUV = uv + velocity;
	
#if 1
	// Disocclusion fallback:
	float depth_current = texture_lineardepth[DTid.xy] * GetCamera().z_far;
	float depth_history = compute_lineardepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 0));
	if (length(velocity) > 0.01 && abs(depth_current - depth_history) > 1)
	{
		output[DTid.xy] = TransformToOutputSpace(current); // We previously converted this
		//output[DTid.xy] = float3(1, 0, 0);
		return;
	}
#endif

	// Neighborhood corners:
	float3 neighborhoodMin = 0;
	float3 neighborhoodMax = 0;
	CalculateNeighborhoodCorners(current, neighborhood, neighborhoodMin, neighborhoodMax);
	
	// Sample history:
	// We cannot avoid the linear filter here because point sampling could sample irrelevant pixels
	float3 history = SampleHistory(prevUV);
	
	// Clip history:
	// Simple correction of image signal incoherency (eg. moving shadows or lighting changes)
	history = CalculateClippedHistory(current, history, neighborhoodMin, neighborhoodMax);

	// Blend factor:
	float blendfactor = CalculateBlendFactor(current, history, neighborhoodMin, neighborhoodMax);
	
	// Increase blend based on velocity:
	blendfactor = lerp(blendfactor, 0.8f, saturate(length(velocity) * 40));
	
	// if information can not be found on the screen, revert to aliased image:
	blendfactor = is_saturated(prevUV) ? blendfactor : 1.0f;
	
#ifdef HDR_CORRECTION
	history.rgb = ReinhardToneMap(history.rgb);
	current.rgb = ReinhardToneMap(current.rgb);
#endif
	
	// do the temporal super sampling by linearly accumulating previous samples with the current one:
	float3 resolved = lerp(history.rgb, current.rgb, blendfactor);

#ifdef HDR_CORRECTION
	resolved.rgb = ReinhardToneMapInv(resolved.rgb);
#endif

	resolved = TransformToOutputSpace(resolved);

	output[DTid.xy] = resolved;
}
