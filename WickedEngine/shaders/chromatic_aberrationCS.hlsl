#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float amount = xPPParams0.x;

	const float2 distortion = (uv - 0.5f) * amount * xPPResolution_rcp;

	const float2 uv_R = uv + float2(1, 1) * distortion;
	const float2 uv_G = uv + float2(0, 0) * distortion;
	const float2 uv_B = uv - float2(1, 1) * distortion;

	const float R = input.SampleLevel(sampler_linear_clamp, uv_R, 0).r;
	const float G = input.SampleLevel(sampler_linear_clamp, uv_G, 0).g;
	const float B = input.SampleLevel(sampler_linear_clamp, uv_B, 0).b;
	
	const float4 color = float4(R, G, B, 1);

	output[DTid.xy] = color;
}
