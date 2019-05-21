#ifndef _SHADERINTEROP_RENDERER_H_
#define _SHADERINTEROP_RENDERER_H_
#include "ShaderInterop.h"

struct ShaderEntityType
{
	uint params;
	float3 positionVS;
	float range;
	float3 directionVS;
	float3 positionWS;
	float energy;
	uint color;
	float3 directionWS;
	float coneAngleCos;
	float shadowKernel;
	float shadowBias;
	uint userdata;
	float4 texMulAdd;

	inline uint GetType()
	{
		return params & 0xFFFF;
	}
	inline uint GetFlags()
	{
		return (params >> 16) & 0xFFFF;
	}

	inline void SetType(uint type)
	{
		params = type & 0xFFFF; // also initializes to zero, so flags must be set after the type
	}
	inline void SetFlags(uint flags)
	{
		params |= (flags & 0xFFFF) << 16;
	}

	inline void SetShadowIndices(uint shadowMatrixIndex, uint shadowMapIndex)
	{
		userdata = shadowMatrixIndex & 0xFFFF;
		userdata |= (shadowMapIndex & 0xFFFF) << 16;
	}
	inline uint GetShadowMatrixIndex()
	{
		return userdata & 0xFFFF;
	}
	inline uint GetShadowMapIndex()
	{
		return (userdata >> 16) & 0xFFFF;
	}
	inline bool IsCastingShadow()
	{
		return userdata != ~0;
	}

	// Load uncompressed color:
	inline float4 GetColor()
	{
		float4 fColor;

		fColor.x = (float)((color >> 0) & 0x000000FF) / 255.0f;
		fColor.y = (float)((color >> 8) & 0x000000FF) / 255.0f;
		fColor.z = (float)((color >> 16) & 0x000000FF) / 255.0f;
		fColor.w = (float)((color >> 24) & 0x000000FF) / 255.0f;

		return fColor;
	}

	// Load area light props:
	inline float3 GetRight() { return directionWS; }
	inline float3 GetUp() { return directionVS; }
	inline float3 GetFront() { return positionVS; }
	inline float GetRadius() { return texMulAdd.x; }
	inline float GetWidth() { return texMulAdd.y; }
	inline float GetHeight() { return texMulAdd.z; }

	// Load decal props:
	inline float GetEmissive() { return energy; }
};

static const uint ENTITY_TYPE_DIRECTIONALLIGHT = 0;
static const uint ENTITY_TYPE_POINTLIGHT = 1;
static const uint ENTITY_TYPE_SPOTLIGHT = 2;
static const uint ENTITY_TYPE_SPHERELIGHT = 3;
static const uint ENTITY_TYPE_DISCLIGHT = 4;
static const uint ENTITY_TYPE_RECTANGLELIGHT = 5;
static const uint ENTITY_TYPE_TUBELIGHT = 6;
static const uint ENTITY_TYPE_DECAL = 100;
static const uint ENTITY_TYPE_ENVMAP = 101;
static const uint ENTITY_TYPE_FORCEFIELD_POINT = 200;
static const uint ENTITY_TYPE_FORCEFIELD_PLANE = 201;

static const uint ENTITY_FLAG_LIGHT_STATIC = 1 << 0;

static const uint SHADER_ENTITY_COUNT = 256;
static const uint SHADER_ENTITY_TILE_BUCKET_COUNT = SHADER_ENTITY_COUNT / 32;

static const uint MATRIXARRAY_COUNT = 128;

static const uint TILED_CULLING_BLOCKSIZE = 16;
static const uint TILED_CULLING_THREADSIZE = 8;
static const uint TILED_CULLING_GRANULARITY = TILED_CULLING_BLOCKSIZE / TILED_CULLING_THREADSIZE;

static const int impostorCaptureAngles = 12;

// ---------- Common Constant buffers: -----------------

CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
{
	float2		g_xFrame_ScreenWidthHeight;
	float2		g_xFrame_ScreenWidthHeight_Inverse;

	float2		g_xFrame_InternalResolution;
	float2		g_xFrame_InternalResolution_Inverse;

	float		g_xFrame_Gamma;
	float3		g_xFrame_SunColor;

	float3		g_xFrame_SunDirection;
	uint		g_xFrame_ShadowCascadeCount;

	float3		g_xFrame_Horizon;
	uint		g_xFrame_ConstantOne;						// Just a constant 1 value as uint (can be used to force disable loop unrolling)

	float3		g_xFrame_Zenith;
	float		g_xFrame_CloudScale;

	float3		g_xFrame_Ambient;
	float		g_xFrame_Cloudiness;

	float3		g_xFrame_Fog;								// Fog Start,End,Height
	float		g_xFrame_SpecularAA;

	float		g_xFrame_VoxelRadianceDataSize;				// voxel half-extent in world space units
	float		g_xFrame_VoxelRadianceDataSize_Inverse;		// 1.0 / voxel-half extent
	uint		g_xFrame_VoxelRadianceDataRes;				// voxel grid resolution
	float		g_xFrame_VoxelRadianceDataRes_Inverse;		// 1.0 / voxel grid resolution

	uint		g_xFrame_VoxelRadianceDataMIPs;				// voxel grid mipmap count
	uint		g_xFrame_VoxelRadianceNumCones;				// number of diffuse cones to trace
	float		g_xFrame_VoxelRadianceNumCones_Inverse;		// 1.0 / number of diffuse cones to trace
	float		g_xFrame_VoxelRadianceRayStepSize;			// raymarch step size in voxel space units

	uint		g_xFrame_VoxelRadianceReflectionsEnabled;	// are voxel gi reflections enabled or not
	float3		g_xFrame_VoxelRadianceDataCenter;			// center of the voxel grid in world space units

	uint		g_xFrame_AdvancedRefractions;
	uint3		g_xFrame_EntityCullingTileCount;

	uint		g_xFrame_TransparentShadowsEnabled;
	int			g_xFrame_GlobalEnvProbeIndex;
	uint		g_xFrame_EnvProbeMipCount;
	float		g_xFrame_EnvProbeMipCount_Inverse;

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
	float		g_xFrame_StaticSkyGamma;					// possible values (0: no static sky; 1: hdr static sky; other: actual gamma when ldr)
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

CBUFFER(APICB, CBSLOT_API)
{
	float4		g_xClipPlane;
	float		g_xAlphaRef;
	float3		g_xPadding0_APICB;
};




// ------- On demand Constant buffers: ----------

// The following buffer contains properties for a temporary camera (eg. main camera, reflection camera, shadow camera...)
CBUFFER(CameraCB, CBSLOT_RENDERER_CAMERA)
{
	float4x4	g_xCamera_VP;					// View*Projection
	float4x4	g_xCamera_View;
	float4x4	g_xCamera_Proj;
	float3		g_xCamera_CamPos;				float xPadding0_Camera_CommonCB;
};

CBUFFER(MaterialCB, CBSLOT_RENDERER_MATERIAL)
{
	float4		g_xMat_baseColor;
	float4		g_xMat_emissiveColor;
	float4		g_xMat_texMulAdd;

	float		g_xMat_roughness;
	float		g_xMat_reflectance;
	float		g_xMat_metalness;
	float		g_xMat_refractionIndex;

	float		g_xMat_subsurfaceScattering;
	float		g_xMat_normalMapStrength;
	float		g_xMat_normalMapFlip;
	float		g_xMat_parallaxOcclusionMapping;

	float		g_xMat_displacementMapping;
	uint		g_xMat_useVertexColors;
	int			g_xMat_uvset_baseColorMap;
	int			g_xMat_uvset_surfaceMap;

	int			g_xMat_uvset_normalMap;
	int			g_xMat_uvset_displacementMap;
	int			g_xMat_uvset_emissiveMap;
	int			g_xMat_uvset_occlusionMap;

	uint		g_xMat_specularGlossinessWorkflow;
	uint		g_xMat_occlusion_primary;
	uint		g_xMat_occlusion_secondary;
	uint		g_xMat_padding;
};

CBUFFER(MiscCB, CBSLOT_RENDERER_MISC)
{
	float4x4	g_xTransform;
	float4		g_xColor;
};

CBUFFER(ForwardEntityMaskCB, CBSLOT_RENDERER_FORWARD_LIGHTMASK)
{
	uint2 xForwardLightMask;	// supports indexind 64 lights
	uint xForwardDecalMask;		// supports indexing 32 decals
	uint xForwardEnvProbeMask;	// supports indexing 32 environment probes
};

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
