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

inline void pointLight(in float3 P, in float3 N, in float3 V, in float roughness, in float3 f0,
	out float3 diffuse, out float3 specular)
{
	float3 L = normalize(xLightPos - P);
	BRDF_MAKE(N, L, V);
	specular = xLightColor.rgb * BRDF_SPECULAR(roughness, f0);
	diffuse = xLightColor.rgb * BRDF_DIFFUSE(roughness);
	//color.rgb *= xLightEnerdis.x;

	float  lightdis = distance(P, xLightPos);
	float att = (xLightEnerdis.x * (xLightEnerdis.y / (xLightEnerdis.y + 1 + lightdis)));
	float attenuation = /*saturate*/(att * (xLightEnerdis.y - lightdis) / xLightEnerdis.y);
	diffuse *= attenuation;
	specular *= attenuation;

	float sh = 1;
	[branch]if(xLightEnerdis.w){
		const float3 lv = P - xLightPos.xyz;
		static const float bias = 0.025;
		sh *= texture_shadow_cube.SampleCmpLevelZero(sampler_cmp_depth,lv,length(lv)/ xLightEnerdis.y-bias ).r;
	}
	diffuse *= sh;
	specular *= sh;

	diffuse = max(diffuse, 0);
	specular = max(specular, 0);
}


// MACROS

#define DEFERRED_POINTLIGHT_MAIN	\
	pointLight(P, N, V, roughness, f0, diffuse, specular);

#endif // _POINTLIGHT_HF_
