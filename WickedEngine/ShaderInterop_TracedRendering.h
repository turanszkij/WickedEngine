#ifndef _SHADERINTEROP_TRACEDRENDERING_H_
#define _SHADERINTEROP_TRACEDRENDERING_H_
#include "ShaderInterop.h"


#define TRACEDRENDERING_CLEAR_BLOCKSIZE 8
#define TRACEDRENDERING_LAUNCH_BLOCKSIZE 8
#define TRACEDRENDERING_TRACE_GROUPSIZE 256


CBUFFER(TracedRenderingCB, CBSLOT_RENDERER_TRACED)
{
	float2 xTracePixelOffset;
	float xTraceRandomSeed;
	float xTraceUserData;
	uint4 xTraceUserData2;
};

struct TracedRenderingStoredRay
{
	uint pixelID;
	float3 origin;
	uint3 direction_energy;
	uint primitiveID;
	float2 bary;
};

struct TracedRenderingMaterial
{
	float4		baseColor;
	float4		texMulAdd;
	float		roughness;
	float		reflectance;
	float		metalness;
	float		emissive;
	float		refractionIndex;
	float		subsurfaceScattering;
	float		normalMapStrength;
	float		parallaxOcclusionMapping;

	float4		baseColorAtlasMulAdd;
	float4		surfaceMapAtlasMulAdd;
	float4		normalMapAtlasMulAdd;
};


#endif // _SHADERINTEROP_TRACEDRENDERING_H_
