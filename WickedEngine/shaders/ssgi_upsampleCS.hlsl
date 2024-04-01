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

static const float depthThreshold = 0.1;
static const float normalThreshold = 64;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	uint2 GTid = remap_lane_8x8(groupIndex);
	uint2 pixel = Gid * POSTPROCESS_BLOCKSIZE + GTid;
	const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;
	
	const float depth = input_depth_high[pixel];
	const float linearDepth = compute_lineardepth(depth);
	const float3 N = decode_oct(input_normal_high[pixel].rg);

#if 1
	const float3 P = reconstruct_position(uv, depth);
	const float3 ddxP = P - QuadReadAcrossX(P);
	const float3 ddyP = P - QuadReadAcrossY(P);
	const float curve = saturate(1 - pow(1 - max(dot(ddxP, ddxP), dot(ddyP, ddyP)), 32));
	const float normalPow = lerp(normalThreshold, 1, curve);
#else
	const float normalPow = normalThreshold;
#endif

	float3 result = 0;
	float sum = 0;
#if 1
	const int range = int(postprocess.params0.x);
	const float spread = postprocess.params0.y;
#else
	const int range = 1;
	const float spread = 8;
#endif
	for(int x = -range; x <= range; ++x)
	{
		for(int y = -range; y <= range; ++y)
		{
			const float2 offset = float2(x, y) * spread * postprocess.resolution_rcp;
			const float2 sample_uv = uv + offset;
			
			const float3 sampleDiffuse = input_diffuse_low.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb;
			
			const float sampleDepth = input_depth_low.SampleLevel(sampler_point_clamp, sample_uv, 0);
			const float sampleLinearDepth = compute_lineardepth(sampleDepth);
			float bilateralDepthWeight = 1 - saturate(abs(sampleLinearDepth - linearDepth) * depthThreshold);

			const float3 sampleN = decode_oct(input_normal_low.SampleLevel(sampler_linear_clamp, sample_uv, 0));
			float normalError = pow(saturate(dot(sampleN, N)), normalPow);
			float bilateralNormalWeight = normalError;

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
