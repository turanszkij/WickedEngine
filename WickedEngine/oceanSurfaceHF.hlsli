#ifndef _OCEAN_SURFACE_HF_
#define _OCEAN_SURFACE_HF_
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"


struct GSOut
{
	float4 pos		: SV_POSITION;
	float4 pos2D	: SCREENPOSITION;
	float3 pos3D	: WORLDPOSITION;
	float2 uv		: TEXCOORD0;
	float4 ReflectionMapSamplingPos : REFLECTIONPOS;
};

#endif // _OCEAN_SURFACE_HF_
