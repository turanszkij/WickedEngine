#ifndef _LIGHTHF_
#define _LIGHTHF_
#include "specularHF.hlsli"
#include "toonHF.hlsli"
#include "globalsHF.hlsli"
#include "globals.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

#include "reconstructPositionHF.hlsli"

Texture2D<float> depthMap:register(t0);
Texture2D<float4> normalMap:register(t1);
//Texture2D<float4> specularMap:register(t2);
Texture2D<float4> materialMap:register(t2);

static const float specularMaximumIntensity = 1;



// MACROS

// C	:	(float4) light color
#define DEFERREDLIGHT_MAKEPARAMS(C)													\
	float4 color = float4(C.rgb,1);													\
	float2 screenPos = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;	\
	float depth = depthMap.SampleLevel(sampler_point_clamp, screenPos, 0);			\
	float4 norU = normalMap.SampleLevel(sampler_point_clamp,screenPos,0);			\
	bool unshaded = isUnshaded(norU.w);												\
	float4 material = materialMap.SampleLevel(sampler_point_clamp,screenPos,0);		\
	float specular = material.w*specularMaximumIntensity;							\
	uint specular_power = material.z;												\
	float3 normal = norU.xyz;														\
	bool toonshaded = isToon(norU.w);												\
	float3 pos3D = getPosition(screenPos, depth);									\
	float3 eyevector = normalize(g_xCamera_CamPos - pos3D.xyz);

#define DEFERREDLIGHT_RETURN	\
	return max(unshaded? 1 : color, 0.0f);



#endif // _LIGHTHF_



