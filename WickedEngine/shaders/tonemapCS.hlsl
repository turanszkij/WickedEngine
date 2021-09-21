#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(tonemap_push, PushConstantsTonemap);

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
	{0.59719, 0.35458, 0.04823},
	{0.07600, 0.90834, 0.01566},
	{0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
	{ 1.60475, -0.53108, -0.07367},
	{-0.10208,  1.10813, -0.00605},
	{-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

float3 ACESFitted(float3 color)
{
	color = mul(ACESInputMat, color);

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	color = mul(ACESOutputMat, color);

	// Clamp to [0, 1]
	color = saturate(color);

	return color;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * tonemap_push.resolution_rcp;
	float exposure = tonemap_push.exposure;
	const bool is_dither = tonemap_push.dither != 0;
	const bool is_colorgrading = tonemap_push.texture_colorgrade_lookuptable >= 0;
	const bool is_eyeadaption = tonemap_push.texture_input_luminance >= 0;
	const bool is_distortion = tonemap_push.texture_input_distortion >= 0;

	float2 distortion = 0;
	[branch]
	if (is_distortion)
	{
		distortion = bindless_textures[tonemap_push.texture_input_distortion].SampleLevel(sampler_linear_clamp, uv, 0).rg;
	}

	float4 hdr = bindless_textures[tonemap_push.texture_input].SampleLevel(sampler_linear_clamp, uv + distortion, 0);

	[branch]
	if (is_eyeadaption)
	{
		exposure *= tonemap_push.eyeadaptionkey / max(bindless_textures[tonemap_push.texture_input_luminance][uint2(0, 0)].r, 0.0001);
	}

	hdr.rgb *= exposure;
	float4 ldr = float4(ACESFitted(hdr.rgb), hdr.a);

#if 0 // DEBUG luminance
	if(DTid.x<800)
		ldr = average_luminance;
#endif

	ldr = saturate(ldr);
	ldr.rgb = GAMMA(ldr.rgb);

	if (is_colorgrading)
	{
		ldr.rgb = bindless_textures3D[tonemap_push.texture_colorgrade_lookuptable].SampleLevel(sampler_linear_clamp, ldr.rgb, 0).rgb;
	}

	if (is_dither)
	{
		// dithering before outputting to SDR will reduce color banding:
		ldr.rgb += (dither((float2)DTid.xy) - 0.5f) / 64.0f;
	}

	bindless_rwtextures[tonemap_push.texture_output][DTid.xy] = ldr;
}
