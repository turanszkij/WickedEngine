#ifndef _OCEAN_SURFACE_HF_
#define _OCEAN_SURFACE_HF_
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"


#define PATCH_BLEND_BEGIN		100
#define PATCH_BLEND_END			2000


#define g_texDisplacement	texture_0 // FFT wave displacement map in VS
#define g_texPerlin			texture_1 // FFT wave gradient map in PS
#define g_texGradient		texture_2 // Perlin wave displacement & gradient map in both VS & PS
TEXTURE1D(g_texFresnel, float4, TEXSLOT_ONDEMAND3); // Fresnel factor lookup table
#define g_texReflectCube	texture_env_global

struct VS_OUTPUT
{
	float4 Position	 : SV_POSITION;
	float2 TexCoord	 : TEXCOORD0;
	float3 LocalPos	 : TEXCOORD1;
};

#endif // _OCEAN_SURFACE_HF_
