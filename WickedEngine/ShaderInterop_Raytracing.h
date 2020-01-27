#ifndef WI_SHADERINTEROP_RAYTRACING_H
#define WI_SHADERINTEROP_RAYTRACING_H
#include "ShaderInterop.h"


static const uint RAYTRACING_LAUNCH_BLOCKSIZE = 8;
static const uint RAYTRACING_TRACE_GROUPSIZE = 64;
static const uint RAYTRACING_SORT_GROUPSIZE = 1024;

static const uint RAYTRACE_INDIRECT_OFFSET_TRACE = 0;
static const uint RAYTRACE_INDIRECT_OFFSET_TILESORT = 4 * 3;

// Whether to sort global ray buffer or only smaller bundles (tiles). 
//  The global sorting is slower, but on some GPUs, it is still worth to to this because the raytracing will be faster (more coherent)
#define RAYTRACING_SORT_GLOBAL


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


#endif // WI_SHADERINTEROP_RAYTRACING_H
