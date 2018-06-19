#ifndef _SHADERINTEROP_TRACEDRENDERING_H_
#define _SHADERINTEROP_TRACEDRENDERING_H_
#include "ShaderInterop.h"

#define TRACEDRENDERING_BVH_CLASSIFICATION_GROUPSIZE 64
#define TRACEDRENDERING_BVH_CLUSTERPROCESSOR_GROUPSIZE 64
#define TRACEDRENDERING_BVH_HIERARCHY_GROUPSIZE 64
#define TRACEDRENDERING_BVH_PROPAGATEAABB_GROUPSIZE 64

#define TRACEDRENDERING_CLEAR_BLOCKSIZE 8
#define TRACEDRENDERING_LAUNCH_BLOCKSIZE 8
#define TRACEDRENDERING_TRACE_GROUPSIZE 256

static const uint ARGUMENTBUFFER_OFFSET_HIERARCHY = 0;
static const uint ARGUMENTBUFFER_OFFSET_CLUSTERPROCESSOR = ARGUMENTBUFFER_OFFSET_HIERARCHY + (3 * 4);

CBUFFER(TracedRenderingCB, CBSLOT_RENDERER_TRACED)
{
	float2 xTracePixelOffset;
	uint xTraceBounce;
	uint xTraceMeshTriangleCount;
};

CBUFFER(TracedBVHCB, CBSLOT_RENDERER_TRACED)
{
	float4x4 xTraceBVHWorld;
	uint xTraceBVHMaterialOffset;
	uint xTraceBVHMeshTriangleOffset;
	uint xTraceBVHMeshTriangleCount;
	uint xTraceBVHMeshVertexPOSStride;
};

struct xTracedRenderingStoredRay
{
	uint pixelID;
	float3 origin;
	uint3 direction_energy;
	uint primitiveID;
	float2 bary;
};

struct TracedRenderingMeshTriangle
{
	float3 v0, v1, v2;
	float3 n0, n1, n2;
	float2 t0, t1, t2;
	uint materialIndex;
};
struct TracedRenderingAABB
{
	float3 min;
	float3 max;
};

struct ClusterCone
{
	uint valid;
	float3 position;
	float3 direction;
	float angleCos;
};

struct BVHNode
{
	uint ParentIndex;
	uint LeftChildIndex;
	uint RightChildIndex;
};

#endif // _SHADERINTEROP_TRACEDRENDERING_H_
