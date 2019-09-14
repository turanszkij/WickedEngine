#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_sharp, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(input_blurred, float4, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float focus = xPPParams0.x;

	const float4 color_sharp = input_sharp.SampleLevel(sampler_point_clamp, uv, 0);
	const float4 color_blurred = input_blurred.SampleLevel(sampler_linear_clamp, uv,0);
	const float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0).r * g_xCamera_ZFarP;
	const float difference = abs(focus - depth);

	output[DTid.xy] = lerp(color_sharp, color_blurred, saturate(difference * 0.008f));
}
