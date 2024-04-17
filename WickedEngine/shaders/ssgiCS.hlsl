#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2DArray<float> input_depth : register(t0);
Texture2DArray<float3> input_color : register(t1);
Texture2D<float2> input_normal : register(t2);

RWTexture2D<float4> output_diffuse : register(u0);

#ifdef WIDE
static const uint THREADCOUNT = 16;
static const int TILE_BORDER = 16;
#else
static const uint THREADCOUNT = 8;
static const int TILE_BORDER = 4;
#endif // WIDE
static const int TILE_SIZE = TILE_BORDER + THREADCOUNT + TILE_BORDER;
groupshared uint cache_xy[TILE_SIZE * TILE_SIZE];
groupshared float cache_z[TILE_SIZE * TILE_SIZE];
groupshared uint cache_rgb[TILE_SIZE * TILE_SIZE];
groupshared uint group_valid;

inline uint coord_to_cache(int2 coord)
{
	return flatten2D(clamp(coord, 0, TILE_SIZE - 1), TILE_SIZE);
}

float3 compute_diffuse(
	float3 origin_position,
	float3 origin_normal,
	int2 originLoc, // coord in cache
	int2 sampleLoc // coord in cache
)
{
	const uint t = coord_to_cache(sampleLoc);
	uint c = cache_rgb[t];
	if(c == 0)
		return 0; // early exit if pixel doesn't have lighting
	float3 sample_position;
	sample_position.z = cache_z[t];
	sample_position.xy = unpack_half2(cache_xy[t]);
    const float3 origin_to_sample = sample_position - origin_position;
    float occlusion = saturate(dot(origin_normal, origin_to_sample));		// normal falloff
    occlusion *= saturate(1 + origin_to_sample.z * postprocess.params0.w);	// depth rejection
	
	if(occlusion > 0)
	{
		const float origin_z = origin_position.z;
		const float sample_z = sample_position.z;
		
		// DDA occlusion:
		const int2 start = originLoc;
		const int2 goal = sampleLoc;
	
		const int dx = int(goal.x) - int(start.x);
		const int dy = int(goal.y) - int(start.y);

		int step = max(abs(dx), abs(dy));
		step = (step + 1) / 2; // reduce steps
		const float step_rcp = rcp(step);

		const float x_incr = float(dx) * step_rcp;
		const float y_incr = float(dy) * step_rcp;

		float x = float(start.x);
		float y = float(start.y);

		for (int i = 0; i < step - 1; i++)
		{
			x += x_incr;
			y += y_incr;
			
			const int2 loc = int2(x, y);
			const uint tt = coord_to_cache(loc);
			
			const float dt = float(i) / float(step);
			const float z = lerp(origin_z, sample_z, dt);

			const float sz = cache_z[tt];
			if(sz < z - 0.1)
			{
				c = cache_rgb[tt];
				break;
			}
		}
	}

    return occlusion * Unpack_R11G11B10_FLOAT(c);
}

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint layer = DTid.z;
    const uint2 interleaved_pixel = DTid.xy << 2 | uint2(DTid.z & 3, DTid.z >> 2);

	if(groupIndex == 0)
	{
		group_valid = 0;
	}
	GroupMemoryBarrierWithGroupSync();
	
	const int2 tile_upperleft = Gid.xy * THREADCOUNT - TILE_BORDER;
	for(uint x = GTid.x * 2; x < TILE_SIZE; x += THREADCOUNT * 2)
	for(uint y = GTid.y * 2; y < TILE_SIZE; y += THREADCOUNT * 2)
	{
		const int2 pixel = tile_upperleft + int2(x, y);
		const float3 uvw = float3((pixel + 0.5f) * postprocess.resolution_rcp, layer);
		const float4 depths = input_depth.GatherRed(sampler_linear_clamp, uvw);
		const float4 reds = input_color.GatherRed(sampler_linear_clamp, uvw);
		const float4 greens = input_color.GatherGreen(sampler_linear_clamp, uvw);
		const float4 blues = input_color.GatherBlue(sampler_linear_clamp, uvw);
		const float2 uv0 = (pixel + 0.5 + int2(0, 0)) * postprocess.resolution_rcp;
		const float2 uv1 = (pixel + 0.5 + int2(1, 0)) * postprocess.resolution_rcp;
		const float2 uv2 = (pixel + 0.5 + int2(0, 1)) * postprocess.resolution_rcp;
		const float2 uv3 = (pixel + 0.5 + int2(1, 1)) * postprocess.resolution_rcp;
		const float3 P0 = reconstruct_position(uv0, depths.w, GetCamera().inverse_projection);
		const float3 P1 = reconstruct_position(uv1, depths.z, GetCamera().inverse_projection);
		const float3 P2 = reconstruct_position(uv2, depths.x, GetCamera().inverse_projection);
		const float3 P3 = reconstruct_position(uv3, depths.y, GetCamera().inverse_projection);
		const uint C0 = Pack_R11G11B10_FLOAT(float3(reds.w, greens.w, blues.w));
		const uint C1 = Pack_R11G11B10_FLOAT(float3(reds.z, greens.z, blues.z));
		const uint C2 = Pack_R11G11B10_FLOAT(float3(reds.x, greens.x, blues.x));
		const uint C3 = Pack_R11G11B10_FLOAT(float3(reds.y, greens.y, blues.y));
		
		const uint t = coord_to_cache(int2(x, y));
		cache_xy[t] = pack_half2(P0.xy);
		cache_z[t] = P0.z;
		cache_rgb[t] = C0;
		
		cache_xy[t + 1] = pack_half2(P1.xy);
		cache_z[t + 1] = P1.z;
		cache_rgb[t + 1] = C1;
		
		cache_xy[t + TILE_SIZE] = pack_half2(P2.xy);
		cache_z[t + TILE_SIZE] = P2.z;
		cache_rgb[t + TILE_SIZE] = C2;
		
		cache_xy[t + TILE_SIZE + 1] = pack_half2(P3.xy);
		cache_z[t + TILE_SIZE + 1] = P3.z;
		cache_rgb[t + TILE_SIZE + 1] = C3;
		
		if(C0 || C1 || C2 || C3)
			InterlockedOr(group_valid, 1u);
	}
	GroupMemoryBarrierWithGroupSync();

	[branch]
	if (group_valid == 0)
		return; // if no valid color was cached, whole group can exit early

	const int2 originLoc = GTid.xy + TILE_BORDER;
	const uint t = coord_to_cache(originLoc);
	float3 P;
	P.z = cache_z[t];
	
	[branch]
	if(P.z > GetCamera().z_far - 1)
		return; // if pixel depth is not valid, it can exit early

	P.xy = unpack_half2(cache_xy[t]);
	
	const float3 N = mul((float3x3)GetCamera().view, decode_oct(input_normal[interleaved_pixel].rg));
	
	float3 diffuse = 0;
	float sum = 0;
	const int range = int(postprocess.params0.x);
	const float spread = postprocess.params0.y /*+ dither(DTid.xy)*/;
	const float rangespread_rcp2 = postprocess.params0.z;

	for(int x = -range; x <= range; ++x)
	{
		for(int y = -range; y <= range; ++y)
		{
			const int2 offset = int2(x, y) * spread;
			const float weight = saturate(1 - abs(offset.x) * abs(offset.y) * rangespread_rcp2);
			diffuse += compute_diffuse(P, N, originLoc, originLoc + offset) * weight;
			sum += weight;
		}
	}
	if(sum > 0)
	{
		diffuse = diffuse / sum;
	}

	// interleave result:
    output_diffuse[interleaved_pixel] = float4(diffuse, 1);
}
