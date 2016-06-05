#ifndef _LIGHTHF_
#define _LIGHTHF_
#include "globals.hlsli"
#include "brdf.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

#include "reconstructPositionHF.hlsli"

//Texture2D<float> depthMap:register(t0);
//Texture2D<float4> normalMap:register(t1);
////Texture2D<float4> specularMap:register(t2);
//Texture2D<float4> materialMap:register(t2);

static const float specularMaximumIntensity = 1;


struct LightOutputType
{
	float4 diffuse : SV_TARGET0;
	float4 specular: SV_TARGET1;
};


// MACROS

#define DEFERREDLIGHT_MAKEPARAMS														\
	float3 diffuse, specular;															\
	float2 screenPos = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;		\
	float depth = texture_depth.SampleLevel(sampler_point_clamp, screenPos, 0);			\
	float4 baseColor = texture_gbuffer0.SampleLevel(sampler_point_clamp,screenPos,0);	\
	float4 g1 = texture_gbuffer1.SampleLevel(sampler_point_clamp,screenPos,0);			\
	float4 g2 = texture_gbuffer2.SampleLevel(sampler_point_clamp,screenPos,0);			\
	float3 N = g1.xyz;																	\
	float roughness = g1.w;																\
	float reflectance = g2.z;															\
	float metalness = g2.w;																\
	BRDF_HELPER_MAKEINPUTS( baseColor, reflectance, metalness )							\
	float3 P = getPosition(screenPos, depth);											\
	float3 V = normalize(g_xCamera_CamPos - P);

#define DEFERREDLIGHT_RETURN					\
	LightOutputType Out = (LightOutputType)0;	\
	Out.diffuse = float4(diffuse, 1);			\
	Out.specular = float4(specular, 1);			\
	return Out;



#endif // _LIGHTHF_



