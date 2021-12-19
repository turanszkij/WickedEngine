#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	SamplerState sam = bindless_samplers[image_push.sampler_index];

	float4 uvsets = input.compute_uvs();

	float4 color = unpack_half4(image_push.packed_color);
	[branch]
	if (image_push.texture_base_index >= 0)
	{
		float4 tex = bindless_textures[image_push.texture_base_index].Sample(sam, uvsets.xy);

		if (image_push.flags & IMAGE_FLAG_EXTRACT_NORMALMAP)
		{
			tex.rgb = tex.rgb * 2 - 1;
		}

		color *= tex;
	}

	float4 mask = 1;
	[branch]
	if (image_push.texture_mask_index >= 0)
	{
		mask = bindless_textures[image_push.texture_mask_index].Sample(sam, uvsets.zw);
	}
	color *= mask;

	[branch]
	if (image_push.texture_background_index >= 0)
	{
		float3 background = bindless_textures[image_push.texture_background_index].Sample(sam, input.uv_screen()).rgb;
		color = float4(lerp(background, color.rgb, color.a), mask.a);
	}

	[branch]
	if (image_push.flags & IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084)
	{
		// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/presentPS.hlsl
		const float referenceWhiteNits = 80.0;
		const float st2084max = 10000.0;
		const float hdrScalar = referenceWhiteNits / st2084max;
		// The input is in Rec.709, but the display is Rec.2020
		color.rgb = REC709toREC2020(color.rgb);
		// Apply the ST.2084 curve to the result.
		color.rgb = ApplyREC2084Curve(color.rgb * hdrScalar);
	}
	else if (image_push.flags & IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR)
	{
		color.rgb = DEGAMMA(color.rgb);
		color.rgb *= image_push.hdr_scaling;
	}

	return color;
}
