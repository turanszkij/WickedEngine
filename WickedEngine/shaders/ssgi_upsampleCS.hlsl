#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float> input_depth_low : register(t0);
Texture2D<float2> input_normal_low : register(t1);
Texture2D<float4> input_diffuse_low : register(t2);
Texture2D<float> input_depth_high : register(t3);
Texture2D<float2> input_normal_high : register(t4);

RWTexture2D<float4> output : register(u0);

#ifdef WIDE
static const uint THREADCOUNT = POSTPROCESS_BLOCKSIZE;
static const int TILE_BORDER = 6;
#else
static const uint THREADCOUNT = POSTPROCESS_BLOCKSIZE;
static const int TILE_BORDER = 2;
#endif // WIDE
static const int TILE_SIZE = TILE_BORDER + THREADCOUNT + TILE_BORDER;
groupshared float cache_z[TILE_SIZE * TILE_SIZE];
groupshared uint cache_rgb[TILE_SIZE * TILE_SIZE];
groupshared uint cache_oct[TILE_SIZE * TILE_SIZE];

inline uint coord_to_cache(int2 coord)
{
	return flatten2D(clamp(coord, 0, TILE_SIZE - 1), TILE_SIZE);
}

static const float depthThreshold = 0.1;
static const float normalThreshold = 64;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint2 GTid = remap_lane_8x8(groupIndex);
	
	const int2 tile_upperleft = Gid.xy * THREADCOUNT / 2 - TILE_BORDER;
	for(uint x = GTid.x * 2; x < TILE_SIZE; x += THREADCOUNT * 2)
	for(uint y = GTid.y * 2; y < TILE_SIZE; y += THREADCOUNT * 2)
	{
		const int2 pixel = tile_upperleft + int2(x, y);
		const float2 uv = (pixel + 0.5f) * postprocess.params1.zw;
		const float4 depths = input_depth_low.GatherRed(sampler_linear_clamp, uv);
		const float4 reds = input_diffuse_low.GatherRed(sampler_linear_clamp, uv);
		const float4 greens = input_diffuse_low.GatherGreen(sampler_linear_clamp, uv);
		const float4 blues = input_diffuse_low.GatherBlue(sampler_linear_clamp, uv);
		const float4 xxxx = input_normal_low.GatherRed(sampler_linear_clamp, uv);
		const float4 yyyy = input_normal_low.GatherGreen(sampler_linear_clamp, uv);
		const float Z0 = compute_lineardepth(depths.w);
		const float Z1 = compute_lineardepth(depths.z);
		const float Z2 = compute_lineardepth(depths.x);
		const float Z3 = compute_lineardepth(depths.y);
		const uint C0 = Pack_R11G11B10_FLOAT(float3(reds.w, greens.w, blues.w));
		const uint C1 = Pack_R11G11B10_FLOAT(float3(reds.z, greens.z, blues.z));
		const uint C2 = Pack_R11G11B10_FLOAT(float3(reds.x, greens.x, blues.x));
		const uint C3 = Pack_R11G11B10_FLOAT(float3(reds.y, greens.y, blues.y));
		const uint OCT0 = pack_half2(float2(xxxx.w, yyyy.w));
		const uint OCT1 = pack_half2(float2(xxxx.z, yyyy.z));
		const uint OCT2 = pack_half2(float2(xxxx.x, yyyy.x));
		const uint OCT3 = pack_half2(float2(xxxx.y, yyyy.y));
		
		const uint t = coord_to_cache(int2(x, y));
		cache_z[t] = Z0;
		cache_rgb[t] = C0;
		cache_oct[t] = OCT0;
		
		cache_z[t + 1] = Z1;
		cache_rgb[t + 1] = C1;
		cache_oct[t + 1] = OCT1;
		
		cache_z[t + TILE_SIZE] = Z2;
		cache_rgb[t + TILE_SIZE] = C2;
		cache_oct[t + TILE_SIZE] = OCT2;
		
		cache_z[t + TILE_SIZE + 1] = Z3;
		cache_rgb[t + TILE_SIZE + 1] = C3;
		cache_oct[t + TILE_SIZE + 1] = OCT3;
	}
	GroupMemoryBarrierWithGroupSync();
	
	uint2 pixel = Gid * POSTPROCESS_BLOCKSIZE + GTid;
	const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;
	
	const float depth = input_depth_high[pixel];
	const float linearDepth = compute_lineardepth(depth);
	const float3 N = decode_oct(input_normal_high[pixel].rg);
	
#if 1
	const int range = int(postprocess.params0.x);
	int spread = int(postprocess.params0.y);
#else
	const int range = 2;
	int spread = 2;
#endif

#if 0
	const float3 P = reconstruct_position(uv, depth);
	const float3 ddxP = P - QuadReadAcrossX(P);
	const float3 ddyP = P - QuadReadAcrossY(P);
	const float curve = saturate(1 - pow(1 - max(dot(ddxP, ddxP), dot(ddyP, ddyP)), 64));
	const float normalPow = lerp(normalThreshold, 1, curve);
	//spread *= 1 + curve;
#else
	const float normalPow = normalThreshold;
#endif

	const int2 coord_base = GTid.xy / 2 + TILE_BORDER;

	float3 result = 0;
	float sum = 0;
	for(int x = -range; x <= range; ++x)
	{
		for(int y = -range; y <= range; ++y)
		{
			const int2 coord = coord_base + int2(x, y) * spread;
			const uint t = coord_to_cache(coord);

			const float3 sampleDiffuse = Unpack_R11G11B10_FLOAT(cache_rgb[t]);
			const float sampleLinearDepth = cache_z[t];
			const float3 sampleN = decode_oct(unpack_half2(cache_oct[t]));
		
			float bilateralDepthWeight = 1 - saturate(abs(sampleLinearDepth - linearDepth) * depthThreshold);
			
			float bilateralNormalWeight = pow(saturate(dot(sampleN, N)), normalPow) + 0.001;

			float weight = bilateralDepthWeight * bilateralNormalWeight;
			
			//weight = 1;
			result += sampleDiffuse * weight;
			sum += weight;
		}
	}

	if(sum > 0)
	{
		result /= sum;
	}
	
	result = max(0, result);
	
	output[pixel] = output[pixel] + float4(result, 1);
	//output[pixel] = float4(curve.xxx, 1);
}
