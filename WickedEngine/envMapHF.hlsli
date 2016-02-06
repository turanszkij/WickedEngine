#ifndef _ENVMAP_HF_
#define _ENVMAP_HF_

#include "globals.hlsli"

struct VSOut
{
	float4 pos : SV_Position;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD0;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	float  ao : AMBIENT_OCCLUSION;
};

struct PSIn
{
	float4 pos : SV_Position;
	float3 pos3D : WORLDPOSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD0;
	float  ao : AMBIENT_OCCLUSION;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	uint RTIndex	: SV_RenderTargetArrayIndex;
};


struct VSOut_Sky
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
};

struct PSIn_Sky
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	uint RTIndex : SV_RenderTargetArrayIndex;
};

#endif // _ENVMAP_HF_