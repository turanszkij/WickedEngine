#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float> input_depth : register(t0);
Texture2D<float2> input_normal : register(t1);
Texture2D<float3> input_diffuse : register(t2);

RWTexture2D<float4> output : register(u0);

static const float depthThreshold = 1000.0;
static const float normalThreshold = 1.0;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	uint2 pixel = DTid.xy;
	const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;
	
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
	const float linearDepth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0) * GetCamera().z_far;
	const float3 N = decode_oct(texture_normal.SampleLevel(sampler_point_clamp, uv, 0).rg);
	const float3 P = reconstruct_position(uv, depth);

	float3 result = 0;
	float sum = 0;
	
	for(uint i = 0; i < 4; ++i)
	{
		const float sampleDepth = input_depth.SampleLevel(sampler_linear_clamp, uv, i);
		const float3 sampleN = decode_oct(input_normal.SampleLevel(sampler_linear_clamp, uv, i));
		const float3 sampleDiffuse = input_diffuse.SampleLevel(sampler_linear_clamp, uv, i);
		const float3 sampleP = reconstruct_position(uv, sampleDepth);
					
		float3 dq = P - sampleP;
		float planeError = max(abs(dot(dq, sampleN)), abs(dot(dq, N)));
		float relativeDepthDifference = planeError / linearDepth;
		float bilateralDepthWeight = exp(-sqr(relativeDepthDifference) * depthThreshold);

		float normalError = pow(saturate(dot(sampleN, N)), 4.0);
		float bilateralNormalWeight = saturate(1.0 - (1.0 - normalError) * normalThreshold);

		float bilateralWeight = bilateralDepthWeight * bilateralNormalWeight;

		//float r = length(dq);
		//float sigma = 0.9;
		//float gaussian = exp(-sqr(r / sigma));
		//float weight = (r == 0) ? 1.0 : gaussian * bilateralWeight; // Skip center gaussian peak

		float weight = bilateralDepthWeight;
		//weight = 1;
		result += sampleDiffuse * weight;
		sum += weight;
	}

	if(sum > 0)
	{
		result /= sum;
	}
	
	result = max(0, result);

	output[pixel] = float4(result, 1);
}
