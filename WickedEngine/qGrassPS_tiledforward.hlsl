#define DISABLE_DECALS
#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

[earlydepthstencil]
GBUFFEROutputType_Thin main(QGS_OUT PSIn)
{
	float4 baseColor = texture_0.Sample(sampler_linear_clamp,PSIn.tex);
	baseColor.a *= 1.0 - PSIn.fade;
	clip(baseColor.a - 1.0f / 256.0f); // cancel heaviest overdraw for the alpha composition effect
	float opacity = 1;
	baseColor = DEGAMMA(baseColor);
	float4 color = baseColor;
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
	float3 diffuse = 0;
	float3 specular = 0;
	float2 velocity = 0;

	OBJECT_PS_LIGHT_BEGIN

	OBJECT_PS_LIGHT_TILED

	OBJECT_PS_VOXELRADIANCE

	OBJECT_PS_LIGHT_END

	OBJECT_PS_FOG

	OBJECT_PS_OUT_FORWARD
}