#define DISABLE_DECALS
#define DISABLE_LOCALENVPMAPS
#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ditherHF.hlsli"

[earlydepthstencil]
GBUFFEROutputType_Thin main(VertexToPixel input)
{
	float4 baseColor = texture_0.Sample(sampler_linear_clamp, input.tex);
	baseColor.a *= 1.0 - input.fade;
	clip(baseColor.a - 1.0f / 256.0f); // cancel heaviest overdraw for the alpha composition effect
	float opacity = 1;
	baseColor = DEGAMMA(baseColor);
	float4 color = baseColor;
	float3 P = input.pos3D;
	float3 V = g_xCamera_CamPos - P;
	float dist = length(V);
	V /= dist;
	float emissive = 0;
	float3 N = input.nor;
	float roughness = 1;
	float reflectance = 0;
	float metalness = 0;
	float ao = 1;
	float sss = 0;
	float2 pixel = input.pos.xy;
	float depth = input.pos.z;
	float3 diffuse = 0;
	float3 specular = 0;
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (input.pos2D.xy / input.pos2D.w - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	OBJECT_PS_LIGHT_BEGIN

	OBJECT_PS_LIGHT_TILED

	OBJECT_PS_VOXELRADIANCE

	OBJECT_PS_LIGHT_END

	OBJECT_PS_FOG

	OBJECT_PS_OUT_FORWARD
}