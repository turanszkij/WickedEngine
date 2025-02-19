#ifndef WI_SHADERINTEROP_HAIRPARTICLE_H
#define WI_SHADERINTEROP_HAIRPARTICLE_H

#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

#define THREADCOUNT_SIMULATEHAIR 64

struct alignas(16) PatchSimulationData
{
	float3 prevTail;
	float padding0;
	float3 currentTail;
	float padding1;
};

enum HAIR_FLAGS
{
	HAIR_FLAG_REGENERATE_FRAME = 1 << 0,
	HAIR_FLAG_UNORM_POS = 1 << 1,
	HAIR_FLAG_CAMERA_BEND = 1 << 2,
};

struct HairParticleAtlasRect
{
	float4 texMulAdd;
	float size;
	float aspect;
	float padding1;
	float padding2;
};

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	ShaderTransform xHairTransform;
	ShaderTransform xHairBaseMeshUnormRemap;

	uint xHairFlags;
	float xHairLength;
	float xHairStiffness;
	float xHairDrag;

	uint xHairParticleCount;
	uint xHairStrandCount;
	uint xHairSegmentCount;
	uint xHairRandomSeed;

	float xHairViewDistance;
	uint xHairBaseMeshIndexCount;
	uint xHairInstanceIndex;
	uint xHairLayerMask;

	float xHairRandomness;
	float xHairAspect;
	float xHairUniformity;
	uint xHairAtlasRectCount;

	float xHairGravityPower;
	uint xHairBillboardCount;
	float xHair_padding0;
	float xHair_padding1;

	HairParticleAtlasRect xHairAtlasRects[64];
};

#endif // WI_SHADERINTEROP_HAIRPARTICLE_H
