#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiPrimitive.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "shaders/ShaderInterop_Renderer.h"
#include "wiJobSystem.h"
#include "wiAudio.h"
#include "wiResourceManager.h"
#include "wiSpinLock.h"
#include "wiGPUBVH.h"
#include "wiOcean.h"
#include "wiSprite.h"
#include "wiMath.h"
#include "wiECS.h"
#include "wiVector.h"

#include <string>
#include <memory>
#include <limits>

namespace wi
{
	class Archive;
}

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

	struct PreviousFrameTransformComponent
	{
		// Non-serialized attributes:
		XMFLOAT4X4 world_prev;

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

			TEXTURESLOT_COUNT
		};
		struct TextureMap
		{
			std::string name;
			uint32_t uvset = 0;
			wi::Resource resource;
			const wi::graphics::GPUResource* GetGPUResource() const
			{
				if (!resource.IsValid() || !resource.GetTexture().IsValid())
					return nullptr;
				return &resource.GetTexture();
			}
			int GetUVSet() const
			{
				if (!resource.IsValid() || !resource.GetTexture().IsValid())
					return -1;
				return (int)uvset;
			}
		};
		TextureMap textures[TEXTURESLOT_COUNT];

		int customShaderID = -1;

		// Non-serialized attributes:
		uint32_t layerMask = ~0u;

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

		inline wi::enums::BLENDMODE GetBlendMode() const { if (userBlendMode == wi::enums::BLENDMODE_OPAQUE && (GetRenderTypes() & wi::enums::RENDERTYPE_TRANSPARENT)) return wi::enums::BLENDMODE_ALPHA; else return userBlendMode; }
		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }
		inline bool IsUsingVertexColors() const { return _flags & USE_VERTEXCOLORS; }
		inline bool IsUsingWind() const { return _flags & USE_WIND; }
		inline bool IsReceiveShadow() const { return (_flags & DISABLE_RECEIVE_SHADOW) == 0; }
		inline bool IsUsingSpecularGlossinessWorkflow() const { return _flags & SPECULAR_GLOSSINESS_WORKFLOW; }
		inline bool IsOcclusionEnabled_Primary() const { return _flags & OCCLUSION_PRIMARY; }
		inline bool IsOcclusionEnabled_Secondary() const { return _flags & OCCLUSION_SECONDARY; }
		inline bool IsCustomShader() const { return customShaderID >= 0; }
		inline bool IsDoubleSided() const { return  _flags & DOUBLE_SIDED; }

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
		inline void SetUseSpecularGlossinessWorkflow(bool value) { SetDirty(); if (value) { _flags |= SPECULAR_GLOSSINESS_WORKFLOW; } else { _flags &= ~SPECULAR_GLOSSINESS_WORKFLOW; }  }
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

		// The MaterialComponent will be written to ShaderMaterial (a struct that is optimized for GPU use)
		void WriteShaderMaterial(ShaderMaterial* dest) const;

		// Retrieve the array of textures from the material
		void WriteTextures(const wi::graphics::GPUResource** dest, int count) const;

		// Returns the bitwise OR of all the RENDERTYPE flags applicable to this material
		uint32_t GetRenderTypes() const;

		// Create constant buffer and texture resources for GPU
		void CreateRenderData();

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
			TERRAIN = 1 << 3,
			_DEPRECATED_DIRTY_MORPH = 1 << 4,
			_DEPRECATED_DIRTY_BINDLESS = 1 << 5,
			TLAS_FORCE_DOUBLE_SIDED = 1 << 6,
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

		// Terrain blend materials:
		//	There are 4 blend materials, the first one (default) being the subset material
		//	Must have TERRAIN flag enabled
		//	Must have vertex colors to blend between materials
		//	extra materials that are not set will use the base subset material
		wi::ecs::Entity terrain_material1 = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity terrain_material2 = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity terrain_material3 = wi::ecs::INVALID_ENTITY;

		// Morph Targets
		struct MeshMorphTarget
		{
		    wi::vector<XMFLOAT3> vertex_positions;
		    wi::vector<XMFLOAT3> vertex_normals;
		    float_t weight;
		};
		wi::vector<MeshMorphTarget> targets;

		// Non-serialized attributes:
		wi::primitive::AABB aabb;
		wi::graphics::GPUBuffer indexBuffer;
		wi::graphics::GPUBuffer vertexBuffer_POS;
		wi::graphics::GPUBuffer vertexBuffer_TAN;
		wi::graphics::GPUBuffer vertexBuffer_UV0;
		wi::graphics::GPUBuffer vertexBuffer_UV1;
		wi::graphics::GPUBuffer vertexBuffer_BON;
		wi::graphics::GPUBuffer vertexBuffer_COL;
		wi::graphics::GPUBuffer vertexBuffer_ATL;
		wi::graphics::GPUBuffer vertexBuffer_PRE;
		wi::graphics::GPUBuffer streamoutBuffer_POS;
		wi::graphics::GPUBuffer streamoutBuffer_TAN;
		wi::vector<uint8_t> vertex_subsets;
		wi::graphics::GPUBuffer subsetBuffer;

		wi::graphics::RaytracingAccelerationStructure BLAS;
		enum BLAS_STATE
		{
			BLAS_STATE_NEEDS_REBUILD,
			BLAS_STATE_NEEDS_REFIT,
			BLAS_STATE_COMPLETE,
		};
		mutable BLAS_STATE BLAS_state = BLAS_STATE_NEEDS_REBUILD;

		// Only valid for 1 frame material component indices:
		uint32_t terrain_material1_index = ~0u;
		uint32_t terrain_material2_index = ~0u;
		uint32_t terrain_material3_index = ~0u;

		mutable bool dirty_morph = false;
		mutable bool dirty_subsets = true;

		inline void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		inline void SetDoubleSided(bool value) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
		inline void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }
		inline void SetTerrain(bool value) { if (value) { _flags |= TERRAIN; } else { _flags &= ~TERRAIN; } }
		
		inline bool IsRenderable() const { return _flags & RENDERABLE; }
		inline bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }
		inline bool IsDynamic() const { return _flags & DYNAMIC; }
		inline bool IsTerrain() const { return _flags & TERRAIN; }

		inline float GetTessellationFactor() const { return tessellationFactor; }
		inline wi::graphics::IndexBufferFormat GetIndexFormat() const { return vertex_positions.size() > 65535 ? wi::graphics::IndexBufferFormat::UINT32 : wi::graphics::IndexBufferFormat::UINT16; }
		inline size_t GetIndexStride() const { return GetIndexFormat() == wi::graphics::IndexBufferFormat::UINT32 ? sizeof(uint32_t) : sizeof(uint16_t); }
		inline bool IsSkinned() const { return armatureID != wi::ecs::INVALID_ENTITY; }

		// Recreates GPU resources for index/vertex buffers
		void CreateRenderData();
		void WriteShaderMesh(ShaderMesh* dest) const;

		enum COMPUTE_NORMALS
		{
			COMPUTE_NORMALS_HARD,		// hard face normals, can result in additional vertices generated
			COMPUTE_NORMALS_SMOOTH,		// smooth per vertex normals, this can remove/simplyfy geometry, but slow
			COMPUTE_NORMALS_SMOOTH_FAST	// average normals, vertex count will be unchanged, fast
		};
		void ComputeNormals(COMPUTE_NORMALS compute);
		void FlipCulling();
		void FlipNormals();
		void Recenter();
		void RecenterToBottom();
		wi::primitive::Sphere GetBoundingSphere() const;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);


		struct Vertex_POS
		{
			XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
			uint32_t normal_wind = 0;

			void FromFULL(const XMFLOAT3& _pos, const XMFLOAT3& _nor, uint8_t wind)
			{
				pos.x = _pos.x;
				pos.y = _pos.y;
				pos.z = _pos.z;
				MakeFromParams(_nor, wind);
			}
			inline XMVECTOR LoadPOS() const
			{
				return XMLoadFloat3(&pos);
			}
			inline XMVECTOR LoadNOR() const
			{
				XMFLOAT3 N = GetNor_FULL();
				return XMLoadFloat3(&N);
			}
			inline void MakeFromParams(const XMFLOAT3& normal)
			{
				normal_wind = normal_wind & 0xFF000000; // reset only the normals

				normal_wind |= (uint32_t)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
				normal_wind |= (uint32_t)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
				normal_wind |= (uint32_t)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
			}
			inline void MakeFromParams(const XMFLOAT3& normal, uint8_t wind)
			{
				normal_wind = 0;

				normal_wind |= (uint32_t)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
				normal_wind |= (uint32_t)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
				normal_wind |= (uint32_t)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
				normal_wind |= (uint32_t)wind << 24;
			}
			inline XMFLOAT3 GetNor_FULL() const
			{
				XMFLOAT3 nor_FULL(0, 0, 0);

				nor_FULL.x = (float)((normal_wind >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
				nor_FULL.y = (float)((normal_wind >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
				nor_FULL.z = (float)((normal_wind >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;

				return nor_FULL;
			}
			inline uint8_t GetWind() const
			{
				return (normal_wind >> 24) & 0x000000FF;
			}

			static const wi::graphics::Format FORMAT = wi::graphics::Format::R32G32B32A32_FLOAT;
		};
		struct Vertex_TEX
		{
			XMHALF2 tex = XMHALF2(0.0f, 0.0f);

			void FromFULL(const XMFLOAT2& texcoords)
			{
				tex = XMHALF2(texcoords.x, texcoords.y);
			}

			static const wi::graphics::Format FORMAT = wi::graphics::Format::R16G16_FLOAT;
		};
		struct Vertex_BON
		{
			uint64_t ind = 0;
			uint64_t wei = 0;

			void FromFULL(const XMUINT4& boneIndices, const XMFLOAT4& boneWeights)
			{
				ind = 0;
				wei = 0;

				ind |= (uint64_t)boneIndices.x << 0;
				ind |= (uint64_t)boneIndices.y << 16;
				ind |= (uint64_t)boneIndices.z << 32;
				ind |= (uint64_t)boneIndices.w << 48;

				wei |= (uint64_t)(boneWeights.x * 65535.0f) << 0;
				wei |= (uint64_t)(boneWeights.y * 65535.0f) << 16;
				wei |= (uint64_t)(boneWeights.z * 65535.0f) << 32;
				wei |= (uint64_t)(boneWeights.w * 65535.0f) << 48;
			}
			inline XMUINT4 GetInd_FULL() const
			{
				XMUINT4 ind_FULL(0, 0, 0, 0);

				ind_FULL.x = ((ind >> 0) & 0x0000FFFF);
				ind_FULL.y = ((ind >> 16) & 0x0000FFFF);
				ind_FULL.z = ((ind >> 32) & 0x0000FFFF);
				ind_FULL.w = ((ind >> 48) & 0x0000FFFF);

				return ind_FULL;
			}
			inline XMFLOAT4 GetWei_FULL() const
			{
				XMFLOAT4 wei_FULL(0, 0, 0, 0);

				wei_FULL.x = (float)((wei >> 0) & 0x0000FFFF) / 65535.0f;
				wei_FULL.y = (float)((wei >> 16) & 0x0000FFFF) / 65535.0f;
				wei_FULL.z = (float)((wei >> 32) & 0x0000FFFF) / 65535.0f;
				wei_FULL.w = (float)((wei >> 48) & 0x0000FFFF) / 65535.0f;

				return wei_FULL;
			}
		};
		struct Vertex_COL
		{
			uint32_t color = 0;
			static const wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_UNORM;
		};
		struct Vertex_TAN
		{
			uint32_t tangent = 0;

			void FromFULL(const XMFLOAT4& tan)
			{
				XMVECTOR T = XMLoadFloat4(&tan);
				T = XMVector3Normalize(T);
				XMFLOAT4 t;
				XMStoreFloat4(&t, T);
				t.w = tan.w;
				tangent = 0;
				tangent |= (uint)((t.x * 0.5f + 0.5f) * 255.0f) << 0;
				tangent |= (uint)((t.y * 0.5f + 0.5f) * 255.0f) << 8;
				tangent |= (uint)((t.z * 0.5f + 0.5f) * 255.0f) << 16;
				tangent |= (uint)((t.w * 0.5f + 0.5f) * 255.0f) << 24;
			}

			static const wi::graphics::Format FORMAT = wi::graphics::Format::R8G8B8A8_UNORM;
		};
		
		// Non serialized attributes:
		wi::vector<Vertex_POS> vertex_positions_morphed;

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
		wi::primitive::AABB aabb;
		XMFLOAT4 color;
		float fadeThresholdRadius;
		wi::vector<uint32_t> instances;
		mutable bool render_dirty = false;

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
			IMPOSTOR_PLACEMENT = 1 << 3,
			REQUEST_PLANAR_REFLECTION = 1 << 4,
			LIGHTMAP_RENDER_REQUEST = 1 << 5,
		};
		uint32_t _flags = RENDERABLE | CAST_SHADOW;

		wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;
		uint32_t cascadeMask = 0; // which shadow cascades to skip from lowest detail to highest detail (0: skip none, 1: skip first, etc...)
		uint32_t rendertypeMask = 0;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 emissiveColor = XMFLOAT4(1, 1, 1, 1);

		uint32_t lightmapWidth = 0;
		uint32_t lightmapHeight = 0;
		wi::vector<uint8_t> lightmapTextureData;

		uint8_t userStencilRef = 0;

		// Non-serialized attributes:

		wi::graphics::Texture lightmap;
		wi::graphics::RenderPass renderpass_lightmap_clear;
		wi::graphics::RenderPass renderpass_lightmap_accumulate;
		mutable uint32_t lightmapIterationCount = 0;

		XMFLOAT3 center = XMFLOAT3(0, 0, 0);
		float impostorFadeThresholdRadius;
		float impostorSwapDistance;

		// these will only be valid for a single frame:
		int transform_index = -1;
		int prev_transform_index = -1;

		// occlusion result history bitfield (32 bit->32 frame history)
		mutable uint32_t occlusionHistory = ~0u;
		mutable int occlusionQueries[wi::graphics::GraphicsDevice::GetBufferCount() + 1];

		inline bool IsOccluded() const
		{
			// Perform a conservative occlusion test:
			// If it is visible in any frames in the history, it is determined visible in this frame
			// But if all queries failed in the history, it is occluded.
			// If it pops up for a frame after occluded, it is visible again for some frames
			return occlusionHistory == 0;
		}

		inline void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }
		inline void SetImpostorPlacement(bool value) { if (value) { _flags |= IMPOSTOR_PLACEMENT; } else { _flags &= ~IMPOSTOR_PLACEMENT; } }
		inline void SetRequestPlanarReflection(bool value) { if (value) { _flags |= REQUEST_PLANAR_REFLECTION; } else { _flags &= ~REQUEST_PLANAR_REFLECTION; } }
		inline void SetLightmapRenderRequest(bool value) { if (value) { _flags |= LIGHTMAP_RENDER_REQUEST; } else { _flags &= ~LIGHTMAP_RENDER_REQUEST; } }

		inline bool IsRenderable() const { return _flags & RENDERABLE; }
		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsDynamic() const { return _flags & DYNAMIC; }
		inline bool IsImpostorPlacement() const { return _flags & IMPOSTOR_PLACEMENT; }
		inline bool IsRequestPlanarReflection() const { return _flags & REQUEST_PLANAR_REFLECTION; }
		inline bool IsLightmapRenderRequested() const { return _flags & LIGHTMAP_RENDER_REQUEST; }

		inline float GetTransparency() const { return 1 - color.w; }
		inline uint32_t GetRenderTypes() const { return rendertypeMask; }

		// User stencil value can be in range [0, 15]
		//	Values greater than 0 can be used to override userStencilRef of MaterialComponent
		inline void SetUserStencilRef(uint8_t value)
		{
			assert(value < 16);
			userStencilRef = value & 0x0F;
		}

		void ClearLightmap();
		void SaveLightmap();
		void CompressLightmap();

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

		// Non-serialized attributes:
		void* physicsobject = nullptr;

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
		void* physicsobject = nullptr;
		XMFLOAT4X4 worldMatrix = wi::math::IDENTITY_MATRIX;
		wi::vector<MeshComponent::Vertex_POS> vertex_positions_simulation; // graphics vertices after simulation (world space)
		wi::vector<XMFLOAT4>vertex_tangents_tmp;
		wi::vector<MeshComponent::Vertex_TAN> vertex_tangents_simulation;
		wi::primitive::AABB aabb;

		inline void SetDisableDeactivation(bool value) { if (value) { _flags |= DISABLE_DEACTIVATION; } else { _flags &= ~DISABLE_DEACTIVATION; } }

		inline bool IsDisableDeactivation() const { return _flags & DISABLE_DEACTIVATION; }

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

		wi::vector<ShaderTransform> boneData;
		wi::graphics::GPUBuffer boneBuffer;

		void CreateRenderData();

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
		};
		uint32_t _flags = EMPTY;
		XMFLOAT3 color = XMFLOAT3(1, 1, 1);

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
		float energy = 1.0f;
		float range_local = 10.0f;
		float fov = XM_PIDIV4;

		wi::vector<std::string> lensFlareNames;

		// Non-serialized attributes:
		XMFLOAT3 position;
		float range_global;
		XMFLOAT3 direction;
		XMFLOAT4 rotation;
		XMFLOAT3 scale;
		XMFLOAT3 front;
		XMFLOAT3 right;
		mutable int occlusionquery = -1;

		wi::vector<wi::Resource> lensFlareRimTextures;

		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetVolumetricsEnabled(bool value) { if (value) { _flags |= VOLUMETRICS; } else { _flags &= ~VOLUMETRICS; } }
		inline void SetVisualizerEnabled(bool value) { if (value) { _flags |= VISUALIZER; } else { _flags &= ~VISUALIZER; } }
		inline void SetStatic(bool value) { if (value) { _flags |= LIGHTMAPONLY_STATIC; } else { _flags &= ~LIGHTMAPONLY_STATIC; } }

		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsVolumetricsEnabled() const { return _flags & VOLUMETRICS; }
		inline bool IsVisualizerEnabled() const { return _flags & VISUALIZER; }
		inline bool IsStatic() const { return _flags & LIGHTMAPONLY_STATIC; }

		inline float GetRange() const { return range_global; }

		inline void SetType(LightType val) { type = val; }
		inline LightType GetType() const { return type; }

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
		float zFarP = 800.0f;
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
		wi::Canvas canvas;
		uint32_t sample_count = 1;
		int texture_depth_index = -1;
		int texture_lineardepth_index = -1;
		int texture_gbuffer0_index = -1;
		int texture_gbuffer1_index = -1;
		int texture_reflection_index = -1;
		int texture_refraction_index = -1;
		int texture_waterriples_index = -1;
		int texture_ao_index = -1;
		int texture_ssr_index = -1;
		int texture_rtshadow_index = -1;
		int texture_surfelgi_index = -1;
		int buffer_entitytiles_opaque_index = -1;
		int buffer_entitytiles_transparent_index = -1;

		void CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV = XM_PI / 3.0f);
		void UpdateCamera();
		void TransformCamera(const TransformComponent& transform);
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

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct EnvironmentProbeComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DIRTY = 1 << 0,
			REALTIME = 1 << 1,
		};
		uint32_t _flags = DIRTY;

		// Non-serialized attributes:
		int textureIndex = -1;
		XMFLOAT3 position;
		float range;
		XMFLOAT4X4 inverseMatrix;
		mutable bool render_dirty = false;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline void SetRealTime(bool value) { if (value) { _flags |= REALTIME; } else { _flags &= ~REALTIME; } }

		inline bool IsDirty() const { return _flags & DIRTY; }
		inline bool IsRealTime() const { return _flags & REALTIME; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct ForceFieldComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		int type = ENTITY_TYPE_FORCEFIELD_POINT;
		float gravity = 0.0f; // negative = deflector, positive = attractor
		float range_local = 0.0f; // affection range

		// Non-serialized attributes:
		XMFLOAT3 position;
		float range_global;
		XMFLOAT3 direction;

		inline float GetRange() const { return range_global; }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct DecalComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		// Non-serialized attributes:
		XMFLOAT4 color;
		float emissive;
		XMFLOAT3 front;
		XMFLOAT3 position;
		float range;
		XMFLOAT4X4 world;

		wi::Resource texture;
		wi::Resource normal;

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
			uint32_t _flags = LOOPED;

			wi::ecs::Entity target = wi::ecs::INVALID_ENTITY;
			int samplerIndex = -1;

			enum Path
			{
				TRANSLATION,
				ROTATION,
				SCALE,
				WEIGHTS,
				UNKNOWN,
				TYPE_FORCE_UINT32 = 0xFFFFFFFF
			} path = TRANSLATION;
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
		};
		wi::vector<AnimationChannel> channels;
		wi::vector<AnimationSampler> samplers;

		// Non-serialzied attributes:
		wi::vector<float> morph_weights_temp;

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsLooped() const { return _flags & LOOPED; }
		inline float GetLength() const { return end - start; }
		inline bool IsEnded() const { return timer >= end; }

		inline void Play() { _flags |= PLAYING; }
		inline void Pause() { _flags &= ~PLAYING; }
		inline void Stop() { Pause(); timer = 0.0f; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct WeatherComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			OCEAN_ENABLED = 1 << 0,
			SIMPLE_SKY = 1 << 1,
			REALISTIC_SKY = 1 << 2,
			VOLUMETRIC_CLOUDS = 1 << 3,
			HEIGHT_FOG = 1 << 4,
		};
		uint32_t _flags = EMPTY;

		inline bool IsOceanEnabled() const { return _flags & OCEAN_ENABLED; }
		inline bool IsSimpleSky() const { return _flags & SIMPLE_SKY; }
		inline bool IsRealisticSky() const { return _flags & REALISTIC_SKY; }
		inline bool IsVolumetricClouds() const { return _flags & VOLUMETRIC_CLOUDS; }
		inline bool IsHeightFog() const { return _flags & HEIGHT_FOG; }

		inline void SetOceanEnabled(bool value = true) { if (value) { _flags |= OCEAN_ENABLED; } else { _flags &= ~OCEAN_ENABLED; } }
		inline void SetSimpleSky(bool value = true) { if (value) { _flags |= SIMPLE_SKY; } else { _flags &= ~SIMPLE_SKY; } }
		inline void SetRealisticSky(bool value = true) { if (value) { _flags |= REALISTIC_SKY; } else { _flags &= ~REALISTIC_SKY; } }
		inline void SetVolumetricClouds(bool value = true) { if (value) { _flags |= VOLUMETRIC_CLOUDS; } else { _flags &= ~VOLUMETRIC_CLOUDS; } }
		inline void SetHeightFog(bool value = true) { if (value) { _flags |= HEIGHT_FOG; } else { _flags &= ~HEIGHT_FOG; } }

		XMFLOAT3 sunColor = XMFLOAT3(0, 0, 0);
		XMFLOAT3 sunDirection = XMFLOAT3(0, 1, 0);
		float sunEnergy = 0;
		float skyExposure = 1;
		XMFLOAT3 horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 zenith = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		float fogStart = 100;
		float fogEnd = 1000;
		float fogHeightStart = 1;
		float fogHeightEnd = 3;
		float fogHeightSky = 0;
		float cloudiness = 0.0f;
		float cloudScale = 0.0003f;
		float cloudSpeed = 0.1f;
		XMFLOAT3 windDirection = XMFLOAT3(0, 0, 0);
		float windRandomness = 5;
		float windWaveSize = 1;
		float windSpeed = 1;

		wi::Ocean::OceanParameters oceanParameters;
		AtmosphereParameters atmosphereParameters;
		VolumetricCloudParameters volumetricCloudParameters;

		std::string skyMapName;
		std::string colorGradingMapName;

		// Non-serialized attributes:
		uint32_t most_important_light_index = ~0u;
		wi::Resource skyMap;
		wi::Resource colorGradingMap;

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

		inline void Play() { _flags |= PLAYING; }
		inline void Stop() { _flags &= ~PLAYING; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }
		inline void SetDisable3D(bool value = true) { if (value) { _flags |= DISABLE_3D; } else { _flags &= ~DISABLE_3D; } }

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
		uint32_t chain_length = ~0u; // ~0 means: compute until the root
		uint32_t iteration_count = 1;

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
		uint32_t _flags = RESET;

		float stiffness = 100;
		float damping = 0.8f;
		float wind_affection = 0;

		// Non-serialized attributes:
		XMFLOAT3 center_of_mass;
		XMFLOAT3 velocity;

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

	struct Scene
	{
		wi::ecs::ComponentManager<NameComponent> names;
		wi::ecs::ComponentManager<LayerComponent> layers;
		wi::ecs::ComponentManager<TransformComponent> transforms;
		wi::ecs::ComponentManager<PreviousFrameTransformComponent> prev_transforms;
		wi::ecs::ComponentManager<HierarchyComponent> hierarchy;
		wi::ecs::ComponentManager<MaterialComponent> materials;
		wi::ecs::ComponentManager<MeshComponent> meshes;
		wi::ecs::ComponentManager<ImpostorComponent> impostors;
		wi::ecs::ComponentManager<ObjectComponent> objects;
		wi::ecs::ComponentManager<wi::primitive::AABB> aabb_objects;
		wi::ecs::ComponentManager<RigidBodyPhysicsComponent> rigidbodies;
		wi::ecs::ComponentManager<SoftBodyPhysicsComponent> softbodies;
		wi::ecs::ComponentManager<ArmatureComponent> armatures;
		wi::ecs::ComponentManager<LightComponent> lights;
		wi::ecs::ComponentManager<wi::primitive::AABB> aabb_lights;
		wi::ecs::ComponentManager<CameraComponent> cameras;
		wi::ecs::ComponentManager<EnvironmentProbeComponent> probes;
		wi::ecs::ComponentManager<wi::primitive::AABB> aabb_probes;
		wi::ecs::ComponentManager<ForceFieldComponent> forces;
		wi::ecs::ComponentManager<DecalComponent> decals;
		wi::ecs::ComponentManager<wi::primitive::AABB> aabb_decals;
		wi::ecs::ComponentManager<AnimationComponent> animations;
		wi::ecs::ComponentManager<AnimationDataComponent> animation_datas;
		wi::ecs::ComponentManager<EmittedParticleSystem> emitters;
		wi::ecs::ComponentManager<HairParticleSystem> hairs;
		wi::ecs::ComponentManager<WeatherComponent> weathers;
		wi::ecs::ComponentManager<SoundComponent> sounds;
		wi::ecs::ComponentManager<InverseKinematicsComponent> inverse_kinematics;
		wi::ecs::ComponentManager<SpringComponent> springs;

		// Non-serialized attributes:
		float dt = 0;
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t flags = EMPTY;


		wi::SpinLock locker;
		wi::primitive::AABB bounds;
		wi::vector<wi::primitive::AABB> parallel_bounds;
		WeatherComponent weather;
		wi::graphics::RaytracingAccelerationStructure TLAS;
		wi::graphics::GPUBuffer TLAS_instancesUpload[wi::graphics::GraphicsDevice::GetBufferCount()];
		void* TLAS_instancesMapped = nullptr;
		wi::GPUBVH BVH; // this is for non-hardware accelerated raytracing
		mutable bool acceleration_structure_update_requested = false;
		void SetAccelerationStructureUpdateRequested(bool value = true) { acceleration_structure_update_requested = value; }
		bool IsAccelerationStructureUpdateRequested() const { return acceleration_structure_update_requested; }

		// Shader visible scene parameters:
		ShaderScene shaderscene;

		// Instances for bindless visiblity indexing:
		//	contains in order:
		//		1) objects
		//		2) hair particles
		//		3) emitted particles
		wi::graphics::GPUBuffer instanceUploadBuffer[wi::graphics::GraphicsDevice::GetBufferCount()];
		ShaderMeshInstance* instanceArrayMapped = nullptr;
		size_t instanceArraySize = 0;
		wi::graphics::GPUBuffer instanceBuffer;

		// Meshes for bindless visiblity indexing:
		//	contains in order:
		//		1) meshes
		//		2) hair particles
		//		3) emitted particles
		wi::graphics::GPUBuffer meshUploadBuffer[wi::graphics::GraphicsDevice::GetBufferCount()];
		ShaderMesh* meshArrayMapped = nullptr;
		size_t meshArraySize = 0;
		wi::graphics::GPUBuffer meshBuffer;

		// Materials for bindless visibility indexing:
		wi::graphics::GPUBuffer materialUploadBuffer[wi::graphics::GraphicsDevice::GetBufferCount()];
		ShaderMaterial* materialArrayMapped = nullptr;
		size_t materialArraySize = 0;
		wi::graphics::GPUBuffer materialBuffer;

		// Occlusion query state:
		wi::graphics::GPUQueryHeap queryHeap;
		wi::graphics::GPUBuffer queryResultBuffer[arraysize(ObjectComponent::occlusionQueries)];
		wi::graphics::GPUBuffer queryPredicationBuffer;
		int queryheap_idx = 0;
		mutable std::atomic<uint32_t> queryAllocator{ 0 };

		// Surfel GI resources:
		wi::graphics::GPUBuffer surfelBuffer;
		wi::graphics::GPUBuffer surfelDataBuffer;
		wi::graphics::GPUBuffer surfelAliveBuffer[2];
		wi::graphics::GPUBuffer surfelDeadBuffer;
		wi::graphics::GPUBuffer surfelStatsBuffer;
		wi::graphics::GPUBuffer surfelGridBuffer;
		wi::graphics::GPUBuffer surfelCellBuffer;
		wi::graphics::Texture surfelMomentsTexture[2];


		// Environment probe cubemap array state:
		static constexpr uint32_t envmapCount = 16;
		static constexpr uint32_t envmapRes = 128;
		static constexpr uint32_t envmapMIPs = 8;
		wi::graphics::Texture envrenderingDepthBuffer;
		wi::graphics::Texture envmapArray;
		wi::vector<wi::graphics::RenderPass> renderpasses_envmap;

		// Impostor texture array state:
		static constexpr uint32_t maxImpostorCount = 8;
		static constexpr uint32_t impostorTextureDim = 128;
		wi::graphics::Texture impostorDepthStencil;
		wi::graphics::Texture impostorArray;
		wi::vector<wi::graphics::RenderPass> renderpasses_impostor;

		mutable std::atomic_bool lightmap_refresh_needed{ false };

		// Ocean GPU state:
		wi::Ocean ocean;
		void OceanRegenerate() { ocean.Create(weather.oceanParameters); }

		// Simple water ripple sprites:
		mutable wi::vector<wi::Sprite> waterRipples;
		void PutWaterRipple(const std::string& image, const XMFLOAT3& pos);

		// Update all components by a given timestep (in seconds):
		//	This is an expensive function, prefer to call it only once per frame!
		void Update(float dt);
		// Remove everything from the scene that it owns:
		void Clear();
		// Merge an other scene into this.
		//	The contents of the other scene will be lost (and moved to this)!
		void Merge(Scene& other);

		// Removes a specific entity from the scene (if it exists):
		void Entity_Remove(wi::ecs::Entity entity);
		// Finds the first entity by the name (if it exists, otherwise returns INVALID_ENTITY):
		wi::ecs::Entity Entity_FindByName(const std::string& name);
		// Duplicates all of an entity's components and creates a new entity with them (recursively keeps hierarchy):
		wi::ecs::Entity Entity_Duplicate(wi::ecs::Entity entity);
		// Serializes entity and all of its components to archive:
		//	Returns either the new entity that was read, or the original entity that was written
		//	This serialization is recursive and serializes entity hierarchy as well
		wi::ecs::Entity Entity_Serialize(wi::Archive& archive, wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY);

		wi::ecs::Entity Entity_CreateMaterial(
			const std::string& name
		);
		wi::ecs::Entity Entity_CreateObject(
			const std::string& name
		);
		wi::ecs::Entity Entity_CreateMesh(
			const std::string& name
		);
		wi::ecs::Entity Entity_CreateLight(
			const std::string& name, 
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0), 
			const XMFLOAT3& color = XMFLOAT3(1, 1, 1), 
			float energy = 1, 
			float range = 10,
			LightComponent::LightType type = LightComponent::POINT
		);
		wi::ecs::Entity Entity_CreateForce(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wi::ecs::Entity Entity_CreateEnvironmentProbe(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wi::ecs::Entity Entity_CreateDecal(
			const std::string& name,
			const std::string& textureName,
			const std::string& normalMapName = ""
		);
		wi::ecs::Entity Entity_CreateCamera(
			const std::string& name,
			float width, float height, float nearPlane = 0.01f, float farPlane = 1000.0f, float fov = XM_PIDIV4
		);
		wi::ecs::Entity Entity_CreateEmitter(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wi::ecs::Entity Entity_CreateHair(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wi::ecs::Entity Entity_CreateSound(
			const std::string& name,
			const std::string& filename,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);

		// Attaches an entity to a parent:
		//	child_already_in_local_space	:	child won't be transformed from world space to local space
		void Component_Attach(wi::ecs::Entity entity, wi::ecs::Entity parent, bool child_already_in_local_space = false);
		// Detaches the entity from its parent (if it is attached):
		void Component_Detach(wi::ecs::Entity entity);
		// Detaches all children from an entity (if there are any):
		void Component_DetachChildren(wi::ecs::Entity parent);

		void Serialize(wi::Archive& archive);

		void RunPreviousFrameTransformUpdateSystem(wi::jobsystem::context& ctx);
		void RunAnimationUpdateSystem(wi::jobsystem::context& ctx);
		void RunTransformUpdateSystem(wi::jobsystem::context& ctx);
		void RunHierarchyUpdateSystem(wi::jobsystem::context& ctx);
		void RunSpringUpdateSystem(wi::jobsystem::context& ctx);
		void RunInverseKinematicsUpdateSystem(wi::jobsystem::context& ctx);
		void RunArmatureUpdateSystem(wi::jobsystem::context& ctx);
		void RunMeshUpdateSystem(wi::jobsystem::context& ctx);
		void RunMaterialUpdateSystem(wi::jobsystem::context& ctx);
		void RunImpostorUpdateSystem(wi::jobsystem::context& ctx);
		void RunObjectUpdateSystem(wi::jobsystem::context& ctx);
		void RunCameraUpdateSystem(wi::jobsystem::context& ctx);
		void RunDecalUpdateSystem(wi::jobsystem::context& ctx);
		void RunProbeUpdateSystem(wi::jobsystem::context& ctx);
		void RunForceUpdateSystem(wi::jobsystem::context& ctx);
		void RunLightUpdateSystem(wi::jobsystem::context& ctx);
		void RunParticleUpdateSystem(wi::jobsystem::context& ctx);
		void RunWeatherUpdateSystem(wi::jobsystem::context& ctx);
		void RunSoundUpdateSystem(wi::jobsystem::context& ctx);
	};

	// Returns skinned vertex position in armature local space
	//	N : normal (out, optional)
	XMVECTOR SkinVertex(const MeshComponent& mesh, const ArmatureComponent& armature, uint32_t index, XMVECTOR* N = nullptr);


	// Helper that manages a global scene
	inline Scene& GetScene()
	{
		static Scene scene;
		return scene;
	}

	// Helper that manages a global camera
	inline CameraComponent& GetCamera()
	{
		static CameraComponent camera;
		return camera;
	}

	// Helper function to open a wiscene file and add the contents to the global scene
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	wi::ecs::Entity LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	// Helper function to open a wiscene file and add the contents to the specified scene. This is thread safe as it doesn't modify global scene
	//	scene			:	the scene that will contain the model
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	wi::ecs::Entity LoadModel(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	struct PickResult
	{
		wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		float distance = std::numeric_limits<float>::max();
		int subsetIndex = -1;
		int vertexID0 = -1;
		int vertexID1 = -1;
		int vertexID2 = -1;
		XMFLOAT2 bary = XMFLOAT2(0, 0);
		XMFLOAT4X4 orientation = wi::math::IDENTITY_MATRIX;

		bool operator==(const PickResult& other)
		{
			return entity == other.entity;
		}
	};
	// Given a ray, finds the closest intersection point against all mesh instances
	//	ray				:	the incoming ray that will be traced
	//	renderTypeMask	:	filter based on render type
	//	layerMask		:	filter based on layer
	//	scene			:	the scene that will be traced against the ray
	PickResult Pick(const wi::primitive::Ray& ray, uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene());

	struct SceneIntersectSphereResult
	{
		wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		float depth = 0;
	};
	SceneIntersectSphereResult SceneIntersectSphere(const wi::primitive::Sphere& sphere, uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene());
	SceneIntersectSphereResult SceneIntersectCapsule(const wi::primitive::Capsule& capsule, uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene());

}
