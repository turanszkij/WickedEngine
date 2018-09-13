#ifndef _SHADERINTEROP_HAIRPARTICLE_H_
#define _SHADERINTEROP_HAIRPARTICLE_H_

#include "ShaderInterop.h"

#define THREADCOUNT_SIMULATEHAIR 256

static const uint particleBuffer_stride = 16 + 4 + 4; // pos, normal, tangent 

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	float4x4 xWorld;
	float4 xColor;
	float xLength;
	float LOD0;
	float LOD1;
	float LOD2;
};

#endif // _SHADERINTEROP_HAIRPARTICLE_H_
