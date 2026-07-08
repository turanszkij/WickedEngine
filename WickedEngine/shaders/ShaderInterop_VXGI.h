#ifndef WI_SHADERINTEROP_VXGI_H
#define WI_SHADERINTEROP_VXGI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

// If enabled, geometry shader will be used to voxelize, and axis will be selected by geometry shader
//	If disabled, vertex shader with instance replication will be used for each axis
#if !defined(__APPLE__) && !defined(__metal__)
#define VOXELIZATION_GEOMETRY_SHADER_ENABLED
#endif // !defined(__APPLE__) && !defined(__metal__)

// Conservative rasterization for voxelization. It more accurately voxelizes
// thin geometry (reduces light leaking and missing voxels along edges / thin
// surfaces), but it fattens every triangle and adds pixel-shader work, which is
// expensive on dense, high-overdraw scenes such as terrain (each voxelizer
// fragment also does many atomic writes). For that reason it is OFF by default.
//
// To enable, uncomment the define below. It is kept inside the geometry-shader
// guard because the triangle expansion and the per-voxel AABB clipping that
// make it correct live in the geometry shader; the vertex-shader fallback path
// does not emit the triangle AABB the pixel shader needs. At runtime the
// hardware conservative raster is only turned on if the device supports it,
// otherwise the geometry-shader triangle expansion provides a software
// fallback.
#ifdef VOXELIZATION_GEOMETRY_SHADER_ENABLED
//#define VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
#endif // VOXELIZATION_GEOMETRY_SHADER_ENABLED

// Number of clipmaps, each doubling in size:
static const uint VXGI_CLIPMAP_COUNT = 6;

struct alignas(16) VoxelClipMap
{
	float3 center;		// center of clipmap volume in world space
	float voxelSize;	// half-extent of one voxel
};

struct alignas(16) VXGI
{
	uint	resolution;		// voxel grid resolution
	float	resolution_rcp;	// 1.0 / voxel grid resolution
	float	stepsize;		// raymarch step size in voxel space units
	float	max_distance;	// maximum raymarch distance for voxel GI in world-space

	int		texture_radiance;
	int		texture_sdf;
	int		padding0;
	int		padding1;

	VoxelClipMap clipmaps[VXGI_CLIPMAP_COUNT];

#ifndef __cplusplus
	float3 world_to_clipmap(in float3 P, in VoxelClipMap clipmap)
	{
		float3 diff = (P - clipmap.center) * resolution_rcp / clipmap.voxelSize;
		float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;
		return uvw;
	}
	float3 clipmap_to_world(in float3 uvw, in VoxelClipMap clipmap)
	{
		float3 P = uvw * 2 - 1;
		P.y *= -1;
		P *= clipmap.voxelSize;
		P *= resolution;
		P += clipmap.center;
		return P;
	}
#endif // __cplusplus
};

struct alignas(16) VoxelizerCB
{
	int3 offsetfromPrevFrame;
	int clipmap_index;
};
CONSTANTBUFFER(g_xVoxelizer, VoxelizerCB, CBSLOT_RENDERER_VOXELIZER);

enum VOXELIZATION_CHANNEL
{
	VOXELIZATION_CHANNEL_BASECOLOR_R,
	VOXELIZATION_CHANNEL_BASECOLOR_G,
	VOXELIZATION_CHANNEL_BASECOLOR_B,
	VOXELIZATION_CHANNEL_BASECOLOR_A,
	// Self-emitted radiance per fragment: albedo * directLight / PI + emissive.
	// Precomputed at voxelization time so the temporal pass only has to add the
	// indirect bounce (albedo * indirect). This merges the former separate
	// EMISSIVE and DIRECTLIGHT channels (6 -> 3), cutting voxelization atomic
	// traffic and the render_atomic texture size.
	VOXELIZATION_CHANNEL_SELFRADIANCE_R,
	VOXELIZATION_CHANNEL_SELFRADIANCE_G,
	VOXELIZATION_CHANNEL_SELFRADIANCE_B,
	VOXELIZATION_CHANNEL_NORMAL_R,
	VOXELIZATION_CHANNEL_NORMAL_G,
	VOXELIZATION_CHANNEL_FRAGMENT_COUNTER,

	VOXELIZATION_CHANNEL_COUNT,
};


// Cones from: https://github.com/compix/VoxelConeTracingGI/blob/master/assets/shaders/voxelConeTracing/finalLightingPass.frag

//#define USE_32_CONES
#ifdef USE_32_CONES
// 32 Cones for higher quality (16 on average per hemisphere)
static const int DIFFUSE_CONE_COUNT = 32;
static const float DIFFUSE_CONE_APERTURE = 0.628319f;

static const float3 DIFFUSE_CONE_DIRECTIONS[32] = {
	float3(0.898904f, 0.435512f, 0.0479745f),
	float3(0.898904f, -0.435512f, -0.0479745f),
	float3(0.898904f, 0.0479745f, -0.435512f),
	float3(0.898904f, -0.0479745f, 0.435512f),
	float3(-0.898904f, 0.435512f, -0.0479745f),
	float3(-0.898904f, -0.435512f, 0.0479745f),
	float3(-0.898904f, 0.0479745f, 0.435512f),
	float3(-0.898904f, -0.0479745f, -0.435512f),
	float3(0.0479745f, 0.898904f, 0.435512f),
	float3(-0.0479745f, 0.898904f, -0.435512f),
	float3(-0.435512f, 0.898904f, 0.0479745f),
	float3(0.435512f, 0.898904f, -0.0479745f),
	float3(-0.0479745f, -0.898904f, 0.435512f),
	float3(0.0479745f, -0.898904f, -0.435512f),
	float3(0.435512f, -0.898904f, 0.0479745f),
	float3(-0.435512f, -0.898904f, -0.0479745f),
	float3(0.435512f, 0.0479745f, 0.898904f),
	float3(-0.435512f, -0.0479745f, 0.898904f),
	float3(0.0479745f, -0.435512f, 0.898904f),
	float3(-0.0479745f, 0.435512f, 0.898904f),
	float3(0.435512f, -0.0479745f, -0.898904f),
	float3(-0.435512f, 0.0479745f, -0.898904f),
	float3(0.0479745f, 0.435512f, -0.898904f),
	float3(-0.0479745f, -0.435512f, -0.898904f),
	float3(0.57735f, 0.57735f, 0.57735f),
	float3(0.57735f, 0.57735f, -0.57735f),
	float3(0.57735f, -0.57735f, 0.57735f),
	float3(0.57735f, -0.57735f, -0.57735f),
	float3(-0.57735f, 0.57735f, 0.57735f),
	float3(-0.57735f, 0.57735f, -0.57735f),
	float3(-0.57735f, -0.57735f, 0.57735f),
	float3(-0.57735f, -0.57735f, -0.57735f)
};
#else // 16 cones for lower quality (8 on average per hemisphere)
static const int DIFFUSE_CONE_COUNT = 16;
static const float DIFFUSE_CONE_APERTURE = 0.872665f;

static const float3 DIFFUSE_CONE_DIRECTIONS[16] = {
	float3(0.57735f, 0.57735f, 0.57735f),
	float3(0.57735f, -0.57735f, -0.57735f),
	float3(-0.57735f, 0.57735f, -0.57735f),
	float3(-0.57735f, -0.57735f, 0.57735f),
	float3(-0.903007f, -0.182696f, -0.388844f),
	float3(-0.903007f, 0.182696f, 0.388844f),
	float3(0.903007f, -0.182696f, 0.388844f),
	float3(0.903007f, 0.182696f, -0.388844f),
	float3(-0.388844f, -0.903007f, -0.182696f),
	float3(0.388844f, -0.903007f, 0.182696f),
	float3(0.388844f, 0.903007f, -0.182696f),
	float3(-0.388844f, 0.903007f, 0.182696f),
	float3(-0.182696f, -0.388844f, -0.903007f),
	float3(0.182696f, 0.388844f, -0.903007f),
	float3(-0.182696f, 0.388844f, 0.903007f),
	float3(0.182696f, -0.388844f, 0.903007f)
};
#endif

#endif // WI_SHADERINTEROP_VXGI_H
