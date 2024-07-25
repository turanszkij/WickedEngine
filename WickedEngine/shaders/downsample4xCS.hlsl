#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;

	float4 color = 0;

	uint2 dim;
	input.GetDimensions(dim.x, dim.y);
	float2 dim_rcp = rcp(dim);

	color += input.SampleLevel(sampler_linear_clamp, uv + float2(-1, -1) * dim_rcp, 0);
	color += input.SampleLevel(sampler_linear_clamp, uv + float2(1, -1) * dim_rcp, 0);
	color += input.SampleLevel(sampler_linear_clamp, uv + float2(-1, 1) * dim_rcp, 0);
	color += input.SampleLevel(sampler_linear_clamp, uv + float2(1, 1) * dim_rcp, 0);

	color /= 4.0f;

	output[DTid.xy] = color;
}
