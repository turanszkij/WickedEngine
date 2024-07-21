#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	SamplerState sam = bindless_samplers[image.sampler_index];

	float4 uvsets = input.compute_uvs();

	half4 color = unpack_half4(image.packed_color);
	[branch]
	if (image.texture_base_index >= 0)
	{
		half4 tex = bindless_textures[image.texture_base_index].Sample(sam, uvsets.xy);

		if (image.flags & IMAGE_FLAG_EXTRACT_NORMALMAP)
		{
			tex.rgb = tex.rgb * 2 - 1;
		}

		color *= tex;
	}

	half4 mask = 1;
	[branch]
	if (image.texture_mask_index >= 0)
	{
		mask = bindless_textures[image.texture_mask_index].Sample(sam, uvsets.zw);
	}

	const half2 mask_alpha_range = unpack_half2(image.mask_alpha_range);
	mask.a = smoothstep(mask_alpha_range.x, mask_alpha_range.y, mask.a);
	
	if(image.flags & IMAGE_FLAG_DISTORTION_MASK)
	{
		// Only mask alpha is used for multiplying, rg is used for distorting background:
		color.a *= mask.a;
	}
	else
	{
		color *= mask;
	}

	[branch]
	if (image.texture_background_index >= 0)
	{
		Texture2D backgroundTexture = bindless_textures[image.texture_background_index];
		float2 uv_screen = input.uv_screen();
		if(image.flags & IMAGE_FLAG_DISTORTION_MASK)
		{
			uv_screen += mask.rg * 2 - 1;
		}
		half3 background = backgroundTexture.Sample(sam, uv_screen).rgb;
		color = half4(lerp(background, color.rgb, color.a), mask.a);
	}

	[branch]
	if (image.flags & IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084)
	{
		// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/presentPS.hlsl
		const half referenceWhiteNits = 80.0;
		const half st2084max = 10000.0;
		const half hdrScalar = referenceWhiteNits / st2084max;
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
	
	[branch]
	if (image.border_soften > 0)
	{
		half edge = max(abs(input.edge.x), abs(input.edge.y));
		color.a *= smoothstep(0, image.border_soften, 1 - edge);
	}

	[branch]
	if (image.angular_softness_scale > 0)
	{
		float2 direction = normalize(uvsets.xy - 0.5);
		float dp = dot(direction, image.angular_softness_direction);
		if (image.flags & IMAGE_FLAG_ANGULAR_DOUBLESIDED)
		{
			dp = abs(dp);
		}
		else
		{
			dp = saturate(dp);
		}
		float angular = saturate(mad(dp, image.angular_softness_scale, image.angular_softness_offset));
		if (image.flags & IMAGE_FLAG_ANGULAR_INVERSE)
		{
			angular = 1 - angular;
		}
		angular = smoothstep(0, 1, angular);
		color.a *= angular;
	}

	return color;
}
