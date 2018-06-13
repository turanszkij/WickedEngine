#ifndef _SHADERINTEROP_TRACEDRENDERING_H_
#define _SHADERINTEROP_TRACEDRENDERING_H_
#include "ShaderInterop.h"

#define TRACEDRENDERING_BVH_GROUPSIZE 256

#define TRACEDRENDERING_CLEAR_BLOCKSIZE 8
#define TRACEDRENDERING_LAUNCH_BLOCKSIZE 8
#define TRACEDRENDERING_PRIMARY_GROUPSIZE 256

CBUFFER(TracedRenderingCB, CBSLOT_RENDERER_TRACED)
{
	float2 xTracePixelOffset;
	uint xTraceMeshTriangleCount;
	uint padding_traceCB;
};

CBUFFER(TracedBVHCB, CBSLOT_RENDERER_TRACED)
{
	float4x4 xTraceBVHWorld;
	uint xTraceBVHMaterialOffset;
	uint xTraceBVHMeshTriangleOffset;
	uint xTraceBVHMeshTriangleCount;
	uint xTraceBVHMeshVertexPOSStride;
};

struct TracedRenderingMeshTriangle
{
	float3 v0, v1, v2;
	float3 n0, n1, n2;
	float2 t0, t1, t2;
	uint materialIndex;
};

#endif // _SHADERINTEROP_TRACEDRENDERING_H_
