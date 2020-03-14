#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(input_luminance, float, TEXSLOT_ONDEMAND1);
TEXTURE2D(input_distortion, float4, TEXSLOT_ONDEMAND2);
TEXTURE2D(colorgrade_lookuptable, float4, TEXSLOT_ONDEMAND3);

RWTEXTURE2D(output, unorm float4, 0);

float4 sampleAs3DTexture(in float3 uv, in float width)
{
	float sliceSize = 1.0 / width;              // space of 1 slice
	float slicePixelSize = sliceSize / width;           // space of 1 pixel
	float sliceInnerSize = slicePixelSize * (width - 1.0);  // space of width pixels
	float zSlice0 = min(floor(uv.z * width), width - 1.0);
	float zSlice1 = min(zSlice0 + 1.0, width - 1.0);
	float xOffset = slicePixelSize * 0.5 + uv.x * sliceInnerSize;
	float s0 = xOffset + (zSlice0 * sliceSize);
	float s1 = xOffset + (zSlice1 * sliceSize);
	float4 slice0Color = colorgrade_lookuptable.SampleLevel(sampler_linear_clamp, float2(s0, uv.y), 0);
	float4 slice1Color = colorgrade_lookuptable.SampleLevel(sampler_linear_clamp, float2(s1, uv.y), 0);
	float zOffset = (uv.z * width) % 1.0;
	float4 result = lerp(slice0Color, slice1Color, zOffset);
	return result;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float exposure = xPPParams0.x;

	const float2 distortion = input_distortion.SampleLevel(sampler_linear_clamp, uv, 0).rg;
	float4 hdr = input.SampleLevel(sampler_linear_clamp, uv + distortion, 0);
	float average_luminance = input_luminance[uint2(0, 0)].r;

	hdr.rgb *= exposure;

	float luminance = dot(hdr.rgb, float3(0.2126, 0.7152, 0.0722));
	luminance /= average_luminance; // adaption
	hdr.rgb *= luminance;

	float4 ldr = saturate(float4(tonemap(hdr.rgb), hdr.a));

	ldr.rgb = GAMMA(ldr.rgb);

	if (tonemap_colorgrading)
	{
		float2 dim;
		colorgrade_lookuptable.GetDimensions(dim.x, dim.y);
		ldr.rgb = sampleAs3DTexture(ldr.rgb, dim.y).rgb;
	}

	if (tonemap_dither)
	{
		// dithering before outputting to SDR will reduce color banding:
		ldr.rgb += (dither((float2)DTid.xy) - 0.5f) / 64.0f;
	}

	output[DTid.xy] = ldr;
}