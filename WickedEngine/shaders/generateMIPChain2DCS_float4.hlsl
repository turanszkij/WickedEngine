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
		float4 rrrr = input.GatherRed(customsampler, uv);
		float4 gggg = input.GatherGreen(customsampler, uv);
		float4 bbbb = input.GatherBlue(customsampler, uv);
		float4 aaaa = input.GatherAlpha(customsampler, uv);

		float4 color = 0;
		float sum = aaaa.x + aaaa.y + aaaa.z + aaaa.w;
		const bool preserve_coverage = mipgen.mipgen_options & MIPGEN_OPTION_BIT_PRESERVE_COVERAGE;
		if (preserve_coverage && sum > 0)
		{
			// Weight by alpha if it has even partially opaque pixels:
			//	This avoids losing alpha coverage, it will extrude the areas which have alpha
			//	And also avoids bleeding in background color from transparent area
			color.rgb += float3(rrrr.x, gggg.x, bbbb.x) * aaaa.x;
			color.rgb += float3(rrrr.y, gggg.y, bbbb.y) * aaaa.y;
			color.rgb += float3(rrrr.z, gggg.z, bbbb.z) * aaaa.z;
			color.rgb += float3(rrrr.w, gggg.w, bbbb.w) * aaaa.w;
			color.rgb /= sum;
			color.a = max(aaaa.x, max(aaaa.y, max(aaaa.z, aaaa.w)));
		}
		else
		{
			// If fully transparent, use simple average:
			//	weighting by alpha in this case would produce fully black, but we want to
			//	retain background color there instead (consider if alpha blending or testing is disabled,
			//	we still want seamless mip transition)
			color += float4(rrrr.x, gggg.x, bbbb.x, aaaa.x);
			color += float4(rrrr.y, gggg.y, bbbb.y, aaaa.y);
			color += float4(rrrr.z, gggg.z, bbbb.z, aaaa.z);
			color += float4(rrrr.w, gggg.w, bbbb.w, aaaa.w);
			color /= 4;
		}

		if (mipgen.mipgen_options & MIPGEN_OPTION_BIT_SRGB)
		{
			color.rgb = ApplySRGBCurve_Fast(color.rgb);
		}

		output[DTid.xy] = color;
	}
}
