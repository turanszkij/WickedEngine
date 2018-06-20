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
	uint xTraceMeshTriangleCount;
};

struct TracedRenderingStoredRay
{
	uint pixelID;
	float3 origin;
	uint3 direction_energy;
	uint primitiveID;
	float2 bary;
};


#endif // _SHADERINTEROP_TRACEDRENDERING_H_
