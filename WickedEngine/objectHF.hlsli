#ifndef _OBJECTSHADER_HF_
#define _OBJECTSHADER_HF_
#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"
#include "ditherHF.hlsli"
#include "tangentComputeHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "dirLightHF.hlsli"
#include "brdf.hlsli"
#include "envReflectionHF.hlsli"
#include "packHF.hlsli"

// DEFINITIONS
//////////////////

//#define xTextureMap			texture_0
//#define xReflectionMap		texture_1
//#define xNormalMap			texture_2
//#define xSpecularMap		texture_3
//#define xDisplacementMap	texture_4
//#define	xReflection			texture_5

#define xBaseColorMap		texture_0
#define xNormalMap			texture_1
#define xRoughnessMap		texture_2
#define xReflectanceMap		texture_3
#define xMetalnessMap		texture_4
#define xDisplacementMap	texture_0

#define xReflection			texture_5
#define xRefraction			texture_6
#define	xWaterRipples		texture_7

struct PixelInputType
{
	float4 pos								: SV_POSITION;
	float  clip								: SV_ClipDistance0;
	float2 tex								: TEXCOORD0;
	float3 nor								: NORMAL;
	float4 pos2D							: SCREENPOSITION;
	float3 pos3D							: WORLDPOSITION;
	float4 pos2DPrev						: SCREENPOSITIONPREV;
	float4 ReflectionMapSamplingPos			: TEXCOORD1;
	float  ao								: AMBIENT_OCCLUSION;
	nointerpolation float  dither			: DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	float2 nor2D							: NORMAL2D;
};


struct GBUFFEROutputType
{
	float4 g0	: SV_TARGET0;		// texture_gbuffer0
	float4 g1	: SV_TARGET1;		// texture_gbuffer1
	float4 g2	: SV_TARGET2;		// texture_gbuffer2
	float4 g3	: SV_TARGET3;		// texture_gbuffer3
};


// METHODS
////////////

//inline void BaseColorMapping(in float2 UV, inout float4 baseColor)
//{
//	baseColor *= xBaseColorMap.Sample(sampler_aniso_wrap, UV);
//}

inline void NormalMapping(in float2 UV, in float3 V, inout float3 N, inout float3 bumpColor)
{
	float3 nortex = xNormalMap.Sample(sampler_aniso_wrap, UV).rgb;
	float3x3 tangentFrame = compute_tangent_frame(N, V, -UV);
	bumpColor = 2.0f * nortex.rgb - 1.0f;
	N = normalize(lerp(N, mul(bumpColor, tangentFrame), g_xMat_normalMapStrength));
}

//inline void PlanarReflection(in float2 UV, in float2 reflectionUV, in float3 N, inout float4 baseColor)
//{
//	float colorMat = xReflectionMap.SampleLevel(sampler_aniso_wrap, UV, 0).r;
//	float4 colorReflection = xReflection.SampleLevel(sampler_linear_clamp, reflectionUV + N.xz, 0);
//	baseColor.rgb = lerp(baseColor.rgb, colorReflection.rgb, colorMat);
//}

inline void Refraction(in float2 ScreenCoord, in float2 normal2D, in float3 bumpColor, inout float alpha, inout float3 albedo)
{
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + (normal2D + bumpColor.rg) * g_xMat_refractionIndex;
	float4 refractiveColor = (xRefraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, 0));
	albedo.rgb = lerp(refractiveColor.rgb, albedo.rgb, alpha);
	alpha = 1;
}

inline void DirectionalLight(in float3 N, in float3 V, in float3 f0, in float3 albedo, in float roughness,
	inout float3 diffuse, out float3 specular)
{
	float3 L = GetSunDirection();
	float3 lightColor = GetSunColor();
	BRDF_MAKE(N, L, V);
	specular = lightColor * BRDF_SPECULAR(roughness, f0);
	diffuse = lightColor * BRDF_DIFFUSE(roughness);
}


// MACROS
////////////

#define OBJECT_PS_MAKE_COMMON												\
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;		\
	float4 baseColor = g_xMat_baseColor * float4(input.instanceColor, 1);	\
	baseColor *= xBaseColorMap.Sample(sampler_aniso_wrap, UV);				\
	ALPHATEST(baseColor.a);													\
	float4 color = baseColor;												\
	float roughness = g_xMat_roughness;										\
	roughness *= xRoughnessMap.Sample(sampler_aniso_wrap, UV).r;			\
	roughness = saturate(roughness);										\
	float metalness = g_xMat_metalness;										\
	metalness *= xMetalnessMap.Sample(sampler_aniso_wrap, UV).r;			\
	metalness = saturate(metalness);										\
	float reflectance = g_xMat_reflectance;									\
	reflectance *= xReflectanceMap.Sample(sampler_aniso_wrap, UV).r;		\
	reflectance = saturate(reflectance);									\
	float emissive = g_xMat_emissive;										\
	float sss = g_xMat_subsurfaceScattering;								\
	float3 N = input.nor;													\
	float3 P = input.pos3D;													\
	float3 V = normalize(g_xCamera_CamPos - P);								\
	float3 bumpColor = 0;													\
	float depth = input.pos.z;												\
	float ao = input.ao;

#define OBJECT_PS_MAKE																								\
	OBJECT_PS_MAKE_COMMON																							\
	float lineardepth = input.pos2D.z;																				\
	float2 refUV = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w / 2.0f + 0.5f;\
	float2 ScreenCoord = float2(1, -1) * input.pos2D.xy / input.pos2D.w / 2.0f + 0.5f;								\
	float2 ScreenCoordPrev = float2(1, -1)*input.pos2DPrev.xy / input.pos2DPrev.w / 2.0f + 0.5f;					\
	float2 velocity = ScreenCoord - ScreenCoordPrev;

#define OBJECT_PS_LIGHT_BEGIN																						\
	float3 diffuse, specular;																						\
	BRDF_HELPER_MAKEINPUTS( color, reflectance, metalness )

#define OBJECT_PS_LIGHT_DIRECTIONAL																					\
	DirectionalLight(N, V, f0, albedo, roughness, diffuse, specular);

#define OBJECT_PS_LIGHT_END																							\
	color.rgb = (GetAmbientColor() * ao + diffuse) * albedo + specular;

#define OBJECT_PS_DITHER																							\
	clip(dither(input.pos.xy) - input.dither);

#define OBJECT_PS_NORMALMAPPING																						\
	NormalMapping(UV, P, N, bumpColor);

#define OBJECT_PS_ENVIRONMENTMAPPING																				\
	specular += EnvironmentReflection(N, V, P, roughness, f0);

//#define OBJECT_PS_PLANARREFLECTIONS																					\
//	PlanarReflection(input.tex, refUV, N, color);

#define OBJECT_PS_REFRACTION																						\
	Refraction(ScreenCoord, input.nor2D, bumpColor, color.a, albedo);

#define OBJECT_PS_DEGAMMA																							\
	color = DEGAMMA(color);

#define OBJECT_PS_EMISSIVE																							\
	color.rgb += baseColor.rgb * GetEmissive(emissive);

#define OBJECT_PS_FOG																								\
	color.rgb = applyFog(color.rgb, getFog(getLinearDepth(depth)));

#define OBJECT_PS_OUT_GBUFFER																						\
	GBUFFEROutputType Out = (GBUFFEROutputType)0;																	\
	Out.g0 = float4(color.rgb, 1);									/*FORMAT_R8G8B8A8_UNORM*/						\
	Out.g1 = float4(N.xyz * 0.5f + 0.5f, 0);						/*FORMAT_R11G11B10_FLOAT*/						\
	Out.g2 = float4(velocity * 0.5f + 0.5f, sss, emissive);			/*FORMAT_R8G8B8A8_UNORM*/						\
	Out.g3 = float4(roughness, reflectance, metalness, ao);			/*FORMAT_R8G8B8A8_UNORM*/						\
	return Out;

#define OBJECT_PS_OUT_FORWARD																						\
	return color;

#endif // _OBJECTSHADER_HF_