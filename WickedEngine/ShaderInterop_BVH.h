#ifndef _SHADERINTEROP_BVH_H_
#define _SHADERINTEROP_BVH_H_
#include "ShaderInterop.h"

#define BVH_CLASSIFICATION_GROUPSIZE 64
#define BVH_CLUSTERPROCESSOR_GROUPSIZE 64
#define BVH_HIERARCHY_GROUPSIZE 64

static const uint ARGUMENTBUFFER_OFFSET_HIERARCHY = 0;
static const uint ARGUMENTBUFFER_OFFSET_CLUSTERPROCESSOR = ARGUMENTBUFFER_OFFSET_HIERARCHY + (3 * 4);

CBUFFER(BVHCB, CBSLOT_RENDERER_BVH)
{
	float4x4 xTraceBVHWorld;
	float4 xTraceBVHInstanceColor;
	uint xTraceBVHMaterialOffset;
	uint xTraceBVHMeshTriangleOffset;
	uint xTraceBVHMeshTriangleCount;
	uint xTraceBVHMeshVertexPOSStride;
};

struct BVHMeshTriangle
{
	float3 v0, v1, v2;	// positions
	float3 n0, n1, n2;	// normals
	float4 u0, u1, u2;	// uv sets
	float4 c0, c1, c2;	// vertex colors
	uint2 tangent;
	uint materialIndex;
};
struct BVHAABB
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

#endif // _SHADERINTEROP_BVH_H_
