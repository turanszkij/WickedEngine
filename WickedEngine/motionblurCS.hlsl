#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float3 color = 0;

	// Average velocity in a far-reaching 5-tap pattern:
	float2 velocity_TL = texture_gbuffer1[DTid.xy + int2(-2, -2)].zw;
	float2 velocity_TR = texture_gbuffer1[DTid.xy + int2(2, -2)].zw;
	float2 velocity_BL = texture_gbuffer1[DTid.xy + int2(-2, 2)].zw;
	float2 velocity_BR = texture_gbuffer1[DTid.xy + int2(2, 2)].zw;
	float2 velocity_CE = texture_gbuffer1[DTid.xy].zw;

	const float2 velocity = (velocity_TL + velocity_TR + velocity_BL + velocity_BR + velocity_CE) / 5.0f;

	float numSampling = 1.0f;
	for (float i = -7.5f; i <= 7.5f; i += 1.0f)
	{
		const float strength = 0.025f;
		color.rgb += input.SampleLevel(sampler_linear_clamp, saturate(uv + velocity * i * strength), 0).rgb;
		numSampling++;
	}

	output[DTid.xy] = float4(color / numSampling, 1);
}