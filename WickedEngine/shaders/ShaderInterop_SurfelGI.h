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
	uint life;

	float3 shortMean;
	float vbbr;

	float3 variance;
	float inconsistency;

	float3 hitpos;
	uint hitnormal;

	float3 hitenergy;
	float padding0;

	float3 traceresult;
	float padding1;
};
static const uint SURFEL_CAPACITY = 250000;
static const uint SQRT_SURFEL_CAPACITY = (uint)ceil(sqrt((float)SURFEL_CAPACITY));
static const uint SURFEL_MOMENT_TEXELS = 4 + 2;
static const uint SURFEL_MOMENT_ATLAS_TEXELS = SQRT_SURFEL_CAPACITY * SURFEL_MOMENT_TEXELS;
static const uint3 SURFEL_GRID_DIMENSIONS = uint3(128, 64, 128);
static const uint SURFEL_TABLE_SIZE = SURFEL_GRID_DIMENSIONS.x * SURFEL_GRID_DIMENSIONS.y * SURFEL_GRID_DIMENSIONS.z;
static const float SURFEL_MAX_RADIUS = 1;
struct SurfelGridCell
{
	uint count;
	uint offset;
};
static const uint SURFEL_STATS_OFFSET_COUNT = 0;
static const uint SURFEL_STATS_OFFSET_CELLALLOCATOR = 4;
static const uint SURFEL_STATS_OFFSET_INDIRECT = 8;
static const uint SURFEL_INDIRECT_NUMTHREADS = 32;
static const float SURFEL_TARGET_COVERAGE = 0.5; // how many surfels should affect a pixel fully, higher values will increase quality and cost
static const uint SURFEL_CELL_LIMIT = ~0; // limit the amount of allocated surfels in a cell
#define SURFEL_COVERAGE_HALFRES // runs the coverage shader in half resolution for improved performance
#define SURFEL_GRID_CULLING // if defined, surfels will not be added to grid cells that they do not intersect
//#define SURFEL_USE_HASHING // if defined, hashing will be used to retrieve surfels, hashing is good because it supports infinite world trivially, but slower due to hash collisions

#ifndef __cplusplus
inline int3 surfel_cell(float3 position)
{
#ifdef SURFEL_USE_HASHING
	return floor(position / SURFEL_MAX_RADIUS);
#else
	return floor((position - floor(g_xCamera.CamPos)) / SURFEL_MAX_RADIUS) + SURFEL_GRID_DIMENSIONS / 2;
#endif // SURFEL_USE_HASHING
}
float3 surfel_griduv(float3 position)
{
#ifdef SURFEL_USE_HASHING
	return 0; // hashed grid can't be sampled for colors, it doesn't make sense
#else
	return (((position - floor(g_xCamera.CamPos)) / SURFEL_MAX_RADIUS) + SURFEL_GRID_DIMENSIONS / 2) / SURFEL_GRID_DIMENSIONS;
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
inline bool surfel_cellvalid(int3 cell)
{
#ifdef SURFEL_USE_HASHING
	return true;
#else
	if (cell.x < 0 || cell.x >= SURFEL_GRID_DIMENSIONS.x)
		return false;
	if (cell.y < 0 || cell.y >= SURFEL_GRID_DIMENSIONS.y)
		return false;
	if (cell.z < 0 || cell.z >= SURFEL_GRID_DIMENSIONS.z)
		return false;
	return true;
#endif // SURFEL_USE_HASHING
}
inline bool surfel_cellintersects(Surfel surfel, int3 cell)
{
	if (!surfel_cellvalid(cell))
		return false;

#ifdef SURFEL_GRID_CULLING
#ifdef SURFEL_USE_HASHING
	float3 gridmin = cell * SURFEL_MAX_RADIUS;
	float3 gridmax = (cell + 1) * SURFEL_MAX_RADIUS;
#else
	float3 gridmin = cell - SURFEL_GRID_DIMENSIONS / 2 * SURFEL_MAX_RADIUS + floor(g_xCamera.CamPos);
	float3 gridmax = (cell + 1) - SURFEL_GRID_DIMENSIONS / 2 * SURFEL_MAX_RADIUS + floor(g_xCamera.CamPos);
#endif // SURFEL_USE_HASHING

	float3 closestPointInAabb = min(max(surfel.position, gridmin), gridmax);
	float dist = distance(closestPointInAabb, surfel.position);
	if (dist < surfel.radius)
		return true;
	return false;
#else
	return true;
#endif // SURFEL_GRID_CULLING
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

inline float2 encode(in float3 N)
{
	return float2(atan2(N.y, N.x) / PI, N.z) * 0.5 + 0.5;
}
float2 surfel_moment_pixel(uint surfel_index, float3 normal, float3 direction)
{
	uint2 moments_pixel = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_TEXELS;
	float3 hemi = mul(direction, transpose(GetTangentSpace(normal)));
	hemi.z = abs(hemi.z);
	hemi = normalize(hemi);
	float2 moments_uv = encode_hemioct(hemi) * 0.5 + 0.5;
	//float2 moments_uv = hemi.xy * 0.5 + 0.5;
	return moments_pixel + 1 + moments_uv * (SURFEL_MOMENT_TEXELS - 2);
}
float2 surfel_moment_uv(uint surfel_index, float3 normal, float3 direction)
{
	return surfel_moment_pixel(surfel_index, normal, direction) / SURFEL_MOMENT_ATLAS_TEXELS;
}
float surfel_moment_weight(float2 moments, float dist)
{
	float mean = moments.x;
	float mean2 = moments.y;
	if (dist > mean)
	{
		// Chebishev weight
		float variance = abs(sqr(mean) - mean2);
		return variance / (variance + sqr(dist - mean));
	}
	return 1;
}
#endif // __cplusplus

#endif // WI_SHADERINTEROP_SURFEL_GI_H
