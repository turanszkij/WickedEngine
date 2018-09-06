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

	struct NameComponent
	{
		char name[128];

		inline void operator=(const std::string& str) { strcpy_s(name, str.c_str()); }
	};

	struct LayerComponent
	{
		uint32_t layerMask = ~0;

		inline uint32_t GetLayerMask() const { return layerMask; }
	};
	
	struct TransformComponent
	{
		bool dirty = true;

		XMFLOAT3 scale_local = XMFLOAT3(1, 1, 1);
		XMFLOAT4 rotation_local = XMFLOAT4(0, 0, 0, 1);
		XMFLOAT3 translation_local = XMFLOAT3(0, 0, 0);

		XMFLOAT3 scale;
		XMFLOAT4 rotation;
		XMFLOAT3 translation;
		XMFLOAT4X4 world;
		XMFLOAT4X4 world_prev;

		void UpdateTransform();
		void UpdateParentedTransform(const TransformComponent& parent, const XMFLOAT4X4& inverseParentBindMatrix);
		void ApplyTransform();
		void ClearTransform();
		void Translate(const XMFLOAT3& value);
		void RotateRollPitchYaw(const XMFLOAT3& value);
		void Rotate(const XMFLOAT4& quaternion);
		void Scale(const XMFLOAT3& value);
		void MatrixTransform(const XMFLOAT4X4& matrix);
		void MatrixTransform(const XMMATRIX& matrix);
		void Lerp(const TransformComponent& a, const TransformComponent& b, float t);
		void CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t);
	};

	struct ParentComponent
	{
		wiECS::Entity parentID;
		uint32_t layerMask_bind; // saved child layermask at the time of binding
		XMFLOAT4X4 world_parent_inverse_bind; // saved parent inverse worldmatrix at the time of binding
	};

	struct MaterialComponent
	{
		bool dirty = true; // can trigger constant buffer update

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
		
		XMFLOAT2 texAnimDirection = XMFLOAT2(0, 0);
		float texAnimFrameRate = 0.0f;
		float texAnimSleep = 0.0f;

		std::string baseColorMapName;
		wiGraphicsTypes::Texture2D* baseColorMap = nullptr;

		std::string surfaceMapName;
		wiGraphicsTypes::Texture2D* surfaceMap = nullptr;

		std::string normalMapName;
		wiGraphicsTypes::Texture2D* normalMap = nullptr;

		std::string displacementMapName;
		wiGraphicsTypes::Texture2D* displacementMap = nullptr;

		std::unique_ptr<wiGraphicsTypes::GPUBuffer> constantBuffer;

		inline void SetUserStencilRef(uint8_t value)
		{
			assert(value < 128);
			userStencilRef = value & 0x0F;
		}
		inline UINT GetStencilRef() const
		{
			return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
		}

		wiGraphicsTypes::Texture2D* GetBaseColorMap() const;
		wiGraphicsTypes::Texture2D* GetNormalMap() const;
		wiGraphicsTypes::Texture2D* GetSurfaceMap() const;
		wiGraphicsTypes::Texture2D* GetDisplacementMap() const;

		inline float GetOpacity() const { return baseColor.w; }

		inline bool IsTransparent() const { return GetOpacity() < 1.0f; }
		inline bool IsWater() const { return water; }
		inline bool HasPlanarReflection() const { return planar_reflections || IsWater(); }
		inline bool IsCastingShadow() const { return cast_shadow; }
		inline bool IsAlphaTestEnabled() const { return alphaRef <= 1.0f - 1.0f / 256.0f; }

		inline void SetBaseColor(const XMFLOAT4& value) { dirty = true; baseColor = value; }
		inline void SetRoughness(float value) { dirty = true; roughness = value; }
		inline void SetReflectance(float value) { dirty = true; reflectance = value; }
		inline void SetMetalness(float value) { dirty = true; metalness = value; }
		inline void SetEmissive(float value) { dirty = true; emissive = value; }
		inline void SetRefractionIndex(float value) { dirty = true; refractionIndex = value; }
		inline void SetSubsurfaceScattering(float value) { dirty = true; subsurfaceScattering = value; }
		inline void SetNormalMapStrength(float value) { dirty = true; normalMapStrength = value; }
		inline void SetParallaxOcclusionMapping(float value) { dirty = true; parallaxOcclusionMapping = value; }
		inline void SetOpacity(float value) { dirty = true;  baseColor.w = value; }
		inline void SetAlphaRef(float value) { alphaRef = value; }
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
			wiECS::Entity materialID;
			UINT indexBufferOffset = 0;

			std::vector<uint32_t> subsetIndices;
		};
		std::vector<MeshSubset>		subsets;

		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	indexBuffer;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_POS;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_TEX;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	vertexBuffer_BON;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	streamoutBuffer_POS;
		std::unique_ptr<wiGraphicsTypes::GPUBuffer>	streamoutBuffer_PRE;

		// Dynamic vertexbuffers write into a global pool, these will be the offsets into that:
		bool dynamicVB = false;
		UINT bufferOffset_POS;
		UINT bufferOffset_PRE;

		wiGraphicsTypes::INDEXBUFFER_FORMAT indexFormat = wiGraphicsTypes::INDEXFORMAT_16BIT;

		bool renderable = true;
		bool doubleSided = false;

		AABB aabb;

		wiECS::Entity armatureID;

		struct VertexGroup 
		{
			std::string name;
			std::unordered_map<int, float> vertex_weights; // maps vertex index to weight value...
		};
		std::vector<VertexGroup> vertexGroups;

		wiRenderTarget	impostorTarget;
		float impostorDistance = 100.0f;

		float tessellationFactor;

		bool HasImpostor() const { return impostorTarget.IsInitialized(); }
		inline float GetTessellationFactor() const { return tessellationFactor; }
		inline wiGraphicsTypes::INDEXBUFFER_FORMAT GetIndexFormat() const { return indexFormat; }
		inline bool IsSkinned() const { return armatureID != wiECS::INVALID_ENTITY; }
		inline bool IsDynamicVB() const { return dynamicVB; }

		void CreateRenderData();
		void ComputeNormals(bool smooth);
		void FlipCulling();
		void FlipNormals();
	};

	struct CullableComponent
	{
		AABB aabb;
	};

	struct ObjectComponent
	{
		wiECS::Entity meshID = wiECS::INVALID_ENTITY;
		bool renderable = true;
		bool dynamic = false;
		bool cast_shadow = false;
		int cascadeMask = 0; // which shadow cascades to skip (0: skip none, 1: skip first, etc...)
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);

		uint32_t rendertypeMask = 0;

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
		inline bool IsDynamic() const { return dynamic; }
		inline bool IsCastingShadow() const { return cast_shadow; }
		inline float GetTransparency() const { return 1 - color.w; }
		inline uint32_t GetRenderTypes() const { return rendertypeMask; }
	};

	struct PhysicsComponent
	{
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
		XMFLOAT4X4 inverseBindPoseMatrix;
	};

	struct ArmatureComponent
	{
		std::vector<wiECS::Entity> boneCollection;
		std::vector<XMFLOAT4X4> skinningMatrices;

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
		std::unique_ptr<wiGraphicsTypes::GPUBuffer> boneBuffer;

		// This will be used to eg. mirror the whole skin, without modifying the armature transform itself
		//	It will affect the skin only, so the mesh vertices should be mirrored as well to work correctly!
		XMFLOAT4X4 skinningRemap = XMFLOAT4X4(1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1);
	};

	struct LightComponent
	{
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

		inline void SetType(LightType val) { 
			type = val;
			switch (type)
			{
			case DIRECTIONAL:
			case SPOT:
				shadowBias = 0.0001f;
				break;
			case POINT:
			case SPHERE:
			case DISC:
			case RECTANGLE:
			case TUBE:
			case LIGHTTYPE_COUNT:
				shadowBias = 0.1f;
				break;
			}
		}
		inline LightType GetType() const { return type; }

		XMFLOAT3 color = XMFLOAT3(1, 1, 1);
		float energy = 1.0f;
		float range = 10.0f;
		float fov = XM_PIDIV4;
		bool volumetrics = false;
		bool visualizer = false;
		bool shadow = false;
		std::vector<wiGraphicsTypes::Texture2D*> lensFlareRimTextures;
		std::vector<std::string> lensFlareNames;

		XMFLOAT3 direction;

		int shadowMap_index = -1;
		int entityArray_index = -1;

		float shadowBias = 0.0001f;

		// area light props:
		float radius = 1.0f;
		float width = 1.0f;
		float height = 1.0f;

		inline float GetRange() const { return range; }

		struct SHCAM 
		{
			XMFLOAT4X4 View, Projection;
			XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...
			XMFLOAT3 Eye, At, Up;
			float nearplane, farplane, size;

			SHCAM() {
				nearplane = 0.1f; farplane = 200, size = 0;
				Init(XMQuaternionIdentity());
				Create_Perspective(XM_PI / 2.0f);
			}
			//orthographic
			SHCAM(float size, const XMVECTOR& dir, float nearP, float farP) {
				nearplane = nearP;
				farplane = farP;
				Init(dir);
				Create_Ortho(size);
			};
			//perspective
			SHCAM(const XMFLOAT4& dir, float newNear, float newFar, float newFov) {
				size = 0;
				nearplane = newNear;
				farplane = newFar;
				Init(XMLoadFloat4(&dir));
				Create_Perspective(newFov);
			};
			void Init(const XMVECTOR& dir) {
				XMMATRIX rot = XMMatrixRotationQuaternion(dir);
				XMVECTOR rEye = XMVectorSet(0, 0, 0, 0);
				XMVECTOR rAt = XMVector3Transform(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), rot);
				XMVECTOR rUp = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rot);
				XMMATRIX rView = XMMatrixLookAtLH(rEye, rAt, rUp);

				XMStoreFloat3(&Eye, rEye);
				XMStoreFloat3(&At, rAt);
				XMStoreFloat3(&Up, rUp);
				XMStoreFloat4x4(&View, rView);
			}
			void Create_Ortho(float size) {
				XMMATRIX rProjection = XMMatrixOrthographicOffCenterLH(-size * 0.5f, size*0.5f, -size * 0.5f, size*0.5f, farplane, nearplane);
				XMStoreFloat4x4(&Projection, rProjection);
				rProjection = XMMatrixOrthographicOffCenterLH(-size * 0.5f, size*0.5f, -size * 0.5f, size*0.5f, nearplane, farplane);
				XMStoreFloat4x4(&realProjection, rProjection);
				this->size = size;
			}
			void Create_Perspective(float fov) {
				XMMATRIX rProjection = XMMatrixPerspectiveFovLH(fov, 1, farplane, nearplane);
				XMStoreFloat4x4(&Projection, rProjection);
				rProjection = XMMatrixPerspectiveFovLH(fov, 1, nearplane, farplane);
				XMStoreFloat4x4(&realProjection, rProjection);
			}
			void Update(const XMVECTOR& pos) {
				XMStoreFloat4x4(&View, XMMatrixTranslationFromVector(-pos)
					* XMMatrixLookAtLH(XMLoadFloat3(&Eye), XMLoadFloat3(&At), XMLoadFloat3(&Up))
				);
			}
			void Update(const XMMATRIX& mat) {
				XMVECTOR sca, rot, tra;
				XMMatrixDecompose(&sca, &rot, &tra, mat);

				XMMATRIX mRot = XMMatrixRotationQuaternion(rot);

				XMVECTOR rEye = XMVectorAdd(XMLoadFloat3(&Eye), tra);
				XMVECTOR rAt = XMVectorAdd(XMVector3Transform(XMLoadFloat3(&At), mRot), tra);
				XMVECTOR rUp = XMVector3Transform(XMLoadFloat3(&Up), mRot);

				XMStoreFloat4x4(&View,
					XMMatrixLookAtLH(rEye, rAt, rUp)
				);
			}
			void Update(const XMMATRIX& rot, const XMVECTOR& tra)
			{
				XMVECTOR rEye = XMVectorAdd(XMLoadFloat3(&Eye), tra);
				XMVECTOR rAt = XMVectorAdd(XMVector3Transform(XMLoadFloat3(&At), rot), tra);
				XMVECTOR rUp = XMVector3Transform(XMLoadFloat3(&Up), rot);

				XMStoreFloat4x4(&View,
					XMMatrixLookAtLH(rEye, rAt, rUp)
				);
			}
			XMMATRIX getVP() const {
				return XMMatrixTranspose(XMLoadFloat4x4(&View)*XMLoadFloat4x4(&Projection));
			}
		};

		std::vector<SHCAM> shadowCam_pointLight;
		std::vector<SHCAM> shadowCam_dirLight;
		std::vector<SHCAM> shadowCam_spotLight;

	};

	struct CameraComponent
	{
		float width = 0.0f, height = 0.0f;
		float zNearP = 0.001f, zFarP = 800.0f;
		float fov = XM_PI / 3.0f;
		XMFLOAT3 Eye, At, Up;
		XMFLOAT3X3 rotationMatrix;
		XMFLOAT4X4 View, Projection, VP;
		Frustum frustum;
		XMFLOAT4X4 InvView, InvProjection, InvVP;
		XMFLOAT4X4 realProjection; // because reverse zbuffering projection complicates things...

		void CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV = XM_PI / 3.0f);
		void UpdateProjection();
		void UpdateCamera(const TransformComponent* transform = nullptr);
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
		inline XMMATRIX GetRealProjection() const { return XMLoadFloat4x4(&realProjection); }
	};

	struct EnvironmentProbeComponent
	{
		int textureIndex = -1;
		bool realTime = false;
		bool isUpToDate = false;
		XMFLOAT3 position;
	};

	struct ForceFieldComponent
	{
		int type = ENTITY_TYPE_FORCEFIELD_POINT;
		float gravity = 0.0f; // negative = deflector, positive = attractor
		float range = 0.0f; // affection range
	};

	struct DecalComponent
	{
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 atlasMulAdd = XMFLOAT4(0, 0, 0, 0);
		XMFLOAT4X4 world;
		XMFLOAT3 front;

		float emissive = 0;

		std::string textureName;
		wiGraphicsTypes::Texture2D* texture = nullptr;

		std::string normalMapName;
		wiGraphicsTypes::Texture2D* normal = nullptr;

		inline float GetOpacity() const { return color.w; }
	};

	struct AnimationComponent
	{
		struct AnimationChannel
		{
			wiECS::Entity target = wiECS::INVALID_ENTITY;
			enum class Type
			{
				TRANSLATION,
				ROTATION,
				SCALE
			} type = Type::TRANSLATION;
			enum class Mode
			{
				LINEAR,
				STEP,
			} mode = Mode::LINEAR;
			std::vector<float> keyframe_times;
			std::vector<float> keyframe_data;
		};

		std::vector<AnimationChannel> channels;
		float timer = 0.0f;
		float length = 0.0f;
		bool looped = false;
		bool playing = true;

		inline void Pause() { playing = false; }
		inline void Stop() { playing = false; timer = 0.0f; }
		inline void Play() { playing = true; }
	};

	struct ModelComponent
	{
		std::unordered_set<wiECS::Entity> materials;
		std::unordered_set<wiECS::Entity> objects;
		std::unordered_set<wiECS::Entity> meshes;
		std::unordered_set<wiECS::Entity> armatures;
		std::unordered_set<wiECS::Entity> lights;
		std::unordered_set<wiECS::Entity> cameras;
		std::unordered_set<wiECS::Entity> probes;
		std::unordered_set<wiECS::Entity> forces;
		std::unordered_set<wiECS::Entity> decals;
	};

	struct Scene
	{
		std::unordered_set<wiECS::Entity> owned_entities;

		wiECS::ComponentManager<NameComponent> names;
		wiECS::ComponentManager<LayerComponent> layers;
		wiECS::ComponentManager<TransformComponent> transforms;
		wiECS::ComponentManager<ParentComponent> parents;
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
		wiECS::ComponentManager<AnimationComponent> animations;
		wiECS::ComponentManager<ModelComponent> models;

		AABB bounds;
		XMFLOAT3 sunDirection = XMFLOAT3(0, 1, 0);
		XMFLOAT3 sunColor = XMFLOAT3(0, 0, 0);
		XMFLOAT3 horizon = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 zenith = XMFLOAT3(0.00f, 0.00f, 0.0f);
		XMFLOAT3 ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		float fogStart = 100;
		float fogEnd = 1000;
		float fogHeight = 0;
		XMFLOAT4 waterPlane = XMFLOAT4(0, 1, 0, 0);
		float cloudiness = 0.0f;
		float cloudScale = 0.0003f;
		float cloudSpeed = 0.1f;
		XMFLOAT3 windDirection = XMFLOAT3(0, 0, 0);
		float windRandomness = 5;
		float windWaveSize = 1;

		// Update all components by a given timestep (in milliseconds):
		void Update(float dt);
		// Remove everything from the scene that it owns:
		void Clear();

		// Removes a specific entity from the scene (if it exists):
		void Entity_Remove(wiECS::Entity entity);
		// Finds the first entity by the name (if it exists, otherwise returns INVALID_ENTITY):
		wiECS::Entity Entity_FindByName(const std::string& name);
		// Helper function to create a model entity:
		wiECS::Entity Entity_CreateModel(
			const std::string& name
		);
		// Helper function to create a material entity:
		wiECS::Entity Entity_CreateMaterial(
			const std::string& name
		);
		// Helper function to create a model entity:
		wiECS::Entity Entity_CreateObject(
			const std::string& name
		);
		// Helper function to create a model entity:
		wiECS::Entity Entity_CreateMesh(
			const std::string& name
		);
		// Helper function to create a light entity:
		wiECS::Entity Entity_CreateLight(
			const std::string& name, 
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0), 
			const XMFLOAT3& color = XMFLOAT3(1, 1, 1), 
			float energy = 1, 
			float range = 10
		);
		// Helper function to create a force entity:
		wiECS::Entity Entity_CreateForce(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		// Helper function to create an environment capture probe entity:
		wiECS::Entity Entity_CreateEnvironmentProbe(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		// Helper function to create a decal entity:
		wiECS::Entity Entity_CreateDecal(
			const std::string& name,
			const std::string& textureName,
			const std::string& normalMapName = ""
		);
		// Helper function to create a camera entity:
		wiECS::Entity Entity_CreateCamera(
			const std::string& name,
			float width, float height, float nearPlane = 0.01f, float farPlane = 1000.0f, float fov = XM_PIDIV4
		);

		// Attaches an entity to a parent:
		void Component_Attach(wiECS::Entity entity, wiECS::Entity parent);
		// Detaches the entity from its parent (if it is attached):
		void Component_Detach(wiECS::Entity entity);
		// Detaches all children from an entity (if there are any):
		void Component_DetachChildren(wiECS::Entity parent);
	};

	void RunAnimationUpdateSystem(
		wiECS::ComponentManager<AnimationComponent>& animations,
		wiECS::ComponentManager<TransformComponent>& transforms,
		float dt
	);
	void RunTransformUpdateSystem(wiECS::ComponentManager<TransformComponent>& transforms);
	void RunHierarchyUpdateSystem(
		const wiECS::ComponentManager<ParentComponent>& parents,
		wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<LayerComponent>& layers
	);
	void RunPhysicsUpdateSystem(
		wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<MeshComponent>& meshes,
		wiECS::ComponentManager<ObjectComponent>& objects,
		wiECS::ComponentManager<PhysicsComponent>& physicscomponents
	);
	void RunArmatureUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		const wiECS::ComponentManager<BoneComponent>& bones,
		wiECS::ComponentManager<ArmatureComponent>& armatures
	);
	void RunMaterialUpdateSystem(wiECS::ComponentManager<MaterialComponent>& materials, float dt);
	void RunObjectUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		const wiECS::ComponentManager<MeshComponent>& meshes,
		const wiECS::ComponentManager<MaterialComponent>& materials,
		wiECS::ComponentManager<ObjectComponent>& objects,
		wiECS::ComponentManager<CullableComponent>& cullables,
		AABB& sceneBounds,
		XMFLOAT4& waterPlane
	);
	void RunCameraUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<CameraComponent>& cameras
	);
	void RunDecalUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<CullableComponent>& cullables,
		wiECS::ComponentManager<DecalComponent>& decals
	);
	void RunProbeUpdateSystem(
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<CullableComponent>& cullables,
		wiECS::ComponentManager<EnvironmentProbeComponent>& probes
	);
	void RunLightUpdateSystem(
		const CameraComponent& cascadeCamera,
		const wiECS::ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<CullableComponent>& cullables,
		wiECS::ComponentManager<LightComponent>& lights,
		XMFLOAT3& sunDirection, XMFLOAT3& sunColor
	);

}

