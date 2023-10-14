#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
	float2 bary : TEXCOORD1;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	Texture2D tex = bindless_textures[font.texture_index];
	float value = tex.SampleLevel(sampler_linear_clamp, input.uv, 0).r;
	float4 color = font.color;

	[branch]
	if (font.flags & FONT_FLAG_SDF_RENDERING)
	{
		float2 bary_fw = fwidth(input.bary);
		float w = max(bary_fw.x, bary_fw.y); // screen coverage dependency
		w = max(w, 1.0 / 255.0); // min softness to avoid pixelated hard edge in magnification
		w += font.softness;
		w = saturate(w);
		float mid = lerp(SDF::onedge_value_unorm, 0, font.bolden);
		color.a *= smoothstep(saturate(mid - w), saturate(mid + w), value);
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
		color.rgb = RemoveSRGBCurve_Fast(color.rgb);
		color.rgb *= font.hdr_scaling;
	}

	return color;
}
