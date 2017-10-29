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


CBUFFER(EmittedParticleCB, CBSLOT_OTHER_EMITTEDPARTICLE)
{
	float4x4	xEmitterWorld;

	uint		xEmitterMeshIndexCount;
	uint		xEmitterMeshIndexStride;
	uint		xEmitterMeshVertexPositionStride;
	uint		xEmitterMeshVertexNormalStride;

	uint		xEmitCount;
	float		xEmitterRandomness;
	float		xParticleSize;
	float		xParticleScaling;

	float		xParticleRandomFactor;
	float		xParticleNormalFactor;
	float		xParticleLifeSpan;
	float		xParticleLifeSpanRandomness;

	float		xParticleMotionBlurAmount;
	float		xParticleRotation;
	float2		xPadding_EmitterCB;
};

#define THREADCOUNT_EMIT 256
#define THREADCOUNT_SIMULATION 256


#endif // _SHADERINTEROP_EMITTEDPARTICLE_H_

