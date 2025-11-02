#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	
	float4 center = input.SampleLevel(sampler_linear_clamp, uv, 0);
	float4 top =    input.SampleLevel(sampler_linear_clamp, uv + float2(0, -1) * postprocess.resolution_rcp, 0);
	float4 left =   input.SampleLevel(sampler_linear_clamp, uv + float2(-1, 0) * postprocess.resolution_rcp, 0);
	float4 right =  input.SampleLevel(sampler_linear_clamp, uv + float2(1, 0) * postprocess.resolution_rcp, 0);
	float4 bottom = input.SampleLevel(sampler_linear_clamp, uv + float2(0, 1) * postprocess.resolution_rcp, 0);

	output[DTid.xy] = center + (4 * center - top - bottom - left - right) * postprocess.params0[0];
}
