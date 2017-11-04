#ifndef _SHADERINTEROP_EMITTEDPARTICLE_H_
#define _SHADERINTEROP_EMITTEDPARTICLE_H_

#include "ShaderInterop.h"

struct Particle
{
	float3 position;
	float size;
	float rotation;
	float3 velocity;
	float rotationalVelocity;
	float maxLife;
	float life;
	float opacity;
	float2 sizeBeginEnd;
	float2 mirror;
};

struct ParticleCounters
{
	uint aliveCount;
	uint deadCount;
	uint realEmitCount;
	uint aliveCount_afterSimulation;
};

CBUFFER(EmittedParticleCB, CBSLOT_OTHER_EMITTEDPARTICLE)
{
	float4x4	xEmitterWorld;

	uint		xEmitCount;
	uint		xEmitterMeshIndexCount;
	uint		xEmitterMeshVertexPositionStride;
	uint		xEmitterMeshVertexNormalStride;

	float		xEmitterRandomness;
	float		xParticleSize;
	float		xParticleScaling;
	float		xParticleRotation;

	float		xParticleRandomFactor;
	float		xParticleNormalFactor;
	float		xParticleLifeSpan;
	float		xParticleLifeSpanRandomness;

	float		xParticleMotionBlurAmount;
	float3		xPadding_EmitterCB;
};

CBUFFER(SortConstants, 0)
{
	int4 job_params;
};
#define SORT_SIZE 512

#define THREADCOUNT_EMIT 256
#define THREADCOUNT_SIMULATION 256


#endif // _SHADERINTEROP_EMITTEDPARTICLE_H_

