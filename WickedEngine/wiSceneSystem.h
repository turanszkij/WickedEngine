#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiImageEffects.h"
#include "wiIntersectables.h"
#include "ShaderInterop.h"
#include "wiFrustum.h"
#include "wiRenderTarget.h"

#include "wiECS.h"
#include "wiSceneSystem_Decl.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

namespace wiSceneSystem
{

	struct NodeComponent
	{
		uint32_t layerMask = ~0;
		std::string name;
	};
	
	struct TransformComponent
	{
		wiECS::ComponentManager<TransformComponent>::ref parent_ref;

		XMFLOAT3 scale_local = XMFLOAT3(1, 1, 1);
		XMFLOAT4 rotation_local = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT3 translation_local = XMFLOAT3(0, 0, 0);

		bool dirty = true;
		XMFLOAT3 scale;
		XMFLOAT4 rotation;
		XMFLOAT3 translation;
		XMFLOAT4X4 world;
		XMFLOAT4X4 world_prev;
		XMFLOAT4X4 world_parent_bind;

		void AttachTo(const TransformComponent* const parent);
		void UpdateTransform(const TransformComponent* const parent = nullptr);
		void ClearTransform();
		void Translate(const XMFLOAT3& value);
		void RotateRollPitchYaw(const XMFLOAT3& value);
		void Rotate(const XMFLOAT4& quaternion);
		void Scale(const XMFLOAT3& value);
		void Lerp(const TransformComponent& a, const TransformComponent& b, float t);
		void CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t);
	};

	struct MaterialComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;

		bool dirty = true;

		STENCILREF engineStencilRef = STENCILREF_DEFAULT;
		uint8_t userStencilRef = 0;
		BLENDMODE blendFlag = BLENDMODE_OPAQUE;

		XMFLOAT4 baseColor = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0);
		float roughness = 0.2f;
		float reflectance = 0.02f;
		float metalness = 0.0f;
		float emissive = 0.0f;
		float refractionIndex = 0.0f;
		float subsurfaceScattering = 0.0f;
		float normalMapStrength = 1.0f;
		float parallaxOcclusionMapping = 0.0f;

		float alphaRef = 1.0f;

		bool cast_shadow = true;
		bool planar_reflections = false;
		bool water = false;

		std::string baseColorMapName;
		wiGraphicsTypes::Texture2D* baseColorMap = nullptr;

		std::string surfaceMapName;
		wiGraphicsTypes::Texture2D* surfaceMap = nullptr;

		std::string normalMapName;
		wiGraphicsTypes::Texture2D* normalMap = nullptr;

		std::string displacementMapName;
		wiGraphicsTypes::Texture2D* displacementMap = nullptr;

		wiGraphicsTypes::GPUBuffer* constantBuffer = nullptr;

		inline void SetUserStencilRef(uint8_t value)
		{
			assert(value < 128);
			userStencilRef = value & 0x0F;
		}
		inline UINT GetStencilRef()
		{
			return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
		}

		wiGraphicsTypes::Texture2D* GetBaseColorMap() const;
		wiGraphicsTypes::Texture2D* GetNormalMap() const;
		wiGraphicsTypes::Texture2D* GetSurfaceMap() const;
		wiGraphicsTypes::Texture2D* GetDisplacementMap() const;

		inline bool IsTransparent() const { return baseColor.w < 1.0f; }
		inline bool IsWater() const { return water; }
		inline bool HasPlanarReflection() const { return planar_reflections || IsWater(); }
		inline bool IsCastingShadow() const { return cast_shadow; }
		inline bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }
	};

	struct MeshComponent
	{
		struct Vertex_FULL
		{
			XMFLOAT4 pos = XMFLOAT4(0, 0, 0, 0); //pos, wind
			XMFLOAT4 nor = XMFLOAT4(0, 0, 0, 1); //normal, unused
			XMFLOAT4 tex = XMFLOAT4(0, 0, 0, 0); //tex, matIndex, unused
			XMFLOAT4 ind = XMFLOAT4(0, 0, 0, 0); //bone indices
			XMFLOAT4 wei = XMFLOAT4(0, 0, 0, 0); //bone weights
		};
		struct Vertex_POS
		{
			XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
			uint32_t normal_wind_matID = 0;

			void FromFULL(const Vertex_FULL& vert)
			{
				pos.x = vert.pos.x;
				pos.y = vert.pos.y;
				pos.z = vert.pos.z;
				MakeFromParams(XMFLOAT3(vert.nor.x, vert.nor.y, vert.nor.z), vert.pos.w, static_cast<uint32_t>(vert.tex.z));
			}
			inline XMVECTOR LoadPOS() const
			{
				return XMLoadFloat3(&pos);
			}
			inline XMVECTOR LoadNOR() const
			{
				return XMLoadFloat3(&GetNor_FULL());
			}
			inline void MakeFromParams(const XMFLOAT3& normal)
			{
				normal_wind_matID = normal_wind_matID & 0xFF000000; // reset only the normals

				normal_wind_matID |= (uint32_t)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
				normal_wind_matID |= (uint32_t)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
				normal_wind_matID |= (uint32_t)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
			}
			inline void MakeFromParams(const XMFLOAT3& normal, float wind, uint32_t materialIndex)
			{
				assert(materialIndex < 16); // subset materialIndex is packed onto 4 bits

				normal_wind_matID = 0;

				normal_wind_matID |= (uint32_t)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
				normal_wind_matID |= (uint32_t)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
				normal_wind_matID |= (uint32_t)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
				normal_wind_matID |= ((uint32_t)(wind * 15.0f) & 0x0000000F) << 24;
				normal_wind_matID |= (materialIndex & 0x0000000F) << 28;
			}
			inline XMFLOAT3 GetNor_FULL() const
			{
				XMFLOAT3 nor_FULL(0, 0, 0);

				nor_FULL.x = (float)((normal_wind_matID >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
				nor_FULL.y = (float)((normal_wind_matID >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
				nor_FULL.z = (float)((normal_wind_matID >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;

				return nor_FULL;
			}
			inline float GetWind() const
			{
				return (float)((normal_wind_matID >> 24) & 0x0000000F) / 15.0f;
			}
			inline uint32_t GetMaterialIndex() const
			{
				return (normal_wind_matID >> 28) & 0x0000000F;
			}

			static const wiGraphicsTypes::FORMAT FORMAT = wiGraphicsTypes::FORMAT::FORMAT_R32G32B32A32_FLOAT;
		};
		struct Vertex_TEX
		{
			XMHALF2 tex = XMHALF2(0.0f, 0.0f);

			void FromFULL(const Vertex_FULL& vert)
			{
				tex = XMHALF2(vert.tex.x, vert.tex.y);
			}

			static const wiGraphicsTypes::FORMAT FORMAT = wiGraphicsTypes::FORMAT::FORMAT_R16G16_FLOAT;
		};
		struct Vertex_BON
		{
			uint64_t ind = 0;
			uint64_t wei = 0;

			void FromFULL(const Vertex_FULL& vert)
			{
				ind = 0;
				wei = 0;

				ind |= (uint64_t)vert.ind.x << 0;
				ind |= (uint64_t)vert.ind.y << 16;
				ind |= (uint64_t)vert.ind.z << 32;
				ind |= (uint64_t)vert.ind.w << 48;

				wei |= (uint64_t)(vert.wei.x * 65535.0f) << 0;
				wei |= (uint64_t)(vert.wei.y * 65535.0f) << 16;
				wei |= (uint64_t)(vert.wei.z * 65535.0f) << 32;
				wei |= (uint64_t)(vert.wei.w * 65535.0f) << 48;
			}
			inline XMFLOAT4 GetInd_FULL() const
			{
				XMFLOAT4 ind_FULL(0, 0, 0, 0);

				ind_FULL.x = (float)((ind >> 0) & 0x0000FFFF);
				ind_FULL.y = (float)((ind >> 16) & 0x0000FFFF);
				ind_FULL.z = (float)((ind >> 32) & 0x0000FFFF);
				ind_FULL.w = (float)((ind >> 48) & 0x0000FFFF);

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

		std::vector<Vertex_FULL>	vertices_FULL;
		std::vector<Vertex_POS>		vertices_POS; // position(xyz), normal+wind(w as uint)
		std::vector<Vertex_TEX>		vertices_TEX; // texcoords
		std::vector<Vertex_BON>		vertices_BON; // bone indices, bone weights
		std::vector<Vertex_POS>		vertices_Transformed_POS; // for soft body simulation
		std::vector<Vertex_POS>		vertices_Transformed_PRE; // for soft body simulation
		std::vector<uint32_t>		indices;

		struct MeshSubset
		{
			wiECS::ComponentManager<MaterialComponent>::ref material_ref;
			UINT indexBufferOffset = 0;

			std::vector<uint32_t> subsetIndices;
		};
		std::vector<MeshSubset>		subsets;

		wiGraphicsTypes::GPUBuffer*	indexBuffer = nullptr;
		wiGraphicsTypes::GPUBuffer*	vertexBuffer_POS = nullptr;
		wiGraphicsTypes::GPUBuffer*	vertexBuffer_TEX = nullptr;
		wiGraphicsTypes::GPUBuffer*	vertexBuffer_BON = nullptr;
		wiGraphicsTypes::GPUBuffer*	streamoutBuffer_POS = nullptr;
		wiGraphicsTypes::GPUBuffer*	streamoutBuffer_PRE = nullptr;

		wiGraphicsTypes::INDEXBUFFER_FORMAT indexFormat = wiGraphicsTypes::INDEXFORMAT_16BIT;

		bool renderable = true;
		bool doubleSided = false;
		bool renderDataComplete = false;

		AABB aabb;

		wiECS::ComponentManager<ArmatureComponent>::ref armature_ref;

		struct VertexGroup 
		{
			std::string name;
			std::unordered_map<int, float> vertex_weights; // maps vertex index to weight value...
		};
		std::vector<VertexGroup> vertexGroups;

		wiRenderTarget	impostorTarget;
		float impostorDistance = 100.0f;
		static wiGraphicsTypes::GPUBuffer impostorVB_POS;
		static wiGraphicsTypes::GPUBuffer impostorVB_TEX;

		float tessellationFactor;

		inline wiGraphicsTypes::INDEXBUFFER_FORMAT GetIndexFormat() { return indexFormat; }
		inline bool IsSkinned() { return armature_ref != wiECS::ComponentManager<ArmatureComponent>::ref(); }
	};

	struct CullableComponent
	{
		AABB aabb;
	};

	struct ObjectComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;
		wiECS::ComponentManager<MeshComponent>::ref mesh_ref;
		wiECS::ComponentManager<CullableComponent>::ref cullable_ref;

		bool renderable = true;
		int cascadeMask = 0; // which shadow cascades to skip (0: skip none, 1: skip first, etc...)
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);

		// occlusion result history bitfield (32 bit->32 frame history)
		uint32_t occlusionHistory = ~0;
		// occlusion query pool index
		int occlusionQueryID = -1;
	};

	struct PhysicsComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;
		wiECS::ComponentManager<MeshComponent>::ref mesh_ref;

		std::string collisionShape;
		std::string physicsType;

		int physicsObjectID = -1;
		bool rigidBody = false;
		bool softBody = false;
		bool kinematic = false;
		float mass = 1.0f;
		float friction = 1.0f;
		float restitution = 1.0f;
		float damping = 1.0f;

		int massVG; // vertex group ID for masses
		int goalVG; // vertex group ID for soft body goals (how much the vertex is driven by anim/other than physics systems)
		int softVG; // vertex group ID for soft body softness (how much to blend in soft body sim to a vertex)
		std::vector<XMFLOAT3> goalPositions;
		std::vector<XMFLOAT3> goalNormals;

		std::vector<XMFLOAT3> physicsverts;
		std::vector<uint32_t> physicsindices;
		std::vector<int>	  physicalmapGP;
	};

	struct BoneComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;

		XMFLOAT4X4 inverseBindPoseMatrix;
		XMFLOAT4X4 skinningMatrix;
	};

	struct ArmatureComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;

		std::vector<wiECS::ComponentManager<BoneComponent>::ref> bone_refs;

		GFX_STRUCT ShaderBoneType
		{
			XMFLOAT4A pose0;
			XMFLOAT4A pose1;
			XMFLOAT4A pose2;

			void Create(const XMFLOAT4X4& matIn)
			{
				pose0 = XMFLOAT4A(matIn._11, matIn._21, matIn._31, matIn._41);
				pose1 = XMFLOAT4A(matIn._12, matIn._22, matIn._32, matIn._42);
				pose2 = XMFLOAT4A(matIn._13, matIn._23, matIn._33, matIn._43);
			}

			ALIGN_16
		};
		std::vector<ShaderBoneType> boneData;
		wiGraphicsTypes::GPUBuffer boneBuffer;

		// This will be used to eg. mirror the whole skin, without modifying the armature transform itself
		//	It will affect the skin only, so the mesh vertices should be mirrored as well to work correctly!
		XMFLOAT4X4 skinningRemap = XMFLOAT4X4(1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1);
	};

	struct LightComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;
		wiECS::ComponentManager<CullableComponent>::ref cullable_ref;

		enum LightType {
			DIRECTIONAL			= ENTITY_TYPE_DIRECTIONALLIGHT,
			POINT				= ENTITY_TYPE_POINTLIGHT,
			SPOT				= ENTITY_TYPE_SPOTLIGHT,
			SPHERE				= ENTITY_TYPE_SPHERELIGHT,
			DISC				= ENTITY_TYPE_DISCLIGHT,
			RECTANGLE			= ENTITY_TYPE_RECTANGLELIGHT,
			TUBE				= ENTITY_TYPE_TUBELIGHT,
			LIGHTTYPE_COUNT,
		} type = POINT;

		XMFLOAT3 color = XMFLOAT3(1, 1, 1);
		float energy = 1.0f;
		float range = 10.0f;
		bool volumetrics = false;
		bool visualizer = false;
		bool shadow = false;
		std::vector<wiGraphicsTypes::Texture2D*> lensFlareRimTextures;
		std::vector<std::string> lensFlareNames;

		int shadowMap_index = -1;
		int entityArray_index = -1;

		std::vector<XMFLOAT4X4> shadowMatrices;

		float shadowBias = 0.0001f;

		// area light props:
		float radius = 1.0f;
		float width = 1.0f;
		float height = 1.0f;
	};

	struct CameraComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;

		float width = 0.0f, height = 0.0f;
		float zNearP = 0.001f, zFarP = 800.0f;
		float fov = XM_PI / 3.0f;
		XMFLOAT4X4 View, Projection, VP;
		XMFLOAT3 Eye, At, Up;
		Frustum frustum;
		XMFLOAT4X4 InvView, InvProjection, InvVP;
		XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...

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
		inline XMMATRIX GetRealProjection() const { return XMLoadFloat4x4(&realProjection); }
	};

	struct EnvironmentProbeComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;
		int textureIndex = -1;
		bool realTime = false;
		bool isUpToDate = false;
	};

	struct ForceFieldComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;

		int type = ENTITY_TYPE_FORCEFIELD_POINT;
		float gravity = 0.0f; // negative = deflector, positive = attractor
		float range = 0.0f; // affection range
	};

	struct DecalComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;
		wiECS::ComponentManager<MaterialComponent>::ref material_ref;

		XMFLOAT4 atlasMulAdd = XMFLOAT4(0, 0, 0, 0);
	};

	struct ModelComponent
	{
		wiECS::ComponentManager<NodeComponent>::ref node_ref;
		wiECS::ComponentManager<TransformComponent>::ref transform_ref;

		std::vector<wiECS::ComponentManager<MaterialComponent>::ref> material_refs;
		std::vector<wiECS::ComponentManager<MeshComponent>::ref> mesh_refs;
		std::vector<wiECS::ComponentManager<ObjectComponent>::ref> object_refs;
		std::vector<wiECS::ComponentManager<PhysicsComponent>::ref> physicscomponent_refs;
		std::vector<wiECS::ComponentManager<CullableComponent>::ref> cullable_refs;
		std::vector<wiECS::ComponentManager<BoneComponent>::ref> bone_refs;
		std::vector<wiECS::ComponentManager<ArmatureComponent>::ref> armature_refs;
		std::vector<wiECS::ComponentManager<LightComponent>::ref> light_refs;
		std::vector<wiECS::ComponentManager<EnvironmentProbeComponent>::ref> probe_refs;
		std::vector<wiECS::ComponentManager<ForceFieldComponent>::ref> force_refs;
		std::vector<wiECS::ComponentManager<DecalComponent>::ref> decal_refs;
		std::vector<wiECS::ComponentManager<CameraComponent>::ref> camera_refs;
	};

	struct Scene
	{
		std::unordered_set<wiECS::Entity> owned_entities;

		wiECS::ComponentManager<NodeComponent> nodes;
		wiECS::ComponentManager<TransformComponent> transforms;
		wiECS::ComponentManager<MaterialComponent> materials;
		wiECS::ComponentManager<MeshComponent> meshes;
		wiECS::ComponentManager<ObjectComponent> objects;
		wiECS::ComponentManager<PhysicsComponent> physicscomponents;
		wiECS::ComponentManager<CullableComponent> cullables;
		wiECS::ComponentManager<BoneComponent> bones;
		wiECS::ComponentManager<ArmatureComponent> armatures;
		wiECS::ComponentManager<LightComponent> lights;
		wiECS::ComponentManager<CameraComponent> cameras;
		wiECS::ComponentManager<EnvironmentProbeComponent> probes;
		wiECS::ComponentManager<ForceFieldComponent> forces;
		wiECS::ComponentManager<DecalComponent> decals;
		wiECS::ComponentManager<ModelComponent> models;

		AABB bounds;
		XMFLOAT3 sunDirection = XMFLOAT3(0, 1, 0);
		XMFLOAT3 sunColor = XMFLOAT3(0, 0, 0);
		XMFLOAT3 horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 zenith = XMFLOAT3(0.00f, 0.00f, 0.0f);
		XMFLOAT3 ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		XMFLOAT3 fogSEH = XMFLOAT3(100, 1000, 0);
		XMFLOAT4 water = XMFLOAT4(0, 0, 0, 0);
		float cloudiness = 0.0f;
		float cloudScale = 0.0003f;
		float cloudSpeed = 0.1f;
		XMFLOAT3 windDirection = XMFLOAT3(0, 0, 0);
		float windRandomness = 5;
		float windWaveSize = 1;

		void Update(float dt);
		void Remove(wiECS::Entity entity);
		void Clear();
	};

}

