#ifndef _LIGHTHF_
#define _LIGHTHF_
#include "specularHF.hlsli"
#include "globalsHF.hlsli"
#include "globals.hlsli"

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



// MACROS

// C	:	(float4) light color
#define DEFERREDLIGHT_MAKEPARAMS(C)													\
	float4 color = float4(C.rgb,1);													\
	float2 screenPos = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;	\
	float depth = texture_depth.SampleLevel(sampler_point_clamp, screenPos, 0);		\
	float4 norU = texture_gbuffer1.SampleLevel(sampler_point_clamp,screenPos,0);	\
	bool unshaded = isUnshaded(norU.w);												\
	float4 material = texture_gbuffer2.SampleLevel(sampler_point_clamp,screenPos,0);\
	float specular = material.w*specularMaximumIntensity;							\
	uint specular_power = material.z;												\
	float3 N = norU.xyz;															\
	float3 P = getPosition(screenPos, depth);										\
	float3 V = normalize(P - g_xCamera_CamPos);

#define DEFERREDLIGHT_RETURN	\
	return (unshaded? 1 : max(color, 0.0f));



#endif // _LIGHTHF_



