#ifndef WI_SHADERINTEROP_RENDERER_H
#define WI_SHADERINTEROP_RENDERER_H
#include "ShaderInterop.h"

struct ShaderScene
{
	int instancebuffer;
	int meshbuffer;
	int materialbuffer;
	int TLAS;

	int envmaparray;
	int globalenvmap;
	int padding1;
	int padding2;
};

static const uint SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS = 1 << 0;
static const uint SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW = 1 << 1;
static const uint SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY = 1 << 2;
static const uint SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY = 1 << 3;
static const uint SHADERMATERIAL_OPTION_BIT_USE_WIND = 1 << 4;
static const uint SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW = 1 << 5;
static const uint SHADERMATERIAL_OPTION_BIT_CAST_SHADOW = 1 << 6;
static const uint SHADERMATERIAL_OPTION_BIT_DOUBLE_SIDED = 1 << 7;
static const uint SHADERMATERIAL_OPTION_BIT_TRANSPARENT = 1 << 8;

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

static const uint SHADERMESH_FLAG_DOUBLE_SIDED = 1 << 0;

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
	uint blendmaterial1;
	uint blendmaterial2;
	uint blendmaterial3;

	float3 aabb_min;
	uint flags;
	float3 aabb_max;
	float tessellation_factor;

	void init()
	{
		ib = -1;
		vb_pos_nor_wind = -1;
		vb_tan = -1;
		vb_col = -1;

		vb_uv0 = -1;
		vb_uv1 = -1;
		vb_atl = -1;
		vb_pre = -1;

		subsetbuffer = -1;
		blendmaterial1 = 0;
		blendmaterial2 = 0;
		blendmaterial3 = 0;

		aabb_min = float3(0, 0, 0);
		aabb_max = float3(0, 0, 0);

		flags = 0;
	}
};

struct ShaderMeshSubset
{
	uint indexOffset;
	uint materialIndex;

	void init()
	{
		indexOffset = 0;
		materialIndex = 0;
	}
};

struct ShaderTransform
{
	float4 mat0;
	float4 mat1;
	float4 mat2;

	void init()
	{
		mat0 = float4(1, 0, 0, 0);
		mat1 = float4(0, 1, 0, 0);
		mat2 = float4(0, 0, 1, 0);
	}
	void Create(float4x4 mat)
	{
		mat0 = float4(mat._11, mat._21, mat._31, mat._41);
		mat1 = float4(mat._12, mat._22, mat._32, mat._42);
		mat2 = float4(mat._13, mat._23, mat._33, mat._43);
	}
	float4x4 GetMatrix()
#ifdef __cplusplus
		const
#endif // __cplusplus
	{
		return float4x4(
			mat0.x, mat0.y, mat0.z, mat0.w,
			mat1.x, mat1.y, mat1.z, mat1.w,
			mat2.x, mat2.y, mat2.z, mat2.w,
			0, 0, 0, 1
		);
	}
};

struct ShaderMeshInstance
{
	uint uid;
	uint flags;
	uint meshIndex;
	uint color;
	uint emissive;
	int lightmap;
	int padding0;
	int padding1;
	ShaderTransform transform;
	ShaderTransform transformInverseTranspose; // This correctly handles non uniform scaling for normals
	ShaderTransform transformPrev;

	void init()
	{
		uid = 0;
		flags = 0;
		meshIndex = ~0;
		color = ~0u;
		emissive = ~0u;
		lightmap = -1;
		transform.init();
		transformInverseTranspose.init();
		transformPrev.init();
	}

};
struct ShaderMeshInstancePointer
{
	uint instanceID;
	uint userdata;

	void init()
	{
		instanceID = ~0;
		userdata = 0;
	}
	void Create(uint _instanceID, uint frustum_index, float dither)
	{
		instanceID = _instanceID;
		userdata = 0;
		userdata |= frustum_index & 0xF;
		userdata |= (uint(dither * 255.0f) & 0xFF) << 4u;
	}
	uint GetFrustumIndex()
	{
		return userdata & 0xF;
	}
	float GetDither()
	{
		return ((userdata >> 4u) & 0xFF) / 255.0f;
	}
};

struct ObjectPushConstants
{
	uint meshIndex_subsetIndex; // 24-bit mesh, 8-bit subset
	uint materialIndex;
	int instances;
	uint instance_offset;

	void init(
		uint _meshIndex,
		uint _subsetIndex,
		uint _materialIndex,
		int _instances,
		uint _instance_offset
	)
	{
		meshIndex_subsetIndex = 0;
		meshIndex_subsetIndex |= _meshIndex & 0xFFFFFF;
		meshIndex_subsetIndex |= (_subsetIndex & 0xFF) << 24u;
		materialIndex = _materialIndex;
		instances = _instances;
		instance_offset = _instance_offset;
	}
	uint GetMeshIndex()
	{
		return meshIndex_subsetIndex & 0xFFFFFF;
	}
	uint GetSubsetIndex()
	{
		return (meshIndex_subsetIndex >> 24u) & 0xFF;
	}
	uint GetMaterialIndex()
	{
		return materialIndex;
	}
};

struct PrimitiveID
{
	uint primitiveIndex;
	uint instanceIndex;
	uint subsetIndex;

	uint2 pack()
	{
		// 32 bit primitiveID
		// 24 bit instanceID
		// 8  bit subsetID
		return uint2(primitiveIndex, (instanceIndex & 0xFFFFFF) | ((subsetIndex & 0xFF) << 24u));
	}
	void unpack(uint2 value)
	{
		primitiveIndex = value.x;
		instanceIndex = value.y & 0xFFFFFF;
		subsetIndex = (value.y >> 24u) & 0xFF;
	}
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

// These option bits can be read from Options constant buffer value:
static const uint OPTION_BIT_TEMPORALAA_ENABLED = 1 << 0;
static const uint OPTION_BIT_TRANSPARENTSHADOWS_ENABLED = 1 << 1;
static const uint OPTION_BIT_VOXELGI_ENABLED = 1 << 2;
static const uint OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED = 1 << 3;
static const uint OPTION_BIT_VOXELGI_RETARGETTED = 1 << 4;
static const uint OPTION_BIT_SIMPLE_SKY = 1 << 5;
static const uint OPTION_BIT_REALISTIC_SKY = 1 << 6;
static const uint OPTION_BIT_HEIGHT_FOG = 1 << 7;
static const uint OPTION_BIT_RAYTRACED_SHADOWS = 1 << 8;
static const uint OPTION_BIT_SHADOW_MASK = 1 << 9;
static const uint OPTION_BIT_SURFELGI_ENABLED = 1 << 10;
static const uint OPTION_BIT_DISABLE_ALBEDO_MAPS = 1 << 11;
static const uint OPTION_BIT_FORCE_DIFFUSE_LIGHTING = 1 << 12;

// ---------- Common Constant buffers: -----------------

struct FrameCB
{
	float2		CanvasSize;
	float2		CanvasSize_rcp;

	float2		InternalResolution;
	float2		InternalResolution_rcp;

	float3		SunColor;
	float		Gamma;

	float3		SunDirection;
	uint		ShadowCascadeCount;

	float3		Horizon;
	uint		ConstantOne;						// Just a constant 1 value as uint (can be used to force disable loop unrolling)

	float3		Zenith;
	float		CloudScale;

	float3		Ambient;
	float		Cloudiness;

	float4		Fog;								// Fog Start,End,Height Start,Height End

	float		padding0;
	float		FogHeightSky;
	float		SkyExposure;
	float		VoxelRadianceMaxDistance;			// maximum raymarch distance for voxel GI in world-space

	float		VoxelRadianceDataSize;				// voxel half-extent in world space units
	float		VoxelRadianceDataSize_rcp;			// 1.0 / voxel-half extent
	uint		VoxelRadianceDataRes;				// voxel grid resolution
	float		VoxelRadianceDataRes_rcp;			// 1.0 / voxel grid resolution

	uint		VoxelRadianceDataMIPs;				// voxel grid mipmap count
	uint		VoxelRadianceNumCones;				// number of diffuse cones to trace
	float		VoxelRadianceNumCones_rcp;			// 1.0 / number of diffuse cones to trace
	float		VoxelRadianceRayStepSize;			// raymarch step size in voxel space units

	float3		VoxelRadianceDataCenter;			// center of the voxel grid in world space units
	uint		Options;							// wiRenderer bool options packed into bitmask

	uint3		EntityCullingTileCount;
	int			GlobalEnvProbeIndex;

	uint		EnvProbeMipCount;
	float		EnvProbeMipCount_rcp;
	float		Time;
	float		TimePrev;

	float		SunEnergy;
	float		WindSpeed;
	float		DeltaTime;
	uint		FrameCount;

	uint		LightArrayOffset;			// indexing into entity array
	uint		LightArrayCount;			// indexing into entity array
	uint		DecalArrayOffset;			// indexing into entity array
	uint		DecalArrayCount;			// indexing into entity array

	uint		ForceFieldArrayOffset;		// indexing into entity array
	uint		ForceFieldArrayCount;		// indexing into entity array
	uint		EnvProbeArrayOffset;		// indexing into entity array
	uint		EnvProbeArrayCount;		// indexing into entity array

	float3		WindDirection;
	float		WindWaveSize;

	float3		WorldBoundsMin;			// world enclosing AABB min
	float		CloudSpeed;

	float3		WorldBoundsMax;			// world enclosing AABB max
	float		WindRandomness;

	float3		WorldBoundsExtents;		// world enclosing AABB abs(max - min)
	float		StaticSkyGamma;			// possible values (0: no static sky; 1: hdr static sky; other: actual gamma when ldr)

	float3		WorldBoundsExtents_rcp;	// world enclosing AABB 1.0f / abs(max - min)
	uint		TemporalAASampleRotation;

	float		ShadowKernel2D;
	float		ShadowKernelCube;
	int			ObjectShaderSamplerIndex;
	float		BlueNoisePhase;

	int			texture_random64x64_index;
	int			texture_bluenoise_index;
	int			texture_sheenlut_index;
	int			texture_skyviewlut_index;

	int			texture_transmittancelut_index;
	int			texture_multiscatteringlut_index;
	int			texture_skyluminancelut_index;
	int			texture_shadowarray_2d_index;

	int			texture_shadowarray_cube_index;
	int			texture_shadowarray_transparent_2d_index;
	int			texture_shadowarray_transparent_cube_index;
	int			texture_voxelgi_index;

	int			buffer_entityarray_index;
	int			buffer_entitymatrixarray_index;
	int			padding1;
	int			padding2;

	AtmosphereParameters Atmosphere;
	VolumetricCloudParameters VolumetricClouds;

	ShaderScene scene;
};

struct CameraCB
{
	float4x4	VP;			// View*Projection

	float4		ClipPlane;

	float3		CamPos;
	float		DistanceFromOrigin;

	float3		At;
	float		ZNearP;

	float3		Up;
	float		ZFarP;

	float		ZNearP_rcp;
	float		ZFarP_rcp;
	float		ZRange;
	float		ZRange_rcp;

	float4x4	View;
	float4x4	Proj;
	float4x4	InvV;			// Inverse View
	float4x4	InvP;			// Inverse Projection
	float4x4	InvVP;		// Inverse View-Projection

	// Frustum planes:
	//	0 : near
	//	1 : far
	//	2 : left
	//	3 : right
	//	4 : top
	//	5 : bottom
	float4		FrustumPlanes[6];

	float2		TemporalAAJitter;
	float2		TemporalAAJitterPrev;

	float4x4	PrevV;
	float4x4	PrevP;
	float4x4	PrevVP;		// PrevView*PrevProjection
	float4x4	PrevInvVP;	// Inverse(PrevView*PrevProjection)
	float4x4	ReflVP;		// ReflectionView*ReflectionProjection
	float4x4	Reprojection; // view_projection_inverse_matrix * previous_view_projection_matrix

	float2		ApertureShape;
	float		ApertureSize;
	float		FocalLength;
};


CONSTANTBUFFER(g_xFrame, FrameCB, CBSLOT_RENDERER_FRAME);
CONSTANTBUFFER(g_xCamera, CameraCB, CBSLOT_RENDERER_CAMERA);


// ------- On demand Constant buffers: ----------

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

CBUFFER(VolumeLightCB, CBSLOT_RENDERER_VOLUMELIGHT)
{
	float4x4 lightWorld;
	float4 lightColor;
	float4 lightEnerdis;
};

struct LensFlarePush
{
	float3 xLensFlarePos;
	float xLensFlareOffset;
	float2 xLensFlareSize;
	float2 xLensFlare_padding;
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

// MIP Generator params:
static const uint GENERATEMIPCHAIN_1D_BLOCK_SIZE = 64;
static const uint GENERATEMIPCHAIN_2D_BLOCK_SIZE = 8;
static const uint GENERATEMIPCHAIN_3D_BLOCK_SIZE = 4;

struct MipgenPushConstants
{
	uint3 outputResolution;
	uint arrayIndex;
	float3 outputResolution_rcp;
	uint mipgen_options;
	int texture_input;
	int texture_output;
	int sampler_index;
};
static const uint MIPGEN_OPTION_BIT_PRESERVE_COVERAGE = 1 << 0;

struct FilterEnvmapPushConstants
{
	uint2 filterResolution;
	float2 filterResolution_rcp;
	uint filterArrayIndex;
	float filterRoughness;
	uint filterRayCount;
	uint padding_filterCB;
	int texture_input;
	int texture_output;
};

// CopyTexture2D params:
struct CopyTextureCB
{
	int2 xCopyDest;
	int2 xCopySrcSize;
	int2 padding0;
	int  xCopySrcMIP;
	int  xCopyBorderExpandStyle;
};


static const uint PAINT_TEXTURE_BLOCKSIZE = 8;

struct PaintTextureCB
{
	uint2 xPaintBrushCenter;
	uint xPaintBrushRadius;
	float xPaintBrushAmount;

	float xPaintBrushFalloff;
	uint xPaintBrushColor;
	uint2 padding0;
};

CBUFFER(PaintRadiusCB, CBSLOT_RENDERER_MISC)
{
	uint2 xPaintRadResolution;
	uint2 xPaintRadCenter;
	uint xPaintRadUVSET;
	float xPaintRadRadius;
	uint2 pad;
};

struct SkinningPushConstants
{
	int bonebuffer_index;
	int vb_pos_nor_wind;
	int vb_tan;
	int vb_bon;

	int so_pos_nor_wind;
	int so_tan;
};


#endif // WI_SHADERINTEROP_RENDERER_H
