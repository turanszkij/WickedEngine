#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

float4 main(GS_OUT PSIn) : SV_Target
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	float4 color = float4(PSIn.col,1);
	color = DEGAMMA(color);
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

	OBJECT_PS_LIGHT_BEGIN

	OBJECT_PS_LIGHT_TILED

	OBJECT_PS_LIGHT_END

	OBJECT_PS_OUT_FORWARD
}