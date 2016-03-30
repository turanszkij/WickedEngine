#include "postProcessHF.hlsli"


static const float WHITE = pow(1, 2);
static const float ALPHA = 0.72;

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 hdr = xTexture.Load(uint3(PSIn.pos.xy,0));
	float average_luminance = xMaskTex.Load(uint3(0,0,0)).r;

	float luminance = dot(hdr.rgb, float3(0.2126, 0.7152, 0.0722));
	luminance = (luminance * (1 + luminance / WHITE)) / (1 + luminance); // Reinhard
	luminance *= ALPHA / average_luminance;

	//luminance = 1;

	float4 ldr = float4(saturate(hdr.rgb * luminance), hdr.a);

	ldr = GAMMA(ldr);

	return ldr;
}