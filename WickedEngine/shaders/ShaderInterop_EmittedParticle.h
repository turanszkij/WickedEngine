#ifndef WI_SHADERINTEROP_EMITTEDPARTICLE_H
#define WI_SHADERINTEROP_EMITTEDPARTICLE_H

#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

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
	uint color;
};

struct ParticleCounters
{
	uint aliveCount;
	uint deadCount;
	uint realEmitCount;
	uint aliveCount_afterSimulation;
	uint culledCount;
	uint cellAllocator;
};
static const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT = 0;
static const uint PARTICLECOUNTER_OFFSET_DEADCOUNT = PARTICLECOUNTER_OFFSET_ALIVECOUNT + 4;
static const uint PARTICLECOUNTER_OFFSET_REALEMITCOUNT = PARTICLECOUNTER_OFFSET_DEADCOUNT + 4;
static const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION = PARTICLECOUNTER_OFFSET_REALEMITCOUNT + 4;
static const uint PARTICLECOUNTER_OFFSET_CULLEDCOUNT = PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION + 4;
static const uint PARTICLECOUNTER_OFFSET_CELLALLOCATOR = PARTICLECOUNTER_OFFSET_CULLEDCOUNT + 4;

static const uint EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED = 1 << 0;
static const uint EMITTER_OPTION_BIT_SPH_ENABLED = 1 << 1;
static const uint EMITTER_OPTION_BIT_MESH_SHADER_ENABLED = 1 << 2;
static const uint EMITTER_OPTION_BIT_COLLIDERS_DISABLED = 1 << 3;
static const uint EMITTER_OPTION_BIT_USE_RAIN_BLOCKER = 1 << 4;
static const uint EMITTER_OPTION_BIT_TAKE_COLOR_FROM_MESH = 1 << 5;

CBUFFER(EmittedParticleCB, CBSLOT_OTHER_EMITTEDPARTICLE)
{
	ShaderTransform	xEmitterTransform;
	ShaderTransform xEmitterBaseMeshUnormRemap;

	uint		xEmitCount;
	float		xEmitterRandomness;
	float		xParticleRandomColorFactor;
	float		xParticleSize;

	float		xParticleScaling;
	float		xParticleRotation;
	float		xParticleRandomFactor;
	float		xParticleNormalFactor;

	float		xParticleLifeSpan;
	float		xParticleLifeSpanRandomness;
	float		xParticleMass;
	float		xParticleMotionBlurAmount;

	uint		xEmitterMaxParticleCount;
	uint		xEmitterInstanceIndex;
	uint		xEmitterMeshGeometryOffset;
	uint		xEmitterMeshGeometryCount;

	uint2		xEmitterFramesXY;
	uint		xEmitterFrameCount;
	uint		xEmitterFrameStart;

	float2		xEmitterTexMul;
	float		xEmitterFrameRate;
	uint		xEmitterLayerMask;

	float		xSPH_h;					// smoothing radius
	float		xSPH_h_rcp;				// 1.0f / smoothing radius
	float		xSPH_h2;				// smoothing radius ^ 2
	float		xSPH_h3;				// smoothing radius ^ 3

	float		xSPH_poly6_constant;	// precomputed Poly6 kernel constant term
	float		xSPH_spiky_constant;	// precomputed Spiky kernel function constant term
	float		xSPH_visc_constant;	    // precomputed viscosity kernel function constant term
	float		xSPH_K;					// pressure constant

	float		xSPH_e;					// viscosity constant
	float		xSPH_p0;				// reference density
	uint		xEmitterOptions;
	float		xEmitterFixedTimestep;	// we can force a fixed timestep (>0) onto the simulation to avoid blowing up

	float3		xParticleGravity;
	float		xEmitterRestitution;

	float3		xParticleVelocity;
	float		xParticleDrag;
};

static const uint THREADCOUNT_EMIT = 256;
static const uint THREADCOUNT_MESH_SHADER = 32;

static const uint ARGUMENTBUFFER_OFFSET_DISPATCHEMIT = 0;
static const uint ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION = wi::graphics::AlignTo(ARGUMENTBUFFER_OFFSET_DISPATCHEMIT + (3 * 4), IndirectDispatchArgsAlignment);
static const uint ARGUMENTBUFFER_OFFSET_DRAWPARTICLES = wi::graphics::AlignTo(ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION + (3 * 4), IndirectDrawArgsAlignment);


// If this is not defined, SPH will be resolved as N-body simulation (O(n^2) complexity)
// If this is defined, SPH will be sorted into a hashed grid structure and response lookup will be accelerated
#define SPH_USE_ACCELERATION_GRID
static const uint SPH_PARTITION_BUCKET_COUNT = 128 * 128 * 64;

#ifdef SPH_USE_ACCELERATION_GRID
static const uint THREADCOUNT_SIMULATION = 64;
#else
static const uint THREADCOUNT_SIMULATION = 256;
#endif // SPH_USE_ACCELERATION_GRID

inline uint SPH_GridHash(int3 cellIndex)
{
	const uint p1 = 73856093;   // some large primes 
	const uint p2 = 19349663;
	const uint p3 = 83492791;
	int n = p1 * cellIndex.x ^ p2*cellIndex.y ^ p3*cellIndex.z;
	n %= SPH_PARTITION_BUCKET_COUNT;
	return n;
}
struct SPHGridCell
{
	uint count;
	uint offset;
};

// 27 neighbor offsets in a 3D grid, including center cell:
static const int3 sph_neighbor_offsets[27] = {
	int3(-1, -1, -1),
	int3(-1, -1, 0),
	int3(-1, -1, 1),
	int3(-1, 0, -1),
	int3(-1, 0, 0),
	int3(-1, 0, 1),
	int3(-1, 1, -1),
	int3(-1, 1, 0),
	int3(-1, 1, 1),
	int3(0, -1, -1),
	int3(0, -1, 0),
	int3(0, -1, 1),
	int3(0, 0, -1),
	int3(0, 0, 0),
	int3(0, 0, 1),
	int3(0, 1, -1),
	int3(0, 1, 0),
	int3(0, 1, 1),
	int3(1, -1, -1),
	int3(1, -1, 0),
	int3(1, -1, 1),
	int3(1, 0, -1),
	int3(1, 0, 0),
	int3(1, 0, 1),
	int3(1, 1, -1),
	int3(1, 1, 0),
	int3(1, 1, 1),
};

#endif // WI_SHADERINTEROP_EMITTEDPARTICLE_H

