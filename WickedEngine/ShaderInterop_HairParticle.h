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
	uint padding;
};

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	float4x4 xWorld;
	float4 xColor;

	uint xHairRegenerate;
	float xLength;
	float xStiffness;
	float xHairRandomness;

	uint xHairBaseMeshIndexCount;
	uint xHairBaseMeshVertexPositionStride;
	uint xHairNumDispatchGroups;
	uint pad;
};

#endif // _SHADERINTEROP_HAIRPARTICLE_H_
