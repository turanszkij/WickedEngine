#ifndef WI_SHADERINTEROP_RENDERER_H
#define WI_SHADERINTEROP_RENDERER_H
#include "ShaderInterop.h"


static const uint SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS = 1 << 0;
static const uint SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW = 1 << 1;
static const uint SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY = 1 << 2;
static const uint SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY = 1 << 3;
static const uint SHADERMATERIAL_OPTION_BIT_USE_WIND = 1 << 4;

struct ShaderMaterial
{
	float4		baseColor;
	float4		specularColor;
	float4		emissiveColor;
	float4		subsurfaceScattering;
	float4		subsurfaceScattering_inv;
	float4		texMulAdd;

	float		roughness;
	float		reflectance;
	float		metalness;
	float		refraction;

	float		normalMapStrength;
	float		parallaxOcclusionMapping;
	float		alphaTest;
	float		displacementMapping;

	uint		layerMask;
	int			uvset_baseColorMap;
	int			uvset_surfaceMap;
	int			uvset_normalMap;

	int			uvset_displacementMap;
	int			uvset_emissiveMap;
	int			uvset_occlusionMap;
	int			uvset_transmissionMap;

	float2		padding0;
	float		transmission;
	uint		options;

	float4		baseColorAtlasMulAdd;
	float4		surfaceMapAtlasMulAdd;
	float4		emissiveMapAtlasMulAdd;
	float4		normalMapAtlasMulAdd;

	inline bool IsUsingVertexColors() { return options & SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS; }
	inline bool IsUsingSpecularGlossinessWorkflow() { return options & SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW; }
	inline bool IsOcclusionEnabled_Primary() { return options & SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY; }
	inline bool IsOcclusionEnabled_Secondary() { return options & SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY; }
	inline bool IsUsingWind() { return options & SHADERMATERIAL_OPTION_BIT_USE_WIND; }
};

// Warning: the size of this structure directly affects shader performance.
//	Try to reduce it as much as possible!
//	Keep it aligned to 16 bytes for best performance!
//	Right now, this is 48 bytes total
struct ShaderEntity
{
	float3 position;
	uint type8_flags8_range16;

	uint2 direction16_coneAngleCos16;
	uint energy16_X16; // 16 bits free
	uint color;

	uint layerMask;
	uint indices;
	uint cubeRemap;
	uint userdata;

#ifndef __cplusplus
	// Shader-side:
	inline uint GetType()
	{
		return type8_flags8_range16 & 0xFF;
	}
	inline uint GetFlags()
	{
		return (type8_flags8_range16 >> 8) & 0xFF;
	}
	inline float GetRange()
	{
		return f16tof32((type8_flags8_range16 >> 16) & 0xFFFF);
	}
	inline float3 GetDirection()
	{
		return float3(
			f16tof32(direction16_coneAngleCos16.x & 0xFFFF),
			f16tof32((direction16_coneAngleCos16.x >> 16) & 0xFFFF),
			f16tof32(direction16_coneAngleCos16.y & 0xFFFF)
		);
	}
	inline float GetConeAngleCos()
	{
		return f16tof32((direction16_coneAngleCos16.y >> 16) & 0xFFFF);
	}
	inline float GetEnergy()
	{
		return f16tof32(energy16_X16 & 0xFFFF);
	}
	inline float GetCubemapDepthRemapNear()
	{
		return f16tof32(cubeRemap & 0xFFFF);
	}
	inline float GetCubemapDepthRemapFar()
	{
		return f16tof32((cubeRemap >> 16) & 0xFFFF);
	}
	inline float4 GetColor()
	{
		float4 fColor;

		fColor.x = (float)((color >> 0) & 0xFF) / 255.0f;
		fColor.y = (float)((color >> 8) & 0xFF) / 255.0f;
		fColor.z = (float)((color >> 16) & 0xFF) / 255.0f;
		fColor.w = (float)((color >> 24) & 0xFF) / 255.0f;

		return fColor;
	}
	inline uint GetMatrixIndex()
	{
		return indices & 0xFFFF;
	}
	inline uint GetTextureIndex()
	{
		return (indices >> 16) & 0xFFFF;
	}
	inline bool IsCastingShadow()
	{
		return indices != ~0;
	}

	// Load decal props:
	inline float GetEmissive() { return GetEnergy(); }

#else
	// Application-side:
	inline void SetType(uint type)
	{
		type8_flags8_range16 |= type & 0xFF;
	}
	inline void SetFlags(uint flags)
	{
		type8_flags8_range16 |= (flags & 0xFF) << 8;
	}
	inline void SetRange(float value)
	{
		type8_flags8_range16 |= XMConvertFloatToHalf(value) << 16;
	}
	inline void SetDirection(float3 value)
	{
		direction16_coneAngleCos16.x |= XMConvertFloatToHalf(value.x);
		direction16_coneAngleCos16.x |= XMConvertFloatToHalf(value.y) << 16;
		direction16_coneAngleCos16.y |= XMConvertFloatToHalf(value.z);
	}
	inline void SetConeAngleCos(float value)
	{
		direction16_coneAngleCos16.y |= XMConvertFloatToHalf(value) << 16;
	}
	inline void SetEnergy(float value)
	{
		energy16_X16 |= XMConvertFloatToHalf(value);
	}
	inline void SetCubeRemapNear(float value)
	{
		cubeRemap |= XMConvertFloatToHalf(value);
	}
	inline void SetCubeRemapFar(float value)
	{
		cubeRemap |= XMConvertFloatToHalf(value) << 16;
	}
	inline void SetIndices(uint matrixIndex, uint textureIndex)
	{
		indices = matrixIndex & 0xFFFF;
		indices |= (textureIndex & 0xFFFF) << 16;
	}

#endif // __cplusplus
};

static const uint ENTITY_TYPE_DIRECTIONALLIGHT = 0;
static const uint ENTITY_TYPE_POINTLIGHT = 1;
static const uint ENTITY_TYPE_SPOTLIGHT = 2;
//static const uint ENTITY_TYPE_SPHERELIGHT = 3;
//static const uint ENTITY_TYPE_DISCLIGHT = 4;
//static const uint ENTITY_TYPE_RECTANGLELIGHT = 5;
//static const uint ENTITY_TYPE_TUBELIGHT = 6;
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

static const int impostorCaptureAngles = 36;

static const uint MATERIAL_TEXTURE_SLOT_DESCRIPTOR_BASECOLOR = 0;
static const uint MATERIAL_TEXTURE_SLOT_DESCRIPTOR_NORMAL = 1;
static const uint MATERIAL_TEXTURE_SLOT_DESCRIPTOR_SURFACE = 2;
static const uint MATERIAL_TEXTURE_SLOT_DESCRIPTOR_OCCLUSION = 3;
static const uint MATERIAL_TEXTURE_SLOT_DESCRIPTOR_EMISSIVE = 4;
static const uint MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT = 5;

static const uint VERTEXBUFFER_DESCRIPTOR_RAW_POS = 0;
static const uint VERTEXBUFFER_DESCRIPTOR_RAW_TAN = 1;
static const uint VERTEXBUFFER_DESCRIPTOR_RAW_COL = 1;
static const uint VERTEXBUFFER_DESCRIPTOR_RAW_COUNT = 2;

static const uint VERTEXBUFFER_DESCRIPTOR_UV_0 = 0;
static const uint VERTEXBUFFER_DESCRIPTOR_UV_1 = 1;
static const uint VERTEXBUFFER_DESCRIPTOR_UV_ATL = 2;
static const uint VERTEXBUFFER_DESCRIPTOR_UV_COUNT = 3;

// These option bits can be read from g_xFrame_Options constant buffer value:
static const uint OPTION_BIT_TEMPORALAA_ENABLED = 1 << 0;
static const uint OPTION_BIT_TRANSPARENTSHADOWS_ENABLED = 1 << 1;
static const uint OPTION_BIT_VOXELGI_ENABLED = 1 << 2;
static const uint OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED = 1 << 3;
static const uint OPTION_BIT_VOXELGI_RETARGETTED = 1 << 4;
static const uint OPTION_BIT_SIMPLE_SKY = 1 << 5;
static const uint OPTION_BIT_REALISTIC_SKY = 1 << 6;
static const uint OPTION_BIT_RAYTRACED_SHADOWS = 1 << 7;
static const uint OPTION_BIT_DISABLE_ALBEDO_MAPS = 1 << 8;

// ---------- Common Constant buffers: -----------------

CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
{
	float2		g_xFrame_ScreenWidthHeight;
	float2		g_xFrame_ScreenWidthHeight_rcp;

	float2		g_xFrame_InternalResolution;
	float2		g_xFrame_InternalResolution_rcp;

	float3		g_xFrame_SunColor;
	float		g_xFrame_Gamma;

	float3		g_xFrame_SunDirection;
	uint		g_xFrame_ShadowCascadeCount;

	float3		g_xFrame_Horizon;
	uint		g_xFrame_ConstantOne;						// Just a constant 1 value as uint (can be used to force disable loop unrolling)

	float3		g_xFrame_Zenith;
	float		g_xFrame_CloudScale;

	float3		g_xFrame_Ambient;
	float		g_xFrame_Cloudiness;

	float3		g_xFrame_Fog;								// Fog Start,End,Height
	float		g_xFrame_VoxelRadianceMaxDistance;			// maximum raymarch distance for voxel GI in world-space

	float		g_xFrame_VoxelRadianceDataSize;				// voxel half-extent in world space units
	float		g_xFrame_VoxelRadianceDataSize_rcp;			// 1.0 / voxel-half extent
	uint		g_xFrame_VoxelRadianceDataRes;				// voxel grid resolution
	float		g_xFrame_VoxelRadianceDataRes_rcp;			// 1.0 / voxel grid resolution

	uint		g_xFrame_VoxelRadianceDataMIPs;				// voxel grid mipmap count
	uint		g_xFrame_VoxelRadianceNumCones;				// number of diffuse cones to trace
	float		g_xFrame_VoxelRadianceNumCones_rcp;			// 1.0 / number of diffuse cones to trace
	float		g_xFrame_VoxelRadianceRayStepSize;			// raymarch step size in voxel space units

	float3		g_xFrame_VoxelRadianceDataCenter;			// center of the voxel grid in world space units
	uint		g_xFrame_Options;							// wiRenderer bool options packed into bitmask

	uint3		g_xFrame_EntityCullingTileCount;
	int			g_xFrame_GlobalEnvProbeIndex;

	uint		g_xFrame_EnvProbeMipCount;
	float		g_xFrame_EnvProbeMipCount_rcp;
	float		g_xFrame_Time;
	float		g_xFrame_TimePrev;

	float		g_xFrame_SunEnergy;
	float		g_xFrame_WindSpeed;
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

	float3		g_xFrame_WorldBoundsMin;			// world enclosing AABB min
	float		g_xFrame_CloudSpeed;

	float3		g_xFrame_WorldBoundsMax;			// world enclosing AABB max
	float		g_xFrame_WindRandomness;

	float3		g_xFrame_WorldBoundsExtents;		// world enclosing AABB abs(max - min)
	float		g_xFrame_StaticSkyGamma;			// possible values (0: no static sky; 1: hdr static sky; other: actual gamma when ldr)

	float3		g_xFrame_WorldBoundsExtents_rcp;	// world enclosing AABB 1.0f / abs(max - min)
	uint		g_xFrame_TemporalAASampleRotation;

	float		g_xFrame_ShadowKernel2D;
	float		g_xFrame_ShadowKernelCube;
	uint		g_xFrame_RaytracedShadowsSampleCount;
	float		g_xFrame_padding0;
};

CBUFFER(CameraCB, CBSLOT_RENDERER_CAMERA)
{
	float4x4	g_xCamera_VP;			// View*Projection

	float4		g_xCamera_ClipPlane;

	float3		g_xCamera_CamPos;
	float		g_xCamera_DistanceFromOrigin;

	float3		g_xCamera_At;
	float		g_xCamera_ZNearP;

	float3		g_xCamera_Up;
	float		g_xCamera_ZFarP;

	float		g_xCamera_ZNearP_rcp;
	float		g_xCamera_ZFarP_rcp;
	float		g_xCamera_ZRange;
	float		g_xCamera_ZRange_rcp;

	float4x4	g_xCamera_View;
	float4x4	g_xCamera_Proj;
	float4x4	g_xCamera_InvV;			// Inverse View
	float4x4	g_xCamera_InvP;			// Inverse Projection
	float4x4	g_xCamera_InvVP;		// Inverse View-Projection

	float4		g_xCamera_FrustumPlanes[6];

	float2		g_xFrame_TemporalAAJitter;
	float2		g_xFrame_TemporalAAJitterPrev;

	float4x4	g_xCamera_PrevV;
	float4x4	g_xCamera_PrevP;
	float4x4	g_xCamera_PrevVP;		// PrevView*PrevProjection
	float4x4	g_xCamera_PrevInvVP;	// Inverse(PrevView*PrevProjection)
	float4x4	g_xCamera_ReflVP;		// ReflectionView*ReflectionProjection
};




// ------- On demand Constant buffers: ----------

CBUFFER(MaterialCB, CBSLOT_RENDERER_MATERIAL)
{
	ShaderMaterial g_xMaterial;
};
CBUFFER(MaterialCB_Blend1, CBSLOT_RENDERER_MATERIAL_BLEND1)
{
	ShaderMaterial g_xMaterial_blend1;
};
CBUFFER(MaterialCB_Blend2, CBSLOT_RENDERER_MATERIAL_BLEND2)
{
	ShaderMaterial g_xMaterial_blend2;
};
CBUFFER(MaterialCB_Blend3, CBSLOT_RENDERER_MATERIAL_BLEND3)
{
	ShaderMaterial g_xMaterial_blend3;
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
	float3 eye;
	int hasTexNor;
	float3 front;
	float opacity;
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

struct CubemapRenderCam
{
	float4x4 VP;
	uint4 properties;
};
CBUFFER(CubemapRenderCB, CBSLOT_RENDERER_CUBEMAPRENDER)
{
	CubemapRenderCam xCubemapRenderCams[6];
};

CBUFFER(TessellationCB, CBSLOT_RENDERER_TESSELLATION)
{
	float4 g_f4TessFactors;
};


#endif // WI_SHADERINTEROP_RENDERER_H
