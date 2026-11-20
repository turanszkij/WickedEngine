#include "globals.hlsli"
#include "impostorHF.hlsli"

void main(VSOut input)
{
	float3 uv_col = float3(input.uv, input.slice);
	float alpha = impostorTex.Sample(sampler_linear_clamp, uv_col).a;
	clip(alpha - 0.75);
}
