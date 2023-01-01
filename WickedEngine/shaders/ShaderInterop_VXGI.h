#ifndef WI_SHADERINTEROP_VXGI_H
#define WI_SHADERINTEROP_VXGI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

// If enabled, geometry shader will be used to voxelize, and axis will be selected by geometry shader
//	If disabled, vertex shader with instance replication will be used for each axis
#define VOXELIZATION_GEOMETRY_SHADER_ENABLED

static const uint VOXEL_GI_CLIPMAP_COUNT = 6;

struct VoxelClipMap
{
	float3 center;		// center of clipmap volume in world space
	float voxelSize;	// half-extent of one voxel
};

struct VXGI
{
	uint		resolution;		// voxel grid resolution
	float		resolution_rcp;	// 1.0 / voxel grid resolution
	float		stepsize;		// raymarch step size in voxel space units
	float		max_distance;	// maximum raymarch distance for voxel GI in world-space

	VoxelClipMap clipmaps[VOXEL_GI_CLIPMAP_COUNT];

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

struct VoxelizerCB
{
	int3 offsetfromPrevFrame;
	int clipmap_index;
};
CONSTANTBUFFER(g_xVoxelizer, VoxelizerCB, CBSLOT_RENDERER_VOXELIZER);


// Cones from: https://github.com/compix/VoxelConeTracingGI/blob/master/assets/shaders/voxelConeTracing/finalLightingPass.frag

//#define USE_32_CONES
#ifdef USE_32_CONES
// 32 Cones for higher quality (16 on average per hemisphere)
static const int DIFFUSE_CONE_COUNT = 32;
static const float DIFFUSE_CONE_APERTURE = 0.628319;

static const float3 DIFFUSE_CONE_DIRECTIONS[32] = {
	float3(0.898904, 0.435512, 0.0479745),
	float3(0.898904, -0.435512, -0.0479745),
	float3(0.898904, 0.0479745, -0.435512),
	float3(0.898904, -0.0479745, 0.435512),
	float3(-0.898904, 0.435512, -0.0479745),
	float3(-0.898904, -0.435512, 0.0479745),
	float3(-0.898904, 0.0479745, 0.435512),
	float3(-0.898904, -0.0479745, -0.435512),
	float3(0.0479745, 0.898904, 0.435512),
	float3(-0.0479745, 0.898904, -0.435512),
	float3(-0.435512, 0.898904, 0.0479745),
	float3(0.435512, 0.898904, -0.0479745),
	float3(-0.0479745, -0.898904, 0.435512),
	float3(0.0479745, -0.898904, -0.435512),
	float3(0.435512, -0.898904, 0.0479745),
	float3(-0.435512, -0.898904, -0.0479745),
	float3(0.435512, 0.0479745, 0.898904),
	float3(-0.435512, -0.0479745, 0.898904),
	float3(0.0479745, -0.435512, 0.898904),
	float3(-0.0479745, 0.435512, 0.898904),
	float3(0.435512, -0.0479745, -0.898904),
	float3(-0.435512, 0.0479745, -0.898904),
	float3(0.0479745, 0.435512, -0.898904),
	float3(-0.0479745, -0.435512, -0.898904),
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, 0.57735, -0.57735),
	float3(0.57735, -0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, 0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.57735, -0.57735, -0.57735)
};
#else // 16 cones for lower quality (8 on average per hemisphere)
static const int DIFFUSE_CONE_COUNT = 16;
static const float DIFFUSE_CONE_APERTURE = 0.872665;

static const float3 DIFFUSE_CONE_DIRECTIONS[16] = {
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.903007, -0.182696, -0.388844),
	float3(-0.903007, 0.182696, 0.388844),
	float3(0.903007, -0.182696, 0.388844),
	float3(0.903007, 0.182696, -0.388844),
	float3(-0.388844, -0.903007, -0.182696),
	float3(0.388844, -0.903007, 0.182696),
	float3(0.388844, 0.903007, -0.182696),
	float3(-0.388844, 0.903007, 0.182696),
	float3(-0.182696, -0.388844, -0.903007),
	float3(0.182696, 0.388844, -0.903007),
	float3(-0.182696, 0.388844, 0.903007),
	float3(0.182696, -0.388844, 0.903007)
};
#endif

#endif // WI_SHADERINTEROP_VXGI_H
