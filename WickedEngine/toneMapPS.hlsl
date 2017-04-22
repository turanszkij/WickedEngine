#include "postProcessHF.hlsli"
#include "tonemapHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 hdr = xTexture.Load(uint3(PSIn.pos.xy,0));
	float average_luminance = xMaskTex.Load(uint3(0,0,0)).r;

	float luminance = dot(hdr.rgb, float3(0.2126, 0.7152, 0.0722));
	luminance /= average_luminance; // adaption
	hdr *= luminance;

	float4 ldr = saturate(float4(tonemap(hdr.rgb), hdr.a));

	ldr = GAMMA(ldr);

	return ldr;
}