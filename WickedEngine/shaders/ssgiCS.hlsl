#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);
Texture2DArray<float> input_depth : register(t1);
Texture2DArray<float2> input_normal : register(t2);

RWTexture2D<float4> output_diffuse : register(u0);

#ifdef WIDE
static const uint THREADCOUNT = 16;
static const int TILE_BORDER = 18;
#else
static const uint THREADCOUNT = 8;
static const int TILE_BORDER = 4;
#endif // WIDE
static const int TILE_SIZE = TILE_BORDER + THREADCOUNT + TILE_BORDER;
groupshared uint cache_xy[TILE_SIZE * TILE_SIZE];
groupshared float cache_z[TILE_SIZE * TILE_SIZE];
groupshared uint cache_rgb[TILE_SIZE * TILE_SIZE];

inline uint coord_to_cache(int2 coord)
{
	return flatten2D(clamp(TILE_BORDER + coord, 0, TILE_SIZE - 1), TILE_SIZE);
}

static const float radius = 14;
static const float radius2 = radius * radius;
static const float radius2_rcp_negative  = -rcp(radius2);

#if 0
static const uint depth_test_count = 1;
static const float depth_tests[] = {0.33};
#else
static const uint depth_test_count = 3;
static const float depth_tests[] = {0.125, 0.25, 0.75};
#endif

float3 compute_diffuse(
	float3 origin_position,
	float3 origin_normal,
	int2 GTid,
	int2 offset
)
{
	const int2 sampleLoc = GTid + offset;
	const uint t = coord_to_cache(sampleLoc);
	float3 sample_position;
	sample_position.z = cache_z[t];
	if(sample_position.z > GetCamera().z_far - 1)
		return 0;
	sample_position.xy = unpack_half2(cache_xy[t]);
    const float3 origin_to_sample = sample_position - origin_position;
    const float distance2 = dot(origin_to_sample, origin_to_sample);
    float occlusion = saturate(dot(origin_normal, origin_to_sample));
    occlusion *= saturate(distance2 * radius2_rcp_negative + 1.0f);
	
	if(occlusion > 0)
	{
		const float origin_z = origin_position.z;
		const float sample_z = sample_position.z;

#if 1
		// DDA occlusion:
		const int2 start = GTid;
		const int2 goal = sampleLoc;
	
		const int dx = int(goal.x) - int(start.x);
		const int dy = int(goal.y) - int(start.y);

		int step = max(abs(dx), abs(dy));
		step = (step + 1) / 2; // reduce steps

		const float x_incr = float(dx) / step;
		const float y_incr = float(dy) / step;

		float x = float(start.x);
		float y = float(start.y);

		for (int i = 0; i < step - 1; i++)
		{
			x += x_incr;
			y += y_incr;
			
			int2 loc = int2(round(x), round(y));
			const uint tt = coord_to_cache(loc);
			
			float dt = float(i) / float(step);
			float z = lerp(origin_z, sample_z, dt);

			const float sz = cache_z[tt];
			if(sz < z - 0.1)
			{
				return occlusion * Unpack_R11G11B10_FLOAT(cache_rgb[tt]);
			}
		}
#else
		// Simple occlusion:
		for (uint i = 0; i < depth_test_count; ++i)
		{
			const float dt = depth_tests[i];
			const float z = lerp(origin_z, sample_z, dt);
			const int2 loc = round(lerp(float2(GTid), float2(sampleLoc), dt));
			const uint tt = coord_to_cache(loc);
			const float sz = cache_z[tt];
			if (sz < z - 0.1)
			{
				return occlusion * Unpack_R11G11B10_FLOAT(cache_rgb[tt]);
			}
		}
#endif
	}

    return occlusion * Unpack_R11G11B10_FLOAT(cache_rgb[t]);
}

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint layer = DTid.z;
	
	const int2 tile_upperleft = Gid.xy * THREADCOUNT - TILE_BORDER;
	for(uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += THREADCOUNT * THREADCOUNT)
	{
		const int2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		const float depth = input_depth[uint3(pixel, layer)];
		const float2 uv = (pixel + 0.5f) * postprocess.resolution_rcp;
		const float3 P = reconstruct_position(uv, depth, GetCamera().inverse_projection);
		const float3 color = input.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
		cache_xy[t] = pack_half2(P.xy);
		cache_z[t] = P.z;
		cache_rgb[t] = Pack_R11G11B10_FLOAT(color.rgb);
	}
	GroupMemoryBarrierWithGroupSync();

	const uint t = coord_to_cache(GTid.xy);
	float3 P;
	P.z = cache_z[t];
	if(P.z > GetCamera().z_far - 1)
		return;

	P.xy = unpack_half2(cache_xy[t]);
	
	const uint2 pixel = DTid.xy;
	const float3 N = mul((float3x3)GetCamera().view, decode_oct(input_normal[uint3(pixel, layer)].rg));
	
	float3 diffuse = 0;
	float sum = 0;
	const int range = int(postprocess.params0.x);
	const float range_rcp2 = postprocess.params0.y;
	const float spread = postprocess.params0.z /*+ dither(pixel)*/;
	for(int x = -range; x <= range; ++x)
	{
		for(int y = -range; y <= range; ++y)
		{
			const float2 foffset = float2(x, y) * spread;
			const int2 offset = round(foffset);
			const float weight = saturate(1 - abs(x) * abs(y) * range_rcp2);
			diffuse += compute_diffuse(P, N, GTid, offset) * weight;
			sum += weight;
		}
	}
	if(sum > 0)
	{
		diffuse = diffuse / sum;
	}

	// Interleave:
    uint2 OutPixel = DTid.xy << 2 | uint2(DTid.z & 3, DTid.z >> 2);
    output_diffuse[OutPixel] = float4(diffuse, 1);
}
