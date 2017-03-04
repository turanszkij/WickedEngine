#ifndef _SHADER_GLOBALS_
#define _SHADER_GLOBALS_
#include "ShaderInterop.h"
#include "ConstantBufferMapping.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"

TEXTURE2D(texture_depth, float, TEXSLOT_DEPTH)
TEXTURE2D(texture_lineardepth, float, TEXSLOT_LINEARDEPTH)
TEXTURE2D(texture_gbuffer0, float4, TEXSLOT_GBUFFER0)
TEXTURE2D(texture_gbuffer1, float4, TEXSLOT_GBUFFER1)
TEXTURE2D(texture_gbuffer2, float4, TEXSLOT_GBUFFER2)
TEXTURE2D(texture_gbuffer3, float4, TEXSLOT_GBUFFER3)
TEXTURE2D(texture_gbuffer4, float4, TEXSLOT_GBUFFER4)
TEXTURECUBE(texture_env_global, float4, TEXSLOT_ENV_GLOBAL)
TEXTURECUBE(texture_env0, float4, TEXSLOT_ENV0)
TEXTURECUBE(texture_env1, float4, TEXSLOT_ENV1)
TEXTURECUBE(texture_env2, float4, TEXSLOT_ENV2)
TEXTURE2D(texture_decalatlas, float4, TEXSLOT_DECALATLAS)
TEXTURE2DARRAY(texture_shadowarray_2d, float, TEXSLOT_SHADOWARRAY_2D)
TEXTURECUBEARRAY(texture_shadowarray_cube, float, TEXSLOT_SHADOWARRAY_CUBE)
TEXTURE3D(texture_voxelradiance, float4, TEXSLOT_VOXELRADIANCE)

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

CBUFFER(WorldCB, CBSLOT_RENDERER_WORLD)
{
	float3		g_xWorld_Horizon;				float xPadding0_WorldCB;
	float3		g_xWorld_Zenith;				float xPadding1_WorldCB;
	float3		g_xWorld_Ambient;				float xPadding2_WorldCB;
	float3		g_xWorld_Fog;					float xPadding3_WorldCB;
	float2		g_xWorld_ScreenWidthHeight;
	float		g_xWorld_VoxelRadianceDataSize;
	uint		g_xWorld_VoxelRadianceDataRes;
	float3		g_xWorld_VoxelRadianceDataCenter;
	float		xPadding4_WorldCB;
};
CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
{
	float3		g_xFrame_WindDirection;
	float		g_xFrame_WindTime;
	float		g_xFrame_WindWaveSize;
	float		g_xFrame_WindRandomness;
	uint		g_xFrame_FrameCount;
	int			g_xFrame_SunLightArrayIndex;
	// The following are per frame properties for the main camera:
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
};
// The following buffer contains properties for a temporary camera (eg. main camera, reflection camera, shadow camera...)
CBUFFER(Camera_CommonCB, CBSLOT_RENDERER_CAMERA)
{
	float4x4	g_xCamera_VP;					// View*Projection
	float4x4	g_xCamera_View;
	float4x4	g_xCamera_Proj;
	float3		g_xCamera_CamPos;				float xPadding0_Camera_CommonCB;
}
CBUFFER(MaterialCB, CBSLOT_RENDERER_MATERIAL)
{
	float4		g_xMat_baseColor;
	float4		g_xMat_texMulAdd;
	float		g_xMat_roughness;
	float		g_xMat_reflectance;
	float		g_xMat_metalness;
	float		g_xMat_emissive;
	float		g_xMat_refractionIndex;
	float		g_xMat_subsurfaceScattering;
	float		g_xMat_normalMapStrength;
	float		g_xMat_parallaxOcclusionMapping;
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

#define sqr( a )		(a)*(a)
#define pow8( a )		(a)*(a)*(a)*(a)*(a)*(a)*(a)*(a)

#ifdef DISABLE_ALPHATEST
#define ALPHATEST(x)
#else
#define ALPHATEST(x)	clip((x) - (1.0f - g_xAlphaRef));
#endif

static const float		g_GammaValue = 2.2;
#define DEGAMMA(x)		pow(abs(x),g_GammaValue)
#define GAMMA(x)		pow(abs(x),1.0/g_GammaValue)

inline float3 GetHorizonColor() { return GAMMA(g_xWorld_Horizon.rgb); }
inline float3 GetZenithColor() { return GAMMA(g_xWorld_Zenith.rgb); }
inline float3 GetAmbientColor() { return GAMMA(g_xWorld_Ambient.rgb); }
inline float2 GetScreenResolution() { return g_xWorld_ScreenWidthHeight; }
inline float GetScreenWidth() { return g_xWorld_ScreenWidthHeight.x; }
inline float GetScreenHeight() { return g_xWorld_ScreenWidthHeight.y; }
inline float GetTime() { return g_xFrame_WindTime; }
inline float GetEmissive(float emissive) { return emissive * 10.0f; }

struct ComputeShaderInput
{
	uint3 groupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
	uint3 groupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
	uint3 dispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
	uint  groupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};


// Helpers:

// 3D array index to flattened 1D array index
inline uint to1D(uint3 coord, uint3 dim)
{
	return (coord.z * dim.x * dim.y) + (coord.y * dim.x) + coord.x;
}
// flattened array index to 3D array index
inline uint3 to3D(uint idx, uint3 dim)
{
	const uint z = idx / (dim.x * dim.y);
	idx -= (z * dim.x * dim.y);
	const uint y = idx / dim.x;
	const uint x = idx % dim.x;
	return  uint3(x, y, z);
}

#endif // _SHADER_GLOBALS_