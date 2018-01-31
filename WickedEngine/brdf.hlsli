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


	// This is the optimized version 
	float alphaG2 = alphaG * alphaG;

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






struct Surface
{
	float3 P;				// world space position
	float3 N;				// world space normal
	float3 V;				// world space view vector
	float4 baseColor;		// base color [0,1] (rgba)
	float reflectance;		// reflectivity [0:diffuse -> 1:specular]
	float metalness;		// metalness [0:dielectric -> 1:metal]
	float roughness;		// roughness: [0:smooth -> 1:rough]

	float NdotV;			// cos(angle between normal and view vector)
	float3 f0;				// fresnel value (rgb)
	float3 albedo;			// diffuse light absorbtion value (rgb)
	float3 R;				// reflection vector
	float3 F;				// fresnel term computed from f0, N and V
};
float3 ComputeAlbedo(in float4 baseColor, in float reflectance, in float metalness)
{
	return lerp(lerp(baseColor.rgb, float3(0, 0, 0), reflectance), float3(0, 0, 0), metalness);
}
float3 ComputeF0(in float4 baseColor, in float reflectance, in float metalness)
{
	return lerp(lerp(float3(0, 0, 0), float3(1, 1, 1), reflectance), baseColor.rgb, metalness);
}
Surface CreateSurface(in float3 P, in float3 N, in float3 V, in float4 baseColor, in float reflectance, in float metalness, in float roughness)
{
	Surface surface;

	surface.P = P;
	surface.N = N;
	surface.V = V;
	surface.baseColor = baseColor;
	surface.reflectance = reflectance;
	surface.metalness = metalness;
	surface.roughness = roughness;

	surface.NdotV = abs(dot(N, V)) + 1e-5f;

	surface.albedo = ComputeAlbedo(baseColor, reflectance, metalness);
	surface.f0 = ComputeF0(baseColor, reflectance, metalness);

	surface.R = -reflect(V, N);
	float f90 = saturate(50.0 * dot(surface.f0, 0.33));
	surface.F = F_Schlick(surface.f0, f90, surface.NdotV);

	return surface;
}

struct SurfaceToLight
{
	float3 L;				// surface to light vector
	float3 H;				// half-vector between view vector and light vector
	float NdotL;			// cos(angle between N and L)
	float HdotV;			// cos(angle between H and V) = HdotL = cos(angle between H and L)
	float NdotH;			// cos(angle between N and H)
};
SurfaceToLight CreateSurfaceToLight(in Surface surface, in float3 L)
{
	SurfaceToLight surfaceToLight;

	surfaceToLight.L = L;
	surfaceToLight.H = normalize(L + surface.V);

	surfaceToLight.NdotL = saturate(dot(surfaceToLight.L, surface.N));
	surfaceToLight.HdotV = saturate(dot(surfaceToLight.H, surface.V));
	surfaceToLight.NdotH = saturate(dot(surfaceToLight.H, surface.N));

	return surfaceToLight;
}


float3 BRDF_GetSpecular(in Surface surface, in SurfaceToLight surfaceToLight)
{
	float f90 = saturate(50.0 * dot(surface.f0, 0.33));
	float3 F = F_Schlick(surface.f0, f90, surfaceToLight.HdotV);
	float Vis = V_SmithGGXCorrelated(surface.NdotV, surfaceToLight.NdotL, surface.roughness);
	float D = D_GGX(surfaceToLight.NdotH, surface.roughness);
	float3 Fr = D * F * Vis / PI;
	return Fr;
}
float BRDF_GetDiffuse(in Surface surface, in SurfaceToLight surfaceToLight)
{
	float Fd = Fr_DisneyDiffuse(surface.NdotV, surfaceToLight.NdotL, surfaceToLight.HdotV, surface.roughness) / PI;
	return Fd;
}

//float3 GetSpecular(float NdotV, float NdotL, float LdotH, float NdotH, float roughness, float3 f0)
//{
//	float f90 = saturate(50.0 * dot(f0, 0.33));
//	float3 F = F_Schlick(f0, f90, LdotH);
//	float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
//	float D = D_GGX(NdotH, roughness);
//	float3 Fr = D * F * Vis / PI;//
//	return Fr;
//}
//float GetDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
//{
//	float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, linearRoughness) / PI;//	
//	return Fd;
//}

//// N:	float3 normal
//// L:	float3 light vector
//// V:	float3 view vector
//#define BRDF_MAKE( N, L, V )								\
//	const float3	H = normalize(L + V);			  		\
//	const float		VdotN = abs(dot(N, V)) + 1e-5f;			\
//	const float		LdotN = saturate(dot(L, N));  			\
//	const float		HdotV = saturate(dot(H, V));			\
//	const float		HdotN = saturate(dot(H, N)); 			\
//	const float		NdotV = VdotN;					  		\
//	const float		NdotL = LdotN;					  		\
//	const float		VdotH = HdotV;					  		\
//	const float		NdotH = HdotN;					  		\
//	const float		LdotH = HdotV;					  		\
//	const float		HdotL = LdotH;
//
//// ROUGHNESS:	float surface roughness
//// F0:			float3 surface specular color (fresnel f0)
//#define BRDF_SPECULAR( ROUGHNESS, F0 )					\
//	GetSpecular(NdotV, NdotL, LdotH, NdotH, ROUGHNESS, F0)
//
//// ROUGHNESS:		float surface roughness
//#define BRDF_DIFFUSE( ROUGHNESS )							\
//	GetDiffuse(NdotV, NdotL, LdotH, ROUGHNESS)
//
//// BASECOLOR:	float4 surface color
//// REFLECTANCE:	float surface reflectance value
//// METALNESS:	float surface metalness value
//#define BRDF_HELPER_MAKEINPUTS( BASECOLOR, REFLECTANCE, METALNESS )									\
//	float3 albedo = lerp(lerp(BASECOLOR.rgb, float3(0,0,0), REFLECTANCE), float3(0,0,0), METALNESS);\
//	float3 f0 = lerp(lerp(float3(0,0,0), float3(1,1,1), REFLECTANCE), BASECOLOR.rgb, METALNESS);

#endif // _BRDF_HF_