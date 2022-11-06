#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	float value = bindless_textures[font.texture_index].SampleLevel(sampler_linear_clamp, input.uv, 0).r;
	float4 color = unpack_rgba(font.color);

	[branch]
	if (font.flags & FONT_FLAG_SDF_RENDERING)
	{
		color.a *= smoothstep(font.sdf_threshold_bottom, font.sdf_threshold_top, value);
	}
	else
	{
		color.a *= value;
	}

	[branch]
	if (font.flags & FONT_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084)
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
	else if (font.flags & FONT_FLAG_OUTPUT_COLOR_SPACE_LINEAR)
	{
		color.rgb *= font.hdr_scaling;
	}

	return color;
}
