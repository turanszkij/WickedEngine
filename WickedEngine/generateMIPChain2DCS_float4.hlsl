#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE2D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2D(output, MIP_OUTPUT_FORMAT, 0);

SAMPLERSTATE(customsampler, SSLOT_ONDEMAND0);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y)
	{
		float2 uv = ((float2)DTid.xy + 0.5f) * (float2)outputResolution_rcp.xy;
		float4 color = input.SampleLevel(customsampler, uv, 0);

		[branch]
		if (mipgen_options & MIPGEN_OPTION_BIT_PRESERVE_COVERAGE)
		{
			float4 alphas = input.GatherAlpha(customsampler, uv, 0);
			alphas = pow(saturate(alphas), 2.2f);
			float alpha = (alphas.x + alphas.y + alphas.z + alphas.w) / 4.0f;
			alpha = pow(saturate(alpha), 1.0f / 2.2f);
			color.a = alpha;
			//color.a = max(alphas.x, max(alphas.y, max(alphas.z, alphas.w)));
		}

		output[DTid.xy] = color;
	}
}