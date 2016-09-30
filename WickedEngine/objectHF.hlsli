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
#include "lightCullingCSInterop.h"
#include "tiledLightingHF.hlsli"

// DEFINITIONS
//////////////////

TEXTURE2D(LightGrid, uint2, TEXSLOT_LIGHTGRID);
STRUCTUREDBUFFER(LightIndexList, uint, SBSLOT_LIGHTINDEXLIST);
STRUCTUREDBUFFER(LightArray, LightArrayType, SBSLOT_LIGHTARRAY);

#define xBaseColorMap		texture_0
#define xNormalMap			texture_1
#define xRoughnessMap		texture_2
#define xReflectanceMap		texture_3
#define xMetalnessMap		texture_4
#define xDisplacementMap	texture_5

#define xReflection			texture_6
#define xRefraction			texture_7
#define	xWaterRipples		texture_8

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

struct LightingResult
{
	float3 diffuse;
	float3 specular;
};


// METHODS
////////////

inline void NormalMapping(in float2 UV, in float3 V, inout float3 N, in float3x3 TBN, inout float3 bumpColor)
{
	float4 nortex = xNormalMap.Sample(sampler_aniso_wrap, UV);
	bumpColor = 2.0f * nortex.rgb - 1.0f;
	bumpColor *= nortex.a;
	N = normalize(lerp(N, mul(bumpColor, TBN), g_xMat_normalMapStrength));
}

inline float3 PlanarReflection(in float2 UV, in float2 reflectionUV, in float3 N, in float3 V, in float roughness, in float3 f0)
{
	float4 colorReflection = xReflection.SampleLevel(sampler_linear_clamp, reflectionUV + N.xz*0.1f, 0);
	float f90 = saturate(50.0 * dot(f0, 0.33));
	float3 F = F_Schlick(f0, f90, abs(dot(N, V)) + 1e-5f);
	return colorReflection.rgb * F;
}

#define NUM_PARALLAX_OCCLUSION_STEPS 32
inline void ParallaxOcclusionMapping(inout float2 UV, in float3 V, in float3x3 TBN)
{
	V = mul(V, transpose(TBN));
	float layerHeight = 1.0 / NUM_PARALLAX_OCCLUSION_STEPS;
	float curLayerHeight = 0;
	float2 dtex = g_xMat_parallaxOcclusionMapping * V.xy / NUM_PARALLAX_OCCLUSION_STEPS;
	float2 currentTextureCoords = UV;
	float heightFromTexture = 1 - xDisplacementMap.Sample(sampler_aniso_wrap, currentTextureCoords).r;
	[unroll(NUM_PARALLAX_OCCLUSION_STEPS)]
	while (heightFromTexture > curLayerHeight)
	{
		curLayerHeight += layerHeight;
		currentTextureCoords -= dtex;
		heightFromTexture = 1 - xDisplacementMap.Sample(sampler_aniso_wrap, currentTextureCoords).r;
	}
	float2 prevTCoords = currentTextureCoords + dtex;
	float nextH = heightFromTexture - curLayerHeight;
	float prevH = xDisplacementMap.Sample(sampler_aniso_wrap, prevTCoords).r
		- curLayerHeight + layerHeight;
	float weight = nextH / (nextH - prevH);
	float2 finalTexCoords = prevTCoords * weight + currentTextureCoords * (1.0 - weight);
	UV = finalTexCoords;
}

inline void Refraction(in float2 ScreenCoord, in float2 normal2D, in float3 bumpColor, inout float alpha, inout float3 albedo)
{
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + (normal2D + bumpColor.rg) * g_xMat_refractionIndex;
	float4 refractiveColor = (xRefraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, 0));
	albedo.rgb = lerp(refractiveColor.rgb, albedo.rgb, alpha);
	alpha = 1;
}

inline void DirectionalLight(in float3 N, in float3 V, in float3 P, in float3 f0, in float3 albedo, in float roughness,
	inout float3 diffuse, out float3 specular)
{
	float3 L = GetSunDirection();
	float3 lightColor = GetSunColor();
	BRDF_MAKE(N, L, V);
	specular = lightColor * BRDF_SPECULAR(roughness, f0);
	diffuse = lightColor * BRDF_DIFFUSE(roughness);

	float sh = max(NdotL, 0);

	diffuse *= sh;
	specular *= sh;

	diffuse = max(diffuse, 0);
	specular = max(specular, 0);
}

inline void TiledLighting(in float2 pixel, in float3 N, in float3 V, in float3 P, in float3 f0, in float3 albedo, in float roughness,
	inout float3 diffuse, out float3 specular)
{
	uint2 tileIndex = uint2(floor(pixel / BLOCK_SIZE));
	uint startOffset = LightGrid[tileIndex].x;
	uint lightCount = LightGrid[tileIndex].y;

	specular = 0;
	diffuse = 0;
	for (uint i = 0; i < lightCount; i++)
	{
		uint lightIndex = LightIndexList[startOffset + i];
		LightArrayType light = LightArray[lightIndex];

		float3 L = light.PositionWS - P;
		float lightDistance = length(L);
		if (light.type > 0 && lightDistance > light.range)
			continue;
		L /= lightDistance;

		LightingResult result = (LightingResult)0;

		switch (light.type)
		{
		case 0/*DIRECTIONAL*/:
		{
			L = light.direction.xyz;
			BRDF_MAKE(N, L, V);
			result.specular = light.color.rgb * BRDF_SPECULAR(roughness, f0);
			result.diffuse = light.color.rgb * BRDF_DIFFUSE(roughness);

			float sh = max(NdotL, 0);
			float4 ShPos[3];
			ShPos[0] = mul(float4(P, 1), g_xDirLight_ShM[0]);
			ShPos[1] = mul(float4(P, 1), g_xDirLight_ShM[1]);
			ShPos[2] = mul(float4(P, 1), g_xDirLight_ShM[2]);
			float3 ShTex[3];
			ShTex[0] = ShPos[0].xyz*float3(1, -1, 1) / ShPos[0].w / 2.0f + 0.5f;
			ShTex[1] = ShPos[1].xyz*float3(1, -1, 1) / ShPos[1].w / 2.0f + 0.5f;
			ShTex[2] = ShPos[2].xyz*float3(1, -1, 1) / ShPos[2].w / 2.0f + 0.5f;
			const float shadows[3] = {
				shadowCascade(ShPos[0],ShTex[0].xy,texture_shadow0),
				shadowCascade(ShPos[1],ShTex[1].xy,texture_shadow1),
				shadowCascade(ShPos[2],ShTex[2].xy,texture_shadow2)
			};
			[branch]if ((saturate(ShTex[2].x) == ShTex[2].x) && (saturate(ShTex[2].y) == ShTex[2].y) && (saturate(ShTex[2].z) == ShTex[2].z))
			{
				//color.r+=0.5f;
				const float2 lerpVal = abs(ShTex[2].xy * 2 - 1);
				sh *= lerp(shadows[2], shadows[1], pow(max(lerpVal.x, lerpVal.y), 4));
			}
			else[branch]if ((saturate(ShTex[1].x) == ShTex[1].x) && (saturate(ShTex[1].y) == ShTex[1].y) && (saturate(ShTex[1].z) == ShTex[1].z))
			{
				//color.g+=0.5f;
				const float2 lerpVal = abs(ShTex[1].xy * 2 - 1);
				sh *= lerp(shadows[1], shadows[0], pow(max(lerpVal.x, lerpVal.y), 4));
			}
			else[branch]if ((saturate(ShTex[0].x) == ShTex[0].x) && (saturate(ShTex[0].y) == ShTex[0].y) && (saturate(ShTex[0].z) == ShTex[0].z))
			{
				//color.b+=0.5f;
				sh *= shadows[0];
			}

			result.diffuse *= sh;
			result.specular *= sh;

			result.diffuse = max(result.diffuse, 0);
			result.specular = max(result.specular, 0);
		}
		break;
		case 1/*POINT*/:
		{
			BRDF_MAKE(N, L, V);
			result.specular = light.color.rgb * BRDF_SPECULAR(roughness, f0);
			result.diffuse = light.color.rgb * BRDF_DIFFUSE(roughness);
			result.diffuse *= light.energy;
			result.specular *= light.energy;

			float att = (light.energy * (light.range / (light.range + 1 + lightDistance)));
			float attenuation = /*saturate*/(att * (light.range - lightDistance) / light.range);
			result.diffuse *= attenuation;
			result.specular *= attenuation;

			float sh = max(NdotL, 0);
			//[branch]if (xLightEnerDis.w) {
			//	const float3 lv = P - xLightPos.xyz;
			//	static const float bias = 0.025;
			//	sh *= texture_shadow_cube.SampleCmpLevelZero(sampler_cmp_depth, lv, length(lv) / xLightEnerDis.y - bias).r;
			//}
			result.diffuse *= sh;
			result.specular *= sh;

			result.diffuse = max(result.diffuse, 0);
			result.specular = max(result.specular, 0);
		}
		break;
		case 2/*SPOT*/:
		{
		}
		break;
		}

		diffuse += result.diffuse;
		specular += result.specular;
	}
}


// MACROS
////////////

#define OBJECT_PS_MAKE_COMMON												\
	float3 N = input.nor;													\
	float3 P = input.pos3D;													\
	float3 V = normalize(g_xCamera_CamPos - P);								\
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;		\
	float4 baseColor = g_xMat_baseColor * float4(input.instanceColor, 1);	\
	float4 color = baseColor;												\
	float roughness = g_xMat_roughness;										\
	roughness = saturate(roughness);										\
	float metalness = g_xMat_metalness;										\
	metalness = saturate(metalness);										\
	float reflectance = g_xMat_reflectance;									\
	reflectance = saturate(reflectance);									\
	float emissive = g_xMat_emissive;										\
	float sss = g_xMat_subsurfaceScattering;								\
	float3 bumpColor = 0;													\
	float depth = input.pos.z;												\
	float ao = input.ao;													\
	float2 pixel = input.pos.xy;

#define OBJECT_PS_MAKE																								\
	OBJECT_PS_MAKE_COMMON																							\
	float lineardepth = input.pos2D.z;																				\
	float2 refUV = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w / 2.0f + 0.5f;\
	float2 ScreenCoord = float2(1, -1) * input.pos2D.xy / input.pos2D.w / 2.0f + 0.5f;								\

#define OBJECT_PS_COMPUTETANGENTSPACE										\
	float3 T, B;															\
	float3x3 TBN = compute_tangent_frame(N, P, UV, T, B);

#define OBJECT_PS_SAMPLETEXTURES											\
	baseColor *= xBaseColorMap.Sample(sampler_aniso_wrap, UV);				\
	ALPHATEST(baseColor.a);													\
	color = baseColor;														\
	roughness *= xRoughnessMap.Sample(sampler_aniso_wrap, UV).r;			\
	metalness *= xMetalnessMap.Sample(sampler_aniso_wrap, UV).r;			\
	reflectance *= xReflectanceMap.Sample(sampler_aniso_wrap, UV).r;

#define OBJECT_PS_NORMALMAPPING												\
	NormalMapping(UV, P, N, TBN, bumpColor);

#define OBJECT_PS_PARALLAXOCCLUSIONMAPPING									\
	ParallaxOcclusionMapping(UV, V, TBN);

#define OBJECT_PS_LIGHT_BEGIN																						\
	float3 diffuse, specular;																						\
	BRDF_HELPER_MAKEINPUTS( color, reflectance, metalness )

#define OBJECT_PS_LIGHT_DIRECTIONAL																					\
	DirectionalLight(N, V, P, f0, albedo, roughness, diffuse, specular);

#define OBJECT_PS_LIGHT_TILED																						\
	TiledLighting(pixel, N, V, P, f0, albedo, roughness, diffuse, specular);

#define OBJECT_PS_LIGHT_END																							\
	color.rgb = (GetAmbientColor() * ao + diffuse) * albedo + specular;

#define OBJECT_PS_DITHER																							\
	clip(dither(input.pos.xy) - input.dither);

#define OBJECT_PS_PLANARREFLECTIONS																					\
	specular += PlanarReflection(UV, refUV, N, V, roughness, f0);

#define OBJECT_PS_ENVIRONMENTREFLECTIONS																			\
	specular += EnvironmentReflection(N, V, P, roughness, f0);

#define OBJECT_PS_REFRACTION																						\
	Refraction(ScreenCoord, input.nor2D, bumpColor, color.a, albedo);

#define OBJECT_PS_DEGAMMA																							\
	color = DEGAMMA(color);

#define OBJECT_PS_GAMMA																								\
	color = GAMMA(color);

#define OBJECT_PS_EMISSIVE																							\
	color.rgb += baseColor.rgb * GetEmissive(emissive);

#define OBJECT_PS_FOG																								\
	color.rgb = applyFog(color.rgb, getFog(getLinearDepth(depth)));

#define OBJECT_PS_OUT_GBUFFER																						\
	GBUFFEROutputType Out = (GBUFFEROutputType)0;																	\
	Out.g0 = float4(color.rgb, 1);									/*FORMAT_R8G8B8A8_UNORM*/						\
	Out.g1 = float4(encode(N), 0, 0);								/*FORMAT_R16G16_FLOAT*/							\
	Out.g2 = float4(0, 0, sss, emissive);							/*FORMAT_R8G8B8A8_UNORM*/						\
	Out.g3 = float4(roughness, reflectance, metalness, ao);			/*FORMAT_R8G8B8A8_UNORM*/						\
	return Out;

#define OBJECT_PS_OUT_FORWARD																						\
	return color;

#endif // _OBJECTSHADER_HF_