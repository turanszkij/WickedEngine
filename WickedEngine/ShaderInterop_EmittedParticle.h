#ifndef WI_SHADERINTEROP_EMITTEDPARTICLE_H
#define WI_SHADERINTEROP_EMITTEDPARTICLE_H

#include "ShaderInterop.h"

struct Particle
{
	float3 position;
	float mass;
	float3 force;
	float rotationalVelocity;
	float3 velocity;
	float maxLife;
	float2 sizeBeginEnd;
	float life;
	uint color_mirror;
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

static const uint EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED = 1 << 0;
static const uint EMITTER_OPTION_BIT_SPH_ENABLED = 1 << 1;
static const uint EMITTER_OPTION_BIT_MESH_SHADER_ENABLED = 1 << 2;

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

	uint2		xEmitterFramesXY;
	uint		xEmitterFrameCount;
	uint		xEmitterFrameStart;

	float2		xEmitterTexMul;
	float		xEmitterFrameRate;
	float		padding_xEmitter;

	float		xSPH_h;					// smoothing radius
	float		xSPH_h_rcp;				// 1.0f / smoothing radius
	float		xSPH_h2;				// smoothing radius ^ 2
	float		xSPH_h3;				// smoothing radius ^ 3

	float		xSPH_poly6_constant;	// precomputed Poly6 kernel constant term
	float		xSPH_spiky_constant;	// precomputed Spiky kernel function constant term
	float		xSPH_K;					// pressure constant
	float		xSPH_p0;				// reference density

	float		xSPH_e;					// viscosity constant
	uint		xEmitterOptions;
	float		xEmitterFixedTimestep;	// we can force a fixed timestep (>0) onto the simulation to avoid blowing up
	float		xParticleEmissive;

};

static const uint THREADCOUNT_EMIT = 256;
static const uint THREADCOUNT_SIMULATION = 256;
static const uint THREADCOUNT_MESH_SHADER = 32;

static const uint ARGUMENTBUFFER_OFFSET_DISPATCHEMIT = 0;
static const uint ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION = ARGUMENTBUFFER_OFFSET_DISPATCHEMIT + (3 * 4);
static const uint ARGUMENTBUFFER_OFFSET_DRAWPARTICLES = ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION + (3 * 4);


// If this is not defined, SPH will be resolved as N-body simulation (O(n^2) complexity)
// If this is defined, SPH will be sorted into a hashed grid structure and response lookup will be accelerated
#define SPH_USE_ACCELERATION_GRID
static const uint SPH_PARTITION_BUCKET_COUNT = 128 * 128 * 64;

inline uint SPH_GridHash(int3 cellIndex)
{
	const uint p1 = 73856093;   // some large primes 
	const uint p2 = 19349663;
	const uint p3 = 83492791;
	int n = p1 * cellIndex.x ^ p2*cellIndex.y ^ p3*cellIndex.z;
	n %= SPH_PARTITION_BUCKET_COUNT;
	return n;
}

#endif // WI_SHADERINTEROP_EMITTEDPARTICLE_H

