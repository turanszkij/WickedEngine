#pragma once
#include "CommonInclude.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiGPUBVH.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"
#include "wiMath.h"
#include "wiECS.h"
#include "wiScene_Components.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "wiTerrain.h"
#include "wiBVH.h"
#include "wiUnorderedSet.h"

#include <string>
#include <memory>
#include <limits>

namespace wi::scene
{
	struct Scene
	{
		virtual ~Scene() = default;

		wi::ecs::ComponentLibrary componentLibrary;

		wi::ecs::ComponentManager<NameComponent>& names = componentLibrary.Register<NameComponent>("wi::scene::Scene::names");
		wi::ecs::ComponentManager<LayerComponent>& layers = componentLibrary.Register<LayerComponent>("wi::scene::Scene::layers");
		wi::ecs::ComponentManager<TransformComponent>& transforms = componentLibrary.Register<TransformComponent>("wi::scene::Scene::transforms");
		wi::ecs::ComponentManager<HierarchyComponent>& hierarchy = componentLibrary.Register<HierarchyComponent>("wi::scene::Scene::hierarchy");
		wi::ecs::ComponentManager<MaterialComponent>& materials = componentLibrary.Register<MaterialComponent>("wi::scene::Scene::materials", 2); // version = 2
		wi::ecs::ComponentManager<MeshComponent>& meshes = componentLibrary.Register<MeshComponent>("wi::scene::Scene::meshes", 2); // version = 2
		wi::ecs::ComponentManager<ImpostorComponent>& impostors = componentLibrary.Register<ImpostorComponent>("wi::scene::Scene::impostors");
		wi::ecs::ComponentManager<ObjectComponent>& objects = componentLibrary.Register<ObjectComponent>("wi::scene::Scene::objects", 2); // version = 2
		wi::ecs::ComponentManager<RigidBodyPhysicsComponent>& rigidbodies = componentLibrary.Register<RigidBodyPhysicsComponent>("wi::scene::Scene::rigidbodies", 1); // version = 1
		wi::ecs::ComponentManager<SoftBodyPhysicsComponent>& softbodies = componentLibrary.Register<SoftBodyPhysicsComponent>("wi::scene::Scene::softbodies");
		wi::ecs::ComponentManager<ArmatureComponent>& armatures = componentLibrary.Register<ArmatureComponent>("wi::scene::Scene::armatures");
		wi::ecs::ComponentManager<LightComponent>& lights = componentLibrary.Register<LightComponent>("wi::scene::Scene::lights", 2); // version = 2
		wi::ecs::ComponentManager<CameraComponent>& cameras = componentLibrary.Register<CameraComponent>("wi::scene::Scene::cameras");
		wi::ecs::ComponentManager<EnvironmentProbeComponent>& probes = componentLibrary.Register<EnvironmentProbeComponent>("wi::scene::Scene::probes", 1); // version = 1
		wi::ecs::ComponentManager<ForceFieldComponent>& forces = componentLibrary.Register<ForceFieldComponent>("wi::scene::Scene::forces", 1); // version = 1
		wi::ecs::ComponentManager<DecalComponent>& decals = componentLibrary.Register<DecalComponent>("wi::scene::Scene::decals", 1); // version = 1
		wi::ecs::ComponentManager<AnimationComponent>& animations = componentLibrary.Register<AnimationComponent>("wi::scene::Scene::animations", 1); // version = 1
		wi::ecs::ComponentManager<AnimationDataComponent>& animation_datas = componentLibrary.Register<AnimationDataComponent>("wi::scene::Scene::animation_datas");
		wi::ecs::ComponentManager<EmittedParticleSystem>& emitters = componentLibrary.Register<EmittedParticleSystem>("wi::scene::Scene::emitters");
		wi::ecs::ComponentManager<HairParticleSystem>& hairs = componentLibrary.Register<HairParticleSystem>("wi::scene::Scene::hairs");
		wi::ecs::ComponentManager<WeatherComponent>& weathers = componentLibrary.Register<WeatherComponent>("wi::scene::Scene::weathers", 5); // version = 5
		wi::ecs::ComponentManager<SoundComponent>& sounds = componentLibrary.Register<SoundComponent>("wi::scene::Scene::sounds", 1); // version = 1
		wi::ecs::ComponentManager<VideoComponent>& videos = componentLibrary.Register<VideoComponent>("wi::scene::Scene::videos");
		wi::ecs::ComponentManager<InverseKinematicsComponent>& inverse_kinematics = componentLibrary.Register<InverseKinematicsComponent>("wi::scene::Scene::inverse_kinematics");
		wi::ecs::ComponentManager<SpringComponent>& springs = componentLibrary.Register<SpringComponent>("wi::scene::Scene::springs", 1); // version = 1
		wi::ecs::ComponentManager<ColliderComponent>& colliders = componentLibrary.Register<ColliderComponent>("wi::scene::Scene::colliders", 2); // version = 2
		wi::ecs::ComponentManager<ScriptComponent>& scripts = componentLibrary.Register<ScriptComponent>("wi::scene::Scene::scripts");
		wi::ecs::ComponentManager<ExpressionComponent>& expressions = componentLibrary.Register<ExpressionComponent>("wi::scene::Scene::expressions");
		wi::ecs::ComponentManager<HumanoidComponent>& humanoids = componentLibrary.Register<HumanoidComponent>("wi::scene::Scene::humanoids");
		wi::ecs::ComponentManager<wi::terrain::Terrain>& terrains = componentLibrary.Register<wi::terrain::Terrain>("wi::scene::Scene::terrains", 3); // version = 3
		wi::ecs::ComponentManager<wi::Sprite>& sprites = componentLibrary.Register<wi::Sprite>("wi::scene::Scene::sprites");
		wi::ecs::ComponentManager<wi::SpriteFont>& fonts = componentLibrary.Register<wi::SpriteFont>("wi::scene::Scene::fonts");

		// Non-serialized attributes:
		float dt = 0;
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t flags = EMPTY;

		float time = 0;
		CameraComponent camera; // for LOD and 3D sound update
		std::shared_ptr<void> physics_scene;
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

		// AABB culling streams:
		wi::vector<wi::primitive::AABB> aabb_objects;
		wi::vector<wi::primitive::AABB> aabb_lights;
		wi::vector<wi::primitive::AABB> aabb_probes;
		wi::vector<wi::primitive::AABB> aabb_decals;

		// Separate stream of world matrices:
		wi::vector<XMFLOAT4X4> matrix_objects;
		wi::vector<XMFLOAT4X4> matrix_objects_prev;

		// Shader visible scene parameters:
		ShaderScene shaderscene;

		// Instances for bindless visiblity indexing:
		//	contains in order:
		//		1) objects
		//		2) hair particles
		//		3) emitted particles
		//		4) impostors
		wi::graphics::GPUBuffer instanceUploadBuffer[wi::graphics::GraphicsDevice::GetBufferCount()];
		ShaderMeshInstance* instanceArrayMapped = nullptr;
		size_t instanceArraySize = 0;
		wi::graphics::GPUBuffer instanceBuffer;

		// Geometries for bindless visiblity indexing:
		//	contains in order:
		//		1) meshes * mesh.subsetCount
		//		2) hair particles * 1
		//		3) emitted particles * 1
		//		4) impostors * 1
		wi::graphics::GPUBuffer geometryUploadBuffer[wi::graphics::GraphicsDevice::GetBufferCount()];
		ShaderGeometry* geometryArrayMapped = nullptr;
		size_t geometryArraySize = 0;
		wi::graphics::GPUBuffer geometryBuffer;
		std::atomic<uint32_t> geometryAllocator{ 0 };

		// Materials for bindless visibility indexing:
		wi::graphics::GPUBuffer materialUploadBuffer[wi::graphics::GraphicsDevice::GetBufferCount()];
		ShaderMaterial* materialArrayMapped = nullptr;
		size_t materialArraySize = 0;
		wi::graphics::GPUBuffer materialBuffer;

		// Meshlets:
		wi::graphics::GPUBuffer meshletBuffer;
		std::atomic<uint32_t> meshletAllocator{ 0 };

		// Skinning GPU data containining all bones, all morph descriptions:
		wi::graphics::GPUBuffer skinningUploadBuffer[wi::graphics::GraphicsDevice::GetBufferCount()];
		void* skinningDataMapped = nullptr;
		size_t skinningDataSize = 0;
		wi::graphics::GPUBuffer skinningBuffer;
		std::atomic<uint32_t> skinningAllocator{ 0 };

		// Occlusion query state:
		struct OcclusionResult
		{
			int occlusionQueries[wi::graphics::GraphicsDevice::GetBufferCount()];
			// occlusion result history bitfield (32 bit->32 frame history)
			uint32_t occlusionHistory = ~0u;

			constexpr bool IsOccluded() const
			{
				// Perform a conservative occlusion test:
				// If it is visible in any frames in the history, it is determined visible in this frame
				// But if all queries failed in the history, it is occluded.
				// If it pops up for a frame after occluded, it is visible again for some frames
				return occlusionHistory == 0;
			}
		};
		mutable wi::vector<OcclusionResult> occlusion_results_objects;
		wi::graphics::GPUQueryHeap queryHeap;
		wi::graphics::GPUBuffer queryResultBuffer[arraysize(OcclusionResult::occlusionQueries)];
		wi::graphics::GPUBuffer queryPredicationBuffer;
		uint32_t queryheap_idx = 0;
		mutable std::atomic<uint32_t> queryAllocator{ 0 };

		// Surfel GI resources:
		wi::graphics::GPUBuffer surfelBuffer;
		wi::graphics::GPUBuffer surfelDataBuffer;
		wi::graphics::GPUBuffer surfelAliveBuffer[2];
		wi::graphics::GPUBuffer surfelDeadBuffer;
		wi::graphics::GPUBuffer surfelStatsBuffer;
		wi::graphics::GPUBuffer surfelIndirectBuffer;
		wi::graphics::GPUBuffer surfelGridBuffer;
		wi::graphics::GPUBuffer surfelCellBuffer;
		wi::graphics::GPUBuffer surfelRayBuffer;
		wi::graphics::Texture surfelMomentsTexture[2];

		// DDGI resources:
		struct DDGI
		{
			uint frame_index = 0;
			uint3 grid_dimensions = uint3(32, 8, 32); // The scene extents will be subdivided into a grid of this resolution, each grid cell will have one probe
			float3 grid_min = float3(-1, -1, -1);
			float3 grid_max = float3(1, 1, 1);
			float smooth_backface = 0; // smoothness of backface test
			wi::graphics::GPUBuffer ray_buffer;
			wi::graphics::GPUBuffer offset_buffer;
			wi::graphics::GPUBuffer sparse_tile_pool;
			wi::graphics::Texture color_texture[2];
			wi::graphics::Texture color_texture_rw[2]; // alias of color_texture
			wi::graphics::Texture depth_texture[2];

			void Serialize(wi::Archive& archive);
		} ddgi;

		// Voxel GI resources:
		struct VXGI
		{
			uint32_t res = 64;
			float rayStepSize = 1;
			float maxDistance = 100.0f;
			struct ClipMap
			{
				float voxelsize = 0.125;
				XMFLOAT3 center = XMFLOAT3(0, 0, 0);
				XMINT3 offsetfromPrevFrame = XMINT3(0, 0, 0);
				XMFLOAT3 extents = XMFLOAT3(0, 0, 0);
			} clipmaps[VXGI_CLIPMAP_COUNT];
			uint32_t clipmap_to_update = 0;

			wi::graphics::Texture radiance;
			wi::graphics::Texture prev_radiance;
			wi::graphics::Texture render_atomic;
			wi::graphics::Texture sdf;
			wi::graphics::Texture sdf_temp;
			mutable bool pre_clear = true;
		} vxgi;

		EnvironmentProbeComponent global_dynamic_probe; // when no envprobes are placed, this will be the fallback

		// Impostor state:
		static constexpr uint32_t maxImpostorCount = 8;
		static constexpr uint32_t impostorTextureDim = 128;
		wi::graphics::Texture impostorDepthStencil;
		wi::graphics::Texture impostorRenderTarget_Albedo_MSAA;
		wi::graphics::Texture impostorRenderTarget_Normal_MSAA;
		wi::graphics::Texture impostorRenderTarget_Surface_MSAA;
		wi::graphics::Texture impostorRenderTarget_Albedo;
		wi::graphics::Texture impostorRenderTarget_Normal;
		wi::graphics::Texture impostorRenderTarget_Surface;
		wi::graphics::Texture impostorArray;
		wi::graphics::GPUBuffer impostorBuffer;
		uint32_t allocated_impostor_capacity = 0;
		MeshComponent::BufferView impostor_ib32;
		MeshComponent::BufferView impostor_ib16;
		MeshComponent::BufferView impostor_vb_pos;
		MeshComponent::BufferView impostor_vb_nor;
		MeshComponent::BufferView impostor_data;
		MeshComponent::BufferView impostor_indirect;
		wi::graphics::Format impostor_ib_format = wi::graphics::Format::R32_UINT;
		uint32_t impostorInstanceOffset = ~0u;
		uint32_t impostorGeometryOffset = ~0u;
		uint32_t impostorMaterialOffset = ~0u;

		wi::EmittedParticleSystem rainEmitter;
		MaterialComponent rainMaterial;
		uint32_t rainInstanceOffset = ~0u;
		uint32_t rainGeometryOffset = ~0u;
		uint32_t rainMaterialOffset = ~0u;
		LightComponent rain_blocker_dummy_light;

		std::atomic<uint32_t> lightmap_request_allocator{ 0 };
		wi::vector<uint32_t> lightmap_requests;
		wi::vector<TransformComponent> transforms_temp;

		// CPU/GPU Colliders:
		std::atomic<uint32_t> collider_allocator_cpu{ 0 };
		std::atomic<uint32_t> collider_allocator_gpu{ 0 };
		wi::vector<uint8_t> collider_deinterleaved_data;
		uint32_t collider_count_cpu = 0;
		uint32_t collider_count_gpu = 0;
		wi::primitive::AABB* aabb_colliders_cpu = nullptr;
		ColliderComponent* colliders_cpu = nullptr;
		ColliderComponent* colliders_gpu = nullptr;
		wi::BVH collider_bvh;

		// Ocean GPU state:
		wi::Ocean ocean;
		void OceanRegenerate() { ocean.Create(weather.oceanParameters); }

		// Simple water ripple sprites:
		mutable wi::vector<wi::Sprite> waterRipples;
		void PutWaterRipple(const XMFLOAT3& pos);
		void PutWaterRipple(const std::string& image, const XMFLOAT3& pos);

		// Animation processing optimizer:
		struct AnimationQueue
		{
			// The animations within one queue must be processed on the same thread in order
			wi::vector<AnimationComponent*> animations; // pointers for one frame only!
			wi::unordered_set<wi::ecs::Entity> entities;
		};
		wi::vector<AnimationQueue> animation_queues; // different animation queues can be processed in different threads in any order
		size_t animation_queue_count = 0; // to avoid resizing animation queues downwards because the internals for them needs to be reallocated in that case
		wi::jobsystem::context animation_dependency_scan_workload;
		void ScanAnimationDependencies();

		// Update all components by a given timestep (in seconds):
		//	This is an expensive function, prefer to call it only once per frame!
		virtual void Update(float dt);
		// Remove everything from the scene that it owns:
		virtual void Clear();
		// Merge an other scene into this.
		//	The contents of the other scene will be lost (and moved to this)!
		virtual void Merge(Scene& other);
		// Finds all entities in the scene that have any components attached
		void FindAllEntities(wi::unordered_set<wi::ecs::Entity>& entities) const;

		// Removes (deletes) a specific entity from the scene (if it exists):
		//	recursive	: also removes children if true
		//	keep_sorted	: remove all components while keeping sorted order (slow)
		void Entity_Remove(wi::ecs::Entity entity, bool recursive = true, bool keep_sorted = false);
		// Finds the first entity by the name (if it exists, otherwise returns INVALID_ENTITY):
		//	ancestor : you can specify an ancestor entity if you only want to find entities that are descendants of ancestor entity
		wi::ecs::Entity Entity_FindByName(const std::string& name, wi::ecs::Entity ancestor = wi::ecs::INVALID_ENTITY);
		// Duplicates all of an entity's components and creates a new entity with them (recursively keeps hierarchy):
		wi::ecs::Entity Entity_Duplicate(wi::ecs::Entity entity);
		// Check whether entity is a descendant of ancestor
		//	returns true if entity is in the hierarchy tree of ancestor, false otherwise
		bool Entity_IsDescendant(wi::ecs::Entity entity, wi::ecs::Entity ancestor) const;

		enum class EntitySerializeFlags
		{
			NONE = 0,
			RECURSIVE = 1 << 0, // children entities will be also serialized
			KEEP_INTERNAL_ENTITY_REFERENCES = 1 << 1, // entity handles inside components will be kept intact, they won't use remapping of wi::ecs::EntitySerializer
		};
		// Serializes entity and all of its components to archive:
		//	archive		: archive used for serializing data
		//	seri		: serializer state for entity component system
		//	entity		: if archive is in write mode, this is the entity to serialize. If archive is in read mode, it should be INVALID_ENTITY
		//	flags		: specify options as EntitySerializeFlags bits to control internal behaviour
		// 
		//	Returns either the new entity that was read, or the original entity that was written
		wi::ecs::Entity Entity_Serialize(
			wi::Archive& archive,
			wi::ecs::EntitySerializer& seri,
			wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY,
			EntitySerializeFlags flags = EntitySerializeFlags::RECURSIVE
		);

		wi::ecs::Entity Entity_CreateTransform(
			const std::string& name
		);
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
			float intensity = 1, 
			float range = 10,
			LightComponent::LightType type = LightComponent::POINT,
			float outerConeAngle = XM_PIDIV4,
			float innerConeAngle = 0
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
		wi::ecs::Entity Entity_CreateVideo(
			const std::string& name,
			const std::string& filename
		);
		wi::ecs::Entity Entity_CreateCube(
			const std::string& name
		);
		wi::ecs::Entity Entity_CreatePlane(
			const std::string& name
		);
		wi::ecs::Entity Entity_CreateSphere(
			const std::string& name,
			float radius = 1,
			uint32_t latitudeBands = 64,
			uint32_t longitudeBands = 64
		);

		// Attaches an entity to a parent:
		//	child_already_in_local_space	:	child won't be transformed from world space to local space
		void Component_Attach(wi::ecs::Entity entity, wi::ecs::Entity parent, bool child_already_in_local_space = false);
		// Detaches the entity from its parent (if it is attached):
		void Component_Detach(wi::ecs::Entity entity);
		// Detaches all children from an entity (if there are any):
		void Component_DetachChildren(wi::ecs::Entity parent);

		void Serialize(wi::Archive& archive);

		void RunAnimationUpdateSystem(wi::jobsystem::context& ctx);
		void RunTransformUpdateSystem(wi::jobsystem::context& ctx);
		void RunHierarchyUpdateSystem(wi::jobsystem::context& ctx);
		void RunExpressionUpdateSystem(wi::jobsystem::context& ctx);
		void RunProceduralAnimationUpdateSystem(wi::jobsystem::context& ctx);
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
		void RunVideoUpdateSystem(wi::jobsystem::context& ctx);
		void RunScriptUpdateSystem(wi::jobsystem::context& ctx);
		void RunSpriteUpdateSystem(wi::jobsystem::context& ctx);
		void RunFontUpdateSystem(wi::jobsystem::context& ctx);


		struct RayIntersectionResult
		{
			wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
			XMFLOAT3 position = XMFLOAT3(0, 0, 0);
			XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
			XMFLOAT3 velocity = XMFLOAT3(0, 0, 0);
			float distance = std::numeric_limits<float>::max();
			int subsetIndex = -1;
			int vertexID0 = -1;
			int vertexID1 = -1;
			int vertexID2 = -1;
			XMFLOAT2 bary = XMFLOAT2(0, 0);
			XMFLOAT4X4 orientation = wi::math::IDENTITY_MATRIX;

			constexpr bool operator==(const RayIntersectionResult& other) const
			{
				return entity == other.entity;
			}
		};
		// Given a ray, finds the closest intersection point against all mesh instances
		//	ray				:	the incoming ray that will be traced
		//	renderTypeMask	:	filter based on render type
		//	layerMask		:	filter based on layer
		RayIntersectionResult Intersects(const wi::primitive::Ray& ray, uint32_t filterMask = wi::enums::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		struct SphereIntersectionResult
		{
			wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
			XMFLOAT3 position = XMFLOAT3(0, 0, 0);
			XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
			XMFLOAT3 velocity = XMFLOAT3(0, 0, 0);
			float depth = 0;
			int subsetIndex = -1;
			XMFLOAT4X4 orientation = wi::math::IDENTITY_MATRIX;
		};
		SphereIntersectionResult Intersects(const wi::primitive::Sphere& sphere, uint32_t filterMask = wi::enums::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		using CapsuleIntersectionResult = SphereIntersectionResult;
		CapsuleIntersectionResult Intersects(const wi::primitive::Capsule& capsule, uint32_t filterMask = wi::enums::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		// Goes through the hierarchy backwards and computes parent's world space matrix:
		XMMATRIX ComputeParentMatrixRecursive(wi::ecs::Entity entity) const;

		// Retargets an animation from a Humanoid to an other Humanoid such that the new animation will play back on the destination humanoid
		//	dst			:	destination humanoid that the animation will be fit onto
		//	src			:	the animation to copy, it should already target humanoid bones
		//	bake_data	:	if true, the retargeted data will be baked into a new animation data.
		//					if false, it will reuse the source animation data without creating a new one and retargeting will be applied at runtime on every Update
		//	src_scene	:	(optional) specify if you want to retarget from an other scene. This scene must be kept alive while you use the resulting animation!
		//					If you use a separate scene, then you mustn't serialize the scene while there are animations referencing the src_scene!
		//
		//	returns entity ID of the new animation or INVALID_ENTITY if retargeting was not successful
		wi::ecs::Entity RetargetAnimation(wi::ecs::Entity dst, wi::ecs::Entity src, bool bake_data, const Scene* src_scene = nullptr);

		// If you don't know which armature the bone is contained int, this function can be used to find the first such armature and return the bone's rest matrix
		//	If not found, return identity matrix
		XMMATRIX FindBoneRestPose(wi::ecs::Entity bone) const;
	};

	// Returns skinned vertex position in armature local space
	//	N : normal (out, optional)
	XMVECTOR SkinVertex(const MeshComponent& mesh, const ArmatureComponent& armature, uint32_t index, XMVECTOR* N = nullptr);


	// Helper that manages a global scene
	//	(You don't need to use it, but it's an option for simplicity)
	inline Scene& GetScene()
	{
		static Scene scene;
		return scene;
	}

	// Helper that manages a global camera
	//	(You don't need to use it, but it's an option for simplicity)
	inline CameraComponent& GetCamera()
	{
		static CameraComponent camera;
		return camera;
	}

	// Helper function to open a wiscene file and add the contents to the global scene
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	if true, everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	wi::ecs::Entity LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	// Helper function to open a wiscene file and add the contents to the specified scene. This is thread safe as it doesn't modify global scene
	//	scene			:	the scene that will contain the model
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	if true, everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	wi::ecs::Entity LoadModel(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	// Deprecated, use Scene::Intersects() function instead
	using PickResult = Scene::RayIntersectionResult;
	PickResult Pick(const wi::primitive::Ray& ray, uint32_t filterMask = wi::enums::FILTER_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene(), uint32_t lod = 0);

	// Deprecated, use Scene::Intersects() function instead
	using SceneIntersectSphereResult = Scene::SphereIntersectionResult;
	SceneIntersectSphereResult SceneIntersectSphere(const wi::primitive::Sphere& sphere, uint32_t filterMask = wi::enums::FILTER_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene(), uint32_t lod = 0);

	// Deprecated, use Scene::Intersects() function instead
	using SceneIntersectCapsuleResult = Scene::SphereIntersectionResult;
	SceneIntersectCapsuleResult SceneIntersectCapsule(const wi::primitive::Capsule& capsule, uint32_t filterMask = wi::enums::FILTER_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene(), uint32_t lod = 0);

}

template<>
struct enable_bitmask_operators<wi::scene::Scene::EntitySerializeFlags> {
	static const bool enable = true;
};
