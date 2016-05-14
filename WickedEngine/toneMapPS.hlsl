#include "postProcessHF.hlsli"

static const float strength = 0.85;

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 hdr = xTexture.Load(uint3(PSIn.pos.xy,0));
	float average_luminance = xMaskTex.Load(uint3(0,0,0)).r;

	if (PSIn.pos.x < 100 && PSIn.pos.y < 100)
		return float4(average_luminance.xxx,1);

	float luminance = dot(hdr.rgb, float3(0.2126, 0.7152, 0.0722));
	luminance = (luminance * (1 + luminance)) / (1 + luminance); // Reinhard
	luminance *= strength / average_luminance; // adaption

	//luminance = 1;

	//hdr = GAMMA(hdr);

	float4 ldr = float4(saturate(hdr.rgb * luminance), hdr.a);

	ldr = GAMMA(ldr);

	return ldr;
}