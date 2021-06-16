#ifndef WI_SHADERINTEROP_RENDERER_H
#define WI_SHADERINTEROP_RENDERER_H
#include "ShaderInterop.h"


static const uint SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS = 1 << 0;
static const uint SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW = 1 << 1;
static const uint SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY = 1 << 2;
static const uint SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY = 1 << 3;
static const uint SHADERMATERIAL_OPTION_BIT_USE_WIND = 1 << 4;
static const uint SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW = 1 << 5;
static const uint SHADERMATERIAL_OPTION_BIT_CAST_SHADOW = 1 << 6;

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

	float		transmission;
	uint		options;
	int			padding0;
	int			padding1;

	uint		layerMask;
	int			uvset_baseColorMap;
	int			uvset_surfaceMap;
	int			uvset_normalMap;

	int			uvset_displacementMap;
	int			uvset_emissiveMap;
	int			uvset_occlusionMap;
	int			uvset_transmissionMap;

	int			uvset_sheenColorMap;
	int			uvset_sheenRoughnessMap;
	int			uvset_clearcoatMap;
	int			uvset_clearcoatRoughnessMap;

	int			uvset_clearcoatNormalMap;
	int			uvset_specularMap;
	int			padding2;
	int			padding3;

	float		sheenRoughness;
	float		clearcoat;
	float		clearcoatRoughness;
	float		padding4;

	float4		sheenColor;

	float4		baseColorAtlasMulAdd;
	float4		surfaceMapAtlasMulAdd;
	float4		emissiveMapAtlasMulAdd;
	float4		normalMapAtlasMulAdd;

	int			texture_basecolormap_index;
	int			texture_surfacemap_index;
	int			texture_emissivemap_index;
	int			texture_normalmap_index;

	int			texture_displacementmap_index;
	int			texture_occlusionmap_index;
	int			texture_transmissionmap_index;
	int			texture_sheencolormap_index;

	int			texture_sheenroughnessmap_index;
	int			texture_clearcoatmap_index;
	int			texture_clearcoatroughnessmap_index;
	int			texture_clearcoatnormalmap_index;

	int			texture_specularmap_index;
	int			padding5;
	int			padding6;
	int			padding7;

	inline bool IsUsingVertexColors() { return options & SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS; }
	inline bool IsUsingSpecularGlossinessWorkflow() { return options & SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW; }
	inline bool IsOcclusionEnabled_Primary() { return options & SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY; }
	inline bool IsOcclusionEnabled_Secondary() { return options & SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY; }
	inline bool IsUsingWind() { return options & SHADERMATERIAL_OPTION_BIT_USE_WIND; }
	inline bool IsReceiveShadow() { return options & SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW; }
	inline bool IsCastingShadow() { return options & SHADERMATERIAL_OPTION_BIT_CAST_SHADOW; }
};

struct ShaderMesh
{
	int ib;
	int vb_pos_nor_wind;
	int vb_tan;
	int vb_col;

	int vb_uv0;
	int vb_uv1;
	int vb_atl;
	int vb_pre;

	int subsetbuffer;
	int blendmaterial1;
	int blendmaterial2;
	int blendmaterial3;
};

struct ShaderMeshSubset
{
	uint indexOffset;
	uint indexCount;
	int mesh;
	int material;
};

struct ObjectPushConstants
{
	int mesh;
	int material;
	int instances;
	uint instance_offset;
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

struct AtmosphereParameters
{
		float2 padding0;
	// Radius of the planet (center to ground)
	float bottomRadius;
	// Maximum considered atmosphere height (center to atmosphere top)
	float topRadius;

	// Center of the planet
	float3 planetCenter;
	// Rayleigh scattering exponential distribution scale in the atmosphere
	float rayleighDensityExpScale;

	// Rayleigh scattering coefficients
	float3 rayleighScattering;
	// Mie scattering exponential distribution scale in the atmosphere
	float mieDensityExpScale;

	// Mie scattering coefficients
	float3 mieScattering;	float padding1;

	// Mie extinction coefficients
	float3 mieExtinction;	float padding2;

	// Mie absorption coefficients
	float3 mieAbsorption;	
	// Mie phase function excentricity
	float miePhaseG;

	// Another medium type in the atmosphere
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;

		float3 padding3;
	float absorptionDensity1LinearTerm;

	// This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float3 absorptionExtinction;	float padding4;

	// The albedo of the ground.
	float3 groundAlbedo;	float padding5;

	// Init default values (All units in kilometers)
	void init(
		float earthBottomRadius = 6360.0f,
		float earthTopRadius = 6460.0f, // 100km atmosphere radius, less edge visible and it contain 99.99% of the atmosphere medium https://en.wikipedia.org/wiki/K%C3%A1rm%C3%A1n_line
		float earthRayleighScaleHeight = 8.0f,
		float earthMieScaleHeight = 1.2f
	)
	{

		// Values shown here are the result of integration over wavelength power spectrum integrated with paricular function.
		// Refer to https://github.com/ebruneton/precomputed_atmospheric_scattering for details.

		// Translation from Bruneton2017 parameterisation.
		rayleighDensityExpScale = -1.0f / earthRayleighScaleHeight;
		mieDensityExpScale = -1.0f / earthMieScaleHeight;
		absorptionDensity0LayerWidth = 25.0f;
		absorptionDensity0ConstantTerm = -2.0f / 3.0f;
		absorptionDensity0LinearTerm = 1.0f / 15.0f;
		absorptionDensity1ConstantTerm = 8.0f / 3.0f;
		absorptionDensity1LinearTerm = -1.0f / 15.0f;

		miePhaseG = 0.8f;
		rayleighScattering = float3(0.005802f, 0.013558f, 0.033100f);
		mieScattering = float3(0.003996f, 0.003996f, 0.003996f);
		mieExtinction = float3(0.004440f, 0.004440f, 0.004440f);
		mieAbsorption.x = mieExtinction.x - mieScattering.x;
		mieAbsorption.y = mieExtinction.y - mieScattering.y;
		mieAbsorption.z = mieExtinction.z - mieScattering.z;

		absorptionExtinction = float3(0.000650f, 0.001881f, 0.000085f);

		groundAlbedo = float3(0.3f, 0.3f, 0.3f); // 0.3 for earths ground albedo, see https://nssdc.gsfc.nasa.gov/planetary/factsheet/earthfact.html
		bottomRadius = earthBottomRadius;
		topRadius = earthTopRadius;
		planetCenter = float3(0.0f, -earthBottomRadius - 0.1f, 0.0f); // Spawn 100m in the air 
	}


#ifdef __cplusplus
	AtmosphereParameters() { init(); }
#endif // __cplusplus
};


struct VolumetricCloudParameters
{
	float3 Albedo; // Cloud albedo is normally very close to 1
	float CloudAmbientGroundMultiplier; // [0; 1] Amount of ambient light to reach the bottom of clouds

	float3 ExtinctionCoefficient; // * 0.05 looks good too
	float BeerPowder;

	float BeerPowderPower;
	float PhaseG; // [-0.999; 0.999]
	float PhaseG2; // [-0.999; 0.999]
	float PhaseBlend; // [0; 1]

	float MultiScatteringScattering;
	float MultiScatteringExtinction;
	float MultiScatteringEccentricity;
	float ShadowStepLength;

	float HorizonBlendAmount;
	float HorizonBlendPower;
	float WeatherDensityAmount; // Rain clouds disabled by default.
	float CloudStartHeight;

	float CloudThickness;
	float SkewAlongWindDirection;
	float TotalNoiseScale;
	float DetailScale;

	float WeatherScale;
	float CurlScale;
	float ShapeNoiseHeightGradientAmount;
	float ShapeNoiseMultiplier;

	float2 ShapeNoiseMinMax;
	float ShapeNoisePower;
	float DetailNoiseModifier;

	float DetailNoiseHeightFraction;
	float CurlNoiseModifier;
	float CoverageAmount;
	float CoverageMinimum;

	float TypeAmount;
	float TypeOverall;
	float AnvilAmount; // Anvil clouds disabled by default.
	float AnvilOverhangHeight;

	// Animation
	float AnimationMultiplier;
	float WindSpeed;
	float WindAngle;
	float WindUpAmount;

		float2 padding0;
	float CoverageWindSpeed;
	float CoverageWindAngle;

	// Cloud types
	// 4 positions of a black, white, white, black gradient
	float4 CloudGradientSmall;
	float4 CloudGradientMedium;
	float4 CloudGradientLarge;

	// Performance
	int MaxStepCount; // Maximum number of iterations. Higher gives better images but may be slow.
	float MaxMarchingDistance; // Clamping the marching steps to be within a certain distance.
	float InverseDistanceStepCount; // Distance over which the raymarch steps will be evenly distributed.
	float RenderDistance; // Maximum distance to march before returning a miss.

	float LODDistance; // After a certain distance, noises will get higher LOD
	float LODMin; // 
	float BigStepMarch; // How long inital rays should be until they hit something. Lower values may ives a better image but may be slower.
	float TransmittanceThreshold; // Default: 0.005. If the clouds transmittance has reached it's desired opacity, there's no need to keep raymarching for performance.

		float2 padding1;
	float ShadowSampleCount;
	float GroundContributionSampleCount;

	void init()
	{


		// Lighting
		Albedo = float3(0.9f, 0.9f, 0.9f);
		CloudAmbientGroundMultiplier = 0.75f;
		ExtinctionCoefficient = float3(0.71f * 0.1f, 0.86f * 0.1f, 1.0f * 0.1f);
		BeerPowder = 20.0f;
		BeerPowderPower = 0.5f;
		PhaseG = 0.5f; // [-0.999; 0.999]
		PhaseG2 = -0.5f; // [-0.999; 0.999]
		PhaseBlend = 0.2f; // [0; 1]
		MultiScatteringScattering = 1.0f;
		MultiScatteringExtinction = 0.1f;
		MultiScatteringEccentricity = 0.2f;
		ShadowStepLength = 3000.0f;
		HorizonBlendAmount = 1.25f;
		HorizonBlendPower = 2.0f;
		WeatherDensityAmount = 0.0f;

		// Modelling
		CloudStartHeight = 1500.0f;
		CloudThickness = 4000.0f;
		SkewAlongWindDirection = 700.0f;

		TotalNoiseScale = 1.0f;
		DetailScale = 5.0f;
		WeatherScale = 0.0625f;
		CurlScale = 7.5f;

		ShapeNoiseHeightGradientAmount = 0.2f;
		ShapeNoiseMultiplier = 0.8f;
		ShapeNoisePower = 6.0f;
		ShapeNoiseMinMax = float2(0.25f, 1.1f);

		DetailNoiseModifier = 0.2f;
		DetailNoiseHeightFraction = 10.0f;
		CurlNoiseModifier = 550.0f;

		CoverageAmount = 2.0f;
		CoverageMinimum = 1.05f;
		TypeAmount = 1.0f;
		TypeOverall = 0.0f;
		AnvilAmount = 0.0f;
		AnvilOverhangHeight = 3.0f;

		// Animation
		AnimationMultiplier = 2.0f;
		WindSpeed = 15.9f;
		WindAngle = -0.39f;
		WindUpAmount = 0.5f;
		CoverageWindSpeed = 25.0f;
		CoverageWindAngle = 0.087f;

		// Cloud types
		// 4 positions of a black, white, white, black gradient
		CloudGradientSmall = float4(0.02f, 0.07f, 0.12f, 0.28f);
		CloudGradientMedium = float4(0.02f, 0.07f, 0.39f, 0.59f);
		CloudGradientLarge = float4(0.02f, 0.07f, 0.88f, 1.0f);

		// Performance
		MaxStepCount = 128;
		MaxMarchingDistance = 30000.0f;
		InverseDistanceStepCount = 15000.0f;
		RenderDistance = 70000.0f;
		LODDistance = 25000.0f;
		LODMin = 0.0f;
		BigStepMarch = 3.0f;
		TransmittanceThreshold = 0.005f;
		ShadowSampleCount = 5.0f;
		GroundContributionSampleCount = 2.0f;
	}

#ifdef __cplusplus
	VolumetricCloudParameters() { init(); }
#endif // __cplusplus

};

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
static const uint OPTION_BIT_SHADOW_MASK = 1 << 9;

// ---------- Common Constant buffers: -----------------

CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
{
	float2		g_xFrame_CanvasSize;
	float2		g_xFrame_CanvasSize_rcp;

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

	float3		g_xFrame_padding0;
	float		g_xFrame_SkyExposure;

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
	int			g_xFrame_ObjectShaderSamplerIndex;
	float		g_xFrame_BlueNoisePhase;

	AtmosphereParameters g_xFrame_Atmosphere;
	VolumetricCloudParameters g_xFrame_VolumetricClouds;
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

	// Frustum planes:
	//	0 : near
	//	1 : far
	//	2 : left
	//	3 : right
	//	4 : top
	//	5 : bottom
	float4		g_xCamera_FrustumPlanes[6];

	float2		g_xFrame_TemporalAAJitter;
	float2		g_xFrame_TemporalAAJitterPrev;

	float4x4	g_xCamera_PrevV;
	float4x4	g_xCamera_PrevP;
	float4x4	g_xCamera_PrevVP;		// PrevView*PrevProjection
	float4x4	g_xCamera_PrevInvVP;	// Inverse(PrevView*PrevProjection)
	float4x4	g_xCamera_ReflVP;		// ReflectionView*ReflectionProjection
	float4x4	g_xCamera_Reprojection; // view_projection_inverse_matrix * previous_view_projection_matrix

	float2		g_xCamera_ApertureShape;
	float		g_xCamera_ApertureSize;
	float		g_xCamera_FocalLength;
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
	uint2 xForwardLightMask;	// supports indexing 64 lights
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
	float4 xTessellationFactors;
};


#endif // WI_SHADERINTEROP_RENDERER_H
