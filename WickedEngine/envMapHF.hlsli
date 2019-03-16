#ifndef _ENVMAP_HF_
#define _ENVMAP_HF_

#include "globals.hlsli"

struct VSOut_EnvmapRendering
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float2 atl : ATLAS;
	float3 nor : NORMAL;
};

struct PSIn_EnvmapRendering
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float2 atl : ATLAS;
	float3 nor : NORMAL;
	float3 pos3D : WORLDPOSITION;
	uint RTIndex	: SV_RenderTargetArrayIndex;
};


struct VSOut_Sky_EnvmapRendering
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
};

struct PSIn_Sky_EnvmapRendering
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	uint RTIndex : SV_RenderTargetArrayIndex;
};

#endif // _ENVMAP_HF_