#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(bloom, float4, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float4 color = input[DTid.xy];
	color.rgb += bloom.SampleLevel(sampler_linear_clamp, uv, 1.5f).rgb;
	color.rgb += bloom.SampleLevel(sampler_linear_clamp, uv, 3.5f).rgb;
	color.rgb += bloom.SampleLevel(sampler_linear_clamp, uv, 4.5f).rgb;

	output[DTid.xy] = color;
}