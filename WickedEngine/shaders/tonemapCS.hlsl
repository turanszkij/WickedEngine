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

float4x4 saturationMatrix(float saturation)
{
	float3 luminance = float3(0.3086f, 0.6094f, 0.0820f);
	float oneMinusSat = 1.0f - saturation;

	float3 red = float3(luminance * oneMinusSat);
	red += float3(saturation, 0, 0);

	float3 green = float3(luminance * oneMinusSat);
	green += float3(0, saturation, 0);

	float3 blue = float3(luminance * oneMinusSat);
	blue += float3(0, 0, saturation);

	return float4x4(red, 0, green, 0, blue, 0, 0, 0, 0, 1);
}

#ifndef __PSSL__
#undef WICKED_ENGINE_DEFAULT_ROOTSIGNATURE // don't use auto root signature!
[RootSignature(
	"RootConstants(num32BitConstants=16, b999),"
	"DescriptorTable( "
		"SRV(t0, space = 2, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 3, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 4, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 5, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 6, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 7, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 8, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 9, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 10, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 11, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 12, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 13, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 14, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 15, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 16, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 17, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 18, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 19, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"SRV(t0, space = 20, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 21, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 22, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 23, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 24, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 25, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 26, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 27, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 28, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 29, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 30, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 31, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 32, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE),"
		"UAV(u0, space = 33, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)"
	"), "
	"StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"
)]
#endif // __PSSL__

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5f) * tonemap_push.resolution_rcp;
	float exposure = tonemap_push.exposure;

	[branch]
	if (tonemap_push.texture_input_distortion >= 0)
	{
		uv += bindless_textures[tonemap_push.texture_input_distortion].SampleLevel(sampler_linear_clamp, uv, 0).rg;
	}

	float4 hdr = 0;

	[branch]
	if (tonemap_push.texture_input >= 0)
	{
		hdr = bindless_textures[tonemap_push.texture_input].SampleLevel(sampler_linear_clamp, uv, 0);
	}

	exposure *= bindless_buffers[tonemap_push.buffer_input_luminance].Load<float>(LUMINANCE_BUFFER_OFFSET_EXPOSURE);
	hdr.rgb *= exposure;

	[branch]
	if (tonemap_push.texture_bloom >= 0)
	{
		Texture2D<float4> texture_bloom = bindless_textures[tonemap_push.texture_bloom];
		float3 bloom = texture_bloom.SampleLevel(sampler_linear_clamp, uv, 1.5f).rgb;
		bloom += texture_bloom.SampleLevel(sampler_linear_clamp, uv, 3.5f).rgb;
		bloom += texture_bloom.SampleLevel(sampler_linear_clamp, uv, 4.5f).rgb;
		bloom /= 3.0f;
		hdr.rgb += bloom;
	}

	float4 result = hdr;

	[branch]
	if (tonemap_push.display_colorspace == (uint)ColorSpace::SRGB)
	{
		if (tonemap_push.flags & TONEMAP_FLAG_ACES)
		{
			result.rgb = ACESFitted(hdr.rgb);
		}
		else
		{
			result.rgb = tonemap(hdr.rgb);
		}
		result.rgb = ApplySRGBCurve_Fast(result.rgb);
	}

	[branch]
	if (tonemap_push.texture_colorgrade_lookuptable >= 0)
	{
		result.rgb = bindless_textures3D[tonemap_push.texture_colorgrade_lookuptable].SampleLevel(sampler_linear_clamp, result.rgb, 0).rgb;
	}

	[branch]
	if (tonemap_push.flags & TONEMAP_FLAG_DITHER)
	{
		// dithering before outputting to SDR will reduce color banding:
		result.rgb += (dither((float2)DTid.xy) - 0.5f) / 64.0f;
	}

	result.rgb = (result.rgb - 0.5f) * tonemap_push.contrast + 0.5f + tonemap_push.brightness;
	result.rgb = (float3)(mul(saturationMatrix(tonemap_push.saturation), result));

	[branch]
	if (tonemap_push.texture_output >= 0)
	{
		bindless_rwtextures[tonemap_push.texture_output][DTid.xy] = result;
	}
}
