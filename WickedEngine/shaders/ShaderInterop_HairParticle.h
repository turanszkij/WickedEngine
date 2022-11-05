#ifndef WI_SHADERINTEROP_HAIRPARTICLE_H
#define WI_SHADERINTEROP_HAIRPARTICLE_H

#include "ShaderInterop.h"

#define THREADCOUNT_SIMULATEHAIR 256

struct PatchSimulationData
{
	float3 position;
	uint tangent_random;
	float3 normal; // need high precision for the simulation!
	uint binormal_length;
	float3 velocity;
	uint padding;
};

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	ShaderTransform xHairTransform;

	uint xHairRegenerate;
	float xLength;
	float xStiffness;
	float xHairRandomness;

	uint xHairParticleCount;
	uint xHairStrandCount;
	uint xHairSegmentCount;
	uint xHairRandomSeed;

	float xHairViewDistance;
	uint xHairBaseMeshIndexCount;
	uint xHairBaseMeshVertexPositionStride;
	uint xHairInstanceIndex;

	uint2 xHairFramesXY;
	uint xHairFrameCount;
	uint xHairFrameStart;

	float2 xHairTexMul;
	float xHairAspect;
	uint xHairLayerMask;
};

#endif // WI_SHADERINTEROP_HAIRPARTICLE_H
