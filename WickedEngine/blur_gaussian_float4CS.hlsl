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

	BLUR_FORMAT color = 0;
	for (uint i = 0; i < 9; ++i)
	{
		color += input.SampleLevel(sampler_linear_clamp, (DTid.xy + 0.5f + direction * gaussianOffsets[i]) * xPPResolution_rcp, mip) * gaussianWeightsNormalized[i];
	}

	output[DTid.xy] = color;
}
