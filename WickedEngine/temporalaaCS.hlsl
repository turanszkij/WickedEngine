#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_current, float3, TEXSLOT_ONDEMAND0);
TEXTURE2D(input_history, float3, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_depth_history, float, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, float3, 0);

// Neighborhood load optimization:
#define USE_LDS

// This hack can improve bright areas:
#define HDR_CORRECTION

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared uint tile_RG[TILE_SIZE*TILE_SIZE];
groupshared uint tile_B_depth[TILE_SIZE*TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	float3 neighborhoodMin = 100000;
	float3 neighborhoodMax = -100000;
	float3 current;
	float bestDepth = 1;

#ifdef USE_LDS
	const int2 tile_upperleft = Gid.xy * POSTPROCESS_BLOCKSIZE - TILE_BORDER;
	for (uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		const float depth = texture_lineardepth[pixel];
		const float3 color = input_current[pixel].rgb;
		tile_RG[t] = f32tof16(color.r) | (f32tof16(color.g) << 16);
		tile_B_depth[t] = f32tof16(color.b) | (f32tof16(depth) << 16);
	}
	GroupMemoryBarrierWithGroupSync();

	// Search for best velocity and compute color clamping range in 3x3 neighborhood:
	int2 bestOffset = 0;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const int2 offset = int2(x, y);
			const uint idx = flatten2D(GTid.xy + TILE_BORDER + offset, TILE_SIZE);
			const uint RG = tile_RG[idx];
			const uint B_depth = tile_B_depth[idx];

			const float3 neighbor = float3(f16tof32(RG), f16tof32(RG >> 16), f16tof32(B_depth));
			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);
			if (x == 0 && y == 0)
			{
				current = neighbor;
			}

			const float depth = f16tof32(B_depth >> 16);
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestOffset = offset;
			}
		}
	}
	const float2 velocity = texture_gbuffer1[DTid.xy + bestOffset].zw;

#else

	// Search for best velocity and compute color clamping range in 3x3 neighborhood:
	int2 bestPixel = int2(0, 0);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const int2 curPixel = DTid.xy + int2(x, y);

			const float3 neighbor = input_current[curPixel].rgb;
			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);
			if (x == 0 && y == 0)
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
	}
	const float2 velocity = texture_gbuffer1[bestPixel].zw;

#endif // USE_LDS

	const float2 prevUV = uv + velocity;

#if 0
	// Disocclusion fallback:
	float depth_current = texture_lineardepth[DTid.xy] * g_xCamera_ZFarP;
	float depth_history = getLinearDepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 0));
	if (length(velocity) > 0.01 && abs(depth_current - depth_history) > 1)
	{
		output[DTid.xy] = current;
		output[DTid.xy] = float3(1, 0, 0);
		return;
	}
#endif

	// we cannot avoid the linear filter here because point sampling could sample irrelevant pixels but we try to correct it later:
	float3 history = input_history.SampleLevel(sampler_linear_clamp, prevUV, 0).rgb;

	// simple correction of image signal incoherency (eg. moving shadows or lighting changes):
	history.rgb = clamp(history.rgb, neighborhoodMin, neighborhoodMax);

	// the linear filtering can cause blurry image, try to account for that:
	float subpixelCorrection = frac(max(abs(velocity.x)*g_xFrame_InternalResolution.x, abs(velocity.y)*g_xFrame_InternalResolution.y)) * 0.5f;

	// compute a nice blend factor:
	float blendfactor = saturate(lerp(0.05f, 0.8f, subpixelCorrection));

	// if information can not be found on the screen, revert to aliased image:
	blendfactor = is_saturated(prevUV) ? blendfactor : 1.0f;

#ifdef HDR_CORRECTION
	history.rgb = tonemap(history.rgb);
	current.rgb = tonemap(current.rgb);
#endif

	// do the temporal super sampling by linearly accumulating previous samples with the current one:
	float3 resolved = lerp(history.rgb, current.rgb, blendfactor);

#ifdef HDR_CORRECTION
	resolved.rgb = inverseTonemap(resolved.rgb);
#endif

	output[DTid.xy] = resolved;
}
