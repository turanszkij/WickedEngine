#include "globals.hlsli"
#include "impostorHF.hlsli"

void main(VSOut input)
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.dither);
	float3 uv_col = input.tex;
	clip(impostorTex.Sample(sampler_linear_clamp, uv_col).a - 0.5f);
}
