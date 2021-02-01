#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// This will cut out bright parts (>1) and also downsample 4x

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = DTid.xy + 0.5f;

	float3 color = 0;

	color += input.SampleLevel(sampler_linear_clamp, (uv + float2(-1, -1)) * xPPResolution_rcp, 0).rgb;
	color += input.SampleLevel(sampler_linear_clamp, (uv + float2(1, -1)) * xPPResolution_rcp, 0).rgb;
	color += input.SampleLevel(sampler_linear_clamp, (uv + float2(-1, 1)) * xPPResolution_rcp, 0).rgb;
	color += input.SampleLevel(sampler_linear_clamp, (uv + float2(1, 1)) * xPPResolution_rcp, 0).rgb;

	color /= 4.0f;

	const float bloomThreshold = xPPParams0.x;
	color = min(color, 10); // clamp upper limit: avoid incredibly large values to overly dominate bloom (high speculars were causing problems)
	color = max(color - bloomThreshold, 0);

	output[DTid.xy] = float4(color, 1);
}
