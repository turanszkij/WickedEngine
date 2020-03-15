#include "globals.hlsli"
#include "impostorHF.hlsli"

float4 main(VSOut input) : SV_TARGET
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.dither);

	float3 uv_col = input.tex;

	float4 color = impostorTex.Sample(sampler_linear_clamp, uv_col) * unpack_rgba(input.instanceColor);
	ALPHATEST(color.a);

	return color;
}