#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2DArray<float> input_luminance : register(t0);
Texture2DArray<float2> input_chrominance : register(t1);

RWTexture2D<unorm float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float3 uv = float3((DTid.xy + 0.5f) * postprocess.resolution_rcp, 0);

	float luminance = input_luminance.SampleLevel(sampler_linear_clamp, uv, 0);
	float2 chrominance = input_chrominance.SampleLevel(sampler_linear_clamp, uv, 0);

	// https://learn.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
	float C = luminance - 16.0 / 255.0;
	float D = chrominance.x - 0.5;
	float E = chrominance.y - 0.5;

	float r = saturate(1.164383 * C + 1.596027 * E);
	float g = saturate(1.164383 * C - (0.391762 * D) - (0.812968 * E));
	float b = saturate(1.164383 * C + 2.017232 * D);

	output[DTid.xy] = float4(r, g, b, 1);
}
