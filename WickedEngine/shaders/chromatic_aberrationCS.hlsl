#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	const float amount = postprocess.params0.x;

	float2 border = 0.5f * amount * postprocess.resolution_rcp;
	border = min(border, float2(0.49f, 0.49f));
	const float2 uv_base = lerp(border, 1.0f - border, uv);

	const float2 distortion = (uv_base - 0.5f) * amount * postprocess.resolution_rcp;

	const float2 uv_R = uv_base + float2(1, 1) * distortion;
	const float2 uv_G = uv_base; // green stays at the base
	const float2 uv_B = uv_base - float2(1, 1) * distortion;

	const float R = input.SampleLevel(sampler_linear_clamp, uv_R, 0).r;
	const float G = input.SampleLevel(sampler_linear_clamp, uv_G, 0).g;
	const float B = input.SampleLevel(sampler_linear_clamp, uv_B, 0).b;

	output[DTid.xy] = float4(R, G, B, 1);
}
