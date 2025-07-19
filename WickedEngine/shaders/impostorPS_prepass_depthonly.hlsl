#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "objectHF.hlsli"

void main(VSOut input, out uint coverage : SV_Coverage)
{
	float3 uv_col = float3(input.uv, input.slice);
	float alpha = impostorTex.Sample(sampler_linear_clamp, uv_col).a;
	coverage = AlphaToCoverage(alpha, 0.75, input.GetDither(), input.pos);
}
