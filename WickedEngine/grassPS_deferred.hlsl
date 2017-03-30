#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

GBUFFEROutputType main(GS_OUT PSIn)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	float4 color = float4(PSIn.col,1);
	color = DEGAMMA(color);
	float emissive = 0;
	float3 N = PSIn.nor;
	float roughness = 0;
	float reflectance = 0;
	float metalness = 0;
	float ao = 1;
	float sss = 0;
	float2 velocity = 0;

	OBJECT_PS_OUT_GBUFFER
}