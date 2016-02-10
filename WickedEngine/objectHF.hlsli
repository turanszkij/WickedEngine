#ifndef _OBJECTSHADER_HF_
#define _OBJECTSHADER_HF_
#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"
#include "ditherHF.hlsli"
#include "tangentComputeHF.hlsli"
#include "gammaHF.hlsli"
#include "toonHF.hlsli"
#include "specularHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "globalsHF.hlsli"

// DEFINITIONS
//////////////////

#define xTextureMap			texture_0
#define xReflectionMap		texture_1
#define xNormalMap			texture_2
#define xSpecularMap		texture_3
#define xDisplacementMap	texture_4
#define	xReflection			texture_5
#define xRefraction			texture_6
#define	xWaterRipples		texture_7

struct PixelInputType
{
	float4 pos								: SV_POSITION;
	float clip : SV_ClipDistance0;
	float2 tex								: TEXCOORD0;
	float3 nor								: NORMAL;
	float4 pos2D							: SCREENPOSITION;
	float3 pos3D							: WORLDPOSITION;
	float4 pos2DPrev						: SCREENPOSITIONPREV;
	float4 ReflectionMapSamplingPos			: TEXCOORD1;
	float  ao : AMBIENT_OCCLUSION;
	nointerpolation float  dither : DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	float3 nor2D							: NORMAL2D;
};

struct PixelOutputType
{
	float4 col	: SV_TARGET0;		// texture_gbuffer0
	float4 nor	: SV_TARGET1;		// texture_gbuffer1
	float4 vel	: SV_TARGET2;		// texture_gbuffer2
};


// METHODS
////////////

inline void BaseColorMapping(in float2 UV, inout float4 baseColor)
{
	[branch]if (g_xMat_hasTex) {
		baseColor *= xTextureMap.Sample(sampler_aniso_wrap, UV);
	}
}

inline void NormalMapping(in float2 UV, in float3 V, inout float3 N)
{
	if (g_xMat_hasNor) 
	{
		float3 bumpColor = 0;
		float4 nortex = xNormalMap.Sample(sampler_aniso_wrap, UV);
		float3x3 tangentFrame = compute_tangent_frame(N, V, -UV);
		bumpColor = 2.0f * nortex.rgb - 1.0f;
		bumpColor *= nortex.a;
		N = normalize(mul(bumpColor, tangentFrame));
	}
}

inline void SpecularMapping(in float2 UV, inout float4 specularColor)
{
	specularColor = lerp(specularColor, xSpecularMap.Sample(sampler_aniso_wrap, UV).r, g_xMat_hasSpe);
}

inline void PlanarReflection(in float2 UV, in float2 reflectionUV, in float3 N, inout float4 baseColor)
{
	[branch]if (g_xMat_hasRef) 
	{
		float colorMat = xReflectionMap.SampleLevel(sampler_aniso_wrap, UV, 0).r;
		//normal = normalize(lerp(normal, PSIn.nor, pow(abs(colorMat.x), 0.02f)));
		float4 colorReflection = xReflection.SampleLevel(sampler_linear_clamp, reflectionUV + N.xz, 0);
		baseColor.rgb = lerp(baseColor.rgb, colorReflection.rgb, colorMat);
	}
}

// #define ENVMAP_CAMERA_BLEND
inline void EnvironmentReflection(in float3 N, in float3 V, in float3 P, in float roughness, in float metalness, in float4 specularColor,
	inout float4 color)
{
	uint mip0 = 0;
	float2 size0;
	float mipLevels0;
	texture_env0.GetDimensions(mip0, size0.x, size0.y, mipLevels0);
	uint mip1 = 0;
	float2 size1;
	float mipLevels1;
	texture_env1.GetDimensions(mip1, size1.x, size1.y, mipLevels1);
	float3 ref = reflect(V, N);
	float4 envCol0 = texture_env0.SampleLevel(sampler_linear_clamp, ref, roughness*mipLevels0);
	float4 envCol1 = texture_env1.SampleLevel(sampler_linear_clamp, ref, roughness*mipLevels0);
#ifdef ENVMAP_CAMERA_BLEND
	float dist0 = distance(g_xCamera_CamPos, g_xTransform[0].xyz);
	float dist1 = distance(g_xCamera_CamPos, g_xTransform[1].xyz);
#else
	float dist0 = distance(P, g_xTransform[0].xyz);
	float dist1 = distance(P, g_xTransform[1].xyz);
#endif
	static const float blendStrength = 0.05f;
	float blend = clamp((dist0 - dist1)*blendStrength, -1, 1)*0.5f + 0.5f;
	float4 envCol = lerp(envCol0, envCol1, blend);

	color.rgb += envCol.rgb * specularColor.rgb * metalness;
}


// MACROS
////////////

#define OBJECT_PS_MAKE																								\
	float3 N = normalize(PSIn.nor);																					\
	float3 P = PSIn.pos3D;																							\
	float3 V = normalize(P - g_xCamera_CamPos);																		\
	float roughness = 1 - (g_xMat_specular_power / 128.0f);															\
	float metalness = g_xMat_metallic;																				\
	float2 UV = PSIn.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;												\
	float4 specularColor = g_xMat_specular;																			\
	float2 refUV = float2(1, -1)*PSIn.ReflectionMapSamplingPos.xy / PSIn.ReflectionMapSamplingPos.w / 2.0f + 0.5f;	\
	float2 ScreenCoord = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;								\
	float2 ScreenCoordPrev = float2(1, -1)*PSIn.pos2DPrev.xy / PSIn.pos2DPrev.w / 2.0f + 0.5f;						\
	float depth = PSIn.pos.z / PSIn.pos.w;																			\
	float4 baseColor = g_xMat_diffuseColor * float4(PSIn.instanceColor, 1);											\
	BaseColorMapping(UV, baseColor);

#define OBJECT_PS_MISC							\
	clip(dither(PSIn.pos.xy) - PSIn.dither);	\
	ALPHATEST(baseColor.a)

#define OBJECT_PS_NORMALMAPPING			\
	NormalMapping(UV, V, N);

#define OBJECT_PS_SPECULARMAPPING		\
	SpecularMapping(UV, specularColor);

#define OBJECT_PS_ENVIRONMENTMAPPING										\
	EnvironmentReflection(N, V, P, roughness, metalness, specularColor, baseColor);

#define OBJECT_PS_PLANARREFLECTIONS							\
	PlanarReflection(PSIn.tex, refUV, N, baseColor);

#define OBJECT_PS_OUT_GBUFFER																		\
	bool unshaded = g_xMat_shadeless;																\
	float properties = unshaded ? RT_UNSHADED : 0.0f;												\
	PixelOutputType Out = (PixelOutputType)0;														\
	Out.col = float4(baseColor.rgb*(1 + g_xMat_emissive)*PSIn.ao, 1);								\
	Out.nor = float4(N.xyz, properties);															\
	Out.vel = float4(ScreenCoord - ScreenCoordPrev, g_xMat_specular_power, specularColor.a);		\
	return Out;

#endif // _OBJECTSHADER_HF_