#ifndef WI_SHADERINTEROP_DDGI_H
#define WI_SHADERINTEROP_DDGI_H
// Shared (CPU/GPU) DDGI data: constants, the ShaderDDGI constant-buffer block,
// push constants and the packed ray/variance records. This is a leaf header
// (like ShaderInterop_Terrain.h / ShaderInterop_VoxelGrid.h) so it can be
// included by ShaderInterop_Renderer.h. The GetScene()-based helper functions
// live in ddgiHF.hlsli instead; DDGIProbe lives in ShaderInterop_Renderer.h
// next to the SH namespace it depends on.
#include "ShaderInterop.h"

static const uint DDGI_MAX_RAYCOUNT = 512; // affects global ray buffer size
static const uint DDGI_COLOR_RESOLUTION = 8; // this should not be modified, border update code is fixed
static const uint DDGI_COLOR_TEXELS = DDGI_COLOR_RESOLUTION; // no border, color is stored in SH
static const uint DDGI_DEPTH_RESOLUTION = 16; // this should not be modified, border update code is fixed
static const uint DDGI_DEPTH_TEXELS = 1 + DDGI_DEPTH_RESOLUTION + 1; // with border
static const float DDGI_KEEP_DISTANCE = 0.1f; // how much distance should probes keep from surfaces
static const uint DDGI_RAY_BUCKET_COUNT = 4; // ray count per bucket

// Number of DDGI probe-grid cascades (fine inner grid + coarser outer grids).
static const uint DDGI_CASCADE_COUNT = 2;

#define DDGI_LINEAR_BLENDING

struct DDGIPushConstants
{
	uint instanceInclusionMask;
	uint frameIndex;
	uint rayCount;
	float blendSpeed;
};

#ifndef __cplusplus
struct DDGIRayData
{
	half3 direction;
	half depth;
	half3 radiance;
};
struct DDGIVarianceData
{
	half3 mean;
	half3 shortMean;
	half vbbr;
	half3 variance;
	half inconsistency;
};
#endif // __cplusplus

struct DDGIRayDataPacked
{
	uint4 data;

#ifndef __cplusplus
	inline void store(DDGIRayData rayData)
	{
		data.xy = pack_half4(half4(rayData.direction, rayData.depth));
		data.zw = pack_half3(rayData.radiance);
	}
	inline DDGIRayData load()
	{
		DDGIRayData rayData;
		half4 unpk = unpack_half4(data.xy);
		rayData.direction = unpk.xyz;
		rayData.depth = unpk.w;
		rayData.radiance = unpack_half3(data.zw);
		return rayData;
	}
#endif // __cplusplus
};

struct DDGIVarianceDataPacked
{
	uint4 data;

#ifndef __cplusplus
	inline void store(DDGIVarianceData varianceData)
	{
		data.x = Pack_R11G11B10_FLOAT(varianceData.mean);
		data.y = Pack_R11G11B10_FLOAT(varianceData.shortMean);
		data.z = Pack_R11G11B10_FLOAT(varianceData.variance);
		data.w = pack_half2(float2(varianceData.vbbr, varianceData.inconsistency));
	}
	inline DDGIVarianceData load()
	{
		DDGIVarianceData varianceData;
		varianceData.mean = Unpack_R11G11B10_FLOAT(data.x);
		varianceData.shortMean = Unpack_R11G11B10_FLOAT(data.y);
		varianceData.variance = Unpack_R11G11B10_FLOAT(data.z);
		half2 other = unpack_half2(data.w);
		varianceData.vbbr = other.x;
		varianceData.inconsistency = other.y;
		return varianceData;
	}
#endif // __cplusplus
};

// The DDGI global constant-buffer block, held by ShaderScene as `ddgi`. One per
// scene. Groups the shared resources/parameters plus a per-cascade array.
//
// A cascade is one camera-centered, toroidally-scrolling probe grid. Cascade 0
// is the fine inner grid; higher cascades are coarser and cover a larger area.
// All cascades share the probe/variance/ray buffers (probes concatenated by
// probe_offset) and one depth atlas (each cascade occupies a distinct region at
// depth_atlas_offset).
struct alignas(16) ShaderDDGI
{
	struct alignas(16) Cascade
	{
		uint3 grid_dimensions;
		uint probe_count;

		float3 grid_min;
		uint probe_offset; // start index of this cascade in the shared buffers

		float3 grid_extents;
		float max_distance;

		float3 grid_extents_rcp;
		int reset; // nonzero: reset every probe in this cascade this frame

		float3 cell_size;
		float pad_cs;

		float3 cell_size_rcp;
		float pad_csr;

		int3 scroll_offset;
		int pad_so;

		int3 scroll_delta;
		int pad_sd;

		uint2 depth_atlas_offset; // pixel offset of this cascade in the depth atlas
		uint2 pad_da;
	};

	// Shared across cascades:
	int probe_buffer;
	int depth_texture;
	float smooth_backface;
	uint total_probe_count;

	uint2 depth_texture_resolution;
	float2 depth_texture_resolution_rcp;

	uint cascade_count;
	uint3 pad;

	Cascade cascades[DDGI_CASCADE_COUNT];
};

#endif // WI_SHADERINTEROP_DDGI_H
