#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

PUSHCONSTANT(mipgen, MipgenPushConstants);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < mipgen.outputResolution.x && DTid.y < mipgen.outputResolution.y)
	{
		SamplerState customsampler = bindless_samplers[mipgen.sampler_index];
		Texture2D input = bindless_textures[mipgen.texture_input];
		RWTexture2D<MIP_OUTPUT_FORMAT> output = bindless_rwtextures[mipgen.texture_output];

		float2 uv = ((float2)DTid.xy + 0.5f) * (float2)mipgen.outputResolution_rcp.xy;
		float4 color = input.SampleLevel(customsampler, uv, 0);

		[branch]
		if (mipgen.mipgen_options & MIPGEN_OPTION_BIT_PRESERVE_COVERAGE)
		{
			float4 alphas = input.GatherAlpha(customsampler, uv, 0);
#if 0
			alphas = pow(saturate(alphas), 2.2f);
			float alpha = (alphas.x + alphas.y + alphas.z + alphas.w) / 4.0f;
			alpha = pow(saturate(alpha), 1.0f / 2.2f);
			color.a = alpha;
#else
			color.a = max(alphas.x, max(alphas.y, max(alphas.z, alphas.w)));
#endif
		}

		if (mipgen.mipgen_options & MIPGEN_OPTION_BIT_SRGB)
		{
			color.rgb = ApplySRGBCurve_Fast(color.rgb);
		}

		output[DTid.xy] = color;
	}
}
