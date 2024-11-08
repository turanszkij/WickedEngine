#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(tonemap_push, PushConstantsTonemap);

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const half3x3 ACESInputMat =
{
	{0.59719, 0.35458, 0.04823},
	{0.07600, 0.90834, 0.01566},
	{0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const half3x3 ACESOutputMat =
{
	{ 1.60475, -0.53108, -0.07367},
	{-0.10208,  1.10813, -0.00605},
	{-0.00327, -0.07276,  1.07602}
};

half3 RRTAndODTFit(half3 v)
{
	half3 a = v * (v + 0.0245786) - 0.000090537;
	half3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return a / b;
}

half3 ACESFitted(half3 color)
{
	color = mul(ACESInputMat, color);

	color = clamp(color, 0, 100); // fix for half precision issue with very bright values near point light center

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
	float2 uv = (DTid.xy + 0.5) * tonemap_push.resolution_rcp;
	
	if(!GetCamera().is_uv_inside_scissor(uv)) // Note: uv scissoring is used because this supports upscaled resolution (FSR 2)
		return;
	
	half4 exposure_brightness_contrast_saturation = unpack_half4(tonemap_push.exposure_brightness_contrast_saturation);
	half exposure = exposure_brightness_contrast_saturation.x;
	half brightness = exposure_brightness_contrast_saturation.y;
	half contrast = exposure_brightness_contrast_saturation.z;
	half saturation = exposure_brightness_contrast_saturation.w;
	min16uint flags = tonemap_push.flags_hdrcalibration & 0xFFFF;
	half hdr_calibration = f16tof32(tonemap_push.flags_hdrcalibration >> 16u);

	[branch]
	if (tonemap_push.texture_input_distortion >= 0)
	{
		uv += bindless_textures[tonemap_push.texture_input_distortion].SampleLevel(sampler_linear_clamp, uv, 0).rg;
	}

	[branch]
	if (tonemap_push.texture_input_distortion_overlay >= 0)
	{
		uv += bindless_textures[tonemap_push.texture_input_distortion_overlay].SampleLevel(sampler_linear_clamp, uv, 0).rg * 2 - 1;
	}

	half4 result = 0;

	[branch]
	if (tonemap_push.texture_input >= 0)
	{
		result = bindless_textures[tonemap_push.texture_input].SampleLevel(sampler_linear_clamp, uv, 0);
	}

	exposure *= bindless_buffers[tonemap_push.buffer_input_luminance].Load<float>(LUMINANCE_BUFFER_OFFSET_EXPOSURE);
	result.rgb *= exposure;

	[branch]
	if (tonemap_push.texture_bloom >= 0)
	{
		Texture2D<float4> texture_bloom = bindless_textures[tonemap_push.texture_bloom];
		half3 bloom = texture_bloom.SampleLevel(sampler_linear_clamp, uv, 1.5).rgb;
		bloom += texture_bloom.SampleLevel(sampler_linear_clamp, uv, 3.5).rgb;
		bloom += texture_bloom.SampleLevel(sampler_linear_clamp, uv, 4.5).rgb;
		bloom /= 3.0;
		result.rgb += bloom;
	}

	[branch]
	if (flags & TONEMAP_FLAG_SRGB)
	{
		if (flags & TONEMAP_FLAG_ACES)
		{
			result.rgb = ACESFitted(result.rgb);
		}
		else
		{
			result.rgb = tonemap(result.rgb);
		}
		result.rgb = ApplySRGBCurve_Fast(result.rgb);
	}
	else
	{
		result *= hdr_calibration;
	}

	[branch]
	if (tonemap_push.texture_colorgrade_lookuptable >= 0)
	{
		result.rgb = bindless_textures3D[tonemap_push.texture_colorgrade_lookuptable].SampleLevel(sampler_linear_clamp, result.rgb, 0).rgb;
	}

	[branch]
	if (flags & TONEMAP_FLAG_DITHER)
	{
		// dithering before outputting to SDR will reduce color banding:
		result.rgb += (dither(DTid.xy) - 0.5) / 64.0;
	}

	result.rgb = (result.rgb - 0.5) * contrast + 0.5 + brightness;
	result.rgb = mul(saturationMatrix(saturation), result.rgb);

	result = saturateMediump(result);

	[branch]
	if (tonemap_push.texture_output >= 0)
	{
		bindless_rwtextures[tonemap_push.texture_output][DTid.xy] = result;
	}
}
