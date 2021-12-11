#include "imageHF.hlsli"

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	SamplerState sam = bindless_samplers[push.sampler_index];

	float4 color = unpack_half4(push.packed_color);
	[branch]
	if (push.texture_base_index >= 0)
	{
		color *= bindless_textures[push.texture_base_index].Sample(sam, uv);
	}

	[branch]
	if (push.flags & IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084)
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
	else if (push.flags & IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR)
	{
		color.rgb = DEGAMMA(color.rgb);
		color.rgb *= push.corners0.x; // hdr_scaling (corners0 is not used for anything else in full screen image rendering)
	}

	return color;
}
