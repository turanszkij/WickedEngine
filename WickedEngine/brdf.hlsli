#ifndef _BRDF_HF_
#define _BRDF_HF_
#include "globals.hlsli"

// BRDF from Frostbite presentation:
// Moving Frostbite to Physically Based Rendering// S´ebastien Lagarde - Electronic Arts Frostbite
// Charles de Rousiers - Electronic Arts Frostbite
// SIGGRAPH 2014

float3 F_Schlick(in float3 f0, in float f90, in float u)
{
	return f0 + (f90 - f0) * pow(1.f - u, 5.f);
}

float3 F_Fresnel(float3 SpecularColor, float VoH)
{
	float3 SpecularColorSqrt = sqrt(min(SpecularColor, float3(0.99, 0.99, 0.99)));
	float3 n = (1 + SpecularColorSqrt) / (1 - SpecularColorSqrt);
	float3 g = sqrt(n*n + VoH*VoH - 1);
	return 0.5 * sqr((g - VoH) / (g + VoH)) * (1 + sqr(((g + VoH)*VoH - 1) / ((g - VoH)*VoH + 1)));
}

float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float energyBias = lerp(0, 0.5, linearRoughness);
	float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * LdotH*LdotH * linearRoughness;
	float3 f0 = float3(1.0f, 1.0f, 1.0f);
	float lightScatter = F_Schlick(f0, fd90, NdotL).r;
	float viewScatter = F_Schlick(f0, fd90, NdotV).r;

	return lightScatter * viewScatter * energyFactor;
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
	// Original formulation of G_SmithGGX Correlated 
	// lambda_v = (-1 + sqrt(alphaG2 * (1 - NdotL2) / NdotL2 + 1)) * 0.5f; 
	// lambda_l = (-1 + sqrt(alphaG2 * (1 - NdotV2) / NdotV2 + 1)) * 0.5f; 
	// G_SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l); 
	// V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0f * NdotL * NdotV); 


	// This is the optimize version 
	float alphaG2 = alphaG * alphaG;
	alphaG2 = alphaG2 + 0.0000001; // cg miatt

								   // Caution: the "NdotL *" and "NdotV *" are explicitely inversed , this is not a mistake. 
	float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
	float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

	return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float D_GGX(float NdotH, float m)
{
	// Divide by PI is apply later 
	float m2 = m * m;
	float f = (NdotH * m2 - NdotH) * NdotH + 1;
	return m2 / (f * f);
}

float3 GetSpecular(float NdotV, float NdotL, float LdotH, float NdotH, float roughness, float3 f0)
{
	float f90 = saturate(50.0 * dot(f0, 0.33));
	float3 F = F_Schlick(f0, f90, LdotH);
	float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 Fr = D * F * Vis / PI;
	return Fr;
}
float GetDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, linearRoughness) / PI;	
	return Fd;
}

// N:	float3 normal
// L:	float3 light vector
// V:	float3 view vector
#define BRDF_MAKE( N, L, V )								\
	const float3	H = normalize(L + V);			  		\
	const float		VdotN = abs(dot(N, V)) + 1e-5f;			\
	const float		LdotN = saturate(dot(L, N));  			\
	const float		HdotV = saturate(dot(H, V));			\
	const float		HdotN = saturate(dot(H, N)); 			\
	const float		NdotV = VdotN;					  		\
	const float		NdotL = LdotN;					  		\
	const float		VdotH = HdotV;					  		\
	const float		NdotH = HdotN;					  		\
	const float		LdotH = HdotV;					  		\
	const float		HdotL = LdotH;

// ROUGHNESS:	float surface roughness
// F0:			float3 surface specular color (fresnel f0)
#define BRDF_SPECULAR( ROUGHNESS, F0 )					\
	GetSpecular(NdotV, NdotL, LdotH, NdotH, ROUGHNESS, F0)

// ROUGHNESS:		float surface roughness
#define BRDF_DIFFUSE( ROUGHNESS )							\
	GetDiffuse(NdotV, NdotL, LdotH, ROUGHNESS)

// BASECOLOR:	float4 surface color
// REFLECTANCE:	float surface reflectance value
// METALNESS:	float surface metalness value
#define BRDF_HELPER_MAKEINPUTS( BASECOLOR, REFLECTANCE, METALNESS )									\
	float3 albedo = lerp(lerp(BASECOLOR.rgb, float3(0,0,0), REFLECTANCE), float3(0,0,0), METALNESS);\
	float3 f0 = lerp(lerp(float3(0,0,0), float3(1,1,1), REFLECTANCE), BASECOLOR.rgb, METALNESS);

#endif // _BRDF_HF_