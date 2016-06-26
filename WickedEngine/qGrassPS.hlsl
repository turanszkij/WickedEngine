#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

GBUFFEROutputType main(QGS_OUT PSIn)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	float4 color = texture_0.Sample(sampler_linear_clamp,PSIn.tex);
	ALPHATEST(color.a)
	color = DEGAMMA(color);
	float emissive = 0;
	float3 N = PSIn.nor;
	float roughness = 1;
	float reflectance = 0;
	float metalness = 0;
	float ao = 1;
	float sss = 0;

	OBJECT_PS_OUT_GBUFFER
}