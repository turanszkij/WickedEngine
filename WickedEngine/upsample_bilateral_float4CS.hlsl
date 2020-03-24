#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifndef UPSAMPLE_FORMAT
#define UPSAMPLE_FORMAT float4
#endif // UPSAMPLE_FORMAT

TEXTURE2D(input, UPSAMPLE_FORMAT, TEXSLOT_ONDEMAND0);

// Note: this post process can be either a pixel shader or compute shader, depending on use case

#ifdef USE_PIXELSHADER
// Run this post process as pixel shader:
float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
#else
// Run this post process as compute shader:
RWTEXTURE2D(output, UPSAMPLE_FORMAT, 0);
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
#endif // USE_PIXELSHADER


	const float threshold = xPPParams0.x;
	const float2 lowres_texel_size = xPPParams0.yz;
	const float lowres_depthchain_mip = xPPParams0.w;

	const float2 uv00 = uv - 0.5f * lowres_texel_size;
	const float2 uv10 = uv00 + float2(lowres_texel_size.x, 0);
	const float2 uv01 = uv00 + float2(0, lowres_texel_size.y);
	const float2 uv11 = uv00 + lowres_texel_size;

	const float4 lineardepth_highres = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0).xxxx;
	const float4 lineardepth_lowres = float4(
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv00, lowres_depthchain_mip),
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv10, lowres_depthchain_mip),
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv01, lowres_depthchain_mip),
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv11, lowres_depthchain_mip)
	);

	const float4 depth_diff = abs(lineardepth_highres - lineardepth_lowres) * g_xCamera_ZFarP;
	const float accum_diff = dot(depth_diff, float4(1, 1, 1, 1));

	UPSAMPLE_FORMAT color;

	[branch]
	if (accum_diff < threshold)
	{
		// small error, take bilinear sample:
		color = input.SampleLevel(sampler_linear_clamp, uv, 0);
	}
	else
	{
		// large error, find nearest sample to current depth:
		float min_depth_diff = depth_diff[0];
		float2 uv_nearest = uv00;

		if (depth_diff[1] < min_depth_diff)
		{
			uv_nearest = uv10;
			min_depth_diff = depth_diff[1];
		}

		if (depth_diff[2] < min_depth_diff)
		{
			uv_nearest = uv01;
			min_depth_diff = depth_diff[2];
		}

		if (depth_diff[3] < min_depth_diff)
		{
			uv_nearest = uv11;
			min_depth_diff = depth_diff[3];
		}

		color = input.SampleLevel(sampler_point_clamp, uv_nearest, 0);
	}

#ifdef USE_PIXELSHADER
	return color;
#else
	output[DTid.xy] = color;
#endif // USE_PIXELSHADER
}
