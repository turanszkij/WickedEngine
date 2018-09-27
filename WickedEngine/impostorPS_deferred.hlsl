#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "ditherHF.hlsli"
#include "objectHF.hlsli"

GBUFFEROutputType main(VSOut input)
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.dither);

	float3 uv_col = input.tex;
	float3 uv_nor = uv_col + impostorCaptureAngles;
	float3 uv_sur = uv_nor + impostorCaptureAngles;

	float4 color = impostorTex.Sample(sampler_linear_clamp, uv_col);
	ALPHATEST(color.a);

	float4 normal_roughness = impostorTex.Sample(sampler_linear_clamp, uv_nor);
	float4 surface = impostorTex.Sample(sampler_linear_clamp, uv_sur);

	float3 N = normal_roughness.rgb;
	float roughness = normal_roughness.a;

	float reflectance = surface.r;
	float metalness = surface.g;
	float emissive = surface.b;

	GBUFFEROutputType Out;
	Out.g0 = float4(color.rgb, 1);								/*FORMAT_R8G8B8A8_UNORM*/
	Out.g1 = float4(encode(N), 0, 0);							/*FORMAT_R16G16B16A16_FLOAT*/
	Out.g2 = float4(0, 0, 0, emissive);							/*FORMAT_R8G8B8A8_UNORM*/
	Out.g3 = float4(roughness, reflectance, metalness, 1);		/*FORMAT_R8G8B8A8_UNORM*/
	return Out;
}