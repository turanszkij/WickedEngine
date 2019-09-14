#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifndef BLUR_FORMAT
#define BLUR_FORMAT float4
#endif // BLUR_FORMAT

TEXTURE2D(input, BLUR_FORMAT, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, BLUR_FORMAT, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 direction = xPPParams0.xy;
	const float mip = xPPParams0.z;
	const float depth_threshold = xPPParams0.w;

	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	const float center_depth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0);
	const BLUR_FORMAT center_color = input.SampleLevel(sampler_linear_clamp, uv, mip);

	BLUR_FORMAT color = 0;
	for (uint i = 0; i < 9; ++i)
	{
		const float2 uv2 = uv + direction * gaussianOffsets[i] * xPPResolution_rcp;
		const float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv2, 0);
		const float weight = saturate(abs(depth - center_depth) * g_xCamera_ZFarP * depth_threshold);
		color += lerp(input.SampleLevel(sampler_linear_clamp, uv2, mip), center_color, weight) * gaussianWeightsNormalized[i];
	}

	output[DTid.xy] = color;
}
