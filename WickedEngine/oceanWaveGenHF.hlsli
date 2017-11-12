#ifndef _OCEAN_SIMULATOR_HF_
#define _OCEAN_SIMULATOR_HF_
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

//---------------------------------------- Vertex Shaders ------------------------------------------
struct VS_QUAD_OUTPUT
{
	float4 Position		: SV_POSITION;	// vertex position
	float2 TexCoord		: TEXCOORD0;	// vertex texture coords 
};

//----------------------------------------- Pixel Shaders ------------------------------------------

//// Textures and sampling states
//#define g_samplerDisplacementMap texture_0
//
//SAMPLERSTATE(LinearSampler, SSLOT_ONDEMAND0);


#endif // _OCEAN_SIMULATOR_HF_
