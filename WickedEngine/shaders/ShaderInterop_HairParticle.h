#ifndef WI_SHADERINTEROP_HAIRPARTICLE_H
#define WI_SHADERINTEROP_HAIRPARTICLE_H

#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

#define THREADCOUNT_SIMULATEHAIR 64

struct PatchSimulationData
{
	float3 position;
	uint tangent_random;
	uint3 normal_velocity; // packed fp16
	uint binormal_length;
};

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	ShaderTransform xHairTransform;
	ShaderTransform xHairBaseMeshUnormRemap;

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
	uint xHairInstanceIndex;
	uint padding0_xHair;

	uint2 xHairFramesXY;
	uint xHairFrameCount;
	uint xHairFrameStart;

	float2 xHairTexMul;
	float xHairAspect;
	uint xHairLayerMask;
};

#endif // WI_SHADERINTEROP_HAIRPARTICLE_H
