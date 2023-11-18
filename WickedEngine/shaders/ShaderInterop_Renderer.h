#ifndef WI_SHADERINTEROP_RENDERER_H
#define WI_SHADERINTEROP_RENDERER_H
#include "ShaderInterop.h"
#include "ShaderInterop_Weather.h"
#include "ShaderInterop_VXGI.h"

struct ShaderScene
{
	int instancebuffer;
	int geometrybuffer;
	int materialbuffer;
	int meshletbuffer;

	int globalenvmap; // static sky, not guaranteed to be cubemap, mipmaps or format, just whatever is imported
	int globalprobe; // rendered probe with guaranteed mipmaps, hdr, etc.
	int impostorInstanceOffset;
	int padding0;

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

	struct DDGI
	{
		uint3 grid_dimensions;
		uint probe_count;

		uint2 color_texture_resolution;
		float2 color_texture_resolution_rcp;

		uint2 depth_texture_resolution;
		float2 depth_texture_resolution_rcp;

		float3 grid_min;
		int color_texture;

		float3 grid_extents;
		int depth_texture;

		float3 cell_size;
		float max_distance;

		float3 grid_extents_rcp;
		int offset_buffer;

		float3 cell_size_rcp;
		float smooth_backface;
	};
	DDGI ddgi;
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

// Same as MaterialComponent::TEXTURESLOT
enum TEXTURESLOT
{
	BASECOLORMAP,
	NORMALMAP,
	SURFACEMAP,
	EMISSIVEMAP,
	DISPLACEMENTMAP,
	OCCLUSIONMAP,
	TRANSMISSIONMAP,
	SHEENCOLORMAP,
	SHEENROUGHNESSMAP,
	CLEARCOATMAP,
	CLEARCOATROUGHNESSMAP,
	CLEARCOATNORMALMAP,
	SPECULARMAP,
	ANISOTROPYMAP,

	TEXTURESLOT_COUNT
};

static const uint SVT_TILE_SIZE = 256u;
static const uint SVT_TILE_BORDER = 4u;
static const uint SVT_TILE_SIZE_PADDED = SVT_TILE_SIZE + SVT_TILE_BORDER * 2;
static const uint SVT_PACKED_MIP_COUNT = 6;
static const uint2 SVT_PACKED_MIP_OFFSETS[SVT_PACKED_MIP_COUNT] = {
	uint2(0, 0),
	uint2(SVT_TILE_SIZE / 2 + SVT_TILE_BORDER * 2, 0),
	uint2(SVT_TILE_SIZE / 2 + SVT_TILE_BORDER * 2 + SVT_TILE_SIZE / 4 + SVT_TILE_BORDER * 2, 0),
	// shift the offset down a bit to not go over the tile limit:
	uint2(SVT_TILE_SIZE / 2 + SVT_TILE_BORDER * 2, SVT_TILE_SIZE / 4 + SVT_TILE_BORDER * 2),
	uint2(SVT_TILE_SIZE / 2 + SVT_TILE_BORDER * 2 + SVT_TILE_SIZE / 16 + SVT_TILE_BORDER * 2, SVT_TILE_SIZE / 4 + SVT_TILE_BORDER * 2),
	uint2(SVT_TILE_SIZE / 2 + SVT_TILE_BORDER * 2 + SVT_TILE_SIZE / 16 + SVT_TILE_BORDER * 2 + SVT_TILE_SIZE / 32 + SVT_TILE_BORDER * 2, SVT_TILE_SIZE / 4 + SVT_TILE_BORDER * 2),
};

#ifndef __cplusplus
#ifdef TEXTURE_SLOT_NONUNIFORM
#define UniformTextureSlot(x) NonUniformResourceIndex(x)
#else
#define UniformTextureSlot(x) (x)
#endif // TEXTURE_SLOT_NONUNIFORM

inline float get_lod(in uint2 dim, in float2 uv_dx, in float2 uv_dy)
{
	// https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#7.18.11%20LOD%20Calculations
	return log2(max(length(uv_dx * dim), length(uv_dy * dim)));
}
#endif // __cplusplus

struct ShaderTextureSlot
{
	uint uvset_lodclamp;
	int texture_descriptor;
	int sparse_residencymap_descriptor;
	int sparse_feedbackmap_descriptor;

	inline void init()
	{
		uvset_lodclamp = 0;
		texture_descriptor = -1;
		sparse_residencymap_descriptor = -1;
		sparse_feedbackmap_descriptor = -1;
	}

	inline bool IsValid()
	{
		return texture_descriptor >= 0;
	}
	inline uint GetUVSet()
	{
		return uvset_lodclamp & 1u;
	}

#ifndef __cplusplus
	inline float GetLodClamp()
	{
		return f16tof32((uvset_lodclamp >> 1u) & 0xFFFF);
	}
	Texture2D GetTexture()
	{
		return bindless_textures[UniformTextureSlot(texture_descriptor)];
	}
	float4 SampleVirtual(
		in Texture2D tex,
		in SamplerState sam,
		in float2 uv,
		in Texture2D<uint> residency_map,
		in uint2 virtual_tile_count,
		in uint2 virtual_image_dim,
		in float virtual_lod
	)
	{
#ifdef SVT_FEEDBACK
		[branch]
		if (sparse_feedbackmap_descriptor >= 0)
		{
			RWTexture2D<uint> feedback_map = bindless_rwtextures_uint[UniformTextureSlot(sparse_feedbackmap_descriptor)];
			uint2 pixel = uv * virtual_tile_count;
			InterlockedMin(feedback_map[pixel], uint(virtual_lod));
		}
#endif // SVT_FEEDBACK

		float2 atlas_dim;
		tex.GetDimensions(atlas_dim.x, atlas_dim.y);
		const float2 atlas_dim_rcp = rcp(atlas_dim);

		const uint max_nonpacked_lod = uint(GetLodClamp());
		virtual_lod = min(virtual_lod, max_nonpacked_lod + SVT_PACKED_MIP_COUNT);
		bool packed_mips = uint(virtual_lod) > max_nonpacked_lod;

		uint2 pixel = uv * virtual_tile_count;
		uint residency = residency_map.Load(uint3(pixel >> uint(virtual_lod), min(max_nonpacked_lod, uint(virtual_lod))));
		uint2 tile = packed_mips ? uint2((residency >> 16u) & 0xFF, (residency >> 24u) & 0xFF) : uint2(residency & 0xFF, (residency >> 8u) & 0xFF);
		float clamped_lod = virtual_lod < max_nonpacked_lod ? max(virtual_lod, (residency >> 16u) & 0xFF) : virtual_lod;

		// Mip - more detailed:
		float4 value0;
		{
			uint lod0 = floor(clamped_lod);
			const uint packed_mip_idx = packed_mips ? uint(virtual_lod - max_nonpacked_lod - 1) : 0;
			uint2 tile_pixel_upperleft = tile * SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER + SVT_PACKED_MIP_OFFSETS[packed_mip_idx];
			uint2 virtual_lod_dim = max(4u.xx, virtual_image_dim >> lod0);
			float2 virtual_pixel = uv * virtual_lod_dim;
			float2 virtual_tile_pixel = fmod(virtual_pixel, SVT_TILE_SIZE);
			float2 atlas_tile_pixel = tile_pixel_upperleft + 0.5 + virtual_tile_pixel;
			float2 atlas_uv = atlas_tile_pixel * atlas_dim_rcp;
			value0 = tex.SampleLevel(sam, atlas_uv, 0);
		}

		// Mip - less detailed:
		float4 value1;
		{
			uint lod1 = ceil(clamped_lod);
			packed_mips = uint(lod1) > max_nonpacked_lod;
			const uint packed_mip_idx = packed_mips ? uint(lod1 - max_nonpacked_lod - 1) : 0;
			residency = residency_map.Load(uint3(pixel >> lod1, min(max_nonpacked_lod, lod1)));
			tile = packed_mips ? uint2((residency >> 16u) & 0xFF, (residency >> 24u) & 0xFF) : uint2(residency & 0xFF, (residency >> 8u) & 0xFF);
			uint2 tile_pixel_upperleft = tile * SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER + SVT_PACKED_MIP_OFFSETS[packed_mip_idx];
			uint2 virtual_lod_dim = max(4u.xx, virtual_image_dim >> lod1);
			float2 virtual_pixel = uv * virtual_lod_dim;
			float2 virtual_tile_pixel = fmod(virtual_pixel, SVT_TILE_SIZE);
			float2 atlas_tile_pixel = tile_pixel_upperleft + 0.5 + virtual_tile_pixel;
			float2 atlas_uv = atlas_tile_pixel * atlas_dim_rcp;
			value1 = tex.SampleLevel(sam, atlas_uv, 0);
		}

		return lerp(value0, value1, frac(virtual_lod)); // custom trilinear filtering
	}
	float4 Sample(in SamplerState sam, in float4 uvsets)
	{
		Texture2D tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint> residency_map = bindless_textures_uint[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			float virtual_lod = get_lod(virtual_image_dim, ddx(uv), ddy(uv));
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, virtual_lod);
		}
#endif // DISABLE_SVT

		return tex.Sample(sam, uv);
	}

	float4 SampleLevel(in SamplerState sam, in float4 uvsets, in float lod)
	{
		Texture2D tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint> residency_map = bindless_textures_uint[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, lod);
		}
#endif // DISABLE_SVT

		return tex.SampleLevel(sam, uv, lod);
	}

	float4 SampleBias(in SamplerState sam, in float4 uvsets, in float bias)
	{
		Texture2D tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint> residency_map = bindless_textures_uint[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			float virtual_lod = get_lod(virtual_image_dim, ddx(uv), ddy(uv));
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, virtual_lod + bias);
		}
#endif // DISABLE_SVT

		return tex.SampleBias(sam, uv, bias);
	}

	float4 SampleGrad(in SamplerState sam, in float4 uvsets, in float4 uvsets_dx, in float4 uvsets_dy)
	{
		Texture2D tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;
		float2 uv_dx = GetUVSet() == 0 ? uvsets_dx.xy : uvsets_dx.zw;
		float2 uv_dy = GetUVSet() == 0 ? uvsets_dy.xy : uvsets_dy.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint> residency_map = bindless_textures_uint[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			float virtual_lod = get_lod(virtual_image_dim, uv_dx, uv_dy);
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, virtual_lod);
		}
#endif // DISABLE_SVT

		return tex.SampleGrad(sam, uv, uv_dx, uv_dy);
	}
#endif // __cplusplus
};

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
	uint		emissive_r11g11b10;
	uint		specular_r11g11b10;
	uint		sheenColor_r11g11b10;

	float		sheenRoughness;
	float		clearcoat;
	float		clearcoatRoughness;
	uint		stencilRef;

	float		anisotropy_strength;
	float		anisotropy_rotation_sin;
	float		anisotropy_rotation_cos;
	float		padding0;

	int			sampler_descriptor;
	uint		options;
	uint		layerMask;
	uint		shaderType;

	uint4		userdata;

	ShaderTextureSlot textures[TEXTURESLOT_COUNT];

	void init()
	{
		baseColor = float4(1, 1, 1, 1);
		subsurfaceScattering = float4(0, 0, 0, 0);
		subsurfaceScattering_inv = float4(0, 0, 0, 0);
		texMulAdd = float4(1, 1, 0, 0);

		roughness = 0;
		reflectance = 0;
		metalness = 0;
		refraction = 0;

		normalMapStrength = 0;
		parallaxOcclusionMapping = 0;
		alphaTest = 0;
		displacementMapping = 0;

		transmission = 0;
		options = 0u;
		emissive_r11g11b10 = 0;
		specular_r11g11b10 = 0;

		layerMask = ~0u;
		sheenColor_r11g11b10 = 0;
		sheenRoughness = 0;

		clearcoat = 0;
		clearcoatRoughness = 0;
		stencilRef = 0;
		shaderType = 0;

		userdata = uint4(0, 0, 0, 0);

		sampler_descriptor = -1;

		for (int i = 0; i < TEXTURESLOT_COUNT; ++i)
		{
			textures[i].init();
		}
	}

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

// For binning shading based on shader types:
struct ShaderTypeBin
{
	uint dispatchX;
	uint dispatchY;
	uint dispatchZ;
	uint shaderType;
#if defined(__SCE__) || defined(__PSSL_)
	uint4 padding; // 32-byte alignment
#endif // __SCE__ || __PSSL__
};
static const uint SHADERTYPE_BIN_COUNT = 10;

struct VisibilityTile
{
	uint visibility_tile_id;
	uint entity_flat_tile_index;
	uint execution_mask_0;
	uint execution_mask_1;

	inline bool check_thread_valid(uint groupIndex)
	{
		if (groupIndex < 32)
		{
			return execution_mask_0 & (1u << groupIndex);
		}
		return execution_mask_1 & (1u << (groupIndex - 32u));
	}
};

static const uint SHADERMESH_FLAG_DOUBLE_SIDED = 1 << 0;
static const uint SHADERMESH_FLAG_HAIRPARTICLE = 1 << 1;
static const uint SHADERMESH_FLAG_EMITTEDPARTICLE = 1 << 2;

// This is equivalent to a Mesh + MeshSubset
//	But because these are always loaded toghether by shaders, they are unrolled into one to reduce individual buffer loads
struct ShaderGeometry
{
	int ib;
	int vb_pos_wind;
	int vb_uvs;
	int vb_nor;

	int vb_tan;
	int vb_col;
	int vb_atl;
	int vb_pre;

	uint materialIndex;
	uint meshletOffset; // offset of this subset in meshlets (locally within the mesh)
	uint meshletCount;
	int impostorSliceOffset;

	float3 aabb_min;
	uint flags;
	float3 aabb_max;
	float tessellation_factor;

	float2 uv_range_min;
	float2 uv_range_max;

	uint indexOffset;
	uint indexCount;
	int padding0;
	int padding1;

	void init()
	{
		ib = -1;
		vb_pos_wind = -1;
		vb_uvs = -1;

		vb_nor = -1;
		vb_tan = -1;
		vb_col = -1;
		vb_atl = -1;

		vb_pre = -1;
		materialIndex = 0;
		meshletOffset = 0;
		meshletCount = 0;

		aabb_min = float3(0, 0, 0);
		flags = 0;
		aabb_max = float3(0, 0, 0);
		tessellation_factor = 0;

		uv_range_min = float2(0, 0);
		uv_range_max = float2(1, 1);

		impostorSliceOffset = -1;
		indexOffset = 0;
		indexCount = 0;
	}
};

// 256 triangle batch of a ShaderGeometry
static const uint MESHLET_TRIANGLE_COUNT = 256u;
inline uint triangle_count_to_meshlet_count(uint triangleCount)
{
	return (triangleCount + MESHLET_TRIANGLE_COUNT - 1u) / MESHLET_TRIANGLE_COUNT;
}
struct ShaderMeshlet
{
	uint instanceIndex;
	uint geometryIndex;
	uint primitiveOffset;
	uint padding;
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
	uint flags;	// high 8 bits: user stencilRef
	uint layerMask;
	uint geometryOffset;	// offset of all geometries for currently active LOD

	uint geometryCount;		// number of all geometries in currently active LOD
	uint color;
	uint emissive;
	int lightmap;

	uint meshletOffset; // offset in the global meshlet buffer for first subset (for LOD0)
	float fadeDistance;
	uint baseGeometryOffset;	// offset of all geometries of the instance (if no LODs, then it is equal to geometryOffset)
	uint baseGeometryCount;		// number of all geometries of the instance (if no LODs, then it is equal to geometryCount)

	float3 center;
	float radius;

	ShaderTransform transform;
	ShaderTransform transformInverseTranspose; // This correctly handles non uniform scaling for normals
	ShaderTransform transformPrev;

	void init()
	{
		uid = 0;
		flags = 0;
		layerMask = 0;
		color = ~0u;
		emissive = ~0u;
		lightmap = -1;
		geometryOffset = 0;
		geometryCount = 0;
		baseGeometryOffset = 0;
		baseGeometryCount = 0;
		meshletOffset = ~0u;
		fadeDistance = 0;
		center = float3(0, 0, 0);
		radius = 0;
		transform.init();
		transformInverseTranspose.init();
		transformPrev.init();
	}

	inline void SetUserStencilRef(uint stencilRef)
	{
		flags |= (stencilRef & 0xFF) << 24u;
	}
	inline uint GetUserStencilRef()
	{
		return flags >> 24u;
	}
};
struct ShaderMeshInstancePointer
{
	uint data;

	void init()
	{
		data = 0;
	}
	void Create(uint _instanceIndex, uint camera_index = 0, float dither = 0)
	{
		data = 0;
		data |= _instanceIndex & 0xFFFFFF;
		data |= (camera_index & 0xF) << 24u;
		data |= (uint(dither * 15.0f) & 0xF) << 28u;
	}
	uint GetInstanceIndex()
	{
		return data & 0xFFFFFF;
	}
	uint GetCameraIndex()
	{
		return (data >> 24u) & 0xF;
	}
	float GetDither()
	{
		return float((data >> 28u) & 0xF) / 15.0f;
	}
};

struct ObjectPushConstants
{
	uint geometryIndex;
	uint materialIndex;
	int instances;
	uint instance_offset;
};


// Warning: the size of this structure directly affects shader performance.
//	Try to reduce it as much as possible!
//	Keep it aligned to 16 bytes for best performance!
struct ShaderEntity
{
	float3 position;
	uint type8_flags8_range16;

	uint2 direction16_coneAngleCos16; // coneAngleCos is used for cascade count in directional light
	uint2 color; // half4 packed

	uint layerMask;
	uint indices;
	uint remap;
	uint radius16_length16;

	float4 shadowAtlasMulAdd;

#ifndef __cplusplus
	// Shader-side:
	inline uint GetType()
	{
		return type8_flags8_range16 & 0xFF;
	}
	inline uint GetFlags()
	{
		return (type8_flags8_range16 >> 8u) & 0xFF;
	}
	inline float GetRange()
	{
		return f16tof32(type8_flags8_range16 >> 16u);
	}
	inline float GetRadius()
	{
		return f16tof32(radius16_length16);
	}
	inline float GetLength()
	{
		return f16tof32(radius16_length16 >> 16u);
	}
	inline float3 GetDirection()
	{
		return normalize(float3(
			f16tof32(direction16_coneAngleCos16.x),
			f16tof32(direction16_coneAngleCos16.x >> 16u),
			f16tof32(direction16_coneAngleCos16.y)
		));
	}
	inline float GetConeAngleCos()
	{
		return f16tof32(direction16_coneAngleCos16.y >> 16u);
	}
	inline uint GetShadowCascadeCount()
	{
		return direction16_coneAngleCos16.y >> 16u;
	}
	inline float GetAngleScale()
	{
		return f16tof32(remap);
	}
	inline float GetAngleOffset()
	{
		return f16tof32(remap >> 16u);
	}
	inline float GetCubemapDepthRemapNear()
	{
		return f16tof32(remap);
	}
	inline float GetCubemapDepthRemapFar()
	{
		return f16tof32(remap >> 16u);
	}
	inline float4 GetColor()
	{
		float4 retVal;
		retVal.x = f16tof32(color.x);
		retVal.y = f16tof32(color.x >> 16u);
		retVal.z = f16tof32(color.y);
		retVal.w = f16tof32(color.y >> 16u);
		return retVal;
	}
	inline uint GetMatrixIndex()
	{
		return indices & 0xFFFF;
	}
	inline uint GetTextureIndex()
	{
		return indices >> 16u;
	}
	inline bool IsCastingShadow()
	{
		return indices != ~0;
	}
	inline float GetGravity()
	{
		return GetConeAngleCos();
	}
	inline float3 GetColliderTip()
	{
		return shadowAtlasMulAdd.xyz;
	}

#else
	// Application-side:
	inline void SetType(uint type)
	{
		type8_flags8_range16 |= type & 0xFF;
	}
	inline void SetFlags(uint flags)
	{
		type8_flags8_range16 |= (flags & 0xFF) << 8u;
	}
	inline void SetRange(float value)
	{
		type8_flags8_range16 |= XMConvertFloatToHalf(value) << 16u;
	}
	inline void SetRadius(float value)
	{
		radius16_length16 |= XMConvertFloatToHalf(value);
	}
	inline void SetLength(float value)
	{
		radius16_length16 |= XMConvertFloatToHalf(value) << 16u;
	}
	inline void SetColor(float4 value)
	{
		color.x |= XMConvertFloatToHalf(value.x);
		color.x |= XMConvertFloatToHalf(value.y) << 16u;
		color.y |= XMConvertFloatToHalf(value.z);
		color.y |= XMConvertFloatToHalf(value.w) << 16u;
	}
	inline void SetDirection(float3 value)
	{
		direction16_coneAngleCos16.x |= XMConvertFloatToHalf(value.x);
		direction16_coneAngleCos16.x |= XMConvertFloatToHalf(value.y) << 16u;
		direction16_coneAngleCos16.y |= XMConvertFloatToHalf(value.z);
	}
	inline void SetConeAngleCos(float value)
	{
		direction16_coneAngleCos16.y |= XMConvertFloatToHalf(value) << 16u;
	}
	inline void SetShadowCascadeCount(uint value)
	{
		direction16_coneAngleCos16.y |= (value & 0xFFFF) << 16u;
	}
	inline void SetAngleScale(float value)
	{
		remap |= XMConvertFloatToHalf(value);
	}
	inline void SetAngleOffset(float value)
	{
		remap |= XMConvertFloatToHalf(value) << 16u;
	}
	inline void SetCubeRemapNear(float value)
	{
		remap |= XMConvertFloatToHalf(value);
	}
	inline void SetCubeRemapFar(float value)
	{
		remap |= XMConvertFloatToHalf(value) << 16u;
	}
	inline void SetIndices(uint matrixIndex, uint textureIndex)
	{
		indices = matrixIndex & 0xFFFF;
		indices |= (textureIndex & 0xFFFF) << 16u;
	}
	inline void SetGravity(float value)
	{
		SetConeAngleCos(value);
	}
	inline void SetColliderTip(float3 value)
	{
		shadowAtlasMulAdd = float4(value.x, value.y, value.z, 0);
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

enum SHADER_ENTITY_TYPE
{
	ENTITY_TYPE_DIRECTIONALLIGHT,
	ENTITY_TYPE_POINTLIGHT,
	ENTITY_TYPE_SPOTLIGHT,
	ENTITY_TYPE_DECAL,
	ENTITY_TYPE_ENVMAP,
	ENTITY_TYPE_FORCEFIELD_POINT,
	ENTITY_TYPE_FORCEFIELD_PLANE,
	ENTITY_TYPE_COLLIDER_SPHERE,
	ENTITY_TYPE_COLLIDER_CAPSULE,
	ENTITY_TYPE_COLLIDER_PLANE,

	ENTITY_TYPE_COUNT
};

static const uint ENTITY_FLAG_LIGHT_STATIC = 1 << 0;
static const uint ENTITY_FLAG_LIGHT_VOLUMETRICCLOUDS = 1 << 1;

static const uint ENTITY_FLAG_DECAL_BASECOLOR_ONLY_ALPHA = 1 << 0;

static const uint SHADER_ENTITY_COUNT = 256;
static const uint SHADER_ENTITY_TILE_BUCKET_COUNT = SHADER_ENTITY_COUNT / 32;

static const uint MATRIXARRAY_COUNT = SHADER_ENTITY_COUNT;
static const uint MAX_SHADER_DECAL_COUNT = 128;
static const uint MAX_SHADER_PROBE_COUNT = 32;

static const uint TILED_CULLING_BLOCKSIZE = 16;
static const uint TILED_CULLING_THREADSIZE = 8;
static const uint TILED_CULLING_GRANULARITY = TILED_CULLING_BLOCKSIZE / TILED_CULLING_THREADSIZE;

static const uint VISIBILITY_BLOCKSIZE = 8;
static const uint VISIBILITY_TILED_CULLING_GRANULARITY = TILED_CULLING_BLOCKSIZE / VISIBILITY_BLOCKSIZE;

static const int impostorCaptureAngles = 36;

// These option bits can be read from options constant buffer value:
static const uint OPTION_BIT_TEMPORALAA_ENABLED = 1 << 0;
static const uint OPTION_BIT_TRANSPARENTSHADOWS_ENABLED = 1 << 1;
static const uint OPTION_BIT_VXGI_ENABLED = 1 << 2;
static const uint OPTION_BIT_VXGI_REFLECTIONS_ENABLED = 1 << 3;
static const uint OPTION_BIT_REALISTIC_SKY = 1 << 6;
static const uint OPTION_BIT_HEIGHT_FOG = 1 << 7;
static const uint OPTION_BIT_RAYTRACED_SHADOWS = 1 << 8;
static const uint OPTION_BIT_SHADOW_MASK = 1 << 9;
static const uint OPTION_BIT_SURFELGI_ENABLED = 1 << 10;
static const uint OPTION_BIT_DISABLE_ALBEDO_MAPS = 1 << 11;
static const uint OPTION_BIT_FORCE_DIFFUSE_LIGHTING = 1 << 12;
static const uint OPTION_BIT_VOLUMETRICCLOUDS_CAST_SHADOW = 1 << 13;
static const uint OPTION_BIT_OVERRIDE_FOG_COLOR = 1 << 14;
static const uint OPTION_BIT_STATIC_SKY_SPHEREMAP = 1 << 15;
static const uint OPTION_BIT_REALISTIC_SKY_AERIAL_PERSPECTIVE = 1 << 16;
static const uint OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY = 1 << 17;
static const uint OPTION_BIT_REALISTIC_SKY_RECEIVE_SHADOW = 1 << 18;
static const uint OPTION_BIT_VOLUMETRICCLOUDS_RECEIVE_SHADOW = 1 << 19;

// ---------- Common Constant buffers: -----------------

struct FrameCB
{
	uint		options;					// wi::renderer bool options packed into bitmask (OPTION_BIT_ values)
	float		time;
	float		time_previous;
	float		delta_time;

	uint		frame_count;
	uint		temporalaa_samplerotation;
	int			texture_shadowatlas_index;
	int			texture_shadowatlas_transparent_index;

	uint2		shadow_atlas_resolution;
	float2		shadow_atlas_resolution_rcp;

	float4x4	cloudShadowLightSpaceMatrix;
	float4x4	cloudShadowLightSpaceMatrixInverse;

	float		cloudShadowFarPlaneKm;
	int			texture_volumetricclouds_shadow_index;
	float		gi_boost;
	int			padding0;

	uint		lightarray_offset;			// indexing into entity array
	uint		lightarray_count;			// indexing into entity array
	uint		decalarray_offset;			// indexing into entity array
	uint		decalarray_count;			// indexing into entity array

	uint		forcefieldarray_offset;		// indexing into entity array
	uint		forcefieldarray_count;		// indexing into entity array
	uint		envprobearray_offset;		// indexing into entity array
	uint		envprobearray_count;		// indexing into entity array

	float		blue_noise_phase;
	int			texture_random64x64_index;
	int			texture_bluenoise_index;
	int			texture_sheenlut_index;

	int			texture_skyviewlut_index;
	int			texture_transmittancelut_index;
	int			texture_multiscatteringlut_index;
	int			texture_skyluminancelut_index;

	int			texture_cameravolumelut_index;
	int			texture_wind_index;
	int			texture_wind_prev_index;
	int			buffer_entity_index;

	float4		rain_blocker_mad;
	float4x4	rain_blocker_matrix;
	float4x4	rain_blocker_matrix_inverse;

	float4		rain_blocker_mad_prev;
	float4x4	rain_blocker_matrix_prev;
	float4x4	rain_blocker_matrix_inverse_prev;

	ShaderScene scene;

	VXGI vxgi;
};

struct ShaderCamera
{
	float4x4	view_projection;

	float3		position;
	uint		output_index; // viewport or rendertarget array index

	float4		clip_plane;
	float4		reflection_plane; // not clip plane (not reversed when camera is under), but the original plane

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
	float4x4	reflection_inverse_view_projection;
	float4x4	reprojection; // view_projection_inverse_matrix * previous_view_projection_matrix

	float2		aperture_shape;
	float		aperture_size;
	float		focal_length;

	float2 canvas_size;
	float2 canvas_size_rcp;
		   
	uint2 internal_resolution;
	float2 internal_resolution_rcp;

	uint4 scissor; // scissor in physical coordinates (left,top,right,bottom) range: [0, internal_resolution]
	float4 scissor_uv; // scissor in screen UV coordinates (left,top,right,bottom) range: [0, 1]

	uint3 entity_culling_tilecount;
	uint sample_count;

	uint2 visibility_tilecount;
	uint visibility_tilecount_flat;
	float distance_from_origin;

	int texture_rtdiffuse_index;
	int texture_primitiveID_index;
	int texture_depth_index;
	int texture_lineardepth_index;

	int texture_velocity_index;
	int texture_normal_index;
	int texture_roughness_index;
	int buffer_entitytiles_opaque_index;

	int buffer_entitytiles_transparent_index;
	int texture_reflection_index;
	int texture_reflection_depth_index;
	int texture_refraction_index;

	int texture_waterriples_index;
	int texture_ao_index;
	int texture_ssr_index;
	int texture_rtshadow_index;

	int texture_surfelgi_index;
	int texture_depth_index_prev;
	int texture_vxgi_diffuse_index;
	int texture_vxgi_specular_index;

#ifdef __cplusplus
	void init()
	{
		view_projection = {};
		position = {};
		output_index = 0;
		clip_plane = {};
		forward = {};
		z_near = {};
		up = {};
		z_far = {};
		z_near_rcp = {};
		z_far_rcp = {};
		z_range = {};
		z_range_rcp = {};
		view = {};
		projection = {};
		inverse_view = {};
		inverse_projection = {};
		inverse_view_projection = {};
		frustum = {};
		temporalaa_jitter = {};
		temporalaa_jitter_prev = {};
		previous_view = {};
		previous_projection = {};
		previous_view_projection = {};
		previous_inverse_view_projection = {};
		reflection_view_projection = {};
		reprojection = {};
		aperture_shape = {};
		aperture_size = {};
		focal_length = {};
		canvas_size = {};
		canvas_size_rcp = {};
		internal_resolution = {};
		internal_resolution_rcp = {};
		scissor = {};
		scissor_uv = {};
		entity_culling_tilecount = {};
		sample_count = {};
		visibility_tilecount = {};
		visibility_tilecount_flat = {};
		distance_from_origin = {};

		texture_rtdiffuse_index = -1;
		texture_primitiveID_index = -1;
		texture_depth_index = -1;
		texture_lineardepth_index = -1;
		texture_velocity_index = -1;
		texture_normal_index = -1;
		texture_roughness_index = -1;
		buffer_entitytiles_opaque_index = -1;
		buffer_entitytiles_transparent_index = -1;
		texture_reflection_index = -1;
		texture_refraction_index = -1;
		texture_waterriples_index = -1;
		texture_ao_index = -1;
		texture_ssr_index = -1;
		texture_rtshadow_index = -1;
		texture_surfelgi_index = -1;
		texture_depth_index_prev = -1;
		texture_vxgi_diffuse_index = -1;
		texture_vxgi_specular_index = -1;
	}

#else
	inline float2 clamp_uv_to_scissor(in float2 uv)
	{
		return float2(
			clamp(uv.x, scissor_uv.x, scissor_uv.z),
			clamp(uv.y, scissor_uv.y, scissor_uv.w)
		);
	}
	inline float2 clamp_pixel_to_scissor(in uint2 pixel)
	{
		return uint2(
			clamp(pixel.x, scissor.x, scissor.z),
			clamp(pixel.y, scissor.y, scissor.w)
		);
	}
	inline bool is_uv_inside_scissor(float2 uv)
	{
		return uv.x >= scissor_uv.x && uv.x <= scissor_uv.z && uv.y >= scissor_uv.y && uv.y <= scissor_uv.w;
	}
	inline bool is_pixel_inside_scissor(uint2 pixel)
	{
		return pixel.x >= scissor.x && pixel.x <= scissor.z && pixel.y >= scissor.y && pixel.y <= scissor.w;
	}
#endif // __cplusplus
};

struct CameraCB
{
	ShaderCamera cameras[16];

#ifdef __cplusplus
	void init()
	{
		for (int i = 0; i < 16; ++i)
		{
			cameras[i].init();
		}
	}
#endif // __cplusplus
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
	float4x4 xLightWorld;
	float4 xLightColor;
	float4 xLightEnerdis;
};

struct LensFlarePush
{
	float3 xLensFlarePos;
	float xLensFlareOffset;
	float2 xLensFlareSize;
	float2 xLensFlare_padding;
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
	int padding;
};
static const uint MIPGEN_OPTION_BIT_PRESERVE_COVERAGE = 1 << 0;
static const uint MIPGEN_OPTION_BIT_SRGB = 1 << 1;

struct FilterEnvmapPushConstants
{
	uint2 filterResolution;
	float2 filterResolution_rcp;

	float filterRoughness;
	uint filterRayCount;
	uint padding_filterCB;
	int texture_input;

	int texture_output;
	int padding0;
	int padding1;
	int padding2;
};

// CopyTexture2D params:
enum COPY_TEXTURE_FLAGS
{
	COPY_TEXTURE_WRAP = 1 << 0,
	COPY_TEXTURE_SRGB = 1 << 1
};
struct CopyTextureCB
{
	int2 xCopyDst;
	int2 xCopySrc;
	int2 xCopySrcSize;
	int  xCopySrcMIP;
	int  xCopyFlags;
};


static const uint PAINT_TEXTURE_BLOCKSIZE = 8;

struct PaintTextureCB
{
	uint2 xPaintBrushCenter;
	uint xPaintBrushRadius;
	float xPaintBrushAmount;

	float xPaintBrushSmoothness;
	uint xPaintBrushColor;
	uint xPaintReveal;
	float xPaintBrushRotation;

	uint xPaintBrushShape;
	uint padding0;
	uint padding1;
	uint padding2;
};

CBUFFER(PaintRadiusCB, CBSLOT_RENDERER_MISC)
{
	uint2 xPaintRadResolution;
	uint2 xPaintRadCenter;

	uint xPaintRadUVSET;
	float xPaintRadRadius;
	float xPaintRadBrushRotation;
	uint xPaintRadBrushShape;
};

struct SkinningPushConstants
{
	int vb_pos_wind;
	int vb_nor;
	int vb_tan;
	int so_pos;

	int so_nor;
	int so_tan;
	int vb_bon;
	int morphvb_index;

	int skinningbuffer_index;
	uint bone_offset;
	uint morph_offset;
	uint morph_count;

	float3 aabb_min;
	uint vertexCount;

	float3 aabb_max;
	float padding;
};

struct DebugObjectPushConstants
{
	int vb_pos_wind;
};

struct LightmapPushConstants
{
	int vb_pos_wind;
	int vb_nor;
	int vb_atl;
	uint instanceIndex;
};

struct MorphTargetGPU
{
	float weight;
	uint offset_pos;
	uint offset_nor;
	uint offset_tan;
};

struct AerialPerspectiveCapturePushConstants
{
	uint2 resolution;
	float2 resolution_rcp;

	int texture_input;
	int texture_output;
	float padding0;
	float padding1;
};

struct VolumetricCloudCapturePushConstants
{
	uint2 resolution;
	float2 resolution_rcp;

	int texture_input;
	int texture_output;
	int maxStepCount;
	float LODMin;

	float shadowSampleCount;
	float groundContributionSampleCount;
	float padding0;
	float padding1;
};


struct TerrainVirtualTexturePush
{
	int2 offset;
	uint2 write_offset;
	uint write_size;
	float resolution_rcp;
	int region_weights_textureRO;
	int padding;
};
struct VirtualTextureResidencyUpdateCB
{
	uint lodCount;
	uint width;
	uint height;
	int pageBufferRO;

	int4 residencyTextureRW_mips[9]; // because this can be indexed in shader, this structure cannot be push constant, it has to be constant buffer!
};
struct VirtualTextureTileAllocatePush
{
	uint threadCount;
	uint lodCount;
	uint width;
	uint height;
	int pageBufferRO;
	int requestBufferRW;
	int allocationBufferRW;
};
struct VirtualTextureTileRequestsPush
{
	uint lodCount;
	uint width;
	uint height;
	int feedbackTextureRO;
	int requestBufferRW;
	int padding0;
	int padding1;
	int padding2;
};


#endif // WI_SHADERINTEROP_RENDERER_H
