#define DISABLE_DECALS
#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

[earlydepthstencil]
float4 main(QGS_OUT PSIn) : SV_Target
{
	float4 color = texture_0.Sample(sampler_linear_clamp,PSIn.tex);
	color.a *= 1.0 - PSIn.fade;
	clip(color.a - 1.0f / 256.0f); // cancel heaviest overdraw for the alpha composition effect
	float opacity = 1;
	color = DEGAMMA(color);
	float3 P = PSIn.pos3D;
	float3 V = g_xCamera_CamPos - P;
	float dist = length(V);
	V /= dist;
	float emissive = 0;
	float3 N = PSIn.nor;
	float roughness = 1;
	float reflectance = 0;
	float metalness = 0;
	float ao = 1;
	float sss = 0;
	float2 pixel = PSIn.pos.xy; 
	float depth = PSIn.pos.z;

	OBJECT_PS_LIGHT_BEGIN

	OBJECT_PS_LIGHT_TILED

	OBJECT_PS_LIGHT_END

	OBJECT_PS_FOG

	OBJECT_PS_OUT_FORWARD
}