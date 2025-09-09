#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float3> input_current : register(t0);
Texture2D<float3> input_history : register(t1);

RWTexture2D<float3> output : register(u0);

// This hack can improve bright areas:
#define HDR_CORRECTION

static const int THREADCOUNT = POSTPROCESS_BLOCKSIZE;
static const int TILE_BORDER = 1;
static const int TILE_SIZE = TILE_BORDER + THREADCOUNT + TILE_BORDER;
groupshared uint2 tile_cache[TILE_SIZE*TILE_SIZE];

inline uint coord_to_cache(int2 coord)
{
	return flatten2D(clamp(coord, 0, TILE_SIZE - 1), TILE_SIZE);
}

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	// preload grid cache:
	const int2 tile_upperleft = Gid.xy * THREADCOUNT - TILE_BORDER;
	for(uint y = GTid.y * 2; y < TILE_SIZE; y += THREADCOUNT * 2)
	for(uint x = GTid.x * 2; x < TILE_SIZE; x += THREADCOUNT * 2)
	{
		const int2 pixel = tile_upperleft + int2(x, y);
		const float2 uv = (pixel + 1.0) * postprocess.resolution_rcp;
		const half4 dddd = texture_lineardepth.GatherRed(sampler_linear_clamp, uv);
		const half4 rrrr = input_current.GatherRed(sampler_linear_clamp, uv);
		const half4 gggg = input_current.GatherGreen(sampler_linear_clamp, uv);
		const half4 bbbb = input_current.GatherBlue(sampler_linear_clamp, uv);
		const uint t = coord_to_cache(int2(x, y));
		tile_cache[t] = pack_half4(half4(rrrr.w, gggg.w, bbbb.w, dddd.w));
		tile_cache[t + 1] = pack_half4(half4(rrrr.z, gggg.z, bbbb.z, dddd.z));
		tile_cache[t + TILE_SIZE] = pack_half4(half4(rrrr.x, gggg.x, bbbb.x, dddd.x));
		tile_cache[t + TILE_SIZE + 1] = pack_half4(half4(rrrr.y, gggg.y, bbbb.y, dddd.y));
	}
	GroupMemoryBarrierWithGroupSync();
	
	const float2 uv = (DTid.xy + 0.5) * postprocess.resolution_rcp;
	half3 neighborhoodMin = 10000;
	half3 neighborhoodMax = -10000;
	half3 current;
	float bestDepth = 1;
	const int2 pixel_local = TILE_BORDER + GTid.xy;

	// Search for best velocity and compute color clamping range in 3x3 neighborhood:
	int2 bestOffset = 0;
	for (int y = -TILE_BORDER; y <= TILE_BORDER; ++y)
	for (int x = -TILE_BORDER; x <= TILE_BORDER; ++x)
	{
		const int2 offset = int2(x, y);
		const uint t = coord_to_cache(pixel_local + int2(x, y));

		const half4 neighbor = unpack_half4(tile_cache[t]);
		neighborhoodMin = min(neighborhoodMin, neighbor.xyz);
		neighborhoodMax = max(neighborhoodMax, neighbor.xyz);
		if (x == 0 && y == 0)
		{
			current = neighbor.xyz;
		}

		const half depth = neighbor.w;
		if (depth < bestDepth)
		{
			bestDepth = depth;
			bestOffset = offset;
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
	half subpixelCorrection = frac(max(abs(velocity.x) * GetCamera().internal_resolution.x, abs(velocity.y) * GetCamera().internal_resolution.y)) * 0.5;

	// compute a nice blend factor:
	half blendfactor = saturate(lerp(0.05, 0.8, subpixelCorrection));

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
