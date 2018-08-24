#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiImageEffects.h"
#include "wiIntersectables.h"
#include "ShaderInterop.h"
#include "wiFrustum.h"

#include "wiECS.h"
#include "wiSceneSystem_Decl.h"

#include <string>

namespace wiSceneSystem
{

	struct Node
	{
		uint32_t layerMask = ~0;
		std::string name;
	};
	
	struct Transform
	{
		wiECS::ComponentManager<Transform>::iterator parent_ref;

		XMFLOAT3 scale_local = XMFLOAT3(1, 1, 1);
		XMFLOAT4 rotation_local = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT3 translation_local = XMFLOAT3(0, 0, 0);

		bool dirty = true;
		XMFLOAT4X4 world;				// uninitialized on purpose
		XMFLOAT4X4 world_prev;			// uninitialized on purpose
		XMFLOAT4X4 world_parent_bind;	// uninitialized on purpose
	};

	struct Material
	{
		wiECS::ComponentManager<Node>::iterator node_ref;

		bool dirty = true;

		STENCILREF engineStencilRef;
		uint8_t userStencilRef;
		BLENDMODE blendFlag;

		XMFLOAT4 baseColor; // + alpha (.w)
		XMFLOAT4 texMulAdd;
		float roughness;
		float reflectance;
		float metalness;
		float emissive;
		float refractionIndex;
		float subsurfaceScattering;
		float normalMapStrength;
		float parallaxOcclusionMapping;

		float alphaRef;

		bool cast_shadow;
		bool planar_reflections;
		bool water;

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
	};

	struct Mesh
	{
		struct Vertex_FULL
		{
			XMFLOAT4 pos; //pos, wind
			XMFLOAT4 nor; //normal, unused
			XMFLOAT4 tex; //tex, matIndex, unused
			XMFLOAT4 ind; //bone indices
			XMFLOAT4 wei; //bone weights

			Vertex_FULL() {
				pos = XMFLOAT4(0, 0, 0, 0);
				nor = XMFLOAT4(0, 0, 0, 1);
				tex = XMFLOAT4(0, 0, 0, 0);
				ind = XMFLOAT4(0, 0, 0, 0);
				wei = XMFLOAT4(0, 0, 0, 0);
			};
			Vertex_FULL(const XMFLOAT3& newPos) {
				pos = XMFLOAT4(newPos.x, newPos.y, newPos.z, 1);
				nor = XMFLOAT4(0, 0, 0, 1);
				tex = XMFLOAT4(0, 0, 0, 0);
				ind = XMFLOAT4(0, 0, 0, 0);
				wei = XMFLOAT4(0, 0, 0, 0);
			}
		};
		struct Vertex_POS
		{
			XMFLOAT3 pos;
			uint32_t normal_wind_matID;

			Vertex_POS() :pos(XMFLOAT3(0.0f, 0.0f, 0.0f)), normal_wind_matID(0) {}
			Vertex_POS(const Vertex_FULL& vert)
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
			XMHALF2 tex;

			Vertex_TEX() :tex(XMHALF2(0.0f, 0.0f)) {}
			Vertex_TEX(const Vertex_FULL& vert)
			{
				tex = XMHALF2(vert.tex.x, vert.tex.y);
			}

			static const wiGraphicsTypes::FORMAT FORMAT = wiGraphicsTypes::FORMAT::FORMAT_R16G16_FLOAT;
		};
		struct Vertex_BON
		{
			uint64_t ind;
			uint64_t wei;

			Vertex_BON()
			{
				ind = 0;
				wei = 0;
			}
			Vertex_BON(const Vertex_FULL& vert)
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
		std::vector<XMFLOAT3>		physicsverts;
		std::vector<uint32_t>		physicsindices;
		std::vector<int>			physicalmapGP;

		struct MeshSubset
		{
			wiECS::ComponentManager<Material> material_ref;
			UINT indexBufferOffset;

			std::vector<uint32_t> subsetIndices;

			MeshSubset();
			~MeshSubset();
		};
		std::vector<MeshSubset>		subsets;

		wiGraphicsTypes::GPUBuffer*	indexBuffer;
		wiGraphicsTypes::GPUBuffer*	vertexBuffer_POS;
		wiGraphicsTypes::GPUBuffer*	vertexBuffer_TEX;
		wiGraphicsTypes::GPUBuffer*	vertexBuffer_BON;
		wiGraphicsTypes::GPUBuffer*	streamoutBuffer_POS;
		wiGraphicsTypes::GPUBuffer*	streamoutBuffer_PRE;

		// Dynamic vertexbuffers write into a global pool, these will be the offsets into that:
		UINT bufferOffset_POS;
		UINT bufferOffset_PRE;

		wiGraphicsTypes::INDEXBUFFER_FORMAT indexFormat;

		bool renderable = true;
		bool doubleSided = false;
		bool renderDataComplete = false;

		AABB aabb;

		wiECS::ComponentManager<Armature>::iterator armature_ref;

	};

	struct Object
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;
		wiECS::ComponentManager<Mesh>::iterator mesh_ref;

		bool renderable;
		int cascadeMask = 0; // which shadow cascades to skip (0: skip none, 1: skip first, etc...)
		AABB aabb;
		XMFLOAT4 color;

		// occlusion result history bitfield (32 bit->32 frame history)
		uint32_t occlusionHistory;
		// occlusion query pool index
		int occlusionQueryID;
	};

	struct Bone
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;

		XMFLOAT4X4 inverseBindPoseMatrix;
		XMFLOAT4X4 skinningMatrix;
	};

	struct Armature
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;

		std::vector<wiECS::ComponentManager<Bone>::iterator> bone_refs;

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
		XMFLOAT4X4 skinningRemap;
	};

	struct Light
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;

		enum LightType {
			DIRECTIONAL			= ENTITY_TYPE_DIRECTIONALLIGHT,
			POINT				= ENTITY_TYPE_POINTLIGHT,
			SPOT				= ENTITY_TYPE_SPOTLIGHT,
			SPHERE				= ENTITY_TYPE_SPHERELIGHT,
			DISC				= ENTITY_TYPE_DISCLIGHT,
			RECTANGLE			= ENTITY_TYPE_RECTANGLELIGHT,
			TUBE				= ENTITY_TYPE_TUBELIGHT,
			LIGHTTYPE_COUNT,
		};

		XMFLOAT4 color;
		XMFLOAT4 enerDis;
		bool volumetrics = false;
		bool noHalo;
		bool shadow;
		std::vector<wiGraphicsTypes::Texture2D*> lensFlareRimTextures;
		std::vector<std::string> lensFlareNames;

		static wiGraphicsTypes::Texture2D* shadowMapArray_2D;
		static wiGraphicsTypes::Texture2D* shadowMapArray_Cube;
		static wiGraphicsTypes::Texture2D* shadowMapArray_Transparent;
		int shadowMap_index;
		int entityArray_index;

		std::vector<XMFLOAT4X4> shadowMaterices;

		float shadowBias;

		// area light props:
		float radius, width, height;
	};

	struct Camera
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;

		XMFLOAT4X4 View, Projection, VP;
		XMFLOAT3 Eye, At, Up;
		float width, height;
		float zNearP, zFarP;
		float fov;
		Frustum frustum;
		XMFLOAT4X4 InvView, InvProjection, InvVP;
		XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...

		XMVECTOR GetEye() const
		{
			return XMLoadFloat3(&Eye);
		}
		XMVECTOR GetAt() const
		{
			return XMLoadFloat3(&At);
		}
		XMVECTOR GetUp() const
		{
			return XMLoadFloat3(&Up);
		}
		XMVECTOR GetRight() const
		{
			return XMVector3Cross(GetAt(), GetUp());
		}
		XMMATRIX GetView() const
		{
			return XMLoadFloat4x4(&View);
		}
		XMMATRIX GetInvView() const
		{
			return XMLoadFloat4x4(&InvView);
		}
		XMMATRIX GetProjection() const
		{
			return XMLoadFloat4x4(&Projection);
		}
		XMMATRIX GetInvProjection() const
		{
			return XMLoadFloat4x4(&InvProjection);
		}
		XMMATRIX GetViewProjection() const
		{
			return XMLoadFloat4x4(&VP);
		}
		XMMATRIX GetInvViewProjection() const
		{
			return XMLoadFloat4x4(&InvVP);
		}
	};

	struct EnvironmentProbe
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;
		int textureIndex = -1;
		bool realTime = false;
		bool isUpToDate = false;
	};

	struct ForceField
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;
	};

	struct Decal
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;
		wiECS::ComponentManager<Material>::iterator material_ref;

		XMFLOAT4 atlasMulAdd = XMFLOAT4(0, 0, 0, 0);
	};

	struct Model
	{
		wiECS::ComponentManager<Node>::iterator node_ref;
		wiECS::ComponentManager<Transform>::iterator transform_ref;

		std::vector<wiECS::ComponentManager<Material>::iterator> material_refs;
		std::vector<wiECS::ComponentManager<Mesh>::iterator> mesh_refs;
		std::vector<wiECS::ComponentManager<Object>::iterator> object_refs;
		std::vector<wiECS::ComponentManager<Armature>::iterator> armature_refs;
		std::vector<wiECS::ComponentManager<Light>::iterator> light_refs;
		std::vector<wiECS::ComponentManager<EnvironmentProbe>::iterator> probe_refs;
		std::vector<wiECS::ComponentManager<ForceField>::iterator> force_refs;
		std::vector<wiECS::ComponentManager<Decal>::iterator> decal_refs;
		std::vector<wiECS::ComponentManager<Camera>::iterator> camera_refs;
	};

	struct Scene
	{
		wiECS::ComponentManager<Node> nodes;
		wiECS::ComponentManager<Transform> transforms;
		wiECS::ComponentManager<Material> materials;
		wiECS::ComponentManager<Mesh> meshes;
		wiECS::ComponentManager<Object> objects;
		wiECS::ComponentManager<Bone> bones;
		wiECS::ComponentManager<Armature> armatures;
		wiECS::ComponentManager<Light> lights;
		wiECS::ComponentManager<Camera> cameras;
		wiECS::ComponentManager<EnvironmentProbe> probes;
		wiECS::ComponentManager<ForceField> forces;
		wiECS::ComponentManager<Decal> decals;
		wiECS::ComponentManager<Model> models;

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
	};

}

