#include "globals.hlsli"
#include "ShaderInterop_CloudGenerator.h"

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(CLOUDGENERATOR_BLOCKSIZE, CLOUDGENERATOR_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = ((float2)DTid.xy + 0.5f) / xNoiseTexDim;

	float4 result = 0;
	float weight = 1;
	float weight_sum = 0;
	for (uint i = 0; i < xRefinementCount; ++i)
	{
		float2 tc = (uv + xRandomness * sin((i + 1) * float2(0.5f, 0.25f) * PI)) * 1.0f / pow(2, i);
		result += texture_0.SampleLevel(sampler_linear_wrap, tc, 0) * weight;
		weight_sum += weight;
		weight *= 2;
	}
	result /= weight_sum;

	output[DTid.xy] = result;
}
