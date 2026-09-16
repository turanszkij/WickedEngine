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
static const uint DDGI_MIN_RAYCOUNT = 64; // per-cascade budget floor (bucket-aligned)
static const uint DDGI_COLOR_RESOLUTION = 8; // this should not be modified, border update code is fixed
static const uint DDGI_COLOR_TEXELS = DDGI_COLOR_RESOLUTION; // no border, color is stored in SH
static const uint DDGI_DEPTH_RESOLUTION = 16; // this should not be modified, border update code is fixed
static const uint DDGI_DEPTH_TEXELS = 1 + DDGI_DEPTH_RESOLUTION + 1; // with border
static const float DDGI_KEEP_DISTANCE = 0.1f; // how much distance should probes keep from surfaces
static const uint DDGI_RAY_BUCKET_COUNT = 4; // ray count per bucket

// Number of DDGI probe-grid cascades (fine inner grid + coarser outer grids).
static const uint DDGI_CASCADE_COUNT = 6;

// Per-cascade ray budget cap (rays per probe). Cascade 0 gets the full budget;
// each coarser cascade - which covers a larger, more distant, lower-frequency
// area and refreshes less often - halves it, down to DDGI_MIN_RAYCOUNT. This
// bounds both the per-frame trace cost and the transient ray-buffer capacity
// (which is sized for the two cascades that can be active in one frame). Every
// cap is a power of two >= DDGI_MIN_RAYCOUNT, hence
// DDGI_RAY_BUCKET_COUNT-aligned, so ray compaction stays aligned. Shared by the
// ray allocation pass (per-probe cap) and the host (Get_Ray_Buffer_Capacity).
inline uint ddgi_cascade_ray_budget(uint cascade)
{
	const uint budget = DDGI_MAX_RAYCOUNT >> cascade;
	return budget > DDGI_MIN_RAYCOUNT ? budget : DDGI_MIN_RAYCOUNT;
}

#define DDGI_LINEAR_BLENDING

struct DDGIPushConstants
{
	uint instanceInclusionMask;
	uint frameIndex;
	uint rayCount;
	float blendSpeed;
	// Global probe index of the first probe in the dispatched cascade. The ray
	// allocation / update / update-depth passes are dispatched per active
	// cascade (only cascade 0 + one round-robin coarse cascade run each frame),
	// so a group's probe index is SV_GroupID.x + probeIndexOffset. Zero for
	// passes / dispatches that cover cascade 0 or address probes by other means
	// (raytrace reads the probe index from the ray allocation records).
	uint probeIndexOffset;
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
		// Temporal-blend rate multiplier for this cascade = how many frames
		// apart its updates are (1 for cascade 0, CASCADE_COUNT-1 for the
		// round-robin cascades). The update pass multiplies the colour/depth
		// blend factors by this so a cascade refreshed 1/K as often still
		// converges at the same WALL-CLOCK rate as cascade 0, instead of K
		// times slower (which showed as lagging/darkening and cascade-boundary
		// popping while moving).
		float blend_scale;

		float3 cell_size_rcp;
		float pad_csr;

		int3 scroll_offset;
		int active; // nonzero: this cascade is refreshed (traced/updated) this frame

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
	// Capacity of the transient ray / ray-allocation buffers, in rays. Sized
	// for the probes refreshed in a single frame (cascade 0 + one round-robin
	// coarse cascade at DDGI_MAX_RAYCOUNT each), NOT for every probe of every
	// cascade - that is what keeps ray memory independent of
	// DDGI_CASCADE_COUNT. The ray allocation pass clamps to this so the buffers
	// can never overflow.
	uint ray_buffer_capacity;
	uint2 pad;

	Cascade cascades[DDGI_CASCADE_COUNT];
};

#endif // WI_SHADERINTEROP_DDGI_H
