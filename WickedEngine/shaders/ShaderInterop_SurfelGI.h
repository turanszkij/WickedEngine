#ifndef WI_SHADERINTEROP_SURFEL_GI_H
#define WI_SHADERINTEROP_SURFEL_GI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

struct Surfel
{
	float3 position;
	uint normal;
	float4 color;
};
struct SurfelData
{
	uint2 primitiveID;
	uint bary;
	uint uid;

	float3 mean;
	float life;

	float3 shortMean;
	float vbbr;

	float3 variance;
	float inconsistency;
};
static const uint SURFEL_CAPACITY = 250000;
static const uint3 SURFEL_GRID_DIMENSIONS = uint3(128, 64, 128);
struct SurfelGridCell
{
	uint count;
	uint offset;
};
static const uint SURFEL_STATS_OFFSET_COUNT = 0;
static const uint SURFEL_STATS_OFFSET_INDIRECT = 4;
static const uint SURFEL_INDIRECT_NUMTHREADS = 32;
static const uint SURFEL_TARGET_COVERAGE = 1; // how many surfels should affect a pixel fully, higher values will increase quality and cost
static const float SURFEL_NORMAL_TOLERANCE = 1; // default: 1, higher values will put more surfels on edges, to have more detail but increases cost

#ifndef __cplusplus
inline float GetSurfelRadius()
{
	return 1;

	//float3 cellsize = g_xFrame_WorldBoundsExtents / SURFEL_GRID_DIMENSIONS;
	//return min(cellsize.x, min(cellsize.y, cellsize.z));
}
inline int3 surfel_gridpos(float3 position)
{
	//position = position * g_xFrame_WorldBoundsExtents_rcp;
	//position = position * 0.5 + 0.5;
	//position = position * SURFEL_GRID_DIMENSIONS;
	//return position;

	return (position - g_xCamera_CamPos) / GetSurfelRadius() + SURFEL_GRID_DIMENSIONS / 2;
}
inline uint surfel_cellindex(int3 gridpos)
{
	return flatten3D(gridpos, SURFEL_GRID_DIMENSIONS);
}
#endif // __cplusplus

#endif // WI_SHADERINTEROP_SURFEL_GI_H
