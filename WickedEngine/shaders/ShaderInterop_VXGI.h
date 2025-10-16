#ifndef WI_SHADERINTEROP_VXGI_H
#define WI_SHADERINTEROP_VXGI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

// If enabled, geometry shader will be used to voxelize, and axis will be selected by geometry shader
//	If disabled, vertex shader with instance replication will be used for each axis
#define VOXELIZATION_GEOMETRY_SHADER_ENABLED

// If enabled, conservative rasterization will be used to voxelize
//	This can more accurately voxelize thin geometry, but slower
//#define VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED

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
	VOXELIZATION_CHANNEL_EMISSIVE_R,
	VOXELIZATION_CHANNEL_EMISSIVE_G,
	VOXELIZATION_CHANNEL_EMISSIVE_B,
	VOXELIZATION_CHANNEL_DIRECTLIGHT_R,
	VOXELIZATION_CHANNEL_DIRECTLIGHT_G,
	VOXELIZATION_CHANNEL_DIRECTLIGHT_B,
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
