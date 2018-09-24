#ifndef _SHADERINTEROP_RENDERER_H_
#define _SHADERINTEROP_RENDERER_H_
#include "ShaderInterop.h"

// ---------- Persistent: -----------------

CBUFFER(WorldCB, CBSLOT_RENDERER_WORLD)
{
	float2		g_xWorld_ScreenWidthHeight;
	float2		g_xWorld_ScreenWidthHeight_Inverse;

	float2		g_xWorld_InternalResolution;
	float2		g_xWorld_InternalResolution_Inverse;

	float		g_xWorld_Gamma;
	float3		g_xWorld_SunColor;

	float3		g_xWorld_SunDirection; float pad0_WorldCB;

	float3		g_xWorld_Horizon; float pad1_WorldCB;

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

	uint		g_xWorld_VoxelRadianceReflectionsEnabled;	// are voxel gi reflections enabled or not
	float3		g_xWorld_VoxelRadianceDataCenter;			// center of the voxel grid in world space units

	uint		g_xWorld_AdvancedRefractions;
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
	float		pad0_FrameCB;
	uint		g_xFrame_VoxelRadianceRetargetted;
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

	float		g_xFrame_MainCamera_ZNearP_Recip;
	float		g_xFrame_MainCamera_ZFarP_Recip;
	float		g_xFrame_MainCamera_ZRange;
	float		g_xFrame_MainCamera_ZRange_Recip;

	float4		g_xFrame_FrustumPlanesWS[6];	// Frustum planes in world space in order: left,right,top,bottom,near,far

	float3		g_xFrame_WorldBoundsMin;				float pad0_frameCB;		// world enclosing AABB min
	float3		g_xFrame_WorldBoundsMax;				float pad1_frameCB;		// world enclosing AABB max
	float3		g_xFrame_WorldBoundsExtents;			float pad2_frameCB;		// world enclosing AABB abs(max - min)
	float3		g_xFrame_WorldBoundsExtents_Inverse;	float pad3_frameCB;		// world enclosing AABB 1.0f / abs(max - min)
};
// The following buffer contains properties for a temporary camera (eg. main camera, reflection camera, shadow camera...)
CBUFFER(CameraCB, CBSLOT_RENDERER_CAMERA)
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




// ------- On demand: ----------

CBUFFER(DecalCB, CBSLOT_RENDERER_DECAL)
{
	float4x4 xDecalVP;
	int hasTexNor;
	float3 eye;
	float opacity;
	float3 front;
};

CBUFFER(VolumeLightCB, CBSLOT_RENDERER_VOLUMELIGHT)
{
	float4x4 lightWorld;
	float4 lightColor;
	float4 lightEnerdis;
};

CBUFFER(LensFlareCB, CBSLOT_RENDERER_LENSFLARE)
{
	float4		xSunPos; // light position
	float4		xScreen; // screen dimensions
};

CBUFFER(CubemapRenderCB, CBSLOT_RENDERER_CUBEMAPRENDER)
{
	float4x4 xCubeShadowVP[6];
};

CBUFFER(TessellationCB, CBSLOT_RENDERER_TESSELLATION)
{
	float4 g_f4TessFactors;
};

CBUFFER(DispatchParamsCB, CBSLOT_RENDERER_DISPATCHPARAMS)
{
	uint3	xDispatchParams_numThreadGroups;
	uint	xDispatchParams_value0;
	uint3	xDispatchParams_numThreads;
	uint	xDispatchParams_value1;
};

#endif // _SHADERINTEROP_RENDERER_H_
