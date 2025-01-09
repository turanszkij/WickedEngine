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
	Texture2D<half4> tex = bindless_textures_half4[descriptor_index(font.texture_index)];
	half value = tex.SampleLevel(sampler_linear_clamp, input.uv, 0).r;
	half4 color = unpack_half4(font.color);
	
	const half3 softness_bolden_hdrscaling = unpack_half3(font.softness_bolden_hdrscaling);
	const half softness = softness_bolden_hdrscaling.x;
	const half bolden = softness_bolden_hdrscaling.y;
	const half hdr_scaling = softness_bolden_hdrscaling.z;
	const min16uint flags = font.softness_bolden_hdrscaling.y >> 16u;

	[branch]
	if (flags & FONT_FLAG_SDF_RENDERING)
	{
		float2 bary_fw = fwidth(input.bary);
		half w = max(bary_fw.x, bary_fw.y); // screen coverage dependency
		w = max(w, 1.0 / 255.0); // min softness to avoid pixelated hard edge in magnification
		w += softness;
		w = saturate(w);
		half mid = lerp((half)SDF::onedge_value_unorm, 0, bolden);
		color.a *= smoothstep(saturate(mid - w), saturate(mid + w), value);
	}
	else
	{
		color.a *= value;
	}

	[branch]
	if (flags & FONT_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084)
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
	else if (flags & FONT_FLAG_OUTPUT_COLOR_SPACE_LINEAR)
	{
		color.rgb = RemoveSRGBCurve_Fast(color.rgb);
		color.rgb *= hdr_scaling;
	}

	return color;
}
