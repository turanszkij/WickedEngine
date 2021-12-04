#ifndef WI_SHADERINTEROP_RENDERER_H
#define WI_SHADERINTEROP_RENDERER_H
#include "ShaderInterop.h"
#include "ShaderInterop_Weather.h"

struct ShaderScene
{
	int instancebuffer;
	int meshbuffer;
	int materialbuffer;
	int envmaparray;

	int globalenvmap;
	int padding0;
	int padding1;
	int padding2;

	int TLAS;
	int BVH_counter;
	int BVH_nodes;
	int BVH_primitives;

	float3 aabb_min;
	float padding3;
	float3 aabb_max;
	float padding4;
	float3 aabb_extents;		// enclosing AABB abs(max - min)
	float padding5;
	float3 aabb_extents_rcp;	// enclosing AABB 1.0f / abs(max - min)
	float padding6;

	ShaderWeather weather;
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
static const uint SHADERMATERIAL_OPTION_BIT_ADDITIVE = 1 << 9;
static const uint SHADERMATERIAL_OPTION_BIT_UNLIT = 1 << 10;

struct ShaderMaterial
{
	float4		baseColor;
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
	uint		emissive_r11g11b10;
	uint		specular_r11g11b10;

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

	uint		sheenColor_r11g11b10;
	float		sheenRoughness;
	float		clearcoat;
	float		clearcoatRoughness;

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

#ifndef __cplusplus
	float3 GetEmissive() { return Unpack_R11G11B10_FLOAT(emissive_r11g11b10); }
	float3 GetSpecular() { return Unpack_R11G11B10_FLOAT(specular_r11g11b10); }
	float3 GetSheenColor() { return Unpack_R11G11B10_FLOAT(sheenColor_r11g11b10); }
#endif // __cplusplus

	inline bool IsUsingVertexColors() { return options & SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS; }
	inline bool IsUsingSpecularGlossinessWorkflow() { return options & SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW; }
	inline bool IsOcclusionEnabled_Primary() { return options & SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY; }
	inline bool IsOcclusionEnabled_Secondary() { return options & SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY; }
	inline bool IsUsingWind() { return options & SHADERMATERIAL_OPTION_BIT_USE_WIND; }
	inline bool IsReceiveShadow() { return options & SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW; }
	inline bool IsCastingShadow() { return options & SHADERMATERIAL_OPTION_BIT_CAST_SHADOW; }
	inline bool IsUnlit() { return options & SHADERMATERIAL_OPTION_BIT_UNLIT; }
};

static const uint SHADERMESH_FLAG_DOUBLE_SIDED = 1 << 0;
static const uint SHADERMESH_FLAG_HAIRPARTICLE = 1 << 1;
static const uint SHADERMESH_FLAG_EMITTEDPARTICLE = 1 << 2;

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
	uint layerMask;
	uint meshIndex;
	uint color;
	uint emissive;
	int lightmap;
	int padding0;
	ShaderTransform transform;
	ShaderTransform transformInverseTranspose; // This correctly handles non uniform scaling for normals
	ShaderTransform transformPrev;

	void init()
	{
		uid = 0;
		flags = 0;
		layerMask = 0;
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

struct ShaderSphere
{
	float3 center;
	float radius;
};
struct ShaderFrustum
{
	// Frustum planes:
	//	0 : near
	//	1 : far
	//	2 : left
	//	3 : right
	//	4 : top
	//	5 : bottom
	float4 planes[6];

#ifndef __cplusplus
	inline bool intersects(ShaderSphere sphere)
	{
		uint infrustum = 1;
		infrustum &= dot(planes[0], float4(sphere.center, 1)) > -sphere.radius;
		infrustum &= dot(planes[1], float4(sphere.center, 1)) > -sphere.radius;
		infrustum &= dot(planes[2], float4(sphere.center, 1)) > -sphere.radius;
		infrustum &= dot(planes[3], float4(sphere.center, 1)) > -sphere.radius;
		infrustum &= dot(planes[4], float4(sphere.center, 1)) > -sphere.radius;
		infrustum &= dot(planes[5], float4(sphere.center, 1)) > -sphere.radius;
		return infrustum != 0;
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

// These option bits can be read from options constant buffer value:
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
static const uint OPTION_BIT_STATIC_SKY_HDR = 1 << 13;

// ---------- Common Constant buffers: -----------------

struct FrameCB
{
	uint		options;							// wi::renderer bool options packed into bitmask
	uint		shadow_cascade_count;
	float		shadow_kernel_2D;
	float		shadow_kernel_cube;

	float		time;
	float		time_previous;
	float		delta_time;
	uint		frame_count;

	float3		voxelradiance_center;			// center of the voxel grid in world space units
	float		voxelradiance_max_distance;		// maximum raymarch distance for voxel GI in world-space

	float		voxelradiance_size;				// voxel half-extent in world space units
	float		voxelradiance_size_rcp;			// 1.0 / voxel-half extent
	uint		voxelradiance_resolution;		// voxel grid resolution
	float		voxelradiance_resolution_rcp;	// 1.0 / voxel grid resolution

	uint		voxelradiance_mipcount;			// voxel grid mipmap count
	uint		voxelradiance_numcones;			// number of diffuse cones to trace
	float		voxelradiance_numcones_rcp;		// 1.0 / number of diffuse cones to trace
	float		voxelradiance_stepsize;			// raymarch step size in voxel space units

	uint		envprobe_mipcount;
	float		envprobe_mipcount_rcp;
	uint		lightarray_offset;			// indexing into entity array
	uint		lightarray_count;			// indexing into entity array

	uint		decalarray_offset;			// indexing into entity array
	uint		decalarray_count;			// indexing into entity array
	uint		forcefieldarray_offset;		// indexing into entity array
	uint		forcefieldarray_count;		// indexing into entity array

	uint		envprobearray_offset;		// indexing into entity array
	uint		envprobearray_count;			// indexing into entity array
	uint		temporalaa_samplerotation;
	float		blue_noise_phase;

	int			sampler_objectshader_index;
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
	int	padding0;

	ShaderScene scene;
};

struct CameraCB
{
	float4x4	view_projection;

	float4		clip_plane;

	float3		position;
	float		distance_from_origin;

	float3		forward;
	float		z_near;

	float3		up;
	float		z_far;

	float		z_near_rcp;
	float		z_far_rcp;
	float		z_range;
	float		z_range_rcp;

	float4x4	view;
	float4x4	projection;
	float4x4	inverse_view;
	float4x4	inverse_projection;
	float4x4	inverse_view_projection;

	ShaderFrustum frustum;

	float2		temporalaa_jitter;
	float2		temporalaa_jitter_prev;

	float4x4	previous_view;
	float4x4	previous_projection;
	float4x4	previous_view_projection;
	float4x4	previous_inverse_view_projection;
	float4x4	reflection_view_projection;
	float4x4	reprojection; // view_projection_inverse_matrix * previous_view_projection_matrix

	float2		aperture_shape;
	float		aperture_size;
	float		focal_length;

	float2 canvas_size;
	float2 canvas_size_rcp;
		   
	uint2 internal_resolution;
	float2 internal_resolution_rcp;

	uint3 entity_culling_tilecount;
	uint sample_count;

	int texture_depth_index;
	int texture_lineardepth_index;
	int texture_gbuffer0_index;
	int texture_gbuffer1_index;

	int buffer_entitytiles_opaque_index;
	int buffer_entitytiles_transparent_index;
	int texture_reflection_index;
	int texture_refraction_index;

	int texture_waterriples_index;
	int texture_ao_index;
	int texture_ssr_index;
	int texture_rtshadow_index;

	int texture_surfelgi_index;
	int texture_depth_index_prev;
	int padding1;
	int padding2;
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
	float4x4 view_projection;
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
