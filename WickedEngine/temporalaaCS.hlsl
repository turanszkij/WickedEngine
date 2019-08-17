#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_current, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(input_history, float4, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

// Neighborhood load optimization:
#define USE_LDS

// This hack can improve bright areas:
#define HDR_CORRECTION

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared float tile_depths[TILE_SIZE*TILE_SIZE];
groupshared float2 tile_velocities[TILE_SIZE*TILE_SIZE];
groupshared float4 tile_colors[TILE_SIZE*TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	float4 neighborhoodMin = 100000;
	float4 neighborhoodMax = -100000;
	float bestDepth = 1;

#ifdef USE_LDS
	const int2 tile_upperleft = Gid.xy * POSTPROCESS_BLOCKSIZE - TILE_BORDER;
	for (uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		tile_depths[t] = texture_lineardepth[pixel];
		tile_velocities[t] = texture_gbuffer1[pixel].zw;
		tile_colors[t] = input_current[pixel];
	}
	GroupMemoryBarrierWithGroupSync();

	// our currently rendered frame sample:
	const uint idx = flatten2D(GTid.xy + TILE_BORDER, TILE_SIZE);
	float4 current = tile_colors[idx];

	// Search for best velocity and compute color clamping range in 3x3 neighborhood:
	uint bestIdx = 0;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const uint idx = flatten2D(GTid.xy + TILE_BORDER + int2(x, y), TILE_SIZE);

			const float4 neighbor = tile_colors[idx];
			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);

			const float depth = tile_depths[idx];
			[flatten]
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestIdx = idx;
			}
		}
	}
	const float2 velocity = tile_velocities[bestIdx];

#else
	// our currently rendered frame sample:
	float4 current = input_current[DTid.xy];

	// Search for best velocity and compute color clamping range in 3x3 neighborhood:
	int2 bestPixel = int2(0, 0);
	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			int2 curPixel = DTid.xy + int2(i, j);

			float4 neighbor = input_current[curPixel];
			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);

			float depth = texture_lineardepth[curPixel];
			[flatten]
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

	// we cannot avoid the linear filter here because point sampling could sample irrelevant pixels but we try to correct it later:
	float4 history = input_history.SampleLevel(sampler_linear_clamp, prevUV, 0);

	// simple correction of image signal incoherency (eg. moving shadows or lighting changes):
	history = clamp(history, neighborhoodMin, neighborhoodMax);

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
	float4 resolved = float4(lerp(history.rgb, current.rgb, blendfactor), 1);

#ifdef HDR_CORRECTION
	resolved.rgb = inverseTonemap(resolved.rgb);
#endif

	output[DTid.xy] = resolved;
}
