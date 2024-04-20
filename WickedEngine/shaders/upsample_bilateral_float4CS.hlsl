#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifndef UPSAMPLE_FORMAT
#define UPSAMPLE_FORMAT float4
#endif // UPSAMPLE_FORMAT

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<UPSAMPLE_FORMAT> input : register(t0);
Texture2D<float> input_lineardepth_high : register(t1);
Texture2D<float> input_lineardepth_low : register(t2);

static const float2 offsets[] = {
	float2(-0.5, -0.5),
	float2(0.5, -0.5),
	float2(-0.5, 0.5),
	float2(0.5, 0.5),
};

// Note: this post process can be either a pixel shader or compute shader, depending on use case

#ifdef USE_PIXELSHADER
// Run this post process as pixel shader:
float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	const uint2 pixel = pos.xy;
#else
// Run this post process as compute shader:
RWTexture2D<UPSAMPLE_FORMAT> output : register(u0);
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 pixel = DTid.xy;
	const float2 uv = (pixel + 0.5f) * postprocess.resolution_rcp;
#endif // USE_PIXELSHADER

	const float threshold = postprocess.params0.x;
	const uint lowres_depthchain_mip = uint(postprocess.params0.w);
	const float2 lowres_size = postprocess.params1.xy;
	const float2 lowres_texel_size = postprocess.params1.zw;
	
	const float lineardepth_highres = input_lineardepth_high[pixel] * GetCamera().z_far;
	
	UPSAMPLE_FORMAT color = 0;
	float sum = 0;

	int2 lowres_pixel = int2(float2(pixel) * postprocess.params0.w);

	for(uint i = 0; i < 4; ++i)
	{
		const float2 sample_uv = uv + offsets[i] * lowres_texel_size;
		const float4 zzzz = input_lineardepth_low.GatherRed(sampler_linear_clamp, sample_uv) * GetCamera().z_far;
		const float4 wwww = max(0.001, 1 - saturate(abs(zzzz - lineardepth_highres) * threshold));
		const float4 rrrr = input.GatherRed(sampler_linear_clamp, sample_uv);
		const float4 gggg = input.GatherGreen(sampler_linear_clamp, sample_uv);
		const float4 bbbb = input.GatherBlue(sampler_linear_clamp, sample_uv);
		const float4 aaaa = input.GatherAlpha(sampler_linear_clamp, sample_uv);
		
		float2 sam_pixel = sample_uv * lowres_size + (-0.5 + 1.0 / 512.0); // (1.0 / 512.0) correction is described here: https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/
		float2 sam_pixel_frac = frac(sam_pixel);

		color += (UPSAMPLE_FORMAT)float4(
			bilinear(rrrr * wwww, sam_pixel_frac),
			bilinear(gggg * wwww, sam_pixel_frac),
			bilinear(bbbb * wwww, sam_pixel_frac),
			bilinear(aaaa * wwww, sam_pixel_frac)
		);
		
		float weight = bilinear(wwww, sam_pixel_frac);
		sum += weight;
	}
	
	if(sum > 0)
	{
		color /= sum;
	}

#ifdef USE_PIXELSHADER
	return color;
#else
	output[DTid.xy] = color;
#endif // USE_PIXELSHADER
}
