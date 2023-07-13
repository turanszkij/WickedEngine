#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	SamplerState sam = bindless_samplers[image.sampler_index];

	float4 uvsets = input.compute_uvs();

	float4 color = unpack_half4(image.packed_color);
	[branch]
	if (image.texture_base_index >= 0)
	{
		float4 tex = bindless_textures[image.texture_base_index].Sample(sam, uvsets.xy);

		if (image.flags & IMAGE_FLAG_EXTRACT_NORMALMAP)
		{
			tex.rgb = tex.rgb * 2 - 1;
		}

		color *= tex;
	}

	float4 mask = 1;
	[branch]
	if (image.texture_mask_index >= 0)
	{
		mask = bindless_textures[image.texture_mask_index].Sample(sam, uvsets.zw);
	}

	const float2 mask_alpha_range = unpack_half2(image.mask_alpha_range);
	mask.a = smoothstep(mask_alpha_range.x, mask_alpha_range.y, mask.a);
	
	color *= mask;

	[branch]
	if (image.texture_background_index >= 0)
	{
		float3 background = bindless_textures[image.texture_background_index].Sample(sam, input.uv_screen()).rgb;
		color = float4(lerp(background, color.rgb, color.a), mask.a);
	}

	[branch]
	if (image.flags & IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084)
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
	else if (image.flags & IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR)
	{
		color.rgb = RemoveSRGBCurve_Fast(color.rgb);
		color.rgb *= image.hdr_scaling;
	}

	if (image.border_soften > 0)
	{
		float edge = max(abs(input.edge.x), abs(input.edge.y));
		color.a *= smoothstep(0, image.border_soften, 1 - edge);
	}

	return color;
}
