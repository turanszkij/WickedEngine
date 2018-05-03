#ifndef _SHADER_GLOBALS_
#define _SHADER_GLOBALS_
#include "ShaderInterop.h"

TEXTURE2D(texture_depth, float, TEXSLOT_DEPTH)
TEXTURE2D(texture_lineardepth, float, TEXSLOT_LINEARDEPTH)
TEXTURE2D(texture_gbuffer0, float4, TEXSLOT_GBUFFER0)
TEXTURE2D(texture_gbuffer1, float4, TEXSLOT_GBUFFER1)
TEXTURE2D(texture_gbuffer2, float4, TEXSLOT_GBUFFER2)
TEXTURE2D(texture_gbuffer3, float4, TEXSLOT_GBUFFER3)
TEXTURE2D(texture_gbuffer4, float4, TEXSLOT_GBUFFER4)
TEXTURECUBEARRAY(texture_envmaparray, float4, TEXSLOT_ENVMAPARRAY)
TEXTURE2D(texture_decalatlas, float4, TEXSLOT_DECALATLAS)
TEXTURE2DARRAY(texture_shadowarray_2d, float, TEXSLOT_SHADOWARRAY_2D)
TEXTURECUBEARRAY(texture_shadowarray_cube, float, TEXSLOT_SHADOWARRAY_CUBE)
TEXTURE2DARRAY(texture_shadowarray_transparent, float4, TEXSLOT_SHADOWARRAY_TRANSPARENT)
TEXTURE3D(texture_voxelradiance, float4, TEXSLOT_VOXELRADIANCE)

STRUCTUREDBUFFER(EntityIndexList, uint, SBSLOT_ENTITYINDEXLIST);
STRUCTUREDBUFFER(EntityArray, ShaderEntityType, SBSLOT_ENTITYARRAY);
STRUCTUREDBUFFER(MatrixArray, float4x4, SBSLOT_MATRIXARRAY);

TEXTURE2D(texture_0, float4, TEXSLOT_ONDEMAND0)
TEXTURE2D(texture_1, float4, TEXSLOT_ONDEMAND1)
TEXTURE2D(texture_2, float4, TEXSLOT_ONDEMAND2)
TEXTURE2D(texture_3, float4, TEXSLOT_ONDEMAND3)
TEXTURE2D(texture_4, float4, TEXSLOT_ONDEMAND4)
TEXTURE2D(texture_5, float4, TEXSLOT_ONDEMAND5)
TEXTURE2D(texture_6, float4, TEXSLOT_ONDEMAND6)
TEXTURE2D(texture_7, float4, TEXSLOT_ONDEMAND7)
TEXTURE2D(texture_8, float4, TEXSLOT_ONDEMAND8)
TEXTURE2D(texture_9, float4, TEXSLOT_ONDEMAND9)


SAMPLERSTATE(			sampler_linear_clamp,	SSLOT_LINEAR_CLAMP	)
SAMPLERSTATE(			sampler_linear_wrap,	SSLOT_LINEAR_WRAP	)
SAMPLERSTATE(			sampler_linear_mirror,	SSLOT_LINEAR_MIRROR	)
SAMPLERSTATE(			sampler_point_clamp,	SSLOT_POINT_CLAMP	)
SAMPLERSTATE(			sampler_point_wrap,		SSLOT_POINT_WRAP	)
SAMPLERSTATE(			sampler_point_mirror,	SSLOT_POINT_MIRROR	)
SAMPLERSTATE(			sampler_aniso_clamp,	SSLOT_ANISO_CLAMP	)
SAMPLERSTATE(			sampler_aniso_wrap,		SSLOT_ANISO_WRAP	)
SAMPLERSTATE(			sampler_aniso_mirror,	SSLOT_ANISO_MIRROR	)
SAMPLERCOMPARISONSTATE(	sampler_cmp_depth,		SSLOT_CMP_DEPTH		)
SAMPLERSTATE(			sampler_objectshader,	SSLOT_OBJECTSHADER	)

CBUFFER(WorldCB, CBSLOT_RENDERER_WORLD)
{
	float2		g_xWorld_ScreenWidthHeight;
	float2		g_xWorld_ScreenWidthHeight_Inverse;
	float2		g_xWorld_InternalResolution;
	float2		g_xWorld_InternalResolution_Inverse;
	float		g_xWorld_Gamma;
	float3		g_xWorld_Horizon;
	float3		g_xWorld_Zenith;
	float		g_xWorld_CloudScale;
	float3		g_xWorld_Ambient;
	float		g_xWorld_Cloudiness;
	float3		g_xWorld_Fog;					// Fog Start,End,Height
	float		g_xWorld_SpecularAA;
	float		g_xWorld_VoxelRadianceDataSize;				// voxel half-extent in world space units
	float		g_xWorld_VoxelRadianceDataSize_Inverse;		// 1.0 / voxel-half extent
	uint		g_xWorld_VoxelRadianceDataRes;				// voxel grid resolution
	float		g_xWorld_VoxelRadianceDataRes_Inverse;		// 1.0 / voxel grid resolution
	uint		g_xWorld_VoxelRadianceDataMIPs;				// voxel grid mipmap count
	uint		g_xWorld_VoxelRadianceNumCones;				// number of diffuse cones to trace
	float		g_xWorld_VoxelRadianceNumCones_Inverse;		// 1.0 / number of diffuse cones to trace
	float		g_xWorld_VoxelRadianceRayStepSize;			// raymarch step size in voxel space units
	bool		g_xWorld_VoxelRadianceReflectionsEnabled;	// are voxel gi reflections enabled or not
	float3		g_xWorld_VoxelRadianceDataCenter;			// center of the voxel grid in world space units
	bool		g_xWorld_AdvancedRefractions;
	uint3		g_xWorld_EntityCullingTileCount;
	uint		g_xWorld_TransparentShadowsEnabled;
	int			g_xWorld_GlobalEnvProbeIndex;
	uint		g_xWorld_EnvProbeMipCount;
	float		g_xWorld_EnvProbeMipCount_Inverse;
};
CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
{
	float		g_xFrame_Time;
	float		g_xFrame_TimePrev;
	float		g_xFrame_DeltaTime;
	uint		g_xFrame_FrameCount;
	uint		g_xFrame_LightArrayOffset;			// indexing into entity array
	uint		g_xFrame_LightArrayCount;			// indexing into entity array
	uint		g_xFrame_DecalArrayOffset;			// indexing into entity array
	uint		g_xFrame_DecalArrayCount;			// indexing into entity array
	uint		g_xFrame_ForceFieldArrayOffset;		// indexing into entity array
	uint		g_xFrame_ForceFieldArrayCount;		// indexing into entity array
	uint		g_xFrame_EnvProbeArrayOffset;		// indexing into entity array
	uint		g_xFrame_EnvProbeArrayCount;		// indexing into entity array
	float3		g_xFrame_WindDirection;
	float		g_xFrame_WindWaveSize;
	float		g_xFrame_WindRandomness;
	int			g_xFrame_SunEntityArrayIndex;
	bool		g_xFrame_VoxelRadianceRetargetted;
	uint		g_xFrame_TemporalAASampleRotation;
	float2		g_xFrame_TemporalAAJitter;
	float2		g_xFrame_TemporalAAJitterPrev;
	// The following are per frame properties for the main camera:
	float4x4	g_xFrame_MainCamera_VP;			// View*Projection
	float4x4	g_xFrame_MainCamera_View;
	float4x4	g_xFrame_MainCamera_Proj;
	float3		g_xFrame_MainCamera_CamPos;
	float		g_xFrame_MainCamera_DistanceFromOrigin;
	float4x4	g_xFrame_MainCamera_PrevV;
	float4x4	g_xFrame_MainCamera_PrevP;
	float4x4	g_xFrame_MainCamera_PrevVP;		// PrevView*PrevProjection
	float4x4	g_xFrame_MainCamera_PrevInvVP;	// Inverse(PrevView*PrevProjection)
	float4x4	g_xFrame_MainCamera_ReflVP;		// ReflectionView*ReflectionProjection
	float4x4	g_xFrame_MainCamera_InvV;		// Inverse View
	float4x4	g_xFrame_MainCamera_InvP;		// Inverse Projection
	float4x4	g_xFrame_MainCamera_InvVP;		// Inverse View-Projection
	float3		g_xFrame_MainCamera_At;
	float		g_xFrame_MainCamera_ZNearP;
	float3		g_xFrame_MainCamera_Up;
	float		g_xFrame_MainCamera_ZFarP;
	float4		g_xFrame_FrustumPlanesWS[6];	// Frustum planes in world space in order: left,right,top,bottom,near,far
};
// The following buffer contains properties for a temporary camera (eg. main camera, reflection camera, shadow camera...)
CBUFFER(Camera_CommonCB, CBSLOT_RENDERER_CAMERA)
{
	float4x4	g_xCamera_VP;					// View*Projection
	float4x4	g_xCamera_View;
	float4x4	g_xCamera_Proj;
	float3		g_xCamera_CamPos;				float xPadding0_Camera_CommonCB;
};
CBUFFER(MiscCB, CBSLOT_RENDERER_MISC)
{
	float4x4	g_xTransform;
	float4		g_xColor;
};

CBUFFER(APICB, CBSLOT_API)
{
	float4		g_xClipPlane;
	float		g_xAlphaRef;
	float3		g_xPadding0_APICB;
};

static const float		PI = 3.14159265358979323846;
static const float	 SQRT2 = 1.41421356237309504880;

#define sqr(a)		((a)*(a))

#ifdef DISABLE_ALPHATEST
#define ALPHATEST(x)
#else
#define ALPHATEST(x)	clip((x) - (1.0f - g_xAlphaRef));
#endif

#define DEGAMMA(x)		pow(abs(x),g_xWorld_Gamma)
#define GAMMA(x)		pow(abs(x),1.0/g_xWorld_Gamma)

inline float3 GetHorizonColor() { return g_xWorld_Horizon.rgb; }
inline float3 GetZenithColor() { return g_xWorld_Zenith.rgb; }
inline float3 GetAmbientColor() { return g_xWorld_Ambient.rgb; }
inline float2 GetScreenResolution() { return g_xWorld_ScreenWidthHeight; }
inline float GetScreenWidth() { return g_xWorld_ScreenWidthHeight.x; }
inline float GetScreenHeight() { return g_xWorld_ScreenWidthHeight.y; }
inline float2 GetInternalResolution() { return g_xWorld_InternalResolution; }
inline float GetTime() { return g_xFrame_Time; }
inline float GetEmissive(float emissive) { return emissive * 10.0f; }
inline uint2 GetTemporalAASampleRotation() { return float2((g_xFrame_TemporalAASampleRotation >> 0) & 0x000000FF, (g_xFrame_TemporalAASampleRotation >> 8) & 0x000000FF); }

struct ComputeShaderInput
{
	uint3 groupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
	uint3 groupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
	uint3 dispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
	uint  groupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};


// Helpers:

// 2D array index to flattened 1D array index
inline uint flatten2D(uint2 coord, uint2 dim)
{
	return coord.x + coord.y * dim.x;
}
// flattened array index to 2D array index
inline uint2 unflatten2D(uint idx, uint2 dim)
{
	return uint2(idx % dim.x, idx / dim.x);
}

// 3D array index to flattened 1D array index
inline uint flatten3D(uint3 coord, uint3 dim)
{
	return (coord.z * dim.x * dim.y) + (coord.y * dim.x) + coord.x;
}
// flattened array index to 3D array index
inline uint3 unflatten3D(uint idx, uint3 dim)
{
	const uint z = idx / (dim.x * dim.y);
	idx -= (z * dim.x * dim.y);
	const uint y = idx / dim.x;
	const uint x = idx % dim.x;
	return  uint3(x, y, z);
}

// Creates a unit cube triangle strip from just vertex ID (14 vertices)
inline float3 CreateCube(in uint vertexID)
{
	uint b = 1 << vertexID;
	return float3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}

#endif // _SHADER_GLOBALS_