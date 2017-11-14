#ifndef _OCEAN_SURFACE_HF_
#define _OCEAN_SURFACE_HF_
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"


struct GSOut
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

#endif // _OCEAN_SURFACE_HF_
