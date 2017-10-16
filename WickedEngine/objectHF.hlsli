#ifndef _OBJECTSHADER_HF_
#define _OBJECTSHADER_HF_
#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"
#include "ditherHF.hlsli"
#include "tangentComputeHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "brdf.hlsli"
#include "envReflectionHF.hlsli"
#include "packHF.hlsli"
#include "lightingHF.hlsli"

// UNIFORMS
//////////////////

CBUFFER(MaterialCB, CBSLOT_RENDERER_MATERIAL)
{
	float4		g_xMat_baseColor;
	float4		g_xMat_texMulAdd;
	float		g_xMat_roughness;
	float		g_xMat_reflectance;
	float		g_xMat_metalness;
	float		g_xMat_emissive;
	float		g_xMat_refractionIndex;
	float		g_xMat_subsurfaceScattering;
	float		g_xMat_normalMapStrength;
	float		g_xMat_parallaxOcclusionMapping;
};

// DEFINITIONS
//////////////////

#define xBaseColorMap			texture_0
#define xNormalMap				texture_1
#define xRoughnessMap			texture_2
#define xReflectanceMap			texture_3
#define xMetalnessMap			texture_4
#define xDisplacementMap		texture_5

#define xReflection				texture_6
#define xRefraction				texture_7
#define	xWaterRipples			texture_8


struct PixelInputType_Simple
{
	float4 pos								: SV_POSITION;
	float  clip								: SV_ClipDistance0;
	float2 tex								: TEXCOORD0;
	nointerpolation float  dither			: DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
};
struct PixelInputType
{
	float4 pos								: SV_POSITION;
	float  clip								: SV_ClipDistance0;
	float2 tex								: TEXCOORD0;
	nointerpolation float  dither			: DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	float  ao								: AMBIENT_OCCLUSION;
	float3 nor								: NORMAL;
	float4 pos2D							: SCREENPOSITION;
	float3 pos3D							: WORLDPOSITION;
	float4 pos2DPrev						: SCREENPOSITIONPREV;
	float4 ReflectionMapSamplingPos			: TEXCOORD1;
	float2 nor2D							: NORMAL2D;
};

struct GBUFFEROutputType
{
	float4 g0	: SV_TARGET0;		// texture_gbuffer0
	float4 g1	: SV_TARGET1;		// texture_gbuffer1
	float4 g2	: SV_TARGET2;		// texture_gbuffer2
	float4 g3	: SV_TARGET3;		// texture_gbuffer3
};
struct GBUFFEROutputType_Thin
{
	float4 g0	: SV_TARGET0;		// texture_gbuffer0
	float4 g1	: SV_TARGET1;		// texture_gbuffer1
};


// METHODS
////////////

inline void NormalMapping(in float2 UV, in float3 V, inout float3 N, in float3x3 TBN, inout float3 bumpColor)
{
	float4 nortex = xNormalMap.Sample(sampler_objectshader, UV);
	bumpColor = 2.0f * nortex.rgb - 1.0f;
	bumpColor *= nortex.a;
	N = normalize(lerp(N, mul(bumpColor, TBN), g_xMat_normalMapStrength));
	bumpColor *= g_xMat_normalMapStrength;
}

inline void SpecularAA(in float3 N, inout float roughness)
{
	[branch]
	if (g_xWorld_SpecularAA > 0)
	{
		float3 ddxN = ddx_coarse(N);
		float3 ddyN = ddy_coarse(N);
		float curve = pow(max(dot(ddxN, ddxN), dot(ddyN, ddyN)), 1 - g_xWorld_SpecularAA);
		roughness = max(roughness, curve);
	}
}

inline float3 PlanarReflection(in float2 UV, in float2 reflectionUV, in float3 N, in float3 V, in float roughness, in float3 f0)
{
	float4 colorReflection = xReflection.SampleLevel(sampler_linear_clamp, reflectionUV + N.xz*g_xMat_normalMapStrength, 0);
	float f90 = saturate(50.0 * dot(f0, 0.33));
	float3 F = F_Schlick(f0, f90, abs(dot(N, V)) + 1e-5f);
	return colorReflection.rgb * F;
}

#define NUM_PARALLAX_OCCLUSION_STEPS 32
inline void ParallaxOcclusionMapping(inout float2 UV, in float3 V, in float3x3 TBN)
{
	V = mul(TBN, V);
	float layerHeight = 1.0 / NUM_PARALLAX_OCCLUSION_STEPS;
	float curLayerHeight = 0;
	float2 dtex = g_xMat_parallaxOcclusionMapping * V.xy / NUM_PARALLAX_OCCLUSION_STEPS;
	float2 currentTextureCoords = UV;
	float2 derivX = ddx_coarse(UV);
	float2 derivY = ddy_coarse(UV);
	float heightFromTexture = 1 - xDisplacementMap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
	uint iter = 0;
	[loop]
	while (heightFromTexture > curLayerHeight && iter < NUM_PARALLAX_OCCLUSION_STEPS)
	{
		curLayerHeight += layerHeight;
		currentTextureCoords -= dtex;
		heightFromTexture = 1 - xDisplacementMap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
		iter++;
	}
	float2 prevTCoords = currentTextureCoords + dtex;
	float nextH = heightFromTexture - curLayerHeight;
	float prevH = xDisplacementMap.SampleGrad(sampler_linear_wrap, prevTCoords, derivX, derivY).r - curLayerHeight + layerHeight;
	float weight = nextH / (nextH - prevH);
	float2 finalTexCoords = prevTCoords * weight + currentTextureCoords * (1.0 - weight);
	UV = finalTexCoords;
}

inline void Refraction(in float2 ScreenCoord, in float2 normal2D, in float3 bumpColor, in float roughness, inout float3 albedo, inout float4 color)
{
	float2 size;
	float mipLevels;
	xRefraction.GetDimensions(0, size.x, size.y, mipLevels);
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + (normal2D + bumpColor.rg) * g_xMat_refractionIndex;
	float4 refractiveColor = xRefraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, (g_xWorld_AdvancedRefractions ? roughness * mipLevels : 0));
	albedo.rgb = lerp(refractiveColor.rgb, albedo.rgb, color.a);
	color.a = 1;
}

inline void DirectionalLight(in float3 N, in float3 V, in float3 P, in float3 f0, in float3 albedo, in float roughness,
	inout float3 diffuse, out float3 specular)
{
	LightingResult result = DirectionalLight(EntityArray[g_xFrame_SunEntityArrayIndex], N, V, P, roughness, f0);
	diffuse = result.diffuse;
	specular = result.specular;
}


inline void TiledLighting(in float2 pixel, in float3 N, in float3 V, in float3 P, in float3 f0, inout float3 albedo, in float roughness,
	inout float3 diffuse, out float3 specular)
{
	uint2 tileIndex = uint2(floor(pixel / TILED_CULLING_BLOCKSIZE));
	uint startOffset = EntityGrid[tileIndex].x;
	uint arrayProperties = EntityGrid[tileIndex].y;
	uint arrayLength = arrayProperties & 0x00FFFFFF; // count of every element in the tile
	uint decalCount = (arrayProperties & 0xFF000000) >> 24; // count of just the decals in the tile
	uint iterator = 0;

	specular = 0;
	diffuse = 0;

#ifdef DISABLE_DECALS
	// decals are disabled, set the iterator to skip decals:
	iterator = decalCount;
#else
	// decals are enabled, loop through them first:
	float4 decalAccumulation = 0;
	float3 P_dx = ddx_coarse(P);
	float3 P_dy = ddy_coarse(P);

	[loop]
	for (; iterator < decalCount; ++iterator)
	{
		ShaderEntityType decal = EntityArray[EntityIndexList[startOffset + iterator]];

		float4x4 decalProjection = decal.GetProjection();
		float3 clipSpace = mul(float4(P, 1), decalProjection).xyz;
		float3 uvw = clipSpace.xyz*float3(0.5f, -0.5f, 0.5f) + 0.5f;
		[branch]
		if (!any(uvw - saturate(uvw)))
		{
			// mipmapping needs to be performed by hand:
			float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy * decal.texMulAdd.xy;
			float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy * decal.texMulAdd.xy;
			float4 decalColor = texture_decalatlas.SampleGrad(sampler_objectshader, uvw.xy*decal.texMulAdd.xy + decal.texMulAdd.zw, decalDX, decalDY);
			// blend out if close to cube Z:
			float edgeBlend = 1 - pow(saturate(abs(clipSpace.z)), 8);
			decalColor.a *= edgeBlend;
			decalColor *= decal.color;
			// apply emissive:
			specular += max(0, decalColor.rgb * decal.GetEmissive() * edgeBlend);
			// perform manual blending of decals:
			//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
			decalAccumulation.rgb = (1 - decalAccumulation.a) * (decalColor.a*decalColor.rgb) + decalAccumulation.rgb;
			decalAccumulation.a = decalColor.a + (1 - decalColor.a) * decalAccumulation.a;
			// if the accumulation reached 1, we skip the rest of the decals:
			iterator = decalAccumulation.a < 1 ? iterator : decalCount - 1;
		}
	}

	albedo.rgb = lerp(albedo.rgb, decalAccumulation.rgb, decalAccumulation.a);
#endif

	[loop]
	for (; iterator < arrayLength; iterator++)
	{
		ShaderEntityType light = EntityArray[EntityIndexList[startOffset + iterator]];

		LightingResult result = (LightingResult)0;

		switch (light.type)
		{
		case ENTITY_TYPE_DIRECTIONALLIGHT:
		{
			result = DirectionalLight(light, N, V, P, roughness, f0);
		}
		break;
		case ENTITY_TYPE_POINTLIGHT:
		{
			result = PointLight(light, N, V, P, roughness, f0);
		}
		break;
		case ENTITY_TYPE_SPOTLIGHT:
		{
			result = SpotLight(light, N, V, P, roughness, f0);
		}
		break;
		case ENTITY_TYPE_SPHERELIGHT:
		{
			result = SphereLight(light, N, V, P, roughness, f0);
		}
		break;
		case ENTITY_TYPE_DISCLIGHT:
		{
			result = DiscLight(light, N, V, P, roughness, f0);
		}
		break;
		case ENTITY_TYPE_RECTANGLELIGHT:
		{
			result = RectangleLight(light, N, V, P, roughness, f0);
		}
		break;
		case ENTITY_TYPE_TUBELIGHT:
		{
			result = TubeLight(light, N, V, P, roughness, f0);
		}
		break;
		}

		diffuse += max(0.0f, result.diffuse);
		specular += max(0.0f, result.specular);
	}
}


// MACROS
////////////

#define OBJECT_PS_MAKE_SIMPLE												\
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;		\
	float4 baseColor = g_xMat_baseColor * float4(input.instanceColor, 1);	\
	float4 color = baseColor;												\
	float opacity = color.a;												\
	float emissive = g_xMat_emissive;										\
	float2 pixel = input.pos.xy;


#define OBJECT_PS_MAKE_COMMON												\
	OBJECT_PS_MAKE_SIMPLE													\
	float3 diffuse = 0;														\
	float3 specular = 0;													\
	float3 N = normalize(input.nor);										\
	float3 P = input.pos3D;													\
	float3 V = g_xCamera_CamPos - P;										\
	float dist = length(V);													\
	V /= dist;																\
	float roughness = g_xMat_roughness;										\
	roughness = saturate(roughness);										\
	float metalness = g_xMat_metalness;										\
	metalness = saturate(metalness);										\
	float reflectance = g_xMat_reflectance;									\
	reflectance = saturate(reflectance);									\
	float sss = g_xMat_subsurfaceScattering;								\
	float3 bumpColor = 0;													\
	float depth = input.pos.z;												\
	float ao = input.ao;

#define OBJECT_PS_MAKE																								\
	OBJECT_PS_MAKE_COMMON																							\
	float lineardepth = input.pos2D.w;																				\
	float2 refUV = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w * 0.5f + 0.5f;\
	float2 ScreenCoord = float2(1, -1) * input.pos2D.xy / input.pos2D.w * 0.5f + 0.5f;								\
	float2 velocity = ((input.pos2DPrev.xy/input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (input.pos2D.xy/input.pos2D.w - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

#define OBJECT_PS_COMPUTETANGENTSPACE										\
	float3 T, B;															\
	float3x3 TBN = compute_tangent_frame(N, P, UV, T, B);

#define OBJECT_PS_SAMPLETEXTURES_SIMPLE										\
	baseColor *= xBaseColorMap.Sample(sampler_objectshader, UV);			\
	ALPHATEST(baseColor.a);													\
	color = baseColor;														\
	opacity = color.a;														\

#define OBJECT_PS_SAMPLETEXTURES											\
	OBJECT_PS_SAMPLETEXTURES_SIMPLE											\
	roughness *= xRoughnessMap.Sample(sampler_objectshader, UV).r;			\
	metalness *= xMetalnessMap.Sample(sampler_objectshader, UV).r;			\
	reflectance *= xReflectanceMap.Sample(sampler_objectshader, UV).r;

#define OBJECT_PS_NORMALMAPPING												\
	NormalMapping(UV, P, N, TBN, bumpColor);

#define OBJECT_PS_PARALLAXOCCLUSIONMAPPING									\
	ParallaxOcclusionMapping(UV, V, TBN);

#define OBJECT_PS_SPECULARANTIALIASING										\
	SpecularAA(N, roughness);

#define OBJECT_PS_LIGHT_BEGIN																						\
	BRDF_HELPER_MAKEINPUTS( color, reflectance, metalness )

#define OBJECT_PS_REFRACTION																						\
	Refraction(ScreenCoord, input.nor2D, bumpColor, roughness, albedo, color);

#define OBJECT_PS_LIGHT_DIRECTIONAL																					\
	DirectionalLight(N, V, P, f0, albedo, roughness, diffuse, specular);

#define OBJECT_PS_LIGHT_TILED																						\
	TiledLighting(pixel, N, V, P, f0, albedo, roughness, diffuse, specular);

#define OBJECT_PS_VOXELRADIANCE																						\
	VoxelRadiance(N, V, P, f0, roughness, diffuse, specular, ao);

#define OBJECT_PS_LIGHT_END																							\
	color.rgb = lerp(1, GetAmbientColor() * ao + diffuse, opacity) * albedo + specular;

#define OBJECT_PS_DITHER																							\
	clip(dither(input.pos.xy) - input.dither);

#define OBJECT_PS_PLANARREFLECTIONS																					\
	specular = max(specular, PlanarReflection(UV, refUV, N, V, roughness, f0));

#define OBJECT_PS_ENVIRONMENTREFLECTIONS																			\
	specular = max(specular, EnvironmentReflection(N, V, P, roughness, f0));

#define OBJECT_PS_DEGAMMA																							\
	color = DEGAMMA(color);

#define OBJECT_PS_GAMMA																								\
	color = GAMMA(color);

#define OBJECT_PS_EMISSIVE																							\
	color.rgb += baseColor.rgb * GetEmissive(emissive);

#define OBJECT_PS_FOG																								\
	color.rgb = applyFog(color.rgb, getFog(dist));

#define OBJECT_PS_OUT_GBUFFER																						\
	GBUFFEROutputType Out = (GBUFFEROutputType)0;																	\
	Out.g0 = float4(color.rgb, 1);									/*FORMAT_R8G8B8A8_UNORM*/						\
	Out.g1 = float4(encode(N), velocity);							/*FORMAT_R16G16B16_FLOAT*/						\
	Out.g2 = float4(0, 0, sss, emissive);							/*FORMAT_R8G8B8A8_UNORM*/						\
	Out.g3 = float4(roughness, reflectance, metalness, ao);			/*FORMAT_R8G8B8A8_UNORM*/						\
	return Out;

#define OBJECT_PS_OUT_FORWARD																						\
	GBUFFEROutputType_Thin Out = (GBUFFEROutputType_Thin)0;															\
	Out.g0 = color;													/*FORMAT_R16G16B16_FLOAT*/						\
	Out.g1 = float4(encode(N), velocity);							/*FORMAT_R16G16B16_FLOAT*/						\
	return Out;

#define OBJECT_PS_OUT_FORWARD_SIMPLE																				\
	return color;

#endif // _OBJECTSHADER_HF_