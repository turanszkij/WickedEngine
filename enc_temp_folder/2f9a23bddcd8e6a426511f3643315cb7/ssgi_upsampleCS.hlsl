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

static const float depthThreshold = 1000.0;
static const float normalThreshold = 1.0;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	uint2 pixel = DTid.xy;
	const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;
	
	const float depth = input_depth_high[pixel];
	const float linearDepth = compute_lineardepth(depth);
	const float3 N = decode_oct(input_normal_high[pixel].rg);
	const float3 P = reconstruct_position(uv, depth);

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
			
			const float sampleDepth = input_depth_low.SampleLevel(sampler_linear_clamp, sample_uv, 0);
			const float3 sampleN = decode_oct(input_normal_low.SampleLevel(sampler_linear_clamp, sample_uv, 0));
			const float3 sampleDiffuse = input_diffuse_low.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb;
			const float3 sampleP = reconstruct_position(sample_uv, sampleDepth);
					
			float3 dq = P - sampleP;
			float planeError = max(abs(dot(dq, sampleN)), abs(dot(dq, N)));
			float relativeDepthDifference = planeError / linearDepth;
			float bilateralDepthWeight = exp(-sqr(relativeDepthDifference) * depthThreshold);

			float normalError = pow(saturate(dot(sampleN, N)), 4.0);
			float bilateralNormalWeight = saturate(1.0 - (1.0 - normalError) * normalThreshold);

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
	
	output[pixel] = (output[pixel] + float4(result, 1)) ;
}
