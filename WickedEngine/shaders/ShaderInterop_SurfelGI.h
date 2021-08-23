#ifndef WI_SHADERINTEROP_SURFEL_GI_H
#define WI_SHADERINTEROP_SURFEL_GI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

struct Surfel
{
	float3 position;
	uint normal;
};
struct SurfelPayload
{
	uint2 color; // packed as half4
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
static const uint SURFEL_TABLE_SIZE = 500000;
static const uint SURFEL_STATS_OFFSET_COUNT = 0;
static const uint SURFEL_STATS_OFFSET_INDIRECT = 4;
static const uint SURFEL_INDIRECT_NUMTHREADS = 32;
static const float SURFEL_RADIUS = 2;
static const float SURFEL_RADIUS2 = SURFEL_RADIUS * SURFEL_RADIUS;
inline int3 surfel_cell(float3 position)
{
	return int3(int(position.x / SURFEL_RADIUS), int(position.y / SURFEL_RADIUS), int(position.z / SURFEL_RADIUS));
}
inline uint surfel_hash(int3 cell)
{
	const uint p1 = 73856093;   // some large primes 
	const uint p2 = 19349663;
	const uint p3 = 83492791;
	int n = p1 * cell.x ^ p2 * cell.y ^ p3 * cell.z;
	n %= SURFEL_TABLE_SIZE;
	return n;
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
static const uint SURFEL_TARGET_COVERAGE = 1; // how many surfels should affect a pixel fully, higher values will increase quality and cost
static const float SURFEL_NORMAL_TOLERANCE = 1; // default: 1, higher values will put more surfels on edges, to have more detail but increases cost


#endif // WI_SHADERINTEROP_SURFEL_GI_H
