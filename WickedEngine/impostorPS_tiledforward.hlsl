#include "globals.hlsli"
#include "impostorHF.hlsli"

float4 main(VSOut input) : SV_TARGET
{
	float3 uv_col = input.tex;
	float3 uv_nor = uv_col + impostorCaptureAngles;
	float3 uv_sur = uv_nor + impostorCaptureAngles;

	float4 color = impostorTex.Sample(sampler_linear_clamp, uv_col);
	float4 normal_roughness = impostorTex.Sample(sampler_linear_clamp, uv_nor);
	float4 surface = impostorTex.Sample(sampler_linear_clamp, uv_sur);

	return color;
}