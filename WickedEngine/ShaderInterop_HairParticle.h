#ifndef _SHADERINTEROP_HAIRPARTICLE_H_
#define _SHADERINTEROP_HAIRPARTICLE_H_

#include "ShaderInterop.h"

#define THREADCOUNT_SIMULATEHAIR 256

struct Patch
{
	float3 position;
	uint tangent_random;
	float3 normal;
	uint binormal_length;
};

struct PatchSimulationData
{
	float3 velocity;
	uint target;
};

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	float4x4 xWorld;
	float4 xColor;

	float xLength;
	float LOD0;
	float LOD1;
	float LOD2;

	float xStiffness;
	float3 pad;
};

#endif // _SHADERINTEROP_HAIRPARTICLE_H_
