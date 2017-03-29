#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

GBUFFEROutputType_Thin main(GS_OUT PSIn)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	float4 baseColor = float4(PSIn.col,1);
	float opacity = baseColor.a;
	baseColor = DEGAMMA(baseColor);
	float4 color = baseColor;
	float3 P = PSIn.pos3D;
	float3 V = normalize(g_xCamera_CamPos - P);
	float emissive = 0;
	float3 N = PSIn.nor;
	float roughness = 1;
	float reflectance = 0;
	float metalness = 0;
	float ao = 1;
	float sss = 0;
	float2 pixel = PSIn.pos.xy;
	float3 diffuse = 0;
	float3 specular = 0;
	float2 velocity = 0;

	OBJECT_PS_LIGHT_BEGIN

	OBJECT_PS_LIGHT_DIRECTIONAL

	OBJECT_PS_LIGHT_END

	OBJECT_PS_OUT_FORWARD
}