#ifndef WI_SHADERINTEROP_RENDERER_H
#define WI_SHADERINTEROP_RENDERER_H
#include "ShaderInterop.h"
#include "ShaderInterop_Weather.h"
#include "ShaderInterop_VXGI.h"
#include "ShaderInterop_Terrain.h"
#include "ShaderInterop_VoxelGrid.h"

struct alignas(16) ShaderScene
{
	int instancebuffer;
	int geometrybuffer;
	int materialbuffer;
	int meshletbuffer;

	int texturestreamingbuffer;
	int globalenvmap; // static sky, not guaranteed to be cubemap, mipmaps or format, just whatever is imported
	int globalprobe; // rendered probe with guaranteed mipmaps, hdr, etc.
	int impostorInstanceOffset;

	int TLAS;
	int BVH_counter;
	int BVH_nodes;
	int BVH_primitives;

	float3 aabb_min;
	uint instanceCount;
	float3 aabb_max;
	uint geometryCount;
	float3 aabb_extents;		// enclosing AABB abs(max - min)
	uint materialCount;
	float3 aabb_extents_rcp;	// enclosing AABB 1.0f / abs(max - min)
	float padding6;

	ShaderWeather weather;

	struct alignas(16) DDGI
	{
		uint3 grid_dimensions;
		uint probe_count;

		uint2 depth_texture_resolution;
		float2 depth_texture_resolution_rcp;

		float3 grid_min;
		int probe_buffer;

		float3 grid_extents;
		int depth_texture;

		float3 cell_size;
		float max_distance;

		float3 grid_extents_rcp;
		float padding;

		float3 cell_size_rcp;
		float smooth_backface;
	};
	DDGI ddgi;

	ShaderTerrain terrain;

	ShaderVoxelGrid voxelgrid;
};

enum SHADERMATERIAL_OPTIONS
{
	SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS = 1 << 0,
	SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW = 1 << 1,
	SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY = 1 << 2,
	SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY = 1 << 3,
	SHADERMATERIAL_OPTION_BIT_USE_WIND = 1 << 4,
	SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW = 1 << 5,
	SHADERMATERIAL_OPTION_BIT_CAST_SHADOW = 1 << 6,
	SHADERMATERIAL_OPTION_BIT_DOUBLE_SIDED = 1 << 7,
	SHADERMATERIAL_OPTION_BIT_TRANSPARENT = 1 << 8,
	SHADERMATERIAL_OPTION_BIT_ADDITIVE = 1 << 9,
	SHADERMATERIAL_OPTION_BIT_USE_VERTEXAO = 1 << 10,
	SHADERMATERIAL_OPTION_BIT_CAPSULE_SHADOW_DISABLED = 1 << 11,
};

#ifndef __cplusplus
enum SHADERTYPE
{
	SHADERTYPE_PBR,
	SHADERTYPE_PBR_PLANARREFLECTION,
	SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING,
	SHADERTYPE_PBR_ANISOTROPIC,
	SHADERTYPE_WATER,
	SHADERTYPE_CARTOON,
	SHADERTYPE_UNLIT,
	SHADERTYPE_PBR_CLOTH,
	SHADERTYPE_PBR_CLEARCOAT,
	SHADERTYPE_PBR_CLOTH_CLEARCOAT,
	SHADERTYPE_PBR_TERRAINBLENDED,
	SHADERTYPE_INTERIORMAPPING,
	SHADERTYPE_COUNT
};
#endif // __cplusplus

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
	TRANSPARENCYMAP,

	TEXTURESLOT_COUNT
};

static const float SVT_MIP_BIAS = 0.5;
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
#define UniformTextureSlot(x) NonUniformResourceIndex(descriptor_index(x))
#else
#define UniformTextureSlot(x) descriptor_index(x)
#endif // TEXTURE_SLOT_NONUNIFORM

inline float get_lod(in uint2 dim, in float2 uv_dx, in float2 uv_dy, uint anisotropy = 0)
{
	// LOD calculations DirectX specs: https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#7.18.11%20LOD%20Calculations

	// Isotropic:
	if (anisotropy == 0)
		return log2(max(length(uv_dx * dim), length(uv_dy * dim)));

	// Anisotropic:
	uv_dx *= dim;
	uv_dy *= dim;
	float squaredLengthX = dot(uv_dx, uv_dx);
	float squaredLengthY = dot(uv_dy, uv_dy);
	float determinant = abs(uv_dx.x * uv_dy.y - uv_dx.y * uv_dy.x);
	bool isMajorX = squaredLengthX > squaredLengthY;
	float squaredLengthMajor = isMajorX ? squaredLengthX : squaredLengthY;
	float lengthMajor = sqrt(squaredLengthMajor);
	float ratioOfAnisotropy = squaredLengthMajor / determinant;

	float lengthMinor = 0;
	if (ratioOfAnisotropy > anisotropy)
	{
		// ratio is clamped - LOD is based on ratio (preserves area)
		lengthMinor = lengthMajor / anisotropy;
	}
	else
	{
		// ratio not clamped - LOD is based on area
		lengthMinor = determinant / lengthMajor;
	}

	return log2(lengthMinor);
}
#endif // __cplusplus

struct alignas(16) ShaderTextureSlot
{
	uint uvset_aniso_lodclamp;
	int texture_descriptor;
	int sparse_residencymap_descriptor;
	int sparse_feedbackmap_descriptor;

	inline void init()
	{
		uvset_aniso_lodclamp = 0;
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
		return uvset_aniso_lodclamp & 1u;
	}
	inline uint GetAnisotropy()
	{
		return (uvset_aniso_lodclamp >> 1u) & 0x7F;
	}

#ifndef __cplusplus
	inline float GetLodClamp()
	{
		return f16tof32((uvset_aniso_lodclamp >> 16u) & 0xFFFF);
	}
	Texture2D<half4> GetTexture()
	{
		return bindless_textures_half4[UniformTextureSlot(texture_descriptor)];
	}
	half4 SampleVirtual(
		in Texture2D<half4> tex,
		in SamplerState sam,
		in float2 uv,
		in Texture2D<uint4> residency_map,
		in uint2 virtual_tile_count,
		in uint2 virtual_image_dim,
		in float virtual_lod
	)
	{
		virtual_lod = max(0, virtual_lod);

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
		uint4 residency = residency_map.Load(uint3(pixel >> uint(virtual_lod), min(max_nonpacked_lod, uint(virtual_lod))));
		uint2 tile = packed_mips ? residency.zw : residency.xy;
		const float clamped_lod = virtual_lod < max_nonpacked_lod ? max(virtual_lod, residency.z) : virtual_lod;

		// Mip - more detailed:
		half4 value0;
		{
			uint lod0 = uint(clamped_lod);
			const uint packed_mip_idx = packed_mips ? uint(virtual_lod - max_nonpacked_lod - 1) : 0;
			uint2 tile_pixel_upperleft = tile * SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER + SVT_PACKED_MIP_OFFSETS[packed_mip_idx];
			uint2 virtual_lod_dim = max(4u, virtual_image_dim >> lod0);
			float2 virtual_pixel = uv * virtual_lod_dim;
			float2 virtual_tile_pixel = fmod(virtual_pixel, SVT_TILE_SIZE);
			float2 atlas_tile_pixel = tile_pixel_upperleft + 0.5 + virtual_tile_pixel;
			float2 atlas_uv = atlas_tile_pixel * atlas_dim_rcp;
			value0 = tex.SampleLevel(sam, atlas_uv, 0);
		}

		// Mip - less detailed:
		half4 value1;
		{
			uint lod1 = uint(clamped_lod + 1);
			packed_mips = uint(lod1) > max_nonpacked_lod;
			const uint packed_mip_idx = packed_mips ? uint(lod1 - max_nonpacked_lod - 1) : 0;
			residency = residency_map.Load(uint3(pixel >> lod1, min(max_nonpacked_lod, lod1)));
			tile = packed_mips ? residency.zw : residency.xy;
			uint2 tile_pixel_upperleft = tile * SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER + SVT_PACKED_MIP_OFFSETS[packed_mip_idx];
			uint2 virtual_lod_dim = max(4u, virtual_image_dim >> lod1);
			float2 virtual_pixel = uv * virtual_lod_dim;
			float2 virtual_tile_pixel = fmod(virtual_pixel, SVT_TILE_SIZE);
			float2 atlas_tile_pixel = tile_pixel_upperleft + 0.5 + virtual_tile_pixel;
			float2 atlas_uv = atlas_tile_pixel * atlas_dim_rcp;
			value1 = tex.SampleLevel(sam, atlas_uv, 0);
		}

		return lerp(value0, value1, (half)frac(clamped_lod)); // custom trilinear filtering
	}
	half4 Sample(in SamplerState sam, in float4 uvsets)
	{
		Texture2D<half4> tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint4> residency_map = bindless_textures_uint4[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			float virtual_lod = get_lod(virtual_image_dim, ddx_coarse(uv), ddy_coarse(uv), GetAnisotropy()) + SVT_MIP_BIAS;
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, virtual_lod);
		}
#endif // DISABLE_SVT

		return tex.Sample(sam, uv);
	}

	half4 SampleLevel(in SamplerState sam, in float4 uvsets, in float lod)
	{
		Texture2D<half4> tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint4> residency_map = bindless_textures_uint4[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, lod);
		}
#endif // DISABLE_SVT

		return tex.SampleLevel(sam, uv, lod);
	}

	half4 SampleBias(in SamplerState sam, in float4 uvsets, in float bias)
	{
		Texture2D<half4> tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint4> residency_map = bindless_textures_uint4[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			float virtual_lod = get_lod(virtual_image_dim, ddx_coarse(uv), ddy_coarse(uv), GetAnisotropy()) + SVT_MIP_BIAS;
			virtual_lod += bias;
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, virtual_lod + bias);
		}
#endif // DISABLE_SVT

		return tex.SampleBias(sam, uv, bias);
	}

	half4 SampleGrad(in SamplerState sam, in float4 uvsets, in float4 uvsets_dx, in float4 uvsets_dy)
	{
		Texture2D<half4> tex = GetTexture();
		float2 uv = GetUVSet() == 0 ? uvsets.xy : uvsets.zw;
		float2 uv_dx = GetUVSet() == 0 ? uvsets_dx.xy : uvsets_dx.zw;
		float2 uv_dy = GetUVSet() == 0 ? uvsets_dy.xy : uvsets_dy.zw;

#ifndef DISABLE_SVT
		[branch]
		if (sparse_residencymap_descriptor >= 0)
		{
			Texture2D<uint4> residency_map = bindless_textures_uint4[UniformTextureSlot(sparse_residencymap_descriptor)];
			float2 virtual_tile_count;
			residency_map.GetDimensions(virtual_tile_count.x, virtual_tile_count.y);
			float2 virtual_image_dim = virtual_tile_count * SVT_TILE_SIZE;
			float virtual_lod = get_lod(virtual_image_dim, uv_dx, uv_dy, GetAnisotropy()) + SVT_MIP_BIAS;
			return SampleVirtual(tex, sam, uv, residency_map, virtual_tile_count, virtual_image_dim, virtual_lod);
		}
#endif // DISABLE_SVT

		return tex.SampleGrad(sam, uv, uv_dx, uv_dy);
	}
#endif // __cplusplus
};

struct alignas(32) ShaderMaterial
{
	uint2 baseColor;
	uint2 normalmap_pom_alphatest_displacement;

	uint2 roughness_reflectance_metalness_refraction;
	uint2 emissive_cloak;

	uint2 subsurfaceScattering;
	uint2 subsurfaceScattering_inv;

	uint2 specular_chromatic;
	uint2 sheenColor_saturation;

	float4 texMulAdd;

	uint2 transmission_sheenroughness_clearcoat_clearcoatroughness;
	uint2 aniso_anisosin_anisocos_terrainblend;

	int sampler_descriptor : 16;
	int sampler_clamp_descriptor : 16;
	uint options_stencilref;
	uint layerMask;
	uint shaderType_meshblend;

	uint4 userdata;

	ShaderTextureSlot textures[TEXTURESLOT_COUNT];

	float4 padding;

	inline void init()
	{
		baseColor = uint2(0, 0);
		normalmap_pom_alphatest_displacement = uint2(0, 0);

		roughness_reflectance_metalness_refraction = uint2(0, 0);
		emissive_cloak = uint2(0, 0);

		subsurfaceScattering = uint2(0, 0);
		subsurfaceScattering_inv = uint2(0, 0);

		specular_chromatic = uint2(0, 0);
		sheenColor_saturation = uint2(0, 0);

		texMulAdd = float4(1, 1, 0, 0);

		transmission_sheenroughness_clearcoat_clearcoatroughness = uint2(0, 0);
		aniso_anisosin_anisocos_terrainblend = uint2(0, 0);

		sampler_descriptor = -1;
		sampler_clamp_descriptor = -1;
		options_stencilref = 0;
		layerMask = ~0u;
		shaderType_meshblend = 0;

		userdata = uint4(0, 0, 0, 0);
		padding = float4(0, 0, 0, 0);

		for (int i = 0; i < TEXTURESLOT_COUNT; ++i)
		{
			textures[i].init();
		}
	}
	static ShaderMaterial get_null()
	{
		ShaderMaterial ret;
		ret.init();
		return ret;
	}

#ifndef __cplusplus
	inline half4 GetBaseColor() { return unpack_half4(baseColor); }
	inline half4 GetSSS() { return unpack_half4(subsurfaceScattering); }
	inline half4 GetSSSInverse() { return unpack_half4(subsurfaceScattering_inv); }
	inline half3 GetEmissive() { return unpack_half4(emissive_cloak).rgb; }
	inline half GetCloak() { return unpack_half4(emissive_cloak).a; }
	inline half3 GetSpecular() { return unpack_half3(specular_chromatic); }
	inline half GetChromaticAberration() { return unpack_half4(specular_chromatic).w; }
	inline half3 GetSheenColor() { return unpack_half3(sheenColor_saturation); }
	inline half GetSaturation() { return unpack_half4(sheenColor_saturation).w; }
	inline half GetRoughness() { return unpack_half4(roughness_reflectance_metalness_refraction).x; }
	inline half GetReflectance() { return unpack_half4(roughness_reflectance_metalness_refraction).y; }
	inline half GetMetalness() { return unpack_half4(roughness_reflectance_metalness_refraction).z; }
	inline half GetRefraction() { return unpack_half4(roughness_reflectance_metalness_refraction).w; }
	inline half GetNormalMapStrength() { return unpack_half4(normalmap_pom_alphatest_displacement).x; }
	inline half GetParallaxOcclusionMapping() { return unpack_half4(normalmap_pom_alphatest_displacement).y; }
	inline half GetAlphaTest() { return unpack_half4(normalmap_pom_alphatest_displacement).z; }
	inline half GetDisplacement() { return unpack_half4(normalmap_pom_alphatest_displacement).w; }
	inline half GetTransmission() { return unpack_half4(transmission_sheenroughness_clearcoat_clearcoatroughness).x; }
	inline half GetSheenRoughness() { return unpack_half4(transmission_sheenroughness_clearcoat_clearcoatroughness).y; }
	inline half GetClearcoat() { return unpack_half4(transmission_sheenroughness_clearcoat_clearcoatroughness).z; }
	inline half GetClearcoatRoughness() { return unpack_half4(transmission_sheenroughness_clearcoat_clearcoatroughness).w; }
	inline half GetAnisotropy() { return unpack_half4(aniso_anisosin_anisocos_terrainblend).x; }
	inline half GetAnisotropySin() { return unpack_half4(aniso_anisosin_anisocos_terrainblend).y; }
	inline half GetAnisotropyCos() { return unpack_half4(aniso_anisosin_anisocos_terrainblend).z; }
	inline half GetTerrainBlendRcp() { return unpack_half4(aniso_anisosin_anisocos_terrainblend).w; }
	inline half3 GetInteriorScale() { return unpack_half3(subsurfaceScattering); }
	inline half3 GetInteriorOffset() { return unpack_half3(subsurfaceScattering_inv); }
	inline half2 GetInteriorSinCos() { return half2(unpack_half4(subsurfaceScattering).w, unpack_half4(subsurfaceScattering_inv).w); }
	inline uint GetStencilRef() { return options_stencilref >> 24u; }
	inline uint GetShaderType() { return shaderType_meshblend & 0xFFFF; }
	inline half GetMeshBlend() { return f16tof32(shaderType_meshblend >> 16u); }

	inline uint GetOptions() { return options_stencilref; }
	inline bool IsUsingVertexColors() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS; }
	inline bool IsUsingVertexAO() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_USE_VERTEXAO; }
	inline bool IsUsingSpecularGlossinessWorkflow() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW; }
	inline bool IsOcclusionEnabled_Primary() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY; }
	inline bool IsOcclusionEnabled_Secondary() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY; }
	inline bool IsUsingWind() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_USE_WIND; }
	inline bool IsReceiveShadow() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW; }
	inline bool IsCastingShadow() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_CAST_SHADOW; }
	inline bool IsTransparent() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_TRANSPARENT; }
	inline bool IsAdditive() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_ADDITIVE; }
	inline bool IsDoubleSided() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_DOUBLE_SIDED; }
	inline bool IsCapsuleShadowDisabled() { return GetOptions() & SHADERMATERIAL_OPTION_BIT_CAPSULE_SHADOW_DISABLED; }
	inline bool IsUnlit() { return GetShaderType() == SHADERTYPE_UNLIT; }

#endif // __cplusplus
};
#ifdef __cplusplus
static_assert(sizeof(ShaderMaterial) == 384);
inline static const ShaderMaterial shader_material_null = ShaderMaterial::get_null();
#endif // __cplusplus

// For binning shading based on shader types:
struct alignas(16) ShaderTypeBin
{
	uint dispatchX;
	uint dispatchY;
	uint dispatchZ;
	uint shaderType;
#if defined(__SCE__) || defined(__PSSL_)
	uint4 padding; // 32-byte alignment
#endif // __SCE__ || __PSSL__
};
static const uint SHADERTYPE_BIN_COUNT = 12;

struct alignas(16) VisibilityTile
{
	uint64_t execution_mask;
	uint visibility_tile_id;
	uint entity_flat_tile_index;

	inline bool check_thread_valid(uint groupIndex)
	{
		return (execution_mask & (uint64_t(1) << uint64_t(groupIndex))) != 0;
	}
};

enum SHADERMESH_FLAGS
{
	SHADERMESH_FLAG_DOUBLE_SIDED = 1 << 0,
	SHADERMESH_FLAG_HAIRPARTICLE = 1 << 1,
	SHADERMESH_FLAG_EMITTEDPARTICLE = 1 << 2,
};

// This is equivalent to a Mesh + MeshSubset
//	But because these are always loaded toghether by shaders, they are unrolled into one to reduce individual buffer loads
struct alignas(32) ShaderGeometry
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
	int vb_clu;
	int vb_bou;

	int ib_reorder;
	int padding0;
	int padding1;
	int padding2;

	inline void init()
	{
		ib = -1;
		ib_reorder = -1;
		vb_pos_wind = -1;
		vb_uvs = -1;
		vb_nor = -1;
		vb_tan = -1;
		vb_col = -1;
		vb_atl = -1;
		vb_pre = -1;
		vb_clu = -1;
		vb_bou = -1;
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

		padding0 = 0;
		padding1 = 0;
		padding2 = 0;
	}
	static ShaderGeometry get_null()
	{
		ShaderGeometry ret;
		ret.init();
		return ret;
	}
};
#ifdef __cplusplus
static_assert(sizeof(ShaderGeometry) == 128);
inline static const ShaderGeometry shader_geometry_null = ShaderGeometry::get_null();
#endif // __cplusplus

static const uint MESHLET_VERTEX_COUNT = 64u;
static const uint MESHLET_TRIANGLE_COUNT = 124u;
inline uint triangle_count_to_meshlet_count(uint triangleCount)
{
	return (triangleCount + MESHLET_TRIANGLE_COUNT - 1u) / MESHLET_TRIANGLE_COUNT;
}
struct alignas(16) ShaderMeshlet
{
	uint instanceIndex;
	uint geometryIndex;
	uint primitiveOffset; // either direct triangle offset within index buffer, or masked cluster index for clustered geo
	uint padding;
};

struct ShaderClusterTriangle
{
	uint raw;
	void init(uint i0, uint i1, uint i2, uint flags = 0u)
	{
		raw = 0;
		raw |= i0 & 0xFF;
		raw |= (i1 & 0xFF) << 8u;
		raw |= (i2 & 0xFF) << 16u;
		raw |= (flags & 0xFF) << 24u;
	}
	uint i0() { return raw & 0xFF; }
	uint i1() { return (raw >> 8u) & 0xFF; }
	uint i2() { return (raw >> 16u) & 0xFF; }
	uint3 tri() { return uint3(i0(), i1(), i2()); }
	uint flags() { return raw >> 24u; }
};
struct alignas(16) ShaderCluster
{
	uint triangleCount;
	uint vertexCount;
	uint padding0;
	uint padding1;

	uint vertices[MESHLET_VERTEX_COUNT];
	ShaderClusterTriangle triangles[MESHLET_TRIANGLE_COUNT];
};

struct alignas(16) ShaderSphere
{
	float3 center;
	float radius;
};

struct alignas(16) ShaderClusterBounds
{
	ShaderSphere sphere;

	float3 cone_axis;
	float cone_cutoff;
};

struct alignas(16) ShaderTransform
{
	float4 mat0;
	float4 mat1;
	float4 mat2;

	inline void init()
	{
		mat0 = float4(1, 0, 0, 0);
		mat1 = float4(0, 1, 0, 0);
		mat2 = float4(0, 0, 1, 0);
	}
	inline void Create(float4x4 mat)
	{
		mat0 = float4(mat._11, mat._21, mat._31, mat._41);
		mat1 = float4(mat._12, mat._22, mat._32, mat._42);
		mat2 = float4(mat._13, mat._23, mat._33, mat._43);
	}
	inline float4x4 GetMatrix()
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
#ifndef __cplusplus
	inline float3x3 GetMatrixAdjoint()
	{
		return adjoint(GetMatrix());
	}
#endif // __cplusplus
};

struct alignas(32) ShaderMeshInstance
{
	uint64_t uid;
	uint flags;	// high 8 bits: user stencilRef
	uint layerMask;

	uint meshletOffset;			// offset in the global meshlet buffer for first subset (for LOD0)
	uint geometryOffset;		// offset of all geometries for currently active LOD
	uint geometryCount;			// number of all geometries in currently active LOD
	uint baseGeometryOffset;	// offset of all geometries of the instance (if no LODs, then it is equal to geometryOffset)

	uint2 rimHighlight;			// packed half4
	uint baseGeometryCount;		// number of all geometries of the instance (if no LODs, then it is equal to geometryCount)
	float fadeDistance;

	uint2 color; // packed half4
	uint2 emissive; // packed half4

	int vb_ao;
	int vb_wetmap;
	int lightmap;
	uint alphaTest_size; // packed half2

	float3 center;
	float radius;

	ShaderTransform transform; // Note: this could contain quantization remapping from UNORM -> FLOAT depending on vertex position format
	ShaderTransform transformPrev; // Note: this could contain quantization remapping from UNORM -> FLOAT depending on vertex position format
	ShaderTransform transformRaw; // Note: this is the world matrix without any quantization remapping

	float4 padding;

	inline void init()
	{
#ifdef __cplusplus
		using namespace wi::math;
#endif // __cplusplus
		uid = 0;
		flags = 0;
		layerMask = 0;
		color = pack_half4(1, 1, 1, 1);
		emissive = uint2(0, 0);
		lightmap = -1;
		geometryOffset = 0;
		geometryCount = 0;
		baseGeometryOffset = 0;
		baseGeometryCount = 0;
		meshletOffset = ~0u;
		fadeDistance = 0;
		center = float3(0, 0, 0);
		radius = 0;
		vb_ao = -1;
		vb_wetmap = -1;
		alphaTest_size = 0;
		rimHighlight = uint2(0, 0);
		transform.init();
		transformPrev.init();
		transformRaw.init();
		padding = float4(0, 0, 0, 0);
	}
	static ShaderMeshInstance get_null()
	{
		ShaderMeshInstance ret;
		ret.init();
		return ret;
	}

	inline void SetUserStencilRef(uint stencilRef)
	{
		flags |= (stencilRef & 0xFF) << 24u;
	}
	inline uint GetUserStencilRef()
	{
		return flags >> 24u;
	}

#ifndef __cplusplus
	inline half4 GetColor() { return unpack_half4(color); }
	inline half3 GetEmissive() { return unpack_half3(emissive); }
	inline half GetAlphaTest() { return unpack_half2(alphaTest_size).x; }
	inline half GetSize() { return unpack_half2(alphaTest_size).y; }
	inline half4 GetRimHighlight() { return unpack_half4(rimHighlight); }
#endif // __cplusplus
};
#ifdef __cplusplus
static_assert(sizeof(ShaderMeshInstance) == 256);
inline static const ShaderMeshInstance shader_mesh_instance_null = ShaderMeshInstance::get_null();
#endif // __cplusplus

struct ShaderMeshInstancePointer
{
	uint data;

	inline void init()
	{
		data = 0;
	}
	inline void Create(uint _instanceIndex, uint camera_index = 0, float dither = 0)
	{
		data = 0;
		data |= _instanceIndex & 0xFFFFFF;
		data |= (camera_index & 0xF) << 24u;
		data |= (uint(dither * 15.0f) & 0xF) << 28u;
	}

#ifndef __cplusplus
	inline uint GetInstanceIndex()
	{
		return data & 0xFFFFFF;
	}
	inline uint GetCameraIndex()
	{
		return (data >> 24u) & 0xF;
	}
	inline half GetDither()
	{
		return half((data >> 28u) & 0xF) / 15.0;
	}
#endif // __cplusplus
};

struct ObjectPushConstants
{
	uint geometryIndex : 24;
	uint wrapSamplerIndex : 8;
	uint materialIndex : 24;
	uint clampSamplerIndex : 8;
	int instances;
	uint instance_offset;
};
#ifdef __cplusplus
static_assert(sizeof(ObjectPushConstants) == 16);
#endif // __cplusplus


enum SHADER_ENTITY_TYPE
{
	ENTITY_TYPE_DIRECTIONALLIGHT,
	ENTITY_TYPE_POINTLIGHT,
	ENTITY_TYPE_SPOTLIGHT,
	ENTITY_TYPE_RECTLIGHT,
	ENTITY_TYPE_DECAL,
	ENTITY_TYPE_ENVMAP,
	ENTITY_TYPE_FORCEFIELD_POINT,
	ENTITY_TYPE_FORCEFIELD_PLANE,
	ENTITY_TYPE_COLLIDER_SPHERE,
	ENTITY_TYPE_COLLIDER_CAPSULE,
	ENTITY_TYPE_COLLIDER_PLANE,

	ENTITY_TYPE_COUNT
};

enum SHADER_ENTITY_FLAGS
{
	ENTITY_FLAG_LIGHT_STATIC = 1 << 0,
	ENTITY_FLAG_LIGHT_CASTING_SHADOW = 1 << 1,
	ENTITY_FLAG_LIGHT_VOLUMETRICCLOUDS = 1 << 2,
	ENTITY_FLAG_DECAL_BASECOLOR_ONLY_ALPHA = 1 << 3,
	ENTITY_FLAG_CAPSULE_SHADOW_COLLIDER = 1 << 4,
};

struct alignas(16) ShaderEntity
{
	float3 position;
	uint type8_flags8_range16;

	uint2 direction16_coneAngleCos16;
	uint2 color; // half4 packed

	uint layerMask;
	uint indices;
	uint remap;
	uint radius16_length16;

	float4 shadowAtlasMulAdd;

#ifndef __cplusplus
	// Shader-side:
	inline min16uint GetType()
	{
		return type8_flags8_range16 & 0xFF;
	}
	inline min16uint GetFlags()
	{
		return (type8_flags8_range16 >> 8u) & 0xFF;
	}
	inline half GetRange()
	{
		return (half)f16tof32(type8_flags8_range16 >> 16u);
	}
	inline half GetRadius()
	{
		return (half)f16tof32(radius16_length16);
	}
	inline half GetLength()
	{
		return (half)f16tof32(radius16_length16 >> 16u);
	}
	inline half GetHeight()
	{
		return GetRadius();
	}
	inline half3 GetDirection()
	{
		return normalize(half3(
			(half)f16tof32(direction16_coneAngleCos16.x),
			(half)f16tof32(direction16_coneAngleCos16.x >> 16u),
			(half)f16tof32(direction16_coneAngleCos16.y)
		));
	}
	inline half4 GetQuaternion()
	{
		return normalize(half4(
			(half)f16tof32(direction16_coneAngleCos16.x),
			(half)f16tof32(direction16_coneAngleCos16.x >> 16u),
			(half)f16tof32(direction16_coneAngleCos16.y),
			(half)f16tof32(direction16_coneAngleCos16.y >> 16u)
		));
	}
	inline half GetConeAngleCos()
	{
		return (half)f16tof32(direction16_coneAngleCos16.y >> 16u);
	}
	inline min16uint GetShadowCascadeCount()
	{
		return direction16_coneAngleCos16.y >> 16u;
	}
	inline half GetAngleScale()
	{
		return (half)f16tof32(remap);
	}
	inline half GetAngleOffset()
	{
		return (half)f16tof32(remap >> 16u);
	}
	inline half GetCubemapDepthRemapNear()
	{
		return (half)f16tof32(remap);
	}
	inline half GetCubemapDepthRemapFar()
	{
		return (half)f16tof32(remap >> 16u);
	}
	inline half4 GetColor()
	{
		half4 retVal;
		retVal.x = (half)f16tof32(color.x);
		retVal.y = (half)f16tof32(color.x >> 16u);
		retVal.z = (half)f16tof32(color.y);
		retVal.w = (half)f16tof32(color.y >> 16u);
		return retVal;
	}
	inline min16uint GetMatrixIndex()
	{
		return indices & 0xFFF;
	}
	inline min16uint GetTextureIndex()
	{
		return indices >> 12u;
	}
	inline bool IsCastingShadow()
	{
		return GetFlags() & ENTITY_FLAG_LIGHT_CASTING_SHADOW;
	}
	inline bool IsStaticLight()
	{
		return GetFlags() & ENTITY_FLAG_LIGHT_STATIC;
	}
	inline half GetGravity()
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
	inline void SetHeight(float value)
	{
		SetRadius(value);
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
	inline void SetQuaternion(float4 value)
	{
		direction16_coneAngleCos16.x |= XMConvertFloatToHalf(value.x);
		direction16_coneAngleCos16.x |= XMConvertFloatToHalf(value.y) << 16u;
		direction16_coneAngleCos16.y |= XMConvertFloatToHalf(value.z);
		direction16_coneAngleCos16.y |= XMConvertFloatToHalf(value.w) << 16u;
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
		indices = matrixIndex & 0xFFF;
		indices |= (textureIndex & 0xFFFFF) << 12u;
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
#ifdef __cplusplus
static_assert(sizeof(ShaderEntity) == 64);
#endif // __cplusplus

struct alignas(16) ShaderFrustum
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

struct alignas(16) ShaderFrustumCorners
{
	// topleft, topright, bottomleft, bottomright
	float4 cornersNEAR[4];
	float4 cornersFAR[4];

#ifndef __cplusplus
	inline float3 screen_to_nearplane(float2 uv)
	{
		float3 posTOP = lerp(cornersNEAR[0], cornersNEAR[1], uv.x);
		float3 posBOTTOM = lerp(cornersNEAR[2], cornersNEAR[3], uv.x);
		return lerp(posTOP, posBOTTOM, uv.y);
	}
	inline float3 screen_to_farplane(float2 uv)
	{
		float3 posTOP = lerp(cornersFAR[0], cornersFAR[1], uv.x);
		float3 posBOTTOM = lerp(cornersFAR[2], cornersFAR[3], uv.x);
		return lerp(posTOP, posBOTTOM, uv.y);
	}
	inline float3 screen_to_world(float2 uv, float lineardepthNormalized)
	{
		return lerp(screen_to_nearplane(uv), screen_to_farplane(uv), lineardepthNormalized);
	}
#endif // __cplusplus
};

static const float CAPSULE_SHADOW_AFFECTION_RANGE = 4; // how far away the capsule shadow can reach outside of their own radius
static const float CAPSULE_SHADOW_BOLDEN = 1.1f; // multiplier for capsule shadow capsule radiuses globally

static const uint SHADER_ENTITY_COUNT = 256;
static const uint SHADER_ENTITY_TILE_BUCKET_COUNT = SHADER_ENTITY_COUNT / 32;

struct ShaderEntityIterator
{
	uint value;

#ifdef __cplusplus
	ShaderEntityIterator(uint offset, uint count)
	{
		value = offset | (count << 16u);
	}
	constexpr operator uint() const { return value; }
#endif // __cplusplus

	inline uint item_offset()
	{
		return value & 0xFFFF;
	}
	inline uint item_count()
	{
		return value >> 16u;
	}
	inline bool empty()
	{
		return item_count() == 0;
	}
	inline uint first_item()
	{
		return item_offset();
	}
	inline uint last_item() // includes last valid item
	{
		return empty() ? 0 : (item_offset() + item_count() - 1);
	}
	inline uint end_item() // excludes last valid item
	{
		return empty() ? 0 : (item_offset() + item_count());
	}
	inline uint first_bucket()
	{
		return first_item() / 32u;
	}
	inline uint last_bucket()
	{
		return last_item() / 32u;
	}
	inline uint bucket_mask()
	{
		const uint bucket_mask_lo = ~0u << first_bucket();
		const uint bucket_mask_hi = ~0u >> (31u - last_bucket());
		return bucket_mask_lo & bucket_mask_hi;
	}
	inline uint first_bucket_entity_mask()
	{
		return ~0u << (first_item() % 32u);
	}
	inline uint last_bucket_entity_mask()
	{
		return ~0u >> (31u - (last_item() % 32u));
	}
	// This masks out inactive buckets of the current type based on a whole tile bucket mask
	inline uint mask_type(uint tile_mask)
	{
		return tile_mask & bucket_mask();
	}
	// This masks out inactive entities for the current bucket type when processing either the first or the last bucket in the list
	inline uint mask_entity(uint bucket, uint bucket_bits)
	{
		if (bucket == first_bucket())
			bucket_bits &= first_bucket_entity_mask();
		if (bucket == last_bucket())
			bucket_bits &= last_bucket_entity_mask();
		return bucket_bits;
	}
};

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
enum FRAME_OPTIONS
{
	OPTION_BIT_TEMPORALAA_ENABLED = 1 << 0,
	//OPTION_BIT_TRANSPARENTSHADOWS_ENABLED = 1 << 1,
	OPTION_BIT_VXGI_ENABLED = 1 << 2,
	OPTION_BIT_VXGI_REFLECTIONS_ENABLED = 1 << 3,
	OPTION_BIT_REALISTIC_SKY = 1 << 6,
	OPTION_BIT_HEIGHT_FOG = 1 << 7,
	OPTION_BIT_RAYTRACED_SHADOWS = 1 << 8,
	OPTION_BIT_SHADOW_MASK = 1 << 9,
	OPTION_BIT_SURFELGI_ENABLED = 1 << 10,
	OPTION_BIT_DISABLE_ALBEDO_MAPS = 1 << 11,
	OPTION_BIT_FORCE_DIFFUSE_LIGHTING = 1 << 12,
	OPTION_BIT_VOLUMETRICCLOUDS_CAST_SHADOW = 1 << 13,
	OPTION_BIT_OVERRIDE_FOG_COLOR = 1 << 14,
	OPTION_BIT_STATIC_SKY_SPHEREMAP = 1 << 15,
	OPTION_BIT_REALISTIC_SKY_AERIAL_PERSPECTIVE = 1 << 16,
	OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY = 1 << 17,
	OPTION_BIT_REALISTIC_SKY_RECEIVE_SHADOW = 1 << 18,
	OPTION_BIT_VOLUMETRICCLOUDS_RECEIVE_SHADOW = 1 << 19,
	OPTION_BIT_CAPSULE_SHADOW_ENABLED = 1 << 20,
	OPTION_BIT_DISABLE_SHADOWMAPS = 1 << 21,
	OPTION_BIT_FORCE_UNLIT = 1 << 22,
};

// ---------- Common Constant buffers: -----------------

struct alignas(16) FrameCB
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
	uint		giboost_packed; // force fp16 load
	uint		entity_culling_count;

	uint		capsuleshadow_fade_angle;
	int			indirect_debugbufferindex;
	int			padding0;
	int			padding1;

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
	int			texture_caustics_index;

	float4		rain_blocker_mad;
	float4x4	rain_blocker_matrix;
	float4x4	rain_blocker_matrix_inverse;

	float4		rain_blocker_mad_prev;
	float4x4	rain_blocker_matrix_prev;
	float4x4	rain_blocker_matrix_inverse_prev;

	ShaderScene scene;

	VXGI vxgi;

	uint probes;
	uint directional_lights;
	uint spotlights;
	uint pointlights;

	uint rectlights;
	uint lights;
	uint decals;
	uint forces;

	ShaderEntity entityArray[SHADER_ENTITY_COUNT];
	float4x4 matrixArray[SHADER_ENTITY_COUNT];
};

enum SHADERCAMERA_OPTIONS
{
	SHADERCAMERA_OPTION_NONE = 0,
	SHADERCAMERA_OPTION_USE_SHADOW_MASK = 1 << 0,
	SHADERCAMERA_OPTION_ORTHO = 1 << 1,
	SHADERCAMERA_OPTION_DEDICATED_SHADOW_LODBIAS = 1 << 2,
};

struct alignas(16) ShaderCamera
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
	ShaderFrustumCorners frustum_corners;

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

	uint2 entity_culling_tilecount;
	uint entity_culling_tile_bucket_count_flat; // tilecount.x * tilecount.y * SHADER_ENTITY_TILE_BUCKET_COUNT (the total number of uint buckets for the whole screen)
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
	int buffer_entitytiles_index;

	int texture_reflection_index;
	int texture_reflection_depth_index;
	int texture_refraction_index;
	int texture_waterriples_index;

	int texture_ao_index;
	int texture_ssr_index;
	int texture_ssgi_index;
	int texture_rtshadow_index;

	int texture_surfelgi_index;
	int texture_depth_index_prev;
	int texture_vxgi_diffuse_index;
	int texture_vxgi_specular_index;

	int texture_reprojected_depth_index;
	uint options;
	uint padding0;
	uint padding1;

#ifdef __cplusplus
	inline void init()
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
		entity_culling_tile_bucket_count_flat = 0;
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
		buffer_entitytiles_index = -1;
		texture_reflection_index = -1;
		texture_refraction_index = -1;
		texture_waterriples_index = -1;
		texture_ao_index = -1;
		texture_ssr_index = -1;
		texture_ssgi_index = -1;
		texture_rtshadow_index = -1;
		texture_surfelgi_index = -1;
		texture_depth_index_prev = -1;
		texture_vxgi_diffuse_index = -1;
		texture_vxgi_specular_index = -1;
		texture_reprojected_depth_index = -1;

		options = 0;
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

	inline bool IsOrtho() { return options & SHADERCAMERA_OPTION_ORTHO; }

	inline float3 screen_to_nearplane(float4 svposition)
	{
		const float2 ScreenCoord = svposition.xy * internal_resolution_rcp;
		return frustum_corners.screen_to_nearplane(ScreenCoord);
	}
	inline float3 screen_to_farplane(float4 svposition)
	{
		const float2 ScreenCoord = svposition.xy * internal_resolution_rcp;
		return frustum_corners.screen_to_farplane(ScreenCoord);
	}

	// Convert raw screen coordinate from rasterizer to world position
	//	Note: svposition is the SV_Position system value, the .w component can be different in different compilers
	//	You need to ensure that the .w component is used for linear depth (Vulkan: -fvk-use-dx-position-w, Xbox: in case VRS, there is complication with this, read documentation)
	inline float3 screen_to_world(float4 svposition)
	{
		const float2 ScreenCoord = svposition.xy * internal_resolution_rcp;
		const float z = IsOrtho() ? (1 - svposition.z) : ((svposition.w - z_near) * z_range_rcp);
		return frustum_corners.screen_to_world(ScreenCoord, z);
	}
#endif // __cplusplus
};

struct alignas(16) CameraCB
{
	ShaderCamera cameras[16];

#ifdef __cplusplus
	inline void init()
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

struct WetmapPush
{
	int wetmap;
	uint padding;
	uint instanceID;
	float rain_amount;
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

struct PaintTexturePushConstants
{
	uint2 xPaintBrushCenter;
	uint xPaintBrushRadius;
	float xPaintBrushAmount;

	float xPaintBrushSmoothness;
	uint xPaintBrushColor;
	uint xPaintRedirectAlpha;
	float xPaintBrushRotation;

	uint xPaintBrushShape;
	int texture_brush;
	int texture_reveal;
	int texture_output;
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
	uint influence_div4;
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

	int texture_output;
	float padding0;
	float padding1;
	float padding2;
};

struct VolumetricCloudCapturePushConstants
{
	uint2 resolution;
	float2 resolution_rcp;

	int texture_output;
	int maxStepCount;
	float LODMin;
	float shadowSampleCount;

	float groundContributionSampleCount;
	float padding0;
	float padding1;
	float padding2;
};


struct TerrainVirtualTexturePush
{
	int2 offset;
	uint2 write_offset;

	uint write_size;
	float resolution_rcp;
	int blendmap_buffer;
	uint blendmap_buffer_offset;

	int blendmap_texture;
	uint blendmap_layers;
	int output_texture;
	int padding0;
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
	int feedbackTextureRW;
	int requestBufferRW;
	int padding0;
	int padding1;
	int padding2;
};

struct StencilBitPush
{
	float2 output_resolution_rcp;
	uint input_resolution;
	uint bit;
};

CBUFFER(TrailRendererCB, CBSLOT_TRAILRENDERER)
{
	float4x4	g_xTrailTransform;
	float4		g_xTrailColor;
	float4		g_xTrailTexMulAdd;
	float4		g_xTrailTexMulAdd2;
	int			g_xTrailTextureIndex1;
	int			g_xTrailTextureIndex2;
	int			g_xTrailLinearDepthTextureIndex;
	float		g_xTrailDepthSoften;
	float3		g_xTrailPadding;
	float		g_xTrailCameraFar;
};

CBUFFER(PaintDecalCB, CBSLOT_RENDERER_MISC)
{
	float4x4 g_xPaintDecalMatrix;

	int g_xPaintDecalTexture;
	int g_xPaintDecalInstanceID;
	float g_xPaintDecalSlopeBlendPower;
	int g_xPaintDecalpadding;
};

#ifdef __cplusplus
// Copied here from SH_Lite.hlsli to share with C++:
namespace SH
{
	struct L1
	{
		static const uint NumCoefficients = 4;
		float C[NumCoefficients];
	};
	struct L1_RGB
	{
		static const uint NumCoefficients = 4;
		float3 C[NumCoefficients];
		struct Packed
		{
			uint C[3 * NumCoefficients / 2];
		};
	};
	struct L2
	{
		static const uint NumCoefficients = 9;
		float C[NumCoefficients];
	};
	struct L2_RGB
	{
		static const uint NumCoefficients = 9;
		float3 C[NumCoefficients];
	};
}
#endif // __cplusplus

struct alignas(16) DDGIProbe
{
	SH::L1_RGB::Packed radiance;
	uint2 offset;
};

#endif // WI_SHADERINTEROP_RENDERER_H
