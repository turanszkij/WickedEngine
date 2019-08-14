#ifndef _SHADERINTEROP_RAYTRACING_H_
#define _SHADERINTEROP_RAYTRACING_H_
#include "ShaderInterop.h"


#define TRACEDRENDERING_LAUNCH_BLOCKSIZE 8
#define TRACEDRENDERING_TRACE_GROUPSIZE 64
#define TRACEDRENDERING_ACCUMULATE_BLOCKSIZE 8


CBUFFER(RaytracingCB, CBSLOT_RENDERER_TRACED)
{
	float2 xTracePixelOffset;
	float xTraceRandomSeed;
	float xTraceAccumulationFactor;
	uint2 xTraceResolution;
	float2 xTraceResolution_rcp;
	uint4 xTraceUserData;
};

struct RaytracingStoredRay
{
	float3 origin;
	uint pixelID;
	uint3 direction_energy;
	uint primitiveID;
	float2 bary;
	uint2 userdata; // vulkan complains about 16-byte padding here so might as well add userdata here and not pack barycentric coords
};


#endif // _SHADERINTEROP_RAYTRACING_H_
