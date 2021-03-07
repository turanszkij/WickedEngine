#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifdef BINDLESS
PUSHCONSTANT(push, PushConstantsTonemap);
Texture2D<float4> bindless_textures[] : register(t0, space1);
Texture3D<float4> bindless_textures3D[] : register(t0, space2);
RWTexture2D<float4> bindless_rwtextures[] : register(u0, space3);
#else
TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(input_luminance, float, TEXSLOT_ONDEMAND1);
TEXTURE2D(input_distortion, float4, TEXSLOT_ONDEMAND2);
TEXTURE3D(colorgrade_lookuptable, float4, TEXSLOT_ONDEMAND3);

RWTEXTURE2D(output, unorm float4, 0);
#endif // BINDLESS

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
#ifdef BINDLESS
	const float2 uv = (DTid.xy + 0.5f) * push.xPPResolution_rcp;
	const float exposure = push.exposure;
	const bool is_dither = push.dither != 0;
	const bool is_colorgrading = push.colorgrading != 0;

	const float2 distortion = bindless_textures[push.texture_input_distortion].SampleLevel(sampler_linear_clamp, uv, 0).rg;
	float4 hdr = bindless_textures[push.texture_input].SampleLevel(sampler_linear_clamp, uv + distortion, 0);
	float average_luminance = bindless_textures[push.texture_input_luminance][uint2(0, 0)].r;
#else
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float exposure = tonemap_exposure;
	const bool is_dither = tonemap_dither != 0;
	const bool is_colorgrading = tonemap_colorgrading != 0;

	const float2 distortion = input_distortion.SampleLevel(sampler_linear_clamp, uv, 0).rg;
	float4 hdr = input.SampleLevel(sampler_linear_clamp, uv + distortion, 0);
	float average_luminance = input_luminance[uint2(0, 0)].r;
#endif // BINDLESS

	hdr.rgb *= exposure;

	float luminance = dot(hdr.rgb, float3(0.2126, 0.7152, 0.0722));
	luminance /= average_luminance; // adaption
	hdr.rgb *= luminance;

	float4 ldr = saturate(float4(tonemap(hdr.rgb), hdr.a));

	ldr.rgb = GAMMA(ldr.rgb);

	if (is_colorgrading)
	{
#ifdef BINDLESS
		ldr.rgb = bindless_textures3D[push.texture_colorgrade_lookuptable].SampleLevel(sampler_linear_clamp, ldr.rgb, 0).rgb;
#else
		ldr.rgb = colorgrade_lookuptable.SampleLevel(sampler_linear_clamp, ldr.rgb, 0).rgb;
#endif // BINDLESS
	}

	if (is_dither)
	{
		// dithering before outputting to SDR will reduce color banding:
		ldr.rgb += (dither((float2)DTid.xy) - 0.5f) / 64.0f;
	}

#ifdef BINDLESS
	bindless_rwtextures[push.texture_output][DTid.xy] = ldr;
#else
	output[DTid.xy] = ldr;
#endif // BINDLESS
}
