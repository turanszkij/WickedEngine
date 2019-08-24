#ifndef _SHADERINTEROP_RAYTRACING_H_
#define _SHADERINTEROP_RAYTRACING_H_
#include "ShaderInterop.h"


static const uint RAYTRACING_LAUNCH_BLOCKSIZE = 8;
static const uint RAYTRACING_TRACE_GROUPSIZE = 64;


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
	uint pixelID; // flattened pixel index
	uint3 direction_energy; // packed half3 direction | half3 energy
	uint primitiveID;
	float2 bary;
	uint2 color; // packed rgba16
};


#endif // _SHADERINTEROP_RAYTRACING_H_
