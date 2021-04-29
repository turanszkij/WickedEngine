#ifndef WI_OCEAN_SURFACE_HF
#define WI_OCEAN_SURFACE_HF
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"


struct PSIn
{
	float4 pos		: SV_POSITION;
	float3 pos3D	: WORLDPOSITION;
	float2 uv		: TEXCOORD0;
	float4 ReflectionMapSamplingPos : REFLECTIONPOS;
};

#endif // WI_OCEAN_SURFACE_HF
