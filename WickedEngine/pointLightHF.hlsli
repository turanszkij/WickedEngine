#ifndef _POINTLIGHT_HF_
#define _POINTLIGHT_HF_
#include "globals.hlsli"
#include "brdf.hlsli"

CBUFFER(PointLightCB, CBSLOT_RENDERER_POINTLIGHT)
{
	float3 xLightPos; float pad;
	float4 xLightColor;
	float4 xLightEnerdis;
};

inline float4 pointLight(in float3 P, in float3 N, in float3 V, in float roughness, in float3 f0, in float3 albedo)
{
	float4 color = float4(0, 0, 0, 1);

	float3 L = normalize(xLightPos - P);
	BRDF_MAKE(N, L, V);
	float3 lightSpecular = xLightColor.rgb * BRDF_SPECULAR(roughness, f0);
	float3 lightDiffuse = xLightColor.rgb * BRDF_DIFFUSE(roughness, albedo);
	color.rgb = lightDiffuse + lightSpecular;
	//color.rgb *= xLightEnerdis.x;

	float  lightdis = distance(P, xLightPos);
	float att = (xLightEnerdis.x * (xLightEnerdis.y / (xLightEnerdis.y + 1 + lightdis)));
	float attenuation = /*saturate*/(att * (xLightEnerdis.y - lightdis) / xLightEnerdis.y);
	color *= attenuation;

	[branch]if(xLightEnerdis.w){
		const float3 lv = P - xLightPos.xyz;
		static const float bias = 0.025;
		color *= texture_shadow_cube.SampleCmpLevelZero(sampler_cmp_depth,lv,length(lv)/ xLightEnerdis.y-bias ).r;
	}

	return color;
}


// MACROS

#define DEFERRED_POINTLIGHT_MAIN	\
	lightColor = pointLight(P, N, V, roughness, f0, albedo);

#endif // _POINTLIGHT_HF_
