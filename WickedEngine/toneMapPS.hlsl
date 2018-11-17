#include "postProcessHF.hlsli"
#include "tonemapHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float2 distortion = xDistortionTex.SampleLevel(sampler_linear_clamp, PSIn.tex,0).rg;

	float4 hdr = xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex + distortion, 0);
	float exposure = xPPParams0.x;
	hdr.rgb *= exposure;

	float average_luminance = xMaskTex[uint2(0, 0)].r;
	float luminance = dot(hdr.rgb, float3(0.2126, 0.7152, 0.0722));
	luminance /= average_luminance; // adaption
	hdr.rgb *= luminance;

	float4 ldr = saturate(float4(tonemap(hdr.rgb), hdr.a));

	ldr.rgb = GAMMA(ldr.rgb);

	return ldr;
}