#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(input_luminance, float, TEXSLOT_ONDEMAND1);
TEXTURE2D(input_distortion, float4, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float exposure = xPPParams0.x;

	const float2 distortion = input_distortion.SampleLevel(sampler_linear_clamp, uv, 0).rg;
	float4 hdr = input.SampleLevel(sampler_linear_clamp, uv + distortion, 0);
	float average_luminance = input_luminance[uint2(0, 0)].r;

	hdr.rgb *= exposure;

	float luminance = dot(hdr.rgb, float3(0.2126, 0.7152, 0.0722));
	luminance /= average_luminance; // adaption
	hdr.rgb *= luminance;

	float4 ldr = saturate(float4(tonemap(hdr.rgb), hdr.a));

	ldr.rgb = GAMMA(ldr.rgb);

	// dithering before outputting to SDR will reduce color banding:
	ldr.rgb += (dither((float2)DTid.xy) - 0.5f) / 64.0f;

	output[DTid.xy] = ldr;
}