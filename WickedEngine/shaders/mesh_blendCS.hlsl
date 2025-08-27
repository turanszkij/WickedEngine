#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<half4> input : register(t0);
Texture2D<half> mask : register(t1);

RWTexture2D<half4> output : register(u0);

static const float depth_diff_allowed = 0.14; // world space dist

static const uint THREADCOUNT = 16;
static const int TILE_BORDER = 16;
static const int TILE_SIZE = TILE_BORDER + THREADCOUNT + TILE_BORDER;
groupshared uint cache_id_z[TILE_SIZE * TILE_SIZE];
inline uint coord_to_cache(int2 coord)
{
	return flatten2D(clamp(coord, 0, TILE_SIZE - 1), TILE_SIZE);
}

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	uint2 dim = postprocess.resolution;
	float2 dim_rcp = postprocess.resolution_rcp;

	// preload grid cache:
	const int2 tile_upperleft = Gid.xy * THREADCOUNT - TILE_BORDER;
	for (uint y = GTid.y * 2; y < TILE_SIZE; y += THREADCOUNT * 2)
	for (uint x = GTid.x * 2; x < TILE_SIZE; x += THREADCOUNT * 2)
	{
		const int2 pixel = tile_upperleft + int2(x, y);
		const float2 uv = (pixel + 1.0) * dim_rcp;
		const half4 zzzz = texture_lineardepth.GatherRed(sampler_linear_clamp, uv);
        const half4 iiii = mask.GatherRed(sampler_linear_clamp, uv);
		const uint t = coord_to_cache(int2(x, y));
        cache_id_z[t] = pack_half2(half2(iiii.w, zzzz.w));
        cache_id_z[t + 1] = pack_half2(half2(iiii.z, zzzz.z));
        cache_id_z[t + TILE_SIZE] = pack_half2(half2(iiii.x, zzzz.x));
        cache_id_z[t + TILE_SIZE + 1] = pack_half2(half2(iiii.y, zzzz.y));
	}
	GroupMemoryBarrierWithGroupSync();

	ShaderCamera camera = GetCamera();
	half depth_diff_allowed_norm = depth_diff_allowed * camera.z_far_rcp;
	
	const int2 pixel_local = TILE_BORDER + GTid.xy;
	const half2 curr = unpack_half2(cache_id_z[coord_to_cache(pixel_local)]);
	const half current_id = curr.x;
	const half current_depth = curr.y;
	
	half best_dist = MEDIUMP_FLT_MAX;
	half2 best_offset = 0;
	
	const int spread = clamp(400.0 / max(0.001, current_depth), 2, TILE_BORDER);
	const float max_dist = length(float2(spread, spread));
	
	const int step = 2;
	for (int y = -spread; y <= spread; y += step)
	for (int x = -spread; x <= spread; x += step)
	{
		const half2 sam = unpack_half2(cache_id_z[coord_to_cache(pixel_local + int2(x, y))]);
		const half id = sam.x;
		const half depth = sam.y;
		const half diff = abs(current_depth - depth);
		if (id != current_id && diff <= depth_diff_allowed_norm)
		{
			const float2 offset = float2(x, y);
			const float dist = dot(offset, offset);
			if (dist < best_dist)
			{
				best_dist = dist;
				best_offset = offset;
			}
		}
	}
	
	if (best_dist < MEDIUMP_FLT_MAX)
	{
		// After mirror, gather again, this might not be within cached area
		const float2 uv = (DTid.xy + best_offset * 2 + 0.5) * dim_rcp; // *2 : mirroring to the other side of the seam
		const half4 rrrr = input.GatherRed(sampler_linear_clamp, uv);
		const half4 gggg = input.GatherGreen(sampler_linear_clamp, uv);
		const half4 bbbb = input.GatherBlue(sampler_linear_clamp, uv);
		const half4 aaaa = input.GatherAlpha(sampler_linear_clamp, uv);
        const half4 iiii = mask.GatherRed(sampler_linear_clamp, uv);
		const half4 zzzz = texture_lineardepth.GatherRed(sampler_linear_clamp, uv);
		half4 other_color = 0;
		half sum = 0;
		[unroll] for (uint i = 0; i < 4; ++i)
		{
			// The gathered values are validated and invalid samples discarded
			const half depth = zzzz[i];
			const half diff = abs(current_depth - depth);
			const half id = iiii[i];
			if (id != current_id && diff <= depth_diff_allowed_norm)
			{
				other_color += half4(rrrr[i], gggg[i], bbbb[i], aaaa[i]);
				sum++;
			}
		}
		if (sum > 0)
		{
			other_color /= sum;
			const half weight = saturate(0.5 - sqrt(best_dist) / max_dist);
			output[DTid.xy] = lerp(output[DTid.xy], other_color, weight);
		}
	}
}
