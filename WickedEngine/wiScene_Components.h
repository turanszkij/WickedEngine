#pragma once
#include "CommonInclude.h"
#include "wiArchive.h"
#include "wiCanvas.h"
#include "wiAudio.h"
#include "wiVideo.h"
#include "wiEnums.h"
#include "wiOcean.h"
#include "wiPrimitive.h"
#include "shaders/ShaderInterop_Renderer.h"
#include "wiResourceManager.h"
#include "wiVector.h"
#include "wiArchive.h"
#include "wiRectPacker.h"
#include "wiUnorderedSet.h"
#include "wiBVH.h"

namespace wi::scene
{

	struct NameComponent
	{
		std::string name;

		inline void operator=(const std::string& str) { name = str; }
		inline void operator=(std::string&& str) { name = std::move(str); }
		inline bool operator==(const std::string& str) const { return name.compare(str) == 0; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct LayerComponent
	{
		uint32_t layerMask = ~0u;

		// Non-serialized attributes:
		uint32_t propagationMask = ~0u; // This shouldn't be modified by user usually

		inline uint32_t GetLayerMask() const { return layerMask & propagationMask; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct TransformComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
		};
		uint32_t _flags = DIRTY;

		XMFLOAT3 scale_local = XMFLOAT3(1, 1, 1);
		XMFLOAT4 rotation_local = XMFLOAT4(0, 0, 0, 1);	// this is a quaternion
		XMFLOAT3 translation_local = XMFLOAT3(0, 0, 0);

		// Non-serialized attributes:

		// The world matrix can be computed from local scale, rotation, translation
		//	- by calling UpdateTransform()
		//	- or by calling SetDirty() and letting the TransformUpdateSystem handle the updating
		XMFLOAT4X4 world = wi::math::IDENTITY_MATRIX;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		XMFLOAT3 GetPosition() const;
		XMFLOAT4 GetRotation() const;
		XMFLOAT3 GetScale() const;
		XMVECTOR GetPositionV() const;
		XMVECTOR GetRotationV() const;
		XMVECTOR GetScaleV() const;
		// Computes the local space matrix from scale, rotation, translation and returns it
		XMMATRIX GetLocalMatrix() const;
		// Applies the local space to the world space matrix. This overwrites world matrix
		void UpdateTransform();
		// Apply a parent transform relative to the local space. This overwrites world matrix
		void UpdateTransform_Parented(const TransformComponent& parent);
		// Apply the world matrix to the local space. This overwrites scale, rotation, translation
		void ApplyTransform();
		// Clears the local space. This overwrites scale, rotation, translation
		void ClearTransform();
		void Translate(const XMFLOAT3& value);
		void Translate(const XMVECTOR& value);
		void RotateRollPitchYaw(const XMFLOAT3& value);
		void Rotate(const XMFLOAT4& quaternion);
		void Rotate(const XMVECTOR& quaternion);
		void Scale(const XMFLOAT3& value);
		void Scale(const XMVECTOR& value);
		void MatrixTransform(const XMFLOAT4X4& matrix);
		void MatrixTransform(const XMMATRIX& matrix);
		void Lerp(const TransformComponent& a, const TransformComponent& b, float t);
		void CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct HierarchyComponent
	{
		wi::ecs::Entity parentID = wi::ecs::INVALID_ENTITY;
		uint32_t layerMask_bind; // saved child layermask at the time of binding

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct MaterialComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			CAST_SHADOW = 1 << 1,
			_DEPRECATED_PLANAR_REFLECTION = 1 << 2,
			_DEPRECATED_WATER = 1 << 3,
			_DEPRECATED_FLIP_NORMALMAP = 1 << 4,
			USE_VERTEXCOLORS = 1 << 5,
			SPECULAR_GLOSSINESS_WORKFLOW = 1 << 6,
			OCCLUSION_PRIMARY = 1 << 7,
			OCCLUSION_SECONDARY = 1 << 8,
			USE_WIND = 1 << 9,
			DISABLE_RECEIVE_SHADOW = 1 << 10,
			DOUBLE_SIDED = 1 << 11,
			OUTLINE = 1 << 12,
			PREFER_UNCOMPRESSED_TEXTURES = 1 << 13,
		};
		uint32_t _flags = CAST_SHADOW;

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
			SHADERTYPE_COUNT
		} shaderType = SHADERTYPE_PBR;
		static_assert(SHADERTYPE_COUNT == SHADERTYPE_BIN_COUNT, "These values must match!");

		inline static const wi::vector<std::string> shaderTypeDefines[] = {
			{}, // SHADERTYPE_PBR,
			{"PLANARREFLECTION"}, // SHADERTYPE_PBR_PLANARREFLECTION,
			{"PARALLAXOCCLUSIONMAPPING"}, // SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING,
			{"ANISOTROPIC"}, // SHADERTYPE_PBR_ANISOTROPIC,
			{"WATER"}, // SHADERTYPE_WATER,
			{"CARTOON"}, // SHADERTYPE_CARTOON,
			{"UNLIT"}, // SHADERTYPE_UNLIT,
			{"SHEEN"}, // SHADERTYPE_PBR_CLOTH,
			{"CLEARCOAT"}, // SHADERTYPE_PBR_CLEARCOAT,
			{"SHEEN", "CLEARCOAT"}, // SHADERTYPE_PBR_CLOTH_CLEARCOAT,
		};
		static_assert(SHADERTYPE_COUNT == arraysize(shaderTypeDefines), "These values must match!");

		wi::enums::STENCILREF engineStencilRef = wi::enums::STENCILREF_DEFAULT;
		uint8_t userStencilRef = 0;
		wi::enums::BLENDMODE userBlendMode = wi::enums::BLENDMODE_OPAQUE;

		XMFLOAT4 baseColor = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 specularColor = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 emissiveColor = XMFLOAT4(1, 1, 1, 0);
		XMFLOAT4 subsurfaceScattering = XMFLOAT4(1, 1, 1, 0);
		XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0);
		float roughness = 0.2f;
		float reflectance = 0.02f;
		float metalness = 0.0f;
		float normalMapStrength = 1.0f;
		float parallaxOcclusionMapping = 0.0f;
		float displacementMapping = 0.0f;
		float refraction = 0.0f;
		float transmission = 0.0f;
		float alphaRef = 1.0f;
		float anisotropy_strength = 0;
		float anisotropy_rotation = 0; //radians, counter-clockwise

		XMFLOAT4 sheenColor = XMFLOAT4(1, 1, 1, 1);
		float sheenRoughness = 0;
		float clearcoat = 0;
		float clearcoatRoughness = 0;

		wi::graphics::ShadingRate shadingRate = wi::graphics::ShadingRate::RATE_1X1;

		XMFLOAT2 texAnimDirection = XMFLOAT2(0, 0);
		float texAnimFrameRate = 0.0f;
		float texAnimElapsedTime = 0.0f;

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
		struct TextureMap
		{
			std::string name;
			wi::Resource resource;
			uint32_t uvset = 0;
			const wi::graphics::GPUResource* GetGPUResource() const
			{
				if (!resource.IsValid() || !resource.GetTexture().IsValid())
					return nullptr;
				return &resource.GetTexture();
			}

			// Non-serialized attributes:
			float lod_clamp = 0;						// optional, can be used by texture streaming
			int sparse_residencymap_descriptor = -1;	// optional, can be used by texture streaming
			int sparse_feedbackmap_descriptor = -1;		// optional, can be used by texture streaming
		};
		TextureMap textures[TEXTURESLOT_COUNT];

		int customShaderID = -1;
		uint4 userdata = uint4(0, 0, 0, 0); // can be accessed by custom shader

		// Non-serialized attributes:
		uint32_t layerMask = ~0u;
		int sampler_descriptor = -1; // optional

		// User stencil value can be in range [0, 15]
		inline void SetUserStencilRef(uint8_t value)
		{
			assert(value < 16);
			userStencilRef = value & 0x0F;
		}
		uint32_t GetStencilRef() const;

		inline float GetOpacity() const { return baseColor.w; }
		inline float GetEmissiveStrength() const { return emissiveColor.w; }
		inline int GetCustomShaderID() const { return customShaderID; }

		inline bool HasPlanarReflection() const { return shaderType == SHADERTYPE_PBR_PLANARREFLECTION || shaderType == SHADERTYPE_WATER; }

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		inline void SetCastShadow(bool value) { SetDirty(); if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetReceiveShadow(bool value) { SetDirty(); if (value) { _flags &= ~DISABLE_RECEIVE_SHADOW; } else { _flags |= DISABLE_RECEIVE_SHADOW; } }
		inline void SetOcclusionEnabled_Primary(bool value) { SetDirty(); if (value) { _flags |= OCCLUSION_PRIMARY; } else { _flags &= ~OCCLUSION_PRIMARY; } }
		inline void SetOcclusionEnabled_Secondary(bool value) { SetDirty(); if (value) { _flags |= OCCLUSION_SECONDARY; } else { _flags &= ~OCCLUSION_SECONDARY; } }

		inline wi::enums::BLENDMODE GetBlendMode() const { if (userBlendMode == wi::enums::BLENDMODE_OPAQUE && (GetFilterMask() & wi::enums::FILTER_TRANSPARENT)) return wi::enums::BLENDMODE_ALPHA; else return userBlendMode; }
		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }
		inline bool IsUsingVertexColors() const { return _flags & USE_VERTEXCOLORS; }
		inline bool IsUsingWind() const { return _flags & USE_WIND; }
		inline bool IsReceiveShadow() const { return (_flags & DISABLE_RECEIVE_SHADOW) == 0; }
		inline bool IsUsingSpecularGlossinessWorkflow() const { return _flags & SPECULAR_GLOSSINESS_WORKFLOW; }
		inline bool IsOcclusionEnabled_Primary() const { return _flags & OCCLUSION_PRIMARY; }
		inline bool IsOcclusionEnabled_Secondary() const { return _flags & OCCLUSION_SECONDARY; }
		inline bool IsCustomShader() const { return customShaderID >= 0; }
		inline bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }
		inline bool IsOutlineEnabled() const { return _flags & OUTLINE; }
		inline bool IsPreferUncompressedTexturesEnabled() const { return _flags & PREFER_UNCOMPRESSED_TEXTURES; }

		inline void SetBaseColor(const XMFLOAT4& value) { SetDirty(); baseColor = value; }
		inline void SetSpecularColor(const XMFLOAT4& value) { SetDirty(); specularColor = value; }
		inline void SetEmissiveColor(const XMFLOAT4& value) { SetDirty(); emissiveColor = value; }
		inline void SetRoughness(float value) { SetDirty(); roughness = value; }
		inline void SetReflectance(float value) { SetDirty(); reflectance = value; }
		inline void SetMetalness(float value) { SetDirty(); metalness = value; }
		inline void SetEmissiveStrength(float value) { SetDirty(); emissiveColor.w = value; }
		inline void SetTransmissionAmount(float value) { SetDirty(); transmission = value; }
		inline void SetRefractionAmount(float value) { SetDirty(); refraction = value; }
		inline void SetNormalMapStrength(float value) { SetDirty(); normalMapStrength = value; }
		inline void SetParallaxOcclusionMapping(float value) { SetDirty(); parallaxOcclusionMapping = value; }
		inline void SetDisplacementMapping(float value) { SetDirty(); displacementMapping = value; }
		inline void SetSubsurfaceScatteringColor(XMFLOAT3 value)
		{
			SetDirty();
			subsurfaceScattering.x = value.x;
			subsurfaceScattering.y = value.y;
			subsurfaceScattering.z = value.z;
		}
		inline void SetSubsurfaceScatteringAmount(float value) { SetDirty(); subsurfaceScattering.w = value; }
		inline void SetOpacity(float value) { SetDirty(); baseColor.w = value; }
		inline void SetAlphaRef(float value) { SetDirty();  alphaRef = value; }
		inline void SetUseVertexColors(bool value) { SetDirty(); if (value) { _flags |= USE_VERTEXCOLORS; } else { _flags &= ~USE_VERTEXCOLORS; } }
		inline void SetUseWind(bool value) { SetDirty(); if (value) { _flags |= USE_WIND; } else { _flags &= ~USE_WIND; } }
		inline void SetUseSpecularGlossinessWorkflow(bool value) { SetDirty(); if (value) { _flags |= SPECULAR_GLOSSINESS_WORKFLOW; } else { _flags &= ~SPECULAR_GLOSSINESS_WORKFLOW; } }
		inline void SetSheenColor(const XMFLOAT3& value)
		{
			sheenColor = XMFLOAT4(value.x, value.y, value.z, sheenColor.w);
			SetDirty();
		}
		inline void SetSheenRoughness(float value) { sheenRoughness = value; SetDirty(); }
		inline void SetClearcoatFactor(float value) { clearcoat = value; SetDirty(); }
		inline void SetClearcoatRoughness(float value) { clearcoatRoughness = value; SetDirty(); }
		inline void SetCustomShaderID(int id) { customShaderID = id; }
		inline void DisableCustomShader() { customShaderID = -1; }
		inline void SetDoubleSided(bool value = true) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
		inline void SetOutlineEnabled(bool value = true) { if (value) { _flags |= OUTLINE; } else { _flags &= ~OUTLINE; } }
		inline void SetPreferUncompressedTexturesEnabled(bool value = true) { if (value) { _flags |= PREFER_UNCOMPRESSED_TEXTURES; } else { _flags &= ~PREFER_UNCOMPRESSED_TEXTURES; } CreateRenderData(true); }

		// The MaterialComponent will be written to ShaderMaterial (a struct that is optimized for GPU use)
		void WriteShaderMaterial(ShaderMaterial* dest) const;
		void WriteShaderTextureSlot(ShaderMaterial* dest, int slot, int descriptor);

		// Retrieve the array of textures from the material
		void WriteTextures(const wi::graphics::GPUResource** dest, int count) const;

		// Returns the bitwise OR of all the wi::enums::FILTER flags applicable to this material
		uint32_t GetFilterMask() const;

		wi::resourcemanager::Flags GetTextureSlotResourceFlags(TEXTURESLOT slot);

		// Create texture resources for GPU
		void CreateRenderData(bool force_recreate = false);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct MeshComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RENDERABLE = 1 << 0,
			DOUBLE_SIDED = 1 << 1,
			DYNAMIC = 1 << 2,
			_DEPRECATED_TERRAIN = 1 << 3,
			_DEPRECATED_DIRTY_MORPH = 1 << 4,
			_DEPRECATED_DIRTY_BINDLESS = 1 << 5,
			TLAS_FORCE_DOUBLE_SIDED = 1 << 6,
			DOUBLE_SIDED_SHADOW = 1 << 7,
			BVH_ENABLED = 1 << 8,
			QUANTIZED_POSITIONS_DISABLED = 1 << 9,
		};
		uint32_t _flags = RENDERABLE;

		wi::vector<XMFLOAT3> vertex_positions;
		wi::vector<XMFLOAT3> vertex_normals;
		wi::vector<XMFLOAT4> vertex_tangents;
		wi::vector<XMFLOAT2> vertex_uvset_0;
		wi::vector<XMFLOAT2> vertex_uvset_1;
		wi::vector<XMUINT4> vertex_boneindices;
		wi::vector<XMFLOAT4> vertex_boneweights;
		wi::vector<XMFLOAT2> vertex_atlas;
		wi::vector<uint32_t> vertex_colors;
		wi::vector<uint8_t> vertex_windweights;
		wi::vector<uint32_t> indices;

		struct MeshSubset
		{
			wi::ecs::Entity materialID = wi::ecs::INVALID_ENTITY;
			uint32_t indexOffset = 0;
			uint32_t indexCount = 0;

			// Non-serialized attributes:
			uint32_t materialIndex = 0;
		};
		wi::vector<MeshSubset> subsets;

		float tessellationFactor = 0.0f;
		wi::ecs::Entity armatureID = wi::ecs::INVALID_ENTITY;

		struct MorphTarget
		{
			wi::vector<XMFLOAT3> vertex_positions;
			wi::vector<XMFLOAT3> vertex_normals;
			wi::vector<uint32_t> sparse_indices_positions; // optional, these can be used to target vertices indirectly
			wi::vector<uint32_t> sparse_indices_normals; // optional, these can be used to target vertices indirectly
			float weight = 0;

			// Non-serialized attributes:
			uint64_t offset_pos = ~0ull;
			uint64_t offset_nor = ~0ull;
		};
		wi::vector<MorphTarget> morph_targets;

		uint32_t subsets_per_lod = 0; // this needs to be specified if there are multiple LOD levels

		// Non-serialized attributes:
		wi::primitive::AABB aabb;
		wi::graphics::GPUBuffer generalBuffer; // index buffer + all static vertex buffers
		wi::graphics::GPUBuffer streamoutBuffer; // all dynamic vertex buffers
		struct BufferView
		{
			uint64_t offset = ~0ull;
			uint64_t size = 0ull;
			int subresource_srv = -1;
			int descriptor_srv = -1;
			int subresource_uav = -1;
			int descriptor_uav = -1;

			constexpr bool IsValid() const
			{
				return offset != ~0ull;
			}
		};
		BufferView ib;
		BufferView vb_pos_wind;
		BufferView vb_nor;
		BufferView vb_tan;
		BufferView vb_uvs;
		BufferView vb_atl;
		BufferView vb_col;
		BufferView vb_bon;
		BufferView vb_mor;
		BufferView so_pos;
		BufferView so_nor;
		BufferView so_tan;
		BufferView so_pre;
		uint32_t geometryOffset = 0;
		uint32_t meshletCount = 0;
		uint32_t active_morph_count = 0;
		uint32_t morphGPUOffset = 0;
		XMFLOAT2 uv_range_min = XMFLOAT2(0, 0);
		XMFLOAT2 uv_range_max = XMFLOAT2(1, 1);

		wi::vector<wi::graphics::RaytracingAccelerationStructure> BLASes; // one BLAS per LOD
		enum BLAS_STATE
		{
			BLAS_STATE_NEEDS_REBUILD,
			BLAS_STATE_NEEDS_REFIT,
			BLAS_STATE_COMPLETE,
		};
		mutable BLAS_STATE BLAS_state = BLAS_STATE_NEEDS_REBUILD;

		wi::vector<wi::primitive::AABB> bvh_leaf_aabbs;
		wi::BVH bvh;

		inline void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		inline void SetDoubleSided(bool value) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
		inline void SetDoubleSidedShadow(bool value) { if (value) { _flags |= DOUBLE_SIDED_SHADOW; } else { _flags &= ~DOUBLE_SIDED_SHADOW; } }
		inline void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }

		// Enable disable CPU-side BVH acceleration structure
		//	true: BVH will be built immediately if it doesn't exist yet
		//	false: BVH will be deleted immediately if it exists
		inline void SetBVHEnabled(bool value) { if (value) { _flags |= BVH_ENABLED; if (!bvh.IsValid()) { BuildBVH(); } } else { _flags &= ~BVH_ENABLED; bvh = {}; bvh_leaf_aabbs.clear(); } }

		// Disable quantization of position GPU data. You can use this if you notice inaccuracy in positions.
		//	This should be enabled for connecting meshes like terrain chunks if their AABB is not consistent with each other
		inline void SetQuantizedPositionsDisabled(bool value) { if (value) { _flags |= QUANTIZED_POSITIONS_DISABLED; } else { _flags &= ~QUANTIZED_POSITIONS_DISABLED; } }

		inline bool IsRenderable() const { return _flags & RENDERABLE; }
		inline bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }
		inline bool IsDoubleSidedShadow() const { return _flags & DOUBLE_SIDED_SHADOW; }
		inline bool IsDynamic() const { return _flags & DYNAMIC; }
		inline bool IsBVHEnabled() const { return _flags & BVH_ENABLED; }
		inline bool IsQuantizedPositionsDisabled() const { return _flags & QUANTIZED_POSITIONS_DISABLED; }

		inline float GetTessellationFactor() const { return tessellationFactor; }
		inline wi::graphics::IndexBufferFormat GetIndexFormat() const { return wi::graphics::GetIndexBufferFormat((uint32_t)vertex_positions.size()); }
		inline size_t GetIndexStride() const { return GetIndexFormat() == wi::graphics::IndexBufferFormat::UINT32 ? sizeof(uint32_t) : sizeof(uint16_t); }
		inline bool IsSkinned() const { return armatureID != wi::ecs::INVALID_ENTITY; }
		inline uint32_t GetLODCount() const { return subsets_per_lod == 0 ? 1 : ((uint32_t)subsets.size() / subsets_per_lod); }
		inline void GetLODSubsetRange(uint32_t lod, uint32_t& first_subset, uint32_t& last_subset) const
		{
			first_subset = 0;
			last_subset = (uint32_t)subsets.size();
			if (subsets_per_lod > 0)
			{
				lod = std::min(lod, GetLODCount() - 1);
				first_subset = subsets_per_lod * lod;
				last_subset = first_subset + subsets_per_lod;
			}
		}

		// Deletes all GPU resources
		void DeleteRenderData();

		// Recreates GPU resources for index/vertex buffers
		void CreateRenderData();
		void CreateStreamoutRenderData();
		void CreateRaytracingRenderData();

		// Rebuilds CPU-side BVH acceleration structure
		void BuildBVH();

		size_t GetMemoryUsageCPU() const;
		size_t GetMemoryUsageGPU() const;
		size_t GetMemoryUsageBVH() const;

		enum COMPUTE_NORMALS
		{
			COMPUTE_NORMALS_HARD,		// hard face normals, can result in additional vertices generated
			COMPUTE_NORMALS_SMOOTH,		// smooth per vertex normals, this can remove/simplify geometry, but slow
			COMPUTE_NORMALS_SMOOTH_FAST	// average normals, vertex count will be unchanged, fast
		};
		void ComputeNormals(COMPUTE_NORMALS compute);
		void FlipCulling();
		void FlipNormals();
		void Recenter();
		void RecenterToBottom();
		wi::primitive::Sphere GetBoundingSphere() const;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		struct Vertex_POS10
		{
			uint32_t x : 10;
			uint32_t y : 10;
			uint32_t z : 10;
			uint32_t w : 2;

			constexpr void FromFULL(const wi::primitive::AABB& aabb, XMFLOAT3 pos, uint8_t wind)
			{
				pos = wi::math::InverseLerp(aabb._min, aabb._max, pos); // UNORM remap
				x = uint32_t(wi::math::saturate(pos.x) * 1023.0f);
				y = uint32_t(wi::math::saturate(pos.y) * 1023.0f);
				z = uint32_t(wi::math::saturate(pos.z) * 1023.0f);
				w = uint32_t((float(wind) / 255.0f) * 3);
			}
			inline XMVECTOR LoadPOS(const wi::primitive::AABB& aabb) const
			{
				XMFLOAT3 v = GetPOS(aabb);
				return XMLoadFloat3(&v);
			}
			constexpr XMFLOAT3 GetPOS(const wi::primitive::AABB& aabb) const
			{
				XMFLOAT3 v = XMFLOAT3(
					float(x) / 1023.0f,
					float(y) / 1023.0f,
					float(z) / 1023.0f
				);
				return wi::math::Lerp(aabb._min, aabb._max, v);
			}
			constexpr uint8_t GetWind() const
			{
				return uint8_t((float(w) / 3.0f) * 255);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R10G10B10A2_UNORM;
		};
		struct Vertex_POS16
		{
			uint16_t x = 0;
			uint16_t y = 0;
			uint16_t z = 0;
			uint16_t w = 0;

			constexpr void FromFULL(const wi::primitive::AABB& aabb, XMFLOAT3 pos, uint8_t wind)
			{
				pos = wi::math::InverseLerp(aabb._min, aabb._max, pos); // UNORM remap
				x = uint16_t(pos.x * 65535.0f);
				y = uint16_t(pos.y * 65535.0f);
				z = uint16_t(pos.z * 65535.0f);
				w = uint16_t((float(wind) / 255.0f) * 65535.0f);
			}
			inline XMVECTOR LoadPOS(const wi::primitive::AABB& aabb) const
			{
				XMFLOAT3 v = GetPOS(aabb);
				return XMLoadFloat3(&v);
			}
			constexpr XMFLOAT3 GetPOS(const wi::primitive::AABB& aabb) const
			{
				XMFLOAT3 v = XMFLOAT3(
					float(x) / 65535.0f,
					float(y) / 65535.0f,
					float(z) / 65535.0f
				);
				return wi::math::Lerp(aabb._min, aabb._max, v);
			}
			constexpr uint8_t GetWind() const
			{
				return uint8_t((float(w) / 65535.0f) * 255);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R16G16B16A16_UNORM;
		};
		struct Vertex_POS32
		{
			float x = 0;
			float y = 0;
			float z = 0;

			constexpr void FromFULL(const XMFLOAT3& pos)
			{
				x = pos.x;
				y = pos.y;
				z = pos.z;
			}
			inline XMVECTOR LoadPOS() const
			{
				return XMVectorSet(x, y, z, 1);
			}
			constexpr XMFLOAT3 GetPOS() const
			{
				return XMFLOAT3(x, y, z);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R32G32B32_FLOAT;
		};
		struct Vertex_POS32W
		{
			float x = 0;
			float y = 0;
			float z = 0;
			float w = 0;

			constexpr void FromFULL(const XMFLOAT3& pos, uint8_t wind)
			{
				x = pos.x;
				y = pos.y;
				z = pos.z;
				w = float(wind) / 255.0f;
			}
			inline XMVECTOR LoadPOS() const
			{
				return XMVectorSet(x, y, z, 1);
			}
			constexpr XMFLOAT3 GetPOS() const
			{
				return XMFLOAT3(x, y, z);
			}
			constexpr uint8_t GetWind() const
			{
				return uint8_t(w * 255);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R32G32B32A32_FLOAT;
		};
		wi::graphics::Format position_format = Vertex_POS16::FORMAT; // CreateRenderData() will choose the appropriate format

		struct Vertex_TEX
		{
			uint16_t x = 0;
			uint16_t y = 0;

			constexpr void FromFULL(const XMFLOAT2& uv, const XMFLOAT2& uv_range_min = XMFLOAT2(0, 0), const XMFLOAT2& uv_range_max = XMFLOAT2(1, 1))
			{
				x = uint16_t(wi::math::InverseLerp(uv_range_min.x, uv_range_max.x, uv.x) * 65535.0f);
				y = uint16_t(wi::math::InverseLerp(uv_range_min.y, uv_range_max.y, uv.y) * 65535.0f);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R16G16_UNORM;
		};
		struct Vertex_UVS
		{
			Vertex_TEX uv0;
			Vertex_TEX uv1;
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R16G16B16A16_UNORM;
		};
		struct Vertex_BON
		{
			uint16_t ind0 = 0;
			uint16_t ind1 = 0;
			uint16_t ind2 = 0;
			uint16_t ind3 = 0;

			uint16_t wei0 = 0;
			uint16_t wei1 = 1;
			uint16_t wei2 = 2;
			uint16_t wei3 = 3;

			constexpr void FromFULL(const XMUINT4& boneIndices, const XMFLOAT4& boneWeights)
			{
				ind0 = uint16_t(boneIndices.x);
				ind1 = uint16_t(boneIndices.y);
				ind2 = uint16_t(boneIndices.z);
				ind3 = uint16_t(boneIndices.w);

				wei0 = uint16_t(boneWeights.x * 65535.0f);
				wei1 = uint16_t(boneWeights.y * 65535.0f);
				wei2 = uint16_t(boneWeights.z * 65535.0f);
				wei3 = uint16_t(boneWeights.w * 65535.0f);
			}
			constexpr XMUINT4 GetInd_FULL() const
			{
				return XMUINT4(ind0, ind1, ind2, ind3);
			}
			constexpr XMFLOAT4 GetWei_FULL() const
			{
				return XMFLOAT4(
					float(wei0) / 65535.0f,
					float(wei1) / 65535.0f,
					float(wei2) / 65535.0f,
					float(wei3) / 65535.0f
				);
			}
		};
		struct Vertex_COL
		{
			uint32_t color = 0;
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_UNORM;
		};
		struct Vertex_NOR
		{
			int8_t x = 0;
			int8_t y = 0;
			int8_t z = 0;
			int8_t w = 0;

			void FromFULL(const XMFLOAT3& nor)
			{
				XMVECTOR N = XMLoadFloat3(&nor);
				N = XMVector3Normalize(N);
				XMFLOAT3 n;
				XMStoreFloat3(&n, N);

				x = int8_t(n.x * 127.5f);
				y = int8_t(n.y * 127.5f);
				z = int8_t(n.z * 127.5f);
				w = 0;
			}
			inline XMFLOAT3 GetNOR() const
			{
				return XMFLOAT3(
					float(x) / 127.5f,
					float(y) / 127.5f,
					float(z) / 127.5f
				);
			}
			inline XMVECTOR LoadNOR() const
			{
				return XMVectorSet(
					float(x) / 127.5f,
					float(y) / 127.5f,
					float(z) / 127.5f,
					0
				);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_SNORM;
		};
		struct Vertex_TAN
		{
			int8_t x = 0;
			int8_t y = 0;
			int8_t z = 0;
			int8_t w = 0;

			void FromFULL(const XMFLOAT4& tan)
			{
				XMVECTOR T = XMLoadFloat4(&tan);
				T = XMVector3Normalize(T);
				XMFLOAT4 t;
				XMStoreFloat4(&t, T);
				t.w = tan.w;

				x = int8_t(t.x * 127.5f);
				y = int8_t(t.y * 127.5f);
				z = int8_t(t.z * 127.5f);
				w = int8_t(t.w * 127.5f);
			}
			inline XMFLOAT4 GetTAN() const
			{
				return XMFLOAT4(
					float(x) / 127.5f,
					float(y) / 127.5f,
					float(z) / 127.5f,
					float(w) / 127.5f
				);
			}
			static constexpr wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_SNORM;
		};

	};

	struct ImpostorComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
		};
		uint32_t _flags = DIRTY;

		float swapInDistance = 100.0f;

		// Non-serialized attributes:
		mutable bool render_dirty = false;
		int textureIndex = -1;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ObjectComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RENDERABLE = 1 << 0,
			CAST_SHADOW = 1 << 1,
			DYNAMIC = 1 << 2,
			_DEPRECATED_IMPOSTOR_PLACEMENT = 1 << 3,
			REQUEST_PLANAR_REFLECTION = 1 << 4,
			LIGHTMAP_RENDER_REQUEST = 1 << 5,
			LIGHTMAP_DISABLE_BLOCK_COMPRESSION = 1 << 6,
			FOREGROUND = 1 << 7,
			NOT_VISIBLE_IN_MAIN_CAMERA = 1 << 8,
			NOT_VISIBLE_IN_REFLECTIONS = 1 << 9,
		};
		uint32_t _flags = RENDERABLE | CAST_SHADOW;

		wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;
		uint32_t cascadeMask = 0; // which shadow cascades to skip from lowest detail to highest detail (0: skip none, 1: skip first, etc...)
		uint32_t filterMask = 0;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 emissiveColor = XMFLOAT4(1, 1, 1, 1);
		uint8_t userStencilRef = 0;
		float lod_distance_multiplier = 1;
		float draw_distance = std::numeric_limits<float>::max(); // object will begin to fade out at this distance to camera
		uint32_t lightmapWidth = 0;
		uint32_t lightmapHeight = 0;
		wi::vector<uint8_t> lightmapTextureData;
		uint32_t sort_priority = 0; // increase to draw earlier (currently 4 bits will be used)

		// Non-serialized attributes:
		uint32_t filterMaskDynamic = 0;

		wi::graphics::Texture lightmap;
		mutable uint32_t lightmapIterationCount = 0;

		XMFLOAT3 center = XMFLOAT3(0, 0, 0);
		float radius = 0;
		float fadeDistance = 0;

		uint32_t lod = 0;

		// these will only be valid for a single frame:
		uint32_t mesh_index = ~0u;
		uint32_t sort_bits = 0;

		inline void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }
		inline void SetRequestPlanarReflection(bool value) { if (value) { _flags |= REQUEST_PLANAR_REFLECTION; } else { _flags &= ~REQUEST_PLANAR_REFLECTION; } }
		inline void SetLightmapRenderRequest(bool value) { if (value) { _flags |= LIGHTMAP_RENDER_REQUEST; } else { _flags &= ~LIGHTMAP_RENDER_REQUEST; } }
		inline void SetLightmapDisableBlockCompression(bool value) { if (value) { _flags |= LIGHTMAP_DISABLE_BLOCK_COMPRESSION; } else { _flags &= ~LIGHTMAP_DISABLE_BLOCK_COMPRESSION; } }
		// Foreground object will be rendered in front of regular objects
		inline void SetForeground(bool value) { if (value) { _flags |= FOREGROUND; } else { _flags &= ~FOREGROUND; } }
		// With this you can disable object rendering for main camera (DRAWSCENE_MAINCAMERA)
		inline void SetNotVisibleInMainCamera(bool value) { if (value) { _flags |= NOT_VISIBLE_IN_MAIN_CAMERA; } else { _flags &= ~NOT_VISIBLE_IN_MAIN_CAMERA; } }
		// With this you can disable object rendering for reflections
		inline void SetNotVisibleInReflections(bool value) { if (value) { _flags |= NOT_VISIBLE_IN_REFLECTIONS; } else { _flags &= ~NOT_VISIBLE_IN_REFLECTIONS; } }

		inline bool IsRenderable() const { return (_flags & RENDERABLE) && (GetTransparency() < 0.99f); }
		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsDynamic() const { return _flags & DYNAMIC; }
		inline bool IsRequestPlanarReflection() const { return _flags & REQUEST_PLANAR_REFLECTION; }
		inline bool IsLightmapRenderRequested() const { return _flags & LIGHTMAP_RENDER_REQUEST; }
		inline bool IsLightmapDisableBlockCompression() const { return _flags & LIGHTMAP_DISABLE_BLOCK_COMPRESSION; }
		inline bool IsForeground() const { return _flags & FOREGROUND; }
		inline bool IsNotVisibleInMainCamera() const { return _flags & NOT_VISIBLE_IN_MAIN_CAMERA; }
		inline bool IsNotVisibleInReflections() const { return _flags & NOT_VISIBLE_IN_REFLECTIONS; }

		inline float GetTransparency() const { return 1 - color.w; }
		inline uint32_t GetFilterMask() const { return filterMask | filterMaskDynamic; }

		// User stencil value can be in range [0, 15]
		//	Values greater than 0 can be used to override userStencilRef of MaterialComponent
		inline void SetUserStencilRef(uint8_t value)
		{
			assert(value < 16);
			userStencilRef = value & 0x0F;
		}

		void ClearLightmap();
		void SaveLightmap(); // not thread safe if LIGHTMAP_BLOCK_COMPRESSION is enabled!
		void CompressLightmap(); // not thread safe if LIGHTMAP_BLOCK_COMPRESSION is enabled!

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct RigidBodyPhysicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DISABLE_DEACTIVATION = 1 << 0,
			KINEMATIC = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		enum CollisionShape
		{
			BOX,
			SPHERE,
			CAPSULE,
			CONVEX_HULL,
			TRIANGLE_MESH,
			ENUM_FORCE_UINT32 = 0xFFFFFFFF
		};
		CollisionShape shape;
		float mass = 1.0f;
		float friction = 0.5f;
		float restitution = 0.0f;
		float damping_linear = 0.0f;
		float damping_angular = 0.0f;

		struct BoxParams
		{
			XMFLOAT3 halfextents = XMFLOAT3(1, 1, 1);
		} box;
		struct SphereParams
		{
			float radius = 1;
		} sphere;
		struct CapsuleParams
		{
			float radius = 1;
			float height = 1;
		} capsule;

		// This will force LOD level for rigid body if it is a TRIANGLE_MESH shape:
		//	The geometry for LOD level will be taken from MeshComponent.
		//	The physics object will need to be recreated for it to take effect.
		uint32_t mesh_lod = 0;

		// Non-serialized attributes:
		std::shared_ptr<void> physicsobject = nullptr; // You can set to null to recreate the physics object the next time phsyics system will be running.

		inline void SetDisableDeactivation(bool value) { if (value) { _flags |= DISABLE_DEACTIVATION; } else { _flags &= ~DISABLE_DEACTIVATION; } }
		inline void SetKinematic(bool value) { if (value) { _flags |= KINEMATIC; } else { _flags &= ~KINEMATIC; } }

		inline bool IsDisableDeactivation() const { return _flags & DISABLE_DEACTIVATION; }
		inline bool IsKinematic() const { return _flags & KINEMATIC; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct SoftBodyPhysicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			SAFE_TO_REGISTER = 1 << 0,
			DISABLE_DEACTIVATION = 1 << 1,
			FORCE_RESET = 1 << 2,
		};
		uint32_t _flags = DISABLE_DEACTIVATION;

		float mass = 1.0f;
		float friction = 0.5f;
		float restitution = 0.0f;
		wi::vector<uint32_t> physicsToGraphicsVertexMapping; // maps graphics vertex index to physics vertex index of the same position
		wi::vector<uint32_t> graphicsToPhysicsVertexMapping; // maps a physics vertex index to first graphics vertex index of the same position
		wi::vector<float> weights; // weight per physics vertex controlling the mass. (0: disable weight (no physics, only animation), 1: default weight)

		// Non-serialized attributes:
		std::shared_ptr<void> physicsobject = nullptr; // You can set to null to recreate the physics object the next time phsyics system will be running.
		XMFLOAT4X4 worldMatrix = wi::math::IDENTITY_MATRIX;
		wi::vector<MeshComponent::Vertex_POS32> vertex_positions_simulation; // graphics vertices after simulation (world space)
		wi::vector<MeshComponent::Vertex_NOR> vertex_normals_simulation;
		wi::vector<XMFLOAT4>vertex_tangents_tmp;
		wi::vector<MeshComponent::Vertex_TAN> vertex_tangents_simulation;
		wi::primitive::AABB aabb;

		inline void SetDisableDeactivation(bool value) { if (value) { _flags |= DISABLE_DEACTIVATION; } else { _flags &= ~DISABLE_DEACTIVATION; } }

		inline bool IsDisableDeactivation() const { return _flags & DISABLE_DEACTIVATION; }

		inline bool HasVertices() const
		{
			return !vertex_positions_simulation.empty();
		}

		// Create physics represenation of graphics mesh
		void CreateFromMesh(const MeshComponent& mesh);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ArmatureComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		wi::vector<wi::ecs::Entity> boneCollection;
		wi::vector<XMFLOAT4X4> inverseBindMatrices;

		// Non-serialized attributes:
		wi::primitive::AABB aabb;
		uint32_t gpuBoneOffset = 0;
		wi::vector<ShaderTransform> boneData;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct LightComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			CAST_SHADOW = 1 << 0,
			VOLUMETRICS = 1 << 1,
			VISUALIZER = 1 << 2,
			LIGHTMAPONLY_STATIC = 1 << 3,
			VOLUMETRICCLOUDS = 1 << 4,
		};
		uint32_t _flags = EMPTY;

		enum LightType
		{
			DIRECTIONAL = ENTITY_TYPE_DIRECTIONALLIGHT,
			POINT = ENTITY_TYPE_POINTLIGHT,
			SPOT = ENTITY_TYPE_SPOTLIGHT,
			//SPHERE = ENTITY_TYPE_SPHERELIGHT,
			//DISC = ENTITY_TYPE_DISCLIGHT,
			//RECTANGLE = ENTITY_TYPE_RECTANGLELIGHT,
			//TUBE = ENTITY_TYPE_TUBELIGHT,
			LIGHTTYPE_COUNT,
			ENUM_FORCE_UINT32 = 0xFFFFFFFF,
		};
		LightType type = POINT;

		XMFLOAT3 color = XMFLOAT3(1, 1, 1);
		float intensity = 1.0f; // Brightness of light in. The units that this is defined in depend on the type of light. Point and spot lights use luminous intensity in candela (lm/sr) while directional lights use illuminance in lux (lm/m2). https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
		float range = 10.0f;
		float outerConeAngle = XM_PIDIV4;
		float innerConeAngle = 0; // default value is 0, means only outer cone angle is used
		float radius = 0.025f;
		float length = 0;

		wi::vector<float> cascade_distances = { 8,80,800 };
		wi::vector<std::string> lensFlareNames;

		int forced_shadow_resolution = -1; // -1: disabled, greater: fixed shadow map resolution

		// Non-serialized attributes:
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 direction = XMFLOAT3(0, 1, 0);
		XMFLOAT4 rotation = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT3 scale = XMFLOAT3(1, 1, 1);
		mutable int occlusionquery = -1;
		wi::rectpacker::Rect shadow_rect = {};

		wi::vector<wi::Resource> lensFlareRimTextures;

		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetVolumetricsEnabled(bool value) { if (value) { _flags |= VOLUMETRICS; } else { _flags &= ~VOLUMETRICS; } }
		inline void SetVisualizerEnabled(bool value) { if (value) { _flags |= VISUALIZER; } else { _flags &= ~VISUALIZER; } }
		inline void SetStatic(bool value) { if (value) { _flags |= LIGHTMAPONLY_STATIC; } else { _flags &= ~LIGHTMAPONLY_STATIC; } }
		inline void SetVolumetricCloudsEnabled(bool value) { if (value) { _flags |= VOLUMETRICCLOUDS; } else { _flags &= ~VOLUMETRICCLOUDS; } }

		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsVolumetricsEnabled() const { return _flags & VOLUMETRICS; }
		inline bool IsVisualizerEnabled() const { return _flags & VISUALIZER; }
		inline bool IsStatic() const { return _flags & LIGHTMAPONLY_STATIC; }
		inline bool IsVolumetricCloudsEnabled() const { return _flags & VOLUMETRICCLOUDS; }

		inline float GetRange() const
		{
			float retval = range;
			retval = std::max(0.001f, retval);
			retval = std::min(retval, 65504.0f); // clamp to 16-bit float max value
			return retval;
		}

		inline void SetType(LightType val) { type = val; }
		inline LightType GetType() const { return type; }

		// Set energy amount with non physical light units (from before version 0.70.0):
		inline void BackCompatSetEnergy(float energy)
		{
			switch (type)
			{
			case wi::scene::LightComponent::POINT:
				intensity = energy * 20;
				break;
			case wi::scene::LightComponent::SPOT:
				intensity = energy * 200;
				break;
			default:
				break;
			}
		}

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct CameraComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			CUSTOM_PROJECTION = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		float width = 0.0f;
		float height = 0.0f;
		float zNearP = 0.1f;
		float zFarP = 5000.0f;
		float fov = XM_PI / 3.0f;
		float focal_length = 1;
		float aperture_size = 0;
		XMFLOAT2 aperture_shape = XMFLOAT2(1, 1);

		// Non-serialized attributes:
		XMFLOAT3 Eye = XMFLOAT3(0, 0, 0);
		XMFLOAT3 At = XMFLOAT3(0, 0, 1);
		XMFLOAT3 Up = XMFLOAT3(0, 1, 0);
		XMFLOAT3X3 rotationMatrix;
		XMFLOAT4X4 View, Projection, VP;
		wi::primitive::Frustum frustum;
		XMFLOAT4X4 InvView, InvProjection, InvVP;
		XMFLOAT2 jitter;
		XMFLOAT4 clipPlane = XMFLOAT4(0, 0, 0, 0); // default: no clip plane
		XMFLOAT4 clipPlaneOriginal = XMFLOAT4(0, 0, 0, 0); // not reversed clip plane
		wi::Canvas canvas;
		wi::graphics::Rect scissor;
		uint32_t sample_count = 1;
		int texture_primitiveID_index = -1;
		int texture_depth_index = -1;
		int texture_lineardepth_index = -1;
		int texture_velocity_index = -1;
		int texture_normal_index = -1;
		int texture_roughness_index = -1;
		int texture_reflection_index = -1;
		int texture_reflection_depth_index = -1;
		int texture_refraction_index = -1;
		int texture_waterriples_index = -1;
		int texture_ao_index = -1;
		int texture_ssr_index = -1;
		int texture_rtshadow_index = -1;
		int texture_rtdiffuse_index = -1;
		int texture_surfelgi_index = -1;
		int buffer_entitytiles_opaque_index = -1;
		int buffer_entitytiles_transparent_index = -1;
		int texture_vxgi_diffuse_index = -1;
		int texture_vxgi_specular_index = -1;

		void CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV = XM_PI / 3.0f);
		void UpdateCamera();
		void TransformCamera(const XMMATRIX& W);
		void TransformCamera(const TransformComponent& transform) { TransformCamera(XMLoadFloat4x4(&transform.world)); }
		void Reflect(const XMFLOAT4& plane = XMFLOAT4(0, 1, 0, 0));

		inline XMVECTOR GetEye() const { return XMLoadFloat3(&Eye); }
		inline XMVECTOR GetAt() const { return XMLoadFloat3(&At); }
		inline XMVECTOR GetUp() const { return XMLoadFloat3(&Up); }
		inline XMVECTOR GetRight() const { return XMVector3Cross(GetAt(), GetUp()); }
		inline XMMATRIX GetView() const { return XMLoadFloat4x4(&View); }
		inline XMMATRIX GetInvView() const { return XMLoadFloat4x4(&InvView); }
		inline XMMATRIX GetProjection() const { return XMLoadFloat4x4(&Projection); }
		inline XMMATRIX GetInvProjection() const { return XMLoadFloat4x4(&InvProjection); }
		inline XMMATRIX GetViewProjection() const { return XMLoadFloat4x4(&VP); }
		inline XMMATRIX GetInvViewProjection() const { return XMLoadFloat4x4(&InvVP); }

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline void SetCustomProjectionEnabled(bool value = true) { if (value) { _flags |= CUSTOM_PROJECTION; } else { _flags &= ~CUSTOM_PROJECTION; } }
		inline bool IsDirty() const { return _flags & DIRTY; }
		inline bool IsCustomProjectionEnabled() const { return _flags & CUSTOM_PROJECTION; }

		void Lerp(const CameraComponent& a, const CameraComponent& b, float t);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct EnvironmentProbeComponent
	{
		static constexpr uint32_t envmapMSAASampleCount = 8;
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			REALTIME = 1 << 1,
			MSAA = 1 << 2,
		};
		uint32_t _flags = DIRTY;
		uint32_t resolution = 128; // power of two
		std::string textureName; // if texture is coming from an asset

		// Non-serialized attributes:
		wi::graphics::Texture texture;
		wi::Resource resource; // if texture is coming from an asset
		XMFLOAT3 position;
		float range;
		XMFLOAT4X4 inverseMatrix;
		mutable bool render_dirty = false;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; DeleteResource(); } else { _flags &= ~DIRTY; } }
		inline void SetRealTime(bool value) { if (value) { _flags |= REALTIME; } else { _flags &= ~REALTIME; } }
		inline void SetMSAA(bool value) { if (value) { _flags |= MSAA; } else { _flags &= ~MSAA; } SetDirty(); }

		inline bool IsDirty() const { return _flags & DIRTY; }
		inline bool IsRealTime() const { return _flags & REALTIME; }
		inline bool IsMSAA() const { return _flags & MSAA; }

		size_t GetMemorySizeInBytes() const;

		void CreateRenderData();
		void DeleteResource();

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ForceFieldComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		enum class Type
		{
			Point,
			Plane,
		} type = Type::Point;

		float gravity = 0; // negative = deflector, positive = attractor
		float range = 0; // affection range

		// Non-serialized attributes:
		XMFLOAT3 position;
		XMFLOAT3 direction;

		inline float GetRange() const { return range; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct DecalComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			BASECOLOR_ONLY_ALPHA = 1 << 0,
		};
		uint32_t _flags = EMPTY;

		float slopeBlendPower = 0;

		// Set decal to only use alpha from base color texture. Useful for blending normalmap-only decals
		constexpr void SetBaseColorOnlyAlpha(bool value) { if (value) { _flags |= BASECOLOR_ONLY_ALPHA; } else { _flags &= ~BASECOLOR_ONLY_ALPHA; } }

		constexpr bool IsBaseColorOnlyAlpha() const { return _flags & BASECOLOR_ONLY_ALPHA; }

		// Non-serialized attributes:
		float emissive;
		XMFLOAT4 color;
		XMFLOAT3 front;
		float normal_strength;
		XMFLOAT3 position;
		float range;
		XMFLOAT4X4 world;
		XMFLOAT4 texMulAdd;

		wi::Resource texture;
		wi::Resource normal;
		wi::Resource surfacemap;

		inline float GetOpacity() const { return color.w; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct AnimationDataComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		wi::vector<float> keyframe_times;
		wi::vector<float> keyframe_data;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct AnimationComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			LOOPED = 1 << 1,
		};
		uint32_t _flags = LOOPED;
		float start = 0;
		float end = 0;
		float timer = 0;
		float amount = 1;	// blend amount
		float speed = 1;

		struct AnimationChannel
		{
			enum FLAGS
			{
				EMPTY = 0,
			};
			uint32_t _flags = EMPTY;

			wi::ecs::Entity target = wi::ecs::INVALID_ENTITY;
			int samplerIndex = -1;
			int retargetIndex = -1;

			enum class Path
			{
				TRANSLATION,
				ROTATION,
				SCALE,
				WEIGHTS,

				LIGHT_COLOR,
				LIGHT_INTENSITY,
				LIGHT_RANGE,
				LIGHT_INNERCONE,
				LIGHT_OUTERCONE,
				// additional light paths can go here...
				_LIGHT_RANGE_END = LIGHT_COLOR + 1000,

				SOUND_PLAY,
				SOUND_STOP,
				SOUND_VOLUME,
				// additional sound paths can go here...
				_SOUND_RANGE_END = SOUND_PLAY + 1000,

				EMITTER_EMITCOUNT,
				// additional emitter paths can go here...
				_EMITTER_RANGE_END = EMITTER_EMITCOUNT + 1000,

				CAMERA_FOV,
				CAMERA_FOCAL_LENGTH,
				CAMERA_APERTURE_SIZE,
				CAMERA_APERTURE_SHAPE,
				// additional camera paths can go here...
				_CAMERA_RANGE_END = CAMERA_FOV + 1000,

				SCRIPT_PLAY,
				SCRIPT_STOP,
				// additional script paths can go here...
				_SCRIPT_RANGE_END = SCRIPT_PLAY + 1000,

				MATERIAL_COLOR,
				MATERIAL_EMISSIVE,
				MATERIAL_ROUGHNESS,
				MATERIAL_METALNESS,
				MATERIAL_REFLECTANCE,
				MATERIAL_TEXMULADD,
				// additional material paths can go here...
				_MATERIAL_RANGE_END = MATERIAL_COLOR + 1000,

				UNKNOWN,
			} path = Path::UNKNOWN;

			enum class PathDataType
			{
				Event,
				Float,
				Float2,
				Float3,
				Float4,
				Weights,

				Count,
			};
			PathDataType GetPathDataType() const;

			// Non-serialized attributes:
			mutable int next_event = 0;
		};
		struct AnimationSampler
		{
			enum FLAGS
			{
				EMPTY = 0,
			};
			uint32_t _flags = LOOPED;

			wi::ecs::Entity data = wi::ecs::INVALID_ENTITY;

			enum Mode
			{
				LINEAR,
				STEP,
				CUBICSPLINE,
				MODE_FORCE_UINT32 = 0xFFFFFFFF
			} mode = LINEAR;

			// The data is now not part of the sampler, so it can be shared. This is kept only for backwards compatibility with previous versions.
			AnimationDataComponent backwards_compatibility_data;

			// Non-serialized attributes:
			const void* scene = nullptr; // if animation data is in a different scene (if retargetting from a separate scene)
		};
		struct RetargetSourceData
		{
			wi::ecs::Entity source = wi::ecs::INVALID_ENTITY;
			XMFLOAT4X4 dstRelativeMatrix = wi::math::IDENTITY_MATRIX;
			XMFLOAT4X4 srcRelativeParentMatrix = wi::math::IDENTITY_MATRIX;
		};

		wi::vector<AnimationChannel> channels;
		wi::vector<AnimationSampler> samplers;
		wi::vector<RetargetSourceData> retargets;

		// Non-serialzied attributes:
		wi::vector<float> morph_weights_temp;
		float last_update_time = 0;

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsLooped() const { return _flags & LOOPED; }
		inline float GetLength() const { return end - start; }
		inline bool IsEnded() const { return timer >= end; }

		inline void Play() { _flags |= PLAYING; }
		inline void Pause() { _flags &= ~PLAYING; }
		inline void Stop() { Pause(); timer = 0.0f; last_update_time = timer; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct WeatherComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			OCEAN_ENABLED = 1 << 0,
			_DEPRECATED_SIMPLE_SKY = 1 << 1,
			REALISTIC_SKY = 1 << 2,
			VOLUMETRIC_CLOUDS = 1 << 3,
			HEIGHT_FOG = 1 << 4,
			VOLUMETRIC_CLOUDS_CAST_SHADOW = 1 << 5,
			OVERRIDE_FOG_COLOR = 1 << 6,
			REALISTIC_SKY_AERIAL_PERSPECTIVE = 1 << 7,
			REALISTIC_SKY_HIGH_QUALITY = 1 << 8,
			REALISTIC_SKY_RECEIVE_SHADOW = 1 << 9,
			VOLUMETRIC_CLOUDS_RECEIVE_SHADOW = 1 << 10,
		};
		uint32_t _flags = EMPTY;

		inline bool IsOceanEnabled() const { return _flags & OCEAN_ENABLED; }
		inline bool IsRealisticSky() const { return _flags & REALISTIC_SKY; }
		inline bool IsVolumetricClouds() const { return _flags & VOLUMETRIC_CLOUDS; }
		inline bool IsHeightFog() const { return _flags & HEIGHT_FOG; }
		inline bool IsVolumetricCloudsCastShadow() const { return _flags & VOLUMETRIC_CLOUDS_CAST_SHADOW; }
		inline bool IsOverrideFogColor() const { return _flags & OVERRIDE_FOG_COLOR; }
		inline bool IsRealisticSkyAerialPerspective() const { return _flags & REALISTIC_SKY_AERIAL_PERSPECTIVE; }
		inline bool IsRealisticSkyHighQuality() const { return _flags & REALISTIC_SKY_HIGH_QUALITY; }
		inline bool IsRealisticSkyReceiveShadow() const { return _flags & REALISTIC_SKY_RECEIVE_SHADOW; }
		inline bool IsVolumetricCloudsReceiveShadow() const { return _flags & VOLUMETRIC_CLOUDS_RECEIVE_SHADOW; }

		inline void SetOceanEnabled(bool value = true) { if (value) { _flags |= OCEAN_ENABLED; } else { _flags &= ~OCEAN_ENABLED; } }
		inline void SetRealisticSky(bool value = true) { if (value) { _flags |= REALISTIC_SKY; } else { _flags &= ~REALISTIC_SKY; } }
		inline void SetVolumetricClouds(bool value = true) { if (value) { _flags |= VOLUMETRIC_CLOUDS; } else { _flags &= ~VOLUMETRIC_CLOUDS; } }
		inline void SetHeightFog(bool value = true) { if (value) { _flags |= HEIGHT_FOG; } else { _flags &= ~HEIGHT_FOG; } }
		inline void SetVolumetricCloudsCastShadow(bool value = true) { if (value) { _flags |= VOLUMETRIC_CLOUDS_CAST_SHADOW; } else { _flags &= ~VOLUMETRIC_CLOUDS_CAST_SHADOW; } }
		inline void SetOverrideFogColor(bool value = true) { if (value) { _flags |= OVERRIDE_FOG_COLOR; } else { _flags &= ~OVERRIDE_FOG_COLOR; } }
		inline void SetRealisticSkyAerialPerspective(bool value = true) { if (value) { _flags |= REALISTIC_SKY_AERIAL_PERSPECTIVE; } else { _flags &= ~REALISTIC_SKY_AERIAL_PERSPECTIVE; } }
		inline void SetRealisticSkyHighQuality(bool value = true) { if (value) { _flags |= REALISTIC_SKY_HIGH_QUALITY; } else { _flags &= ~REALISTIC_SKY_HIGH_QUALITY; } }
		inline void SetRealisticSkyReceiveShadow(bool value = true) { if (value) { _flags |= REALISTIC_SKY_RECEIVE_SHADOW; } else { _flags &= ~REALISTIC_SKY_RECEIVE_SHADOW; } }
		inline void SetVolumetricCloudsReceiveShadow(bool value = true) { if (value) { _flags |= VOLUMETRIC_CLOUDS_RECEIVE_SHADOW; } else { _flags &= ~VOLUMETRIC_CLOUDS_RECEIVE_SHADOW; } }

		XMFLOAT3 sunColor = XMFLOAT3(0, 0, 0);
		XMFLOAT3 sunDirection = XMFLOAT3(0, 1, 0);
		float skyExposure = 1;
		XMFLOAT3 horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 zenith = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		float fogStart = 100;
		float fogDensity = 0;
		float fogHeightStart = 1;
		float fogHeightEnd = 3;
		XMFLOAT3 windDirection = XMFLOAT3(0, 0, 0);
		float windRandomness = 5;
		float windWaveSize = 1;
		float windSpeed = 1;
		float stars = 0.5f;
		XMFLOAT3 gravity = XMFLOAT3(0, -10, 0);
		float sky_rotation = 0; // horizontal rotation for skyMap texture (in radians)
		float rain_amount = 0;
		float rain_length = 0.04f;
		float rain_speed = 1;
		float rain_scale = 0.005f;
		float rain_splash_scale = 0.1f;
		XMFLOAT4 rain_color = XMFLOAT4(0.6f, 0.8f, 1, 0.5f);

		wi::Ocean::OceanParameters oceanParameters;
		AtmosphereParameters atmosphereParameters;
		VolumetricCloudParameters volumetricCloudParameters;

		std::string skyMapName;
		std::string colorGradingMapName;
		std::string volumetricCloudsWeatherMapFirstName;
		std::string volumetricCloudsWeatherMapSecondName;

		// Non-serialized attributes:
		uint32_t most_important_light_index = ~0u;
		wi::Resource skyMap;
		wi::Resource colorGradingMap;
		wi::Resource volumetricCloudsWeatherMapFirst;
		wi::Resource volumetricCloudsWeatherMapSecond;
		XMFLOAT4 stars_rotation_quaternion = XMFLOAT4(0, 0, 0, 1);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct SoundComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			LOOPED = 1 << 1,
			DISABLE_3D = 1 << 2,
		};
		uint32_t _flags = LOOPED;

		std::string filename;
		wi::Resource soundResource;
		wi::audio::SoundInstance soundinstance;
		float volume = 1;

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsLooped() const { return _flags & LOOPED; }
		inline bool IsDisable3D() const { return _flags & DISABLE_3D; }

		void Play();
		void Stop();
		void SetLooped(bool value = true);
		void SetDisable3D(bool value = true);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct VideoComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			LOOPED = 1 << 1,
		};
		uint32_t _flags = LOOPED;

		std::string filename;
		wi::Resource videoResource;
		wi::video::VideoInstance videoinstance;

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsLooped() const { return _flags & LOOPED; }

		inline void Play() { _flags |= PLAYING; }
		inline void Stop() { _flags &= ~PLAYING; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct InverseKinematicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DISABLED = 1 << 0,
		};
		uint32_t _flags = EMPTY;

		wi::ecs::Entity target = wi::ecs::INVALID_ENTITY; // which entity to follow (must have a transform component)
		uint32_t chain_length = 0; // recursive depth
		uint32_t iteration_count = 1; // computation step count. Increase this too for greater chain length

		inline void SetDisabled(bool value = true) { if (value) { _flags |= DISABLED; } else { _flags &= ~DISABLED; } }
		inline bool IsDisabled() const { return _flags & DISABLED; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct SpringComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			RESET = 1 << 0,
			DISABLED = 1 << 1,
			STRETCH_ENABLED = 1 << 2,
			GRAVITY_ENABLED = 1 << 3,
		};
		uint32_t _flags = RESET | GRAVITY_ENABLED;

		float stiffnessForce = 0.5f;
		float dragForce = 0.5f;
		float windForce = 0.5f;
		float hitRadius = 0;
		float gravityPower = 0.5f;
		XMFLOAT3 gravityDir = {};

		// Non-serialized attributes:
		XMFLOAT3 prevTail = {};
		XMFLOAT3 currentTail = {};
		XMFLOAT3 boneAxis = {};

		inline void Reset(bool value = true) { if (value) { _flags |= RESET; } else { _flags &= ~RESET; } }
		inline void SetDisabled(bool value = true) { if (value) { _flags |= DISABLED; } else { _flags &= ~DISABLED; } }
		inline void SetStretchEnabled(bool value) { if (value) { _flags |= STRETCH_ENABLED; } else { _flags &= ~STRETCH_ENABLED; } }
		inline void SetGravityEnabled(bool value) { if (value) { _flags |= GRAVITY_ENABLED; } else { _flags &= ~GRAVITY_ENABLED; } }

		inline bool IsResetting() const { return _flags & RESET; }
		inline bool IsDisabled() const { return _flags & DISABLED; }
		inline bool IsStretchEnabled() const { return _flags & STRETCH_ENABLED; }
		inline bool IsGravityEnabled() const { return _flags & GRAVITY_ENABLED; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ColliderComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			CPU = 1 << 0,
			GPU = 1 << 1,
		};
		uint32_t _flags = CPU;

		constexpr void SetCPUEnabled(bool value = true) { if (value) { _flags |= CPU; } else { _flags &= ~CPU; } }
		constexpr void SetGPUEnabled(bool value = true) { if (value) { _flags |= GPU; } else { _flags &= ~GPU; } }

		constexpr bool IsCPUEnabled() const { return _flags & CPU; }
		constexpr bool IsGPUEnabled() const { return _flags & GPU; }

		enum class Shape
		{
			Sphere,
			Capsule,
			Plane,
		};
		Shape shape;

		float radius = 0;
		XMFLOAT3 offset = {};
		XMFLOAT3 tail = {};

		// Non-serialized attributes:
		wi::primitive::Sphere sphere;
		wi::primitive::Capsule capsule;
		wi::primitive::Plane plane;
		uint32_t layerMask = ~0u;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ScriptComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			PLAYING = 1 << 0,
			PLAY_ONCE = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		std::string filename;

		// Non-serialized attributes:
		wi::vector<uint8_t> script; // compiled script binary data
		wi::Resource resource;

		inline void Play() { _flags |= PLAYING; }
		inline void SetPlayOnce(bool once = true) { if (once) { _flags |= PLAY_ONCE; } else { _flags &= ~PLAY_ONCE; } }
		inline void Stop() { _flags &= ~PLAYING; }

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsPlayingOnlyOnce() const { return _flags & PLAY_ONCE; }

		void CreateFromFile(const std::string& filename);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ExpressionComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			FORCE_TALKING = 1 << 0,
			TALKING_ENDED = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		// Preset expressions can have common behaviours assigned:
		//	https://github.com/vrm-c/vrm-specification/blob/bd205a6c3839993f2729e4e7c3a74af89877cfce/specification/VRMC_vrm-1.0-beta/expressions.md#preset-expressions
		enum class Preset
		{
			// Emotions:
			Happy,		// Changed from joy
			Angry,		// anger
			Sad,		// Changed from sorrow
			Relaxed,	// Comfortable. Changed from fun
			Surprised,	// surprised. Added new in VRM 1.0

			// Lip sync procedural:
			//	Procedural: A value that can be automatically generated by the system.
			//	- Analyze microphone input, generate from text, etc.
			Aa,			// aa
			Ih,			// i
			Ou,			// u
			Ee,			// eh
			Oh,			// oh

			// Blink procedural:
			//	Procedural: A value that can be automatically generated by the system.
			//	- Randomly blink, etc.
			Blink,		// close both eyelids
			BlinkLeft,	// Close the left eyelid
			BlinkRight,	// Close right eyelid

			// Gaze procedural:
			//	Procedural: A value that can be automatically generated by the system.
			//	- The VRM LookAt will generate a value for the gaze point from time to time (see LookAt Expression Type).
			LookUp,		// For models where the line of sight moves with Expression instead of bone. See eye control.
			LookDown,	// For models where the line of sight moves with Expression instead of bone. See eye control.
			LookLeft,	// For models whose line of sight moves with Expression instead of bone. See eye control.
			LookRight,	// For models where the line of sight moves with Expression instead of bone. See eye control.

			// Other:
			Neutral,	// left for backwards compatibility.

			Count,
		};
		int presets[size_t(Preset::Count)] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

		enum class Override
		{
			None,
			Block,
			Blend,
		};

		float blink_frequency = 0.3f;	// number of blinks per second
		float blink_length = 0.1f;	// blink's completion time in seconds
		int blink_count = 2;
		float look_frequency = 0;	// number of lookAt changes per second
		float look_length = 0.6f;	// lookAt's completion time in seconds

		struct Expression
		{
			enum FLAGS
			{
				EMPTY = 0,
				DIRTY = 1 << 0,
				BINARY = 1 << 1,
			};
			uint32_t _flags = EMPTY;

			std::string name;
			float weight = 0;

			Preset preset = Preset::Count;
			Override override_mouth = Override::None;
			Override override_blink = Override::None;
			Override override_look = Override::None;

			struct MorphTargetBinding
			{
				wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;
				int index = 0;
				float weight = 0;
			};
			wi::vector<MorphTargetBinding> morph_target_bindings;

			constexpr bool IsDirty() const { return _flags & DIRTY; }
			constexpr bool IsBinary() const { return _flags & BINARY; }

			constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
			constexpr void SetBinary(bool value = true) { if (value) { _flags |= BINARY; } else { _flags &= ~BINARY; } }

			// Set weight of expression (also sets dirty state if value is out of date)
			inline void SetWeight(float value)
			{
				if (std::abs(weight - value) > std::numeric_limits<float>::epsilon())
				{
					SetDirty(true);
				}
				weight = value;
			}
		};
		wi::vector<Expression> expressions;

		constexpr bool IsForceTalkingEnabled() const { return _flags & FORCE_TALKING; }

		// Force continuous talking animation, even if no voice is playing
		inline void SetForceTalkingEnabled(bool value = true) { if (value) { _flags |= FORCE_TALKING; } else { _flags &= ~FORCE_TALKING; _flags |= TALKING_ENDED; } }

		// Non-serialized attributes:
		float blink_timer = 0;
		float look_timer = 0;
		float look_weights[4] = {};
		Preset talking_phoneme = Preset::Aa;
		float talking_weight_prev_prev = 0;
		float talking_weight_prev = 0;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct HumanoidComponent
	{
		enum FLAGS
		{
			NONE = 0,
			LOOKAT = 1 << 0,
			RAGDOLL_PHYSICS = 1 << 1,
		};
		uint32_t _flags = LOOKAT;

		// https://github.com/vrm-c/vrm-specification/blob/master/specification/VRMC_vrm-1.0-beta/humanoid.md#list-of-humanoid-bones
		enum class HumanoidBone
		{
			// Torso:
			Hips,			// Required
			Spine,			// Required
			Chest,
			UpperChest,
			Neck,

			// Head:
			Head,			// Required
			LeftEye,
			RightEye,
			Jaw,

			// Leg:
			LeftUpperLeg,	// Required
			LeftLowerLeg,	// Required
			LeftFoot,		// Required
			LeftToes,
			RightUpperLeg,	// Required
			RightLowerLeg,	// Required
			RightFoot,		// Required
			RightToes,

			// Arm:
			LeftShoulder,
			LeftUpperArm,	// Required
			LeftLowerArm,	// Required
			LeftHand,		// Required
			RightShoulder,
			RightUpperArm,	// Required
			RightLowerArm,	// Required
			RightHand,		// Required

			// Finger:
			LeftThumbMetacarpal,
			LeftThumbProximal,
			LeftThumbDistal,
			LeftIndexProximal,
			LeftIndexIntermediate,
			LeftIndexDistal,
			LeftMiddleProximal,
			LeftMiddleIntermediate,
			LeftMiddleDistal,
			LeftRingProximal,
			LeftRingIntermediate,
			LeftRingDistal,
			LeftLittleProximal,
			LeftLittleIntermediate,
			LeftLittleDistal,
			RightThumbMetacarpal,
			RightThumbProximal,
			RightThumbDistal,
			RightIndexIntermediate,
			RightIndexDistal,
			RightIndexProximal,
			RightMiddleProximal,
			RightMiddleIntermediate,
			RightMiddleDistal,
			RightRingProximal,
			RightRingIntermediate,
			RightRingDistal,
			RightLittleProximal,
			RightLittleIntermediate,
			RightLittleDistal,

			Count
		};
		wi::ecs::Entity bones[size_t(HumanoidBone::Count)] = {};

		constexpr bool IsLookAtEnabled() const { return _flags & LOOKAT; }
		constexpr bool IsRagdollPhysicsEnabled() const { return _flags & RAGDOLL_PHYSICS; }

		constexpr void SetLookAtEnabled(bool value = true) { if (value) { _flags |= LOOKAT; } else { _flags &= ~LOOKAT; } }
		constexpr void SetRagdollPhysicsEnabled(bool value = true) { if (value) { _flags |= RAGDOLL_PHYSICS; } else { _flags &= ~RAGDOLL_PHYSICS; } }

		XMFLOAT3 default_look_direction = XMFLOAT3(0, 0, 1);
		XMFLOAT2 head_rotation_max = XMFLOAT2(XM_PI / 3.0f, XM_PI / 6.0f);
		float head_rotation_speed = 0.1f;
		XMFLOAT2 eye_rotation_max = XMFLOAT2(XM_PI / 20.0f, XM_PI / 20.0f);
		float eye_rotation_speed = 0.1f;

		// Non-serialized attributes:
		XMFLOAT3 lookAt = {}; // lookAt target pos, can be set by user
		XMFLOAT4 lookAtDeltaRotationState_Head = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT4 lookAtDeltaRotationState_LeftEye = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT4 lookAtDeltaRotationState_RightEye = XMFLOAT4(0, 0, 0, 1);
		std::shared_ptr<void> ragdoll = nullptr; // physics system implementation-specific object

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};
}
