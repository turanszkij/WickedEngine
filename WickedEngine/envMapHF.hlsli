#ifndef WI_ENVMAP_HF
#define WI_ENVMAP_HF

#include "globals.hlsli"

struct PSIn_EnvmapRendering
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float2 atl : ATLAS;
	float3 nor : NORMAL;
	float4 tan : TANGENT;
	float3 pos3D : WORLDPOSITION;
#ifdef VPRT_EMULATION
	uint RTIndex	: RTINDEX;
#else
	uint RTIndex	: SV_RenderTargetArrayIndex;
#endif // VPRT_EMULATION
};

struct PSIn_Sky_EnvmapRendering
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
#ifdef VPRT_EMULATION
	uint RTIndex	: RTINDEX;
#else
	uint RTIndex	: SV_RenderTargetArrayIndex;
#endif // VPRT_EMULATION
};

#endif // WI_ENVMAP_HF