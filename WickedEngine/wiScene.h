#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiIntersect.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "ShaderInterop_Renderer.h"
#include "wiJobSystem.h"
#include "wiAudio.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiSpinLock.h"

#include "wiECS.h"
#include "wiScene_Decl.h"

#include <string>
#include <vector>
#include <memory>

class wiArchive;

namespace wiScene
{
	struct NameComponent
	{
		std::string name;

		inline void operator=(const std::string& str) { name = str; }
		inline void operator=(std::string&& str) { name = std::move(str); }
		inline bool operator==(const std::string& str) const { return name.compare(str) == 0; }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct LayerComponent
	{
		uint32_t layerMask = ~0;

		inline uint32_t GetLayerMask() const { return layerMask; }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		XMFLOAT4 rotation_local = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT3 translation_local = XMFLOAT3(0, 0, 0);

		// Non-serialized attributes:
		XMFLOAT4X4 world = IDENTITYMATRIX;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		XMFLOAT3 GetPosition() const;
		XMFLOAT4 GetRotation() const;
		XMFLOAT3 GetScale() const;
		XMVECTOR GetPositionV() const;
		XMVECTOR GetRotationV() const;
		XMVECTOR GetScaleV() const;
		XMMATRIX GetLocalMatrix() const;
		void UpdateTransform();
		void UpdateTransform_Parented(const TransformComponent& parent);
		void ApplyTransform();
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

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct PreviousFrameTransformComponent
	{
		// Non-serialized attributes:
		XMFLOAT4X4 world_prev;

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct HierarchyComponent
	{
		wiECS::Entity parentID = wiECS::INVALID_ENTITY;
		uint32_t layerMask_bind; // saved child layermask at the time of binding

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		};
		uint32_t _flags = DIRTY | CAST_SHADOW;

		enum SHADERTYPE
		{
			SHADERTYPE_PBR,
			SHADERTYPE_PBR_PLANARREFLECTION,
			SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING,
			SHADERTYPE_PBR_ANISOTROPIC,
			SHADERTYPE_WATER,
			SHADERTYPE_CARTOON,
			SHADERTYPE_UNLIT,
			SHADERTYPE_COUNT
		} shaderType = SHADERTYPE_PBR;

		enum SUBSURFACE_PROFILE
		{
			SUBSURFACE_SOLID,
			SUBSURFACE_SKIN,
			SUBSURFACE_SNOW,
		} subsurfaceProfile = SUBSURFACE_SOLID;

		STENCILREF engineStencilRef = STENCILREF_DEFAULT;
		uint8_t userStencilRef = 0;
		BLENDMODE userBlendMode = BLENDMODE_OPAQUE;

		XMFLOAT4 baseColor = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 emissiveColor = XMFLOAT4(1, 1, 1, 0);
		XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0);
		float roughness = 0.2f;
		float reflectance = 0.02f;
		float metalness = 0.0f;
		float refractionIndex = 0.0f;
		float normalMapStrength = 1.0f;
		float parallaxOcclusionMapping = 0.0f;
		float displacementMapping = 0.0f;

		float alphaRef = 1.0f;
		wiGraphics::SHADING_RATE shadingRate = wiGraphics::SHADING_RATE_1X1;
		
		XMFLOAT2 texAnimDirection = XMFLOAT2(0, 0);
		float texAnimFrameRate = 0.0f;
		float texAnimElapsedTime = 0.0f;

		std::string baseColorMapName;
		std::string surfaceMapName;
		std::string normalMapName;
		std::string displacementMapName;
		std::string emissiveMapName;
		std::string occlusionMapName;

		uint32_t uvset_baseColorMap = 0;
		uint32_t uvset_surfaceMap = 0;
		uint32_t uvset_normalMap = 0;
		uint32_t uvset_displacementMap = 0;
		uint32_t uvset_emissiveMap = 0;
		uint32_t uvset_occlusionMap = 0;

		// Non-serialized attributes:
		std::shared_ptr<wiResource> baseColorMap;
		std::shared_ptr<wiResource> surfaceMap;
		std::shared_ptr<wiResource> normalMap;
		std::shared_ptr<wiResource> displacementMap;
		std::shared_ptr<wiResource> emissiveMap;
		std::shared_ptr<wiResource> occlusionMap;
		wiGraphics::GPUBuffer constantBuffer;

		int customShaderID = -1; // for now, this is not serialized; need to consider actual proper use case first

		// User stencil value can be in range [0, 15]
		inline void SetUserStencilRef(uint8_t value)
		{
			assert(value < 16);
			userStencilRef = value & 0x0F;
		}
		inline uint32_t GetStencilRef() const
		{
			return wiRenderer::CombineStencilrefs(engineStencilRef, userStencilRef);
		}

		const wiGraphics::Texture* GetBaseColorMap() const;
		const wiGraphics::Texture* GetNormalMap() const;
		const wiGraphics::Texture* GetSurfaceMap() const;
		const wiGraphics::Texture* GetDisplacementMap() const;
		const wiGraphics::Texture* GetEmissiveMap() const;
		const wiGraphics::Texture* GetOcclusionMap() const;

		inline float GetOpacity() const { return baseColor.w; }
		inline float GetEmissiveStrength() const { return emissiveColor.w; }
		inline int GetCustomShaderID() const { return customShaderID; }

		inline bool HasPlanarReflection() const { return shaderType == SHADERTYPE_PBR_PLANARREFLECTION || shaderType == SHADERTYPE_WATER; }

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetOcclusionEnabled_Primary(bool value) { SetDirty(); if (value) { _flags |= OCCLUSION_PRIMARY; } else { _flags &= ~OCCLUSION_PRIMARY; } }
		inline void SetOcclusionEnabled_Secondary(bool value) { SetDirty(); if (value) { _flags |= OCCLUSION_SECONDARY; } else { _flags &= ~OCCLUSION_SECONDARY; } }

		inline BLENDMODE GetBlendMode() const { if (userBlendMode == BLENDMODE_OPAQUE && (GetRenderTypes() & RENDERTYPE_TRANSPARENT)) return BLENDMODE_ALPHA; else return userBlendMode; }
		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }
		inline bool IsUsingVertexColors() const { return _flags & USE_VERTEXCOLORS; }
		inline bool IsUsingWind() const { return _flags & USE_WIND; }
		inline bool IsUsingSpecularGlossinessWorkflow() const { return _flags & SPECULAR_GLOSSINESS_WORKFLOW; }
		inline bool IsOcclusionEnabled_Primary() const { return _flags & OCCLUSION_PRIMARY; }
		inline bool IsOcclusionEnabled_Secondary() const { return _flags & OCCLUSION_SECONDARY; }
		inline bool IsCustomShader() const { return customShaderID >= 0; }

		inline void SetBaseColor(const XMFLOAT4& value) { SetDirty(); baseColor = value; }
		inline void SetEmissiveColor(const XMFLOAT4& value) { SetDirty(); emissiveColor = value; }
		inline void SetRoughness(float value) { SetDirty(); roughness = value; }
		inline void SetReflectance(float value) { SetDirty(); reflectance = value; }
		inline void SetMetalness(float value) { SetDirty(); metalness = value; }
		inline void SetEmissiveStrength(float value) { SetDirty(); emissiveColor.w = value; }
		inline void SetRefractionIndex(float value) { SetDirty(); refractionIndex = value; }
		inline void SetNormalMapStrength(float value) { SetDirty(); normalMapStrength = value; }
		inline void SetParallaxOcclusionMapping(float value) { SetDirty(); parallaxOcclusionMapping = value; }
		inline void SetDisplacementMapping(float value) { SetDirty(); displacementMapping = value; }
		inline void SetOpacity(float value) { SetDirty(); baseColor.w = value; }
		inline void SetAlphaRef(float value) { SetDirty();  alphaRef = value; }
		inline void SetUseVertexColors(bool value) { SetDirty(); if (value) { _flags |= USE_VERTEXCOLORS; } else { _flags &= ~USE_VERTEXCOLORS; } }
		inline void SetUseWind(bool value) { SetDirty(); if (value) { _flags |= USE_WIND; } else { _flags &= ~USE_WIND; } }
		inline void SetUseSpecularGlossinessWorkflow(bool value) { SetDirty(); if (value) { _flags |= SPECULAR_GLOSSINESS_WORKFLOW; } else { _flags &= ~SPECULAR_GLOSSINESS_WORKFLOW; }  }
		inline void SetCustomShaderID(int id) { customShaderID = id; }
		inline void DisableCustomShader() { customShaderID = -1; }
		inline void SetUVSet_BaseColorMap(uint32_t value) { uvset_baseColorMap = value; SetDirty(); }
		inline void SetUVSet_NormalMap(uint32_t value) { uvset_normalMap = value; SetDirty(); }
		inline void SetUVSet_SurfaceMap(uint32_t value) { uvset_surfaceMap = value; SetDirty(); }
		inline void SetUVSet_DisplacementMap(uint32_t value) { uvset_displacementMap = value; SetDirty(); }
		inline void SetUVSet_EmissiveMap(uint32_t value) { uvset_emissiveMap = value; SetDirty(); }
		inline void SetUVSet_OcclusionMap(uint32_t value) { uvset_occlusionMap = value; SetDirty(); }

		void WriteShaderMaterial(ShaderMaterial* dest) const;
		uint32_t GetRenderTypes() const;

		void CreateRenderData(const std::string& content_dir = "");

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		};
		uint32_t _flags = RENDERABLE;

		std::vector<XMFLOAT3> vertex_positions;
		std::vector<XMFLOAT3> vertex_normals;
		std::vector<XMFLOAT4> vertex_tangents;
		std::vector<XMFLOAT2> vertex_uvset_0;
		std::vector<XMFLOAT2> vertex_uvset_1;
		std::vector<XMUINT4> vertex_boneindices;
		std::vector<XMFLOAT4> vertex_boneweights;
		std::vector<XMFLOAT2> vertex_atlas;
		std::vector<uint32_t> vertex_colors;
		std::vector<uint8_t> vertex_windweights;
		std::vector<uint32_t> indices;

		struct MeshSubset
		{
			wiECS::Entity materialID = wiECS::INVALID_ENTITY;
			uint32_t indexOffset = 0;
			uint32_t indexCount = 0;
			int indexBuffer_subresource = -1;
		};
		std::vector<MeshSubset> subsets;

		float tessellationFactor = 0.0f;
		wiECS::Entity armatureID = wiECS::INVALID_ENTITY;

		// Terrain blend materials:
		//	There are 4 blend materials, the first one (default) being the subset material
		//	Must have TERRAIN flag enabled
		//	Must have vertex colors to blend between materials
		//	extra materials that are not set will use the base subset material
		wiECS::Entity terrain_material1 = wiECS::INVALID_ENTITY;
		wiECS::Entity terrain_material2 = wiECS::INVALID_ENTITY;
		wiECS::Entity terrain_material3 = wiECS::INVALID_ENTITY;

		// Non-serialized attributes:
		AABB aabb;
		wiGraphics::GPUBuffer indexBuffer;
		wiGraphics::GPUBuffer vertexBuffer_POS;
		wiGraphics::GPUBuffer vertexBuffer_TAN;
		wiGraphics::GPUBuffer vertexBuffer_UV0;
		wiGraphics::GPUBuffer vertexBuffer_UV1;
		wiGraphics::GPUBuffer vertexBuffer_BON;
		wiGraphics::GPUBuffer vertexBuffer_COL;
		wiGraphics::GPUBuffer vertexBuffer_ATL;
		wiGraphics::GPUBuffer vertexBuffer_PRE;
		wiGraphics::GPUBuffer streamoutBuffer_POS;
		wiGraphics::GPUBuffer streamoutBuffer_TAN;
		wiGraphics::GPUBuffer vertexBuffer_SUB;
		std::vector<uint8_t> vertex_subsets;

		wiGraphics::RaytracingAccelerationStructure BLAS;
		bool BLAS_build_pending = true;
		uint32_t TLAS_geometryOffset = 0;

		inline void SetRenderable(bool value) { if (value) { _flags |= RENDERABLE; } else { _flags &= ~RENDERABLE; } }
		inline void SetDoubleSided(bool value) { if (value) { _flags |= DOUBLE_SIDED; } else { _flags &= ~DOUBLE_SIDED; } }
		inline void SetDynamic(bool value) { if (value) { _flags |= DYNAMIC; } else { _flags &= ~DYNAMIC; } }
		inline void SetTerrain(bool value) { if (value) { _flags |= TERRAIN; } else { _flags &= ~TERRAIN; } }

		inline bool IsRenderable() const { return _flags & RENDERABLE; }
		inline bool IsDoubleSided() const { return _flags & DOUBLE_SIDED; }
		inline bool IsDynamic() const { return _flags & DYNAMIC; }
		inline bool IsTerrain() const { return _flags & TERRAIN; }

		inline float GetTessellationFactor() const { return tessellationFactor; }
		inline wiGraphics::INDEXBUFFER_FORMAT GetIndexFormat() const { return vertex_positions.size() > 65535 ? wiGraphics::INDEXFORMAT_32BIT : wiGraphics::INDEXFORMAT_16BIT; }
		inline size_t GetIndexStride() const { return GetIndexFormat() == wiGraphics::INDEXFORMAT_32BIT ? sizeof(uint32_t) : sizeof(uint16_t); }
		inline bool IsSkinned() const { return armatureID != wiECS::INVALID_ENTITY; }

		void CreateRenderData();
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
		SPHERE GetBoundingSphere() const;

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);


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

			static const wiGraphics::FORMAT FORMAT = wiGraphics::FORMAT::FORMAT_R32G32B32A32_FLOAT;
		};
		struct Vertex_TEX
		{
			XMHALF2 tex = XMHALF2(0.0f, 0.0f);

			void FromFULL(const XMFLOAT2& texcoords)
			{
				tex = XMHALF2(texcoords.x, texcoords.y);
			}

			static const wiGraphics::FORMAT FORMAT = wiGraphics::FORMAT::FORMAT_R16G16_FLOAT;
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
			static const wiGraphics::FORMAT FORMAT = wiGraphics::FORMAT::FORMAT_R8G8B8A8_UNORM;
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

			static const wiGraphics::FORMAT FORMAT = wiGraphics::FORMAT::FORMAT_R8G8B8A8_UNORM;
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
		AABB aabb;
		XMFLOAT4 color;
		float fadeThresholdRadius;
		std::vector<XMFLOAT4X4> instanceMatrices;

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline bool IsDirty() const { return _flags & DIRTY; }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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

		wiECS::Entity meshID = wiECS::INVALID_ENTITY;
		uint32_t cascadeMask = 0; // which shadow cascades to skip (0: skip none, 1: skip first, etc...)
		uint32_t rendertypeMask = 0;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);

		uint32_t lightmapWidth = 0;
		uint32_t lightmapHeight = 0;
		std::vector<uint8_t> lightmapTextureData;

		uint8_t userStencilRef = 0;

		// Non-serialized attributes:

		XMFLOAT4 globalLightMapMulAdd = XMFLOAT4(0, 0, 0, 0);
		wiGraphics::Texture lightmap;
		wiGraphics::RenderPass renderpass_lightmap_clear;
		wiGraphics::RenderPass renderpass_lightmap_accumulate;
		uint32_t lightmapIterationCount = 0;

		XMFLOAT3 center = XMFLOAT3(0, 0, 0);
		float impostorFadeThresholdRadius;
		float impostorSwapDistance;

		// these will only be valid for a single frame:
		int transform_index = -1;
		int prev_transform_index = -1;

		// occlusion result history bitfield (32 bit->32 frame history)
		uint32_t occlusionHistory = ~0;
		// occlusion query pool index
		int occlusionQueryID = -1;

		inline bool IsOccluded() const
		{
			// Perform a conservative occlusion test:
			// If it is visible in any frames in the history, it is determined visible in this frame
			// But if all queries failed in the history, it is occluded.
			// If it pops up for a frame after occluded, it is visible again for some frames
			return ((occlusionQueryID >= 0) && (occlusionHistory & 0xFFFFFFFF) == 0);
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
		wiGraphics::FORMAT GetLightmapFormat();

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		float friction = 1.0f;
		float restitution = 1.0f;
		float damping = 1.0f;

		// Non-serialized attributes:
		void* physicsobject = nullptr;

		inline void SetDisableDeactivation(bool value) { if (value) { _flags |= DISABLE_DEACTIVATION; } else { _flags &= ~DISABLE_DEACTIVATION; } }
		inline void SetKinematic(bool value) { if (value) { _flags |= KINEMATIC; } else { _flags &= ~KINEMATIC; } }

		inline bool IsDisableDeactivation() const { return _flags & DISABLE_DEACTIVATION; }
		inline bool IsKinematic() const { return _flags & KINEMATIC; }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		float friction = 1.0f;
		std::vector<uint32_t> physicsToGraphicsVertexMapping; // maps graphics vertex index to physics vertex index of the same position
		std::vector<uint32_t> graphicsToPhysicsVertexMapping; // maps a physics vertex index to first graphics vertex index of the same position
		std::vector<float> weights; // weight per physics vertex controlling the mass. (0: disable weight (no physics, only animation), 1: default weight)

		// Non-serialized attributes:
		void* physicsobject = nullptr;
		XMFLOAT4X4 worldMatrix = IDENTITYMATRIX;
		std::vector<MeshComponent::Vertex_POS> vertex_positions_simulation; // graphics vertices after simulation (world space)
		std::vector<XMFLOAT4>vertex_tangents_tmp;
		std::vector<MeshComponent::Vertex_TAN> vertex_tangents_simulation;
		AABB aabb;

		inline void SetDisableDeactivation(bool value) { if (value) { _flags |= DISABLE_DEACTIVATION; } else { _flags &= ~DISABLE_DEACTIVATION; } }

		inline bool IsDisableDeactivation() const { return _flags & DISABLE_DEACTIVATION; }

		// Create physics represenation of graphics mesh
		void CreateFromMesh(const MeshComponent& mesh);

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct ArmatureComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		std::vector<wiECS::Entity> boneCollection;
		std::vector<XMFLOAT4X4> inverseBindMatrices;

		// Non-serialized attributes:
		AABB aabb;

		struct ShaderBoneType
		{
			XMFLOAT4 pose0;
			XMFLOAT4 pose1;
			XMFLOAT4 pose2;

			inline void Store(const XMMATRIX& M)
			{
				XMFLOAT4X4 mat;
				XMStoreFloat4x4(&mat, M);
				pose0 = XMFLOAT4(mat._11, mat._21, mat._31, mat._41);
				pose1 = XMFLOAT4(mat._12, mat._22, mat._32, mat._42);
				pose2 = XMFLOAT4(mat._13, mat._23, mat._33, mat._43);
			}
			inline XMMATRIX Load() const
			{
				return XMMATRIX(
					pose0.x, pose1.x, pose2.x, 0, 
					pose0.y, pose1.y, pose2.y, 0, 
					pose0.z, pose1.z, pose2.z, 0, 
					pose0.w, pose1.w, pose2.w, 1
				);
			}
		};
		std::vector<ShaderBoneType> boneData;
		wiGraphics::GPUBuffer boneBuffer;

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
			SPHERE = ENTITY_TYPE_SPHERELIGHT,
			DISC = ENTITY_TYPE_DISCLIGHT,
			RECTANGLE = ENTITY_TYPE_RECTANGLELIGHT,
			TUBE = ENTITY_TYPE_TUBELIGHT,
			LIGHTTYPE_COUNT,
			ENUM_FORCE_UINT32 = 0xFFFFFFFF,
		};
		LightType type = POINT;
		float energy = 1.0f;
		float range_local = 10.0f;
		float fov = XM_PIDIV4;
		float shadowBias = 0.0001f;
		float radius = 1.0f; // area light
		float width = 1.0f;  // area light
		float height = 1.0f; // area light

		std::vector<std::string> lensFlareNames;

		// Non-serialized attributes:
		XMFLOAT3 position;
		float range_global;
		XMFLOAT3 direction;
		XMFLOAT4 rotation;
		XMFLOAT3 scale;
		XMFLOAT3 front;
		XMFLOAT3 right;
		int shadowMap_index = -1;

		std::vector<std::shared_ptr<wiResource>> lensFlareRimTextures;

		inline void SetCastShadow(bool value) { if (value) { _flags |= CAST_SHADOW; } else { _flags &= ~CAST_SHADOW; } }
		inline void SetVolumetricsEnabled(bool value) { if (value) { _flags |= VOLUMETRICS; } else { _flags &= ~VOLUMETRICS; } }
		inline void SetVisualizerEnabled(bool value) { if (value) { _flags |= VISUALIZER; } else { _flags &= ~VISUALIZER; } }
		inline void SetStatic(bool value) { if (value) { _flags |= LIGHTMAPONLY_STATIC; } else { _flags &= ~LIGHTMAPONLY_STATIC; } }

		inline bool IsCastingShadow() const { return _flags & CAST_SHADOW; }
		inline bool IsVolumetricsEnabled() const { return _flags & VOLUMETRICS; }
		inline bool IsVisualizerEnabled() const { return _flags & VISUALIZER; }
		inline bool IsStatic() const { return _flags & LIGHTMAPONLY_STATIC; }

		inline float GetRange() const { return range_global; }

		inline void SetType(LightType val) {
			type = val;
		}
		inline LightType GetType() const { return type; }

		void LoadAssets(const std::string& content_dir);

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		float zNearP = 0.001f;
		float zFarP = 800.0f;
		float fov = XM_PI / 3.0f;

		// Non-serialized attributes:
		XMFLOAT3 Eye = XMFLOAT3(0, 0, 0);
		XMFLOAT3 At = XMFLOAT3(0, 0, 1);
		XMFLOAT3 Up = XMFLOAT3(0, 1, 0);
		XMFLOAT3X3 rotationMatrix;
		XMFLOAT4X4 View, Projection, VP;
		Frustum frustum;
		XMFLOAT4X4 InvView, InvProjection, InvVP;
		XMFLOAT2 jitter;

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

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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

		inline void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		inline void SetRealTime(bool value) { if (value) { _flags |= REALTIME; } else { _flags &= ~REALTIME; } }

		inline bool IsDirty() const { return _flags & DIRTY; }
		inline bool IsRealTime() const { return _flags & REALTIME; }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		XMFLOAT4 atlasMulAdd;
		XMFLOAT4X4 world;

		std::shared_ptr<wiResource> texture;
		std::shared_ptr<wiResource> normal;

		inline float GetOpacity() const { return color.w; }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct AnimationDataComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t _flags = EMPTY;

		std::vector<float> keyframe_times;
		std::vector<float> keyframe_data;

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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

			wiECS::Entity target = wiECS::INVALID_ENTITY;
			int samplerIndex = -1;

			enum Path
			{
				TRANSLATION,
				ROTATION,
				SCALE,
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

			wiECS::Entity data = wiECS::INVALID_ENTITY;

			enum Mode
			{
				LINEAR,
				STEP,
				MODE_FORCE_UINT32 = 0xFFFFFFFF
			} mode = LINEAR;

			// The data is now not part of the sampler, so it can be shared. This is kept only for backwards compatibility with previous versions.
			AnimationDataComponent backwards_compatibility_data;
		};
		std::vector<AnimationChannel> channels;
		std::vector<AnimationSampler> samplers;

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsLooped() const { return _flags & LOOPED; }
		inline float GetLength() const { return end - start; }
		inline bool IsEnded() const { return timer >= end; }

		inline void Play() { _flags |= PLAYING; }
		inline void Pause() { _flags &= ~PLAYING; }
		inline void Stop() { Pause(); timer = 0.0f; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct WeatherComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			OCEAN_ENABLED = 1 << 0,
			SIMPLE_SKY = 1 << 1,
			REALISTIC_SKY = 1 << 2,
		};
		uint32_t _flags = EMPTY;

		inline bool IsOceanEnabled() const { return _flags & OCEAN_ENABLED; }
		inline bool IsSimpleSky() const { return _flags & SIMPLE_SKY; }
		inline bool IsRealisticSky() const { return _flags & REALISTIC_SKY; }

		inline void SetOceanEnabled(bool value = true) { if (value) { _flags |= OCEAN_ENABLED; } else { _flags &= ~OCEAN_ENABLED; } }
		inline void SetSimpleSky(bool value = true) { if (value) { _flags |= SIMPLE_SKY; } else { _flags &= ~SIMPLE_SKY; } }
		inline void SetRealisticSky(bool value = true) { if (value) { _flags |= REALISTIC_SKY; } else { _flags &= ~REALISTIC_SKY; } }

		XMFLOAT3 sunColor = XMFLOAT3(0, 0, 0);
		XMFLOAT3 sunDirection = XMFLOAT3(0, 1, 0);
		float sunEnergy = 0;
		XMFLOAT3 horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 zenith = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		float fogStart = 100;
		float fogEnd = 1000;
		float fogHeight = 0;
		float cloudiness = 0.0f;
		float cloudScale = 0.0003f;
		float cloudSpeed = 0.1f;
		XMFLOAT3 windDirection = XMFLOAT3(0, 0, 0);
		float windRandomness = 5;
		float windWaveSize = 1;
		float windSpeed = 1;

		struct OceanParameters
		{
			// Must be power of 2.
			int dmap_dim = 512;
			// Typical value is 1000 ~ 2000
			float patch_length = 50.0f;

			// Adjust the time interval for simulation.
			float time_scale = 0.3f;
			// Amplitude for transverse wave. Around 1.0
			float wave_amplitude = 1000.0f;
			// Wind direction. Normalization not required.
			XMFLOAT2 wind_dir = XMFLOAT2(0.8f, 0.6f);
			// Around 100 ~ 1000
			float wind_speed = 600.0f;
			// This value damps out the waves against the wind direction.
			// Smaller value means higher wind dependency.
			float wind_dependency = 0.07f;
			// The amplitude for longitudinal wave. Must be positive.
			float choppy_scale = 1.3f;


			XMFLOAT3 waterColor = XMFLOAT3(0.0f, 3.0f / 255.0f, 31.0f / 255.0f);
			float waterHeight = 0.0f;
			uint32_t surfaceDetail = 4;
			float surfaceDisplacementTolerance = 2;
		};
		OceanParameters oceanParameters;

		std::string skyMapName;
		std::shared_ptr<wiResource> skyMap;

		// Non-serialized attributes:
		uint32_t most_important_light_index = ~0;

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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
		std::shared_ptr<wiResource> soundResource;
		wiAudio::SoundInstance soundinstance;
		float volume = 1;

		inline bool IsPlaying() const { return _flags & PLAYING; }
		inline bool IsLooped() const { return _flags & LOOPED; }
		inline bool IsDisable3D() const { return _flags & DISABLE_3D; }

		inline void Play() { _flags |= PLAYING; }
		inline void Stop() { _flags &= ~PLAYING; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }
		inline void SetDisable3D(bool value = true) { if (value) { _flags |= DISABLE_3D; } else { _flags &= ~DISABLE_3D; } }

		void LoadAssets(const std::string& content_dir = "");

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct InverseKinematicsComponent
	{
		enum FLAGS
		{
			EMPTY = 0,
			DISABLED = 1 << 0,
		};
		uint32_t _flags = EMPTY;

		wiECS::Entity target = wiECS::INVALID_ENTITY; // which entity to follow (must have a transform component)
		uint32_t chain_length = ~0; // ~0 means: compute until the root
		uint32_t iteration_count = 1;

		inline void SetDisabled(bool value = true) { if (value) { _flags |= DISABLED; } else { _flags &= ~DISABLED; } }
		inline bool IsDisabled() const { return _flags & DISABLED; }

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
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

		void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);
	};

	struct Scene
	{
		wiECS::ComponentManager<NameComponent> names;
		wiECS::ComponentManager<LayerComponent> layers;
		wiECS::ComponentManager<TransformComponent> transforms;
		wiECS::ComponentManager<PreviousFrameTransformComponent> prev_transforms;
		wiECS::ComponentManager<HierarchyComponent> hierarchy;
		wiECS::ComponentManager<MaterialComponent> materials;
		wiECS::ComponentManager<MeshComponent> meshes;
		wiECS::ComponentManager<ImpostorComponent> impostors;
		wiECS::ComponentManager<ObjectComponent> objects;
		wiECS::ComponentManager<AABB> aabb_objects;
		wiECS::ComponentManager<RigidBodyPhysicsComponent> rigidbodies;
		wiECS::ComponentManager<SoftBodyPhysicsComponent> softbodies;
		wiECS::ComponentManager<ArmatureComponent> armatures;
		wiECS::ComponentManager<LightComponent> lights;
		wiECS::ComponentManager<AABB> aabb_lights;
		wiECS::ComponentManager<CameraComponent> cameras;
		wiECS::ComponentManager<EnvironmentProbeComponent> probes;
		wiECS::ComponentManager<AABB> aabb_probes;
		wiECS::ComponentManager<ForceFieldComponent> forces;
		wiECS::ComponentManager<DecalComponent> decals;
		wiECS::ComponentManager<AABB> aabb_decals;
		wiECS::ComponentManager<AnimationComponent> animations;
		wiECS::ComponentManager<AnimationDataComponent> animation_datas;
		wiECS::ComponentManager<wiEmittedParticle> emitters;
		wiECS::ComponentManager<wiHairParticle> hairs;
		wiECS::ComponentManager<WeatherComponent> weathers;
		wiECS::ComponentManager<SoundComponent> sounds;
		wiECS::ComponentManager<InverseKinematicsComponent> inverse_kinematics;
		wiECS::ComponentManager<SpringComponent> springs;

		// Non-serialized attributes:
		wiSpinLock locker;
		AABB bounds;
		std::vector<AABB> parallel_bounds;
		WeatherComponent weather;
		wiGraphics::RaytracingAccelerationStructure TLAS;
		wiGraphics::DescriptorTable descriptorTable;
		enum DESCRIPTORTABLE_ENTRY
		{
			DESCRIPTORTABLE_ENTRY_SUBSETS_MATERIAL,
			DESCRIPTORTABLE_ENTRY_SUBSETS_TEXTURE_BASECOLOR,
			DESCRIPTORTABLE_ENTRY_SUBSETS_INDEXBUFFER,
			DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_POSITION_NORMAL_WIND,
			DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_UV0,
			DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_UV1,

			DESCRIPTORTABLE_ENTRY_COUNT
		};
		std::atomic<uint32_t> geometryOffset;

		// Update all components by a given timestep (in seconds):
		void Update(float dt);
		// Remove everything from the scene that it owns:
		void Clear();
		// Merge with an other scene.
		void Merge(Scene& other);

		// Removes a specific entity from the scene (if it exists):
		void Entity_Remove(wiECS::Entity entity);
		// Finds the first entity by the name (if it exists, otherwise returns INVALID_ENTITY):
		wiECS::Entity Entity_FindByName(const std::string& name);
		// Duplicates all of an entity's components and creates a new entity with them:
		wiECS::Entity Entity_Duplicate(wiECS::Entity entity);
		// Serializes entity and all of its components to archive:
		//	Returns either the new entity that was read, or the original entity that was written
		wiECS::Entity Entity_Serialize(wiArchive& archive, wiECS::Entity entity = wiECS::INVALID_ENTITY);

		wiECS::Entity Entity_CreateMaterial(
			const std::string& name
		);
		wiECS::Entity Entity_CreateObject(
			const std::string& name
		);
		wiECS::Entity Entity_CreateMesh(
			const std::string& name
		);
		wiECS::Entity Entity_CreateLight(
			const std::string& name, 
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0), 
			const XMFLOAT3& color = XMFLOAT3(1, 1, 1), 
			float energy = 1, 
			float range = 10
		);
		wiECS::Entity Entity_CreateForce(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wiECS::Entity Entity_CreateEnvironmentProbe(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wiECS::Entity Entity_CreateDecal(
			const std::string& name,
			const std::string& textureName,
			const std::string& normalMapName = ""
		);
		wiECS::Entity Entity_CreateCamera(
			const std::string& name,
			float width, float height, float nearPlane = 0.01f, float farPlane = 1000.0f, float fov = XM_PIDIV4
		);
		wiECS::Entity Entity_CreateEmitter(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wiECS::Entity Entity_CreateHair(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		wiECS::Entity Entity_CreateSound(
			const std::string& name,
			const std::string& filename,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);

		// Attaches an entity to a parent:
		//	child_already_in_local_space	:	child won't be transformed from world space to local space
		void Component_Attach(wiECS::Entity entity, wiECS::Entity parent, bool child_already_in_local_space = false);
		// Detaches the entity from its parent (if it is attached):
		void Component_Detach(wiECS::Entity entity);
		// Detaches all children from an entity (if there are any):
		void Component_DetachChildren(wiECS::Entity parent);

		void Serialize(wiArchive& archive);

		void RunPreviousFrameTransformUpdateSystem(wiJobSystem::context& ctx);
		void RunAnimationUpdateSystem(wiJobSystem::context& ctx, float dt);
		void RunTransformUpdateSystem(wiJobSystem::context& ctx);
		void RunHierarchyUpdateSystem(wiJobSystem::context& ctx);
		void RunSpringUpdateSystem(wiJobSystem::context& ctx, float dt);
		void RunInverseKinematicsUpdateSystem(wiJobSystem::context& ctx);
		void RunArmatureUpdateSystem(wiJobSystem::context& ctx);
		void RunMeshUpdateSystem(wiJobSystem::context& ctx);
		void RunMaterialUpdateSystem(wiJobSystem::context& ctx, float dt);
		void RunImpostorUpdateSystem(wiJobSystem::context& ctx);
		void RunObjectUpdateSystem(wiJobSystem::context& ctx);
		void RunCameraUpdateSystem(wiJobSystem::context& ctx);
		void RunDecalUpdateSystem(wiJobSystem::context& ctx);
		void RunProbeUpdateSystem(wiJobSystem::context& ctx);
		void RunForceUpdateSystem(wiJobSystem::context& ctx);
		void RunLightUpdateSystem(wiJobSystem::context& ctx);
		void RunParticleUpdateSystem(wiJobSystem::context& ctx, float dt);
		void RunWeatherUpdateSystem(wiJobSystem::context& ctx);
		void RunSoundUpdateSystem(wiJobSystem::context& ctx);
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

	// Helper function to open a wiscene file and add the contents to the global scene
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	wiECS::Entity LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	// Helper function to open a wiscene file and add the contents to the specified scene. This is thread safe as it doesn't modify global scene
	//	scene			:	the scene that will contain the model
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	wiECS::Entity LoadModel(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	struct PickResult
	{
		wiECS::Entity entity = wiECS::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		float distance = FLT_MAX;
		int subsetIndex = -1;
		int vertexID0 = -1;
		int vertexID1 = -1;
		int vertexID2 = -1;
		XMFLOAT2 bary = XMFLOAT2(0, 0);
		XMFLOAT4X4 orientation = IDENTITYMATRIX;

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
	PickResult Pick(const RAY& ray, uint32_t renderTypeMask = RENDERTYPE_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene());

	struct SceneIntersectSphereResult
	{
		wiECS::Entity entity = wiECS::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		float depth = 0;
	};
	SceneIntersectSphereResult SceneIntersectSphere(const SPHERE& sphere, uint32_t renderTypeMask = RENDERTYPE_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene());
	SceneIntersectSphereResult SceneIntersectCapsule(const CAPSULE& capsule, uint32_t renderTypeMask = RENDERTYPE_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene());

}

