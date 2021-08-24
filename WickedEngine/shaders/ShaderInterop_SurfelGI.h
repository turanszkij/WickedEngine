#ifndef WI_SHADERINTEROP_SURFEL_GI_H
#define WI_SHADERINTEROP_SURFEL_GI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

struct Surfel
{
	float3 position;
	uint normal;
	float3 color;
	float radius;
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
static const uint SURFEL_TABLE_SIZE = SURFEL_GRID_DIMENSIONS.x * SURFEL_GRID_DIMENSIONS.y * SURFEL_GRID_DIMENSIONS.z;
static const float SURFEL_MAX_RADIUS = 1;
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
static const uint SURFEL_CELL_LIMIT = ~0; // limit the amount of allocated surfels in a cell

#ifndef __cplusplus
#define SURFEL_USE_HASHING // if defined, hashing will be used to retrieve surfels, otherwise they will be taken from a finite grid
inline int3 surfel_cell(float3 position)
{
#ifdef SURFEL_USE_HASHING
	return position / SURFEL_MAX_RADIUS;
#else
	return (position - g_xCamera_CamPos) / SURFEL_MAX_RADIUS + SURFEL_GRID_DIMENSIONS / 2;
#endif // SURFEL_USE_HASHING
}
inline uint surfel_cellindex(int3 cell)
{
#ifdef SURFEL_USE_HASHING
	const uint p1 = 73856093;   // some large primes 
	const uint p2 = 19349663;
	const uint p3 = 83492791;
	int n = p1 * cell.x ^ p2 * cell.y ^ p3 * cell.z;
	n %= SURFEL_TABLE_SIZE;
	return n;
#else
	return flatten3D(cell, SURFEL_GRID_DIMENSIONS);
#endif // SURFEL_USE_HASHING
}
// 27 neighbor offsets in a 3D grid, including center cell:
static const int3 surfel_neighbor_offsets[27] = {
	int3(-1, -1, -1),
	int3(-1, -1, 0),
	int3(-1, -1, 1),
	int3(-1, 0, -1),
	int3(-1, 0, 0),
	int3(-1, 0, 1),
	int3(-1, 1, -1),
	int3(-1, 1, 0),
	int3(-1, 1, 1),
	int3(0, -1, -1),
	int3(0, -1, 0),
	int3(0, -1, 1),
	int3(0, 0, -1),
	int3(0, 0, 0),
	int3(0, 0, 1),
	int3(0, 1, -1),
	int3(0, 1, 0),
	int3(0, 1, 1),
	int3(1, -1, -1),
	int3(1, -1, 0),
	int3(1, -1, 1),
	int3(1, 0, -1),
	int3(1, 0, 0),
	int3(1, 0, 1),
	int3(1, 1, -1),
	int3(1, 1, 0),
	int3(1, 1, 1),
};
#endif // __cplusplus

#endif // WI_SHADERINTEROP_SURFEL_GI_H
