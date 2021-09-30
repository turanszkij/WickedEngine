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
	uint life_recycle; // 16bit life frames, 16bit recycle frames

	float3 shortMean;
	float vbbr;

	float3 variance;
	float inconsistency;

	uint GetLife() { return life_recycle & 0xFFFF; }
	uint GetRecycle() { return (life_recycle >> 16u) & 0xFFFF; }
};
static const uint SURFEL_CAPACITY = 100000;
static const uint SQRT_SURFEL_CAPACITY = (uint)ceil(sqrt((float)SURFEL_CAPACITY));
static const uint SURFEL_MOMENT_TEXELS = 4 + 2;
static const uint SURFEL_MOMENT_ATLAS_TEXELS = SQRT_SURFEL_CAPACITY * SURFEL_MOMENT_TEXELS;
static const uint3 SURFEL_GRID_DIMENSIONS = uint3(128, 64, 128);
static const uint SURFEL_TABLE_SIZE = SURFEL_GRID_DIMENSIONS.x * SURFEL_GRID_DIMENSIONS.y * SURFEL_GRID_DIMENSIONS.z;
static const float SURFEL_MAX_RADIUS = 1;
static const float SURFEL_RECYCLE_DISTANCE = 10; // if surfel is behind camera and farther than this distance, it starts preparing for recycling
static const uint SURFEL_RECYCLE_TIME = 240; // if surfel is preparing for recycling, this is how many frames it takes to recycle it
struct SurfelGridCell
{
	uint count;
	uint offset;
};
static const uint SURFEL_STATS_OFFSET_COUNT = 0;
static const uint SURFEL_STATS_OFFSET_NEXTCOUNT = SURFEL_STATS_OFFSET_COUNT + 4;
static const uint SURFEL_STATS_OFFSET_DEADCOUNT = SURFEL_STATS_OFFSET_NEXTCOUNT + 4;
static const uint SURFEL_STATS_OFFSET_CELLALLOCATOR = SURFEL_STATS_OFFSET_DEADCOUNT + 4;
static const uint SURFEL_STATS_OFFSET_INDIRECT = SURFEL_STATS_OFFSET_CELLALLOCATOR + 4;
static const uint SURFEL_STATS_OFFSET_RAYCOUNT = SURFEL_STATS_OFFSET_INDIRECT + 4 * 3;
static const uint SURFEL_INDIRECT_NUMTHREADS = 32;
static const float SURFEL_TARGET_COVERAGE = 0.5f; // how many surfels should affect a pixel fully, higher values will increase quality and cost
static const uint SURFEL_CELL_LIMIT = ~0; // limit the amount of allocated surfels in a cell
static const uint SURFEL_RAY_BUDGET = 200000; // max number of rays per frame
static const uint SURFEL_RAY_BOOST_MAX = 32; // max amount of rays per surfel
#define SURFEL_COVERAGE_HALFRES // runs the coverage shader in half resolution for improved performance
#define SURFEL_GRID_CULLING // if defined, surfels will not be added to grid cells that they do not intersect
#define SURFEL_USE_HASHING // if defined, hashing will be used to retrieve surfels, hashing is good because it supports infinite world trivially, but slower due to hash collisions
#define SURFEL_ENABLE_INFINITE_BOUNCES // if defined, previous frame's surfel data will be sampled at ray tracing hit points
#define SURFEL_ENABLE_IRRADIANCE_SHARING // if defined, surfels will pull color from nearby surfels, this can smooth out the GI a bit

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

uint surfel_raycount(SurfelData surfel_data)
{
	uint rayCount = saturate(surfel_data.inconsistency) * SURFEL_RAY_BOOST_MAX;
	if (surfel_data.GetRecycle() > 60)
	{
		rayCount = 0;
	}
	return rayCount;
}

void MultiscaleMeanEstimator(
	float3 y,
	inout SurfelData data,
	float shortWindowBlend = 0.08f
)
{
	float3 mean = data.mean;
	float3 shortMean = data.shortMean;
	float vbbr = data.vbbr;
	float3 variance = data.variance;
	float inconsistency = data.inconsistency;

	// Suppress fireflies.
	{
		float3 dev = sqrt(max(1e-5, variance));
		float3 highThreshold = 0.1 + shortMean + dev * 8;
		float3 overflow = max(0, y - highThreshold);
		y -= overflow;
	}

	float3 delta = y - shortMean;
	shortMean = lerp(shortMean, y, shortWindowBlend);
	float3 delta2 = y - shortMean;

	// This should be a longer window than shortWindowBlend to avoid bias
	// from the variance getting smaller when the short-term mean does.
	float varianceBlend = shortWindowBlend * 0.5;
	variance = lerp(variance, delta * delta2, varianceBlend);
	float3 dev = sqrt(max(1e-5, variance));

	float3 shortDiff = mean - shortMean;

	float relativeDiff = dot(float3(0.299, 0.587, 0.114),
		abs(shortDiff) / max(1e-5, dev));
	inconsistency = lerp(inconsistency, relativeDiff, 0.08);

	float varianceBasedBlendReduction =
		clamp(dot(float3(0.299, 0.587, 0.114),
			0.5 * shortMean / max(1e-5, dev)), 1.0 / 32, 1);

	float3 catchUpBlend = clamp(smoothstep(0, 1,
		relativeDiff * max(0.02, inconsistency - 0.2)), 1.0 / 256, 1);
	catchUpBlend *= vbbr;

	vbbr = lerp(vbbr, varianceBasedBlendReduction, 0.1);
	mean = lerp(mean, y, saturate(catchUpBlend));

	// Output
	data.mean = mean;
	data.shortMean = shortMean;
	data.vbbr = vbbr;
	data.variance = variance;
	data.inconsistency = inconsistency;
}
#endif // __cplusplus

#endif // WI_SHADERINTEROP_SURFEL_GI_H
