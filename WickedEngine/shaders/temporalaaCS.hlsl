#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float3> input_current : register(t0);
Texture2D<float3> input_history : register(t1);

RWTexture2D<float3> output : register(u0);

// This hack can improve bright areas:
#define HDR_CORRECTION

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared uint2 tile_color[TILE_SIZE*TILE_SIZE];
groupshared float tile_depth[TILE_SIZE*TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if (postprocess.params0.x)
	{
		// Early exit when it is the first frame
		output[DTid.xy] = input_current[DTid.xy].rgb;
		return;
	}
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	half3 neighborhoodMin = 10000;
	half3 neighborhoodMax = -10000;
	half3 current;
	float bestDepth = 1;
	
	const int2 tile_upperleft = Gid.xy * POSTPROCESS_BLOCKSIZE - TILE_BORDER;
	for (uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		const float depth = texture_lineardepth[pixel];
		const half3 color = input_current[pixel].rgb;
		tile_color[t] = pack_half3(color);
		tile_depth[t] = depth;
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

			const half3 neighbor = unpack_half3(tile_color[idx]);
			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);
			if (x == 0 && y == 0)
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
	}
	const float2 velocity = texture_velocity[DTid.xy + bestOffset].xy;

	const float2 prevUV = GetCamera().clamp_uv_to_scissor(uv + velocity);

#if 1
	// Disocclusion fallback:
	float depth_current = texture_lineardepth[DTid.xy] * GetCamera().z_far;
	float depth_history = compute_lineardepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 0));
	if (length(velocity) > 0.01 && abs(depth_current - depth_history) > 1)
	{
		output[DTid.xy] = current;
		//output[DTid.xy] = float3(1, 0, 0);
		return;
	}
#endif

	// we cannot avoid the linear filter here because point sampling could sample irrelevant pixels but we try to correct it later:
	half3 history = input_history.SampleLevel(sampler_linear_clamp, prevUV, 0).rgb;

	// simple correction of image signal incoherency (eg. moving shadows or lighting changes):
	history.rgb = clamp(history.rgb, neighborhoodMin, neighborhoodMax);

	// the linear filtering can cause blurry image, try to account for that:
	float subpixelCorrection = frac(max(abs(velocity.x) * GetCamera().internal_resolution.x, abs(velocity.y) * GetCamera().internal_resolution.y)) * 0.5;

	// compute a nice blend factor:
	float blendfactor = saturate(lerp(0.05, 0.8, subpixelCorrection));

	// if information can not be found on the screen, revert to aliased image:
	blendfactor = is_saturated(prevUV) ? blendfactor : 1.0;

#ifdef HDR_CORRECTION
	history.rgb = tonemap(history.rgb);
	current.rgb = tonemap(current.rgb);
#endif

	// do the temporal super sampling by linearly accumulating previous samples with the current one:
	half3 resolved = lerp(history.rgb, current.rgb, blendfactor);

#ifdef HDR_CORRECTION
	resolved.rgb = inverse_tonemap(resolved.rgb);
#endif

	output[DTid.xy] = saturateMediump(resolved);
}
