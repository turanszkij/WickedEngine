#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "ditherHF.hlsli"

float4 main(VSOut input) : SV_TARGET
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.dither);

	float3 uv_col = input.tex;
	float3 uv_nor = uv_col + impostorCaptureAngles;
	float3 uv_sur = uv_nor + impostorCaptureAngles;

	float4 color = impostorTex.Sample(sampler_linear_clamp, uv_col);
	ALPHATEST(color.a);

	float4 normal_roughness = impostorTex.Sample(sampler_linear_clamp, uv_nor);
	float4 surface = impostorTex.Sample(sampler_linear_clamp, uv_sur);

	return color;
}