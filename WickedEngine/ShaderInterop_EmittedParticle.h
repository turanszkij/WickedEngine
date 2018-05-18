#ifndef _SHADERINTEROP_EMITTEDPARTICLE_H_
#define _SHADERINTEROP_EMITTEDPARTICLE_H_

#include "ShaderInterop.h"

struct Particle
{
	float3 position;
	float rotationalVelocity;
	float3 velocity;
	float maxLife;
	float life;
	uint color_mirror;
	float2 sizeBeginEnd;
};

struct ParticleCounters
{
	uint aliveCount;
	uint deadCount;
	uint realEmitCount;
	uint aliveCount_afterSimulation;
};
static const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT = 0;
static const uint PARTICLECOUNTER_OFFSET_DEADCOUNT = PARTICLECOUNTER_OFFSET_ALIVECOUNT + 4;
static const uint PARTICLECOUNTER_OFFSET_REALEMITCOUNT = PARTICLECOUNTER_OFFSET_DEADCOUNT + 4;
static const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION = PARTICLECOUNTER_OFFSET_REALEMITCOUNT + 4;

CBUFFER(EmittedParticleCB, CBSLOT_OTHER_EMITTEDPARTICLE)
{
	float4x4	xEmitterWorld;

	uint		xEmitCount;
	uint		xEmitterMeshIndexCount;
	uint		xEmitterMeshVertexPositionStride;
	float		xEmitterRandomness;

	float		xParticleSize;
	float		xParticleScaling;
	float		xParticleRotation;
	uint		xParticleColor;

	float		xParticleRandomFactor;
	float		xParticleNormalFactor;
	float		xParticleLifeSpan;
	float		xParticleLifeSpanRandomness;

	float		xParticleMass;
	float		xParticleMotionBlurAmount;
	float		xEmitterOpacity;
	uint		xEmitterMaxParticleCount;

	float		xSPH_h;		// smoothing radius
	float		xSPH_h2;	// smoothing radius ^ 2
	float		xSPH_h3;	// smoothing radius ^ 3
	float		xSPH_h6;	// smoothing radius ^ 6

	float		xSPH_h9;	// smoothing radius ^ 9
	float		xSPH_K;		// pressure constant
	float		xSPH_p0;	// reference density
	float		xSPH_e;		// viscosity constant

};

#define THREADCOUNT_EMIT 256
#define THREADCOUNT_SIMULATION 256

static const uint ARGUMENTBUFFER_OFFSET_DISPATCHEMIT = 0;
static const uint ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION = ARGUMENTBUFFER_OFFSET_DISPATCHEMIT + (3 * 4);
static const uint ARGUMENTBUFFER_OFFSET_DRAWPARTICLES = ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION + (3 * 4);


static const uint SPH_PARTITION_BUCKET_COUNT = 128 * 128 * 128;

inline uint SPH_GridHash(int3 cellIndex)
{
	const uint p1 = 73856093;   // some large primes 
	const uint p2 = 19349663;
	const uint p3 = 83492791;
	int n = p1 * cellIndex.x ^ p2*cellIndex.y ^ p3*cellIndex.z;
	n %= SPH_PARTITION_BUCKET_COUNT;
	return n;
}

#endif // _SHADERINTEROP_EMITTEDPARTICLE_H_

