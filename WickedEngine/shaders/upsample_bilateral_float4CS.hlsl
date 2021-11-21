#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifndef UPSAMPLE_FORMAT
#define UPSAMPLE_FORMAT float4
#endif // UPSAMPLE_FORMAT

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<UPSAMPLE_FORMAT> input : register(t0);

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
	const float lowres_depthchain_mip = postprocess.params0.w;
	const float2 lowres_size = postprocess.params1.xy;
	const float2 lowres_texel_size = postprocess.params1.zw;

	const float2 uv00 = uv - lowres_texel_size * 0.5;
	const float2 uv10 = uv00 + float2(lowres_texel_size.x, 0);
	const float2 uv01 = uv00 + float2(0, lowres_texel_size.y);
	const float2 uv11 = uv00 + lowres_texel_size;

	const float4 lineardepth_highres = texture_lineardepth[pixel].xxxx;
	const float4 lineardepth_lowres = float4(
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv00, lowres_depthchain_mip),
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv10, lowres_depthchain_mip),
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv01, lowres_depthchain_mip),
		texture_lineardepth.SampleLevel(sampler_point_clamp, uv11, lowres_depthchain_mip)
	);

	const float4 depth_diff = abs(lineardepth_highres - lineardepth_lowres) * GetCamera().z_far;
	const float accum_diff = dot(depth_diff, float4(1, 1, 1, 1));

	UPSAMPLE_FORMAT color;

	[branch]
	if (accum_diff < threshold)
	{
#ifdef UPSAMPLE_DISABLE_FILTERING
		color = input[floor(uv * lowres_size)];
#else
		// small error, take bilinear sample:
		color = input.SampleLevel(sampler_linear_clamp, uv, 0);
#endif // UPSAMPLE_DISABLE_FILTERING
	}
	else
	{
		// large error:

#ifdef UPSAMPLE_DISABLE_FILTERING
		// find nearest sample to current depth:
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
		color = input[floor(uv_nearest * lowres_size)];
#else

		// depth-weighted blending:
		float4 weights = saturate(1 - abs(lineardepth_highres - lineardepth_lowres));
		float sum = weights[0] + weights[1] + weights[2] + weights[3];
		weights /= max(sum, 0.0001);

		color = 0;
		color += input.SampleLevel(sampler_point_clamp, uv00, 0) * weights[0];
		color += input.SampleLevel(sampler_point_clamp, uv10, 0) * weights[1];
		color += input.SampleLevel(sampler_point_clamp, uv01, 0) * weights[2];
		color += input.SampleLevel(sampler_point_clamp, uv11, 0) * weights[3];

#endif // UPSAMPLE_DISABLE_FILTERING
	}

#ifdef USE_PIXELSHADER
	return color;
#else
	output[DTid.xy] = color;
#endif // USE_PIXELSHADER
}
