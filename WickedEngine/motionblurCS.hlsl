#define DILATE_VELOCITY_AVG_FAR
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "reconstructPositionHF.hlsli"
#include "postprocessHF.hlsli"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float4, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float3 color = 0;
	float numSampling = 1.0f;

	float2 vel = GetVelocity(DTid.xy);
	vel *= 0.025f;

	[unroll]
	for (float i = -7.5f; i <= 7.5f; i += 1.0f)
	{
		color.rgb += input.SampleLevel(sampler_linear_clamp, saturate(uv + vel * i*0.5f), 0).rgb;
		numSampling++;
	}

	output[DTid.xy] = float4(color / numSampling, 1);
}