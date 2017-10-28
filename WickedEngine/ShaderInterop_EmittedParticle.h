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
	float		xMotionBlurAmount;
	uint		xEmitCount;
	uint		xMeshIndexCount;
	uint		xMeshIndexStride;
	uint		xMeshVertexPositionStride;
	uint		xMeshVertexNormalStride;
	uint2		xPadding_EmitterCB;
	float4x4	xEmitterWorld;
};

#define THREADCOUNT_EMIT 64
#define THREADCOUNT_SIMULATION 256


#endif // _SHADERINTEROP_EMITTEDPARTICLE_H_

