#include "wiPhysics.h"

#if PHYSICSENGINE == PHYSICSENGINE_JOLT
#include "wiScene.h"
#include "wiProfiler.h"
#include "wiBacklog.h"
#include "wiJobSystem.h"
#include "wiRenderer.h"
#include "wiTimer.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif // JPH_DEBUG_RENDERER

#include <iostream>
#include <cstdarg>
#include <thread>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

using namespace wi::ecs;
using namespace wi::scene;

using namespace std;

namespace wi::physics
{
	namespace jolt
	{
		bool ENABLED = true;
		bool SIMULATION_ENABLED = true;
		bool DEBUGDRAW_ENABLED = false;
		int ACCURACY = 8;
		int softbodyIterationCount = 5;
		float TIMESTEP = 1.0f / 120.0f;
		std::mutex physicsLock;

		namespace Layers
		{
			static constexpr ObjectLayer NON_MOVING = 0;
			static constexpr ObjectLayer MOVING = 1;
			static constexpr ObjectLayer NUM_LAYERS = 2;
		};

		/// Class that determines if two object layers can collide
		class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
		{
		public:
			bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
			{
				switch (inObject1)
				{
				case Layers::NON_MOVING:
					return inObject2 == Layers::MOVING; // Non moving only collides with moving
				case Layers::MOVING:
					return true; // Moving collides with everything
				default:
					JPH_ASSERT(false);
					return false;
				}
			}
		};

		// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
		// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
		// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
		// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
		// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
		namespace BroadPhaseLayers
		{
			static constexpr BroadPhaseLayer NON_MOVING(0);
			static constexpr BroadPhaseLayer MOVING(1);
			static constexpr uint NUM_LAYERS(2);
		};

		// BroadPhaseLayerInterface implementation
		// This defines a mapping between object and broadphase layers.
		class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
		{
		public:
			BPLayerInterfaceImpl()
			{
				// Create a mapping table from object to broad phase layer
				mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
				mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
			}

			virtual uint GetNumBroadPhaseLayers() const override
			{
				return BroadPhaseLayers::NUM_LAYERS;
			}

			virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
			{
				JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
				return mObjectToBroadPhase[inLayer];
			}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
			virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
			{
				switch ((BroadPhaseLayer::Type)inLayer)
				{
				case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
				case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
				default:													JPH_ASSERT(false); return "INVALID";
				}
			}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

		private:
			BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
		};

		/// Class that determines if an object layer can collide with a broadphase layer
		class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
		{
		public:
			virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
			{
				switch (inLayer1)
				{
				case Layers::NON_MOVING:
					return inLayer2 == BroadPhaseLayers::MOVING;
				case Layers::MOVING:
					return true;
				default:
					JPH_ASSERT(false);
					return false;
				}
			}
		};

		struct JoltDestroyer
		{
			~JoltDestroyer()
			{
				UnregisterTypes();
				delete Factory::sInstance;
				Factory::sInstance = nullptr;
			}
		} jolt_destroyer; 

		struct PhysicsScene
		{
			PhysicsSystem physics_system;
			BPLayerInterfaceImpl broad_phase_layer_interface;
			ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
			ObjectLayerPairFilterImpl object_vs_object_layer_filter;
		};
		PhysicsScene& GetPhysicsScene(Scene& scene)
		{
			if (scene.physics_scene == nullptr)
			{
				auto physics_scene = std::make_shared<PhysicsScene>();

				const uint cMaxBodies = 65536;
				const uint cNumBodyMutexes = 0;
				const uint cMaxBodyPairs = 65536;
				const uint cMaxContactConstraints = 10240;
				physics_scene->physics_system.Init(
					cMaxBodies,
					cNumBodyMutexes,
					cMaxBodyPairs,
					cMaxContactConstraints,
					physics_scene->broad_phase_layer_interface,
					physics_scene->object_vs_broadphase_layer_filter,
					physics_scene->object_vs_object_layer_filter
				);

				scene.physics_scene = physics_scene;
			}
			return *(PhysicsScene*)scene.physics_scene.get();
		}

		struct RigidBody
		{
			std::shared_ptr<void> physics_scene;
			ShapeRefC shape;
			Body* body = nullptr;
			Entity entity = INVALID_ENTITY;

			Mat44 additionalTransform = Mat44::sIdentity();
			Mat44 additionalTransformInverse = Mat44::sIdentity();

			~RigidBody()
			{
				if (physics_scene == nullptr)
					return;
				BodyInterface& body_interface = ((PhysicsScene*)physics_scene.get())->physics_system.GetBodyInterfaceNoLock();
				body_interface.RemoveBody(body->GetID());
				body_interface.DestroyBody(body->GetID());
			}
		};
		struct SoftBody
		{
			std::shared_ptr<void> physics_scene;
			Body* body = nullptr;
			Entity entity = INVALID_ENTITY;

			SoftBodySharedSettings shared_settings;

			~SoftBody()
			{
				if (physics_scene == nullptr)
					return;
				BodyInterface& body_interface = ((PhysicsScene*)physics_scene.get())->physics_system.GetBodyInterfaceNoLock();
				body_interface.RemoveBody(body->GetID());
				body_interface.DestroyBody(body->GetID());
			}
		};

		RigidBody& GetRigidBody(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
		{
			if (physicscomponent.physicsobject == nullptr)
			{
				physicscomponent.physicsobject = std::make_shared<RigidBody>();
			}
			return *(RigidBody*)physicscomponent.physicsobject.get();
		}
		SoftBody& GetSoftBody(wi::scene::SoftBodyPhysicsComponent& physicscomponent)
		{
			if (physicscomponent.physicsobject == nullptr)
			{
				physicscomponent.physicsobject = std::make_shared<SoftBody>();
			}
			return *(SoftBody*)physicscomponent.physicsobject.get();
		}

		void AddRigidBody(
			wi::scene::Scene& scene,
			Entity entity,
			wi::scene::RigidBodyPhysicsComponent& physicscomponent,
			const wi::scene::TransformComponent& transform,
			const wi::scene::MeshComponent* mesh
		)
		{
			ShapeSettings::ShapeResult shape_result;

			switch (physicscomponent.shape)
			{
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
			{
				BoxShapeSettings settings(Vec3Arg(physicscomponent.box.halfextents.x * transform.scale_local.x, physicscomponent.box.halfextents.y * transform.scale_local.y, physicscomponent.box.halfextents.z * transform.scale_local.z));
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			break;
			case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
			{
				SphereShapeSettings settings(physicscomponent.sphere.radius * transform.scale_local.x);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			break;
			case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
			{
				CapsuleShapeSettings settings(physicscomponent.capsule.height * transform.scale_local.y, physicscomponent.capsule.radius * transform.scale_local.x);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			break;
			case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
			{
				CylinderShapeSettings settings(physicscomponent.box.halfextents.y * transform.scale_local.y, physicscomponent.box.halfextents.x * transform.scale_local.x);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			break;

			case RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL:
			if (mesh != nullptr)
			{
				Array<Vec3> points;
				points.reserve(mesh->vertex_positions.size());
				for (auto& pos : mesh->vertex_positions)
				{
					points.push_back(Vec3(pos.x * transform.scale_local.x, pos.y * transform.scale_local.y, pos.z * transform.scale_local.z));
				}
				ConvexHullShapeSettings settings(points);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			else
			{
				wi::backlog::post("Convex Hull physics requested, but no MeshComponent provided!", wi::backlog::LogLevel::Error);
				assert(0);
			}
			break;

			case RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH:
			if (mesh != nullptr)
			{
				TriangleList trianglelist;

				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh->GetLODSubsetRange(physicscomponent.mesh_lod, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
					const uint32_t* indices = mesh->indices.data() + subset.indexOffset;
					for (uint32_t i = 0; i < subset.indexCount; i += 3)
					{
						Triangle triangle;
						triangle.mMaterialIndex = 0;
						triangle.mV[0] = Float3(mesh->vertex_positions[indices[i + 0]].x * transform.scale_local.x, mesh->vertex_positions[indices[i + 0]].y * transform.scale_local.y, mesh->vertex_positions[indices[i + 0]].z * transform.scale_local.z);
						triangle.mV[1] = Float3(mesh->vertex_positions[indices[i + 1]].x * transform.scale_local.x, mesh->vertex_positions[indices[i + 1]].y * transform.scale_local.y, mesh->vertex_positions[indices[i + 1]].z * transform.scale_local.z);
						triangle.mV[2] = Float3(mesh->vertex_positions[indices[i + 2]].x * transform.scale_local.x, mesh->vertex_positions[indices[i + 2]].y * transform.scale_local.y, mesh->vertex_positions[indices[i + 2]].z * transform.scale_local.z);
						trianglelist.push_back(triangle);
					}
				}

				MeshShapeSettings settings(trianglelist);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			else
			{
				wi::backlog::post("Triangle Mesh physics requested, but no MeshComponent provided!", wi::backlog::LogLevel::Error);
				assert(0);
			}
			break;
			}

			if (!shape_result.IsValid())
			{
				physicscomponent.physicsobject = nullptr;
				return;
			}
			else
			{
				RigidBody& physicsobject = GetRigidBody(physicscomponent);
				physicsobject.physics_scene = scene.physics_scene;
				physicsobject.entity = entity;
				PhysicsScene& physics_scene = GetPhysicsScene(scene);
				const bool isDynamic = (physicscomponent.mass != 0.f && !physicscomponent.IsKinematic());

				physicsobject.shape = shape_result.Get();

				XMVECTOR SCA = {};
				XMVECTOR ROT = {};
				XMVECTOR TRA = {};
				XMMatrixDecompose(&SCA, &ROT, &TRA, XMLoadFloat4x4(&transform.world));
				XMFLOAT4 rot = {};
				XMFLOAT3 tra = {};
				XMStoreFloat4(&rot, ROT);
				XMStoreFloat3(&tra, TRA);

				tra.x += physicscomponent.local_offset.x;
				tra.y += physicscomponent.local_offset.y;
				tra.z += physicscomponent.local_offset.z;

				physicsobject.additionalTransform.SetTranslation(Vec3(physicscomponent.local_offset.x, physicscomponent.local_offset.y, physicscomponent.local_offset.z));
				physicsobject.additionalTransformInverse = physicsobject.additionalTransform.Inversed();

				EMotionType motionType;
				if (isDynamic)
				{
					motionType = EMotionType::Dynamic;
				}
				else if (physicscomponent.IsKinematic())
				{
					motionType = EMotionType::Kinematic;
				}
				else
				{
					motionType = EMotionType::Static;
				}

				BodyCreationSettings settings(
					physicsobject.shape.GetPtr(),
					Vec3Arg(tra.x, tra.y, tra.z),
					QuatArg(rot.x, rot.y, rot.z, rot.w),
					motionType,
					Layers::MOVING
				);
				settings.mRestitution = physicscomponent.restitution;
				settings.mFriction = physicscomponent.friction;
				settings.mLinearDamping = physicscomponent.damping_linear;
				settings.mAngularDamping = physicscomponent.damping_angular;
				settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
				settings.mMassPropertiesOverride.mMass = physicscomponent.mass;
				settings.mAllowSleeping = !physicscomponent.IsDisableDeactivation();

				BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();

				physicsobject.body = body_interface.CreateBody(settings);
				if (physicsobject.body == nullptr)
				{
					physicscomponent.physicsobject = nullptr;
					return;
				}

				physicsobject.body->SetUserData((uint64_t)&physicsobject);

				body_interface.AddBody(physicsobject.body->GetID(), EActivation::Activate);

				if (isDynamic)
				{
					// We must detach dynamic objects, because their physics object is created in world space
					//	and attachment would apply double transformation to the transform
					scene.Component_Detach(entity);
				}
			}
		}
		void AddSoftBody(
			wi::scene::Scene& scene,
			Entity entity,
			wi::scene::SoftBodyPhysicsComponent& physicscomponent,
			const wi::scene::MeshComponent& mesh
		)
		{
			SoftBody& physicsobject = GetSoftBody(physicscomponent);
			physicsobject.physics_scene = scene.physics_scene;
			physicsobject.entity = entity;
			physicscomponent.CreateFromMesh(mesh);
			PhysicsScene& physics_scene = GetPhysicsScene(scene);

			physicsobject.shared_settings.SetEmbedded();

			XMMATRIX worldMatrix = XMLoadFloat4x4(&physicscomponent.worldMatrix);

			const size_t vertexCount = physicscomponent.physicsToGraphicsVertexMapping.size();
			physicsobject.shared_settings.mVertices.resize(vertexCount);
			for (size_t i = 0; i < vertexCount; ++i)
			{
				uint32_t graphicsInd = physicscomponent.physicsToGraphicsVertexMapping[i];

				XMFLOAT3 position = mesh.vertex_positions[graphicsInd];
				XMVECTOR P = XMLoadFloat3(&position);
				P = XMVector3Transform(P, worldMatrix);
				XMStoreFloat3(&position, P);
				physicsobject.shared_settings.mVertices[i].mPosition = Float3(position.x, position.y, position.z);
				physicsobject.shared_settings.mVertices[i].mInvMass = physicscomponent.weights[i];
				physicsobject.shared_settings.mVertices[i].mInvMass = 0.025f;
			}

			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			mesh.GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
				const uint32_t* indices = mesh.indices.data() + subset.indexOffset;
				for (uint32_t i = 0; i < subset.indexCount; i += 3)
				{
					SoftBodySharedSettings::Face& face = physicsobject.shared_settings.mFaces.emplace_back();
					face.mVertex[0] = physicscomponent.graphicsToPhysicsVertexMapping[indices[i + 0]];
					face.mVertex[1] = physicscomponent.graphicsToPhysicsVertexMapping[indices[i + 1]];
					face.mVertex[2] = physicscomponent.graphicsToPhysicsVertexMapping[indices[i + 2]];
				}
			}

			//physicsobject.shared_settings.CalculateBendConstraintConstants();
			//physicsobject.shared_settings.CalculateEdgeLengths();
			//physicsobject.shared_settings.CalculateLRALengths();
			//physicsobject.shared_settings.CalculateSkinnedConstraintNormals();
			//physicsobject.shared_settings.CalculateVolumeConstraintVolumes();
			physicsobject.shared_settings.Optimize();

			SoftBodyCreationSettings creation_settings(&physicsobject.shared_settings, Vec3::sZero(), Quat::sIdentity(), Layers::MOVING);
			creation_settings.mFriction = physicscomponent.friction;
			creation_settings.mRestitution = physicscomponent.restitution;

			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();

			physicsobject.body = body_interface.CreateSoftBody(creation_settings);

			if (physicsobject.body == nullptr)
			{
				physicscomponent.physicsobject = nullptr;
				return;
			}

			physicsobject.body->SetUserData((uint64_t)&physicsobject);

			body_interface.AddBody(physicsobject.body->GetID(), EActivation::Activate);
		}

	}
	using namespace jolt;

	void Initialize()
	{
		wi::Timer timer;

		RegisterDefaultAllocator();

		Factory::sInstance = new Factory();

		RegisterTypes();

		char text[256] = {};
		snprintf(text, arraysize(text), "wi::physics Initialized [Jolt] (%d ms)", (int)std::round(timer.elapsed()));
		wi::backlog::post(text);
	}

	bool IsEnabled() { return ENABLED; }
	void SetEnabled(bool value) { ENABLED = value; }

	bool IsSimulationEnabled() { return ENABLED && SIMULATION_ENABLED; }
	void SetSimulationEnabled(bool value) { SIMULATION_ENABLED = value; }

	bool IsDebugDrawEnabled() { return DEBUGDRAW_ENABLED; }
	void SetDebugDrawEnabled(bool value) { DEBUGDRAW_ENABLED = value; }

	int GetAccuracy() { return ACCURACY; }
	void SetAccuracy(int value) { ACCURACY = value; }

	float GetFrameRate() { return 1.0f / TIMESTEP; }
	void SetFrameRate(float value) { TIMESTEP = 1.0f / value; }

	void RunPhysicsUpdateSystem(
		wi::jobsystem::context& ctx,
		wi::scene::Scene& scene,
		float dt
	)
	{
		if (!IsEnabled() || dt <= 0)
			return;

		auto range = wi::profiler::BeginRangeCPU("Physics");

		PhysicsScene& physics_scene = GetPhysicsScene(scene);
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();

		// System will register rigidbodies to objects:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), 64, [&](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);

			if (physicscomponent.physicsobject == nullptr && scene.transforms.Contains(entity))
			{
				TransformComponent& transform = *scene.transforms.GetComponent(entity);
				const ObjectComponent* object = scene.objects.GetComponent(entity);
				const MeshComponent* mesh = nullptr;
				if (object != nullptr)
				{
					mesh = scene.meshes.GetComponent(object->meshID);
				}
				physicsLock.lock();
				AddRigidBody(scene, entity, physicscomponent, transform, mesh);
				physicsLock.unlock();
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				RigidBody& physicsobject = GetRigidBody(physicscomponent);
				Body* body = physicsobject.body;
				if (body == nullptr)
					return;

				body->SetFriction(physicscomponent.friction);
				body->SetRestitution(physicscomponent.restitution);
				if (physicscomponent.IsKinematic() && body->GetMotionType() != EMotionType::Kinematic)
				{
					// It became kinematic when it wasn't before:
					body->SetMotionType(EMotionType::Kinematic);
				}
				if (!physicscomponent.IsKinematic() && body->GetMotionType() == EMotionType::Kinematic)
				{
					// It became non-kinematic when it was kinematic before:
					body->SetMotionType(physicscomponent.mass == 0 ? EMotionType::Static : EMotionType::Dynamic);
				}

				if (physicscomponent.IsKinematic())
				{
					TransformComponent& transform = *scene.transforms.GetComponent(entity);
					XMFLOAT3 position = transform.GetPosition();
					XMFLOAT4 rotation = transform.GetRotation();

					body_interface.SetPositionAndRotation(
						body->GetID(),
						RVec3Arg(position.x, position.y, position.z),
						QuatArg(rotation.x, rotation.y, rotation.z, rotation.w),
						EActivation::Activate
					);
				}
			}
		});

		// System will register softbodies to meshes and update physics engine state:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.softbodies.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {

			SoftBodyPhysicsComponent& physicscomponent = scene.softbodies[args.jobIndex];
			Entity entity = scene.softbodies.GetEntity(args.jobIndex);
			if (!scene.meshes.Contains(entity))
				return;
			MeshComponent& mesh = *scene.meshes.GetComponent(entity);
			const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;
			mesh.SetDynamic(true);

			if (physicscomponent._flags & SoftBodyPhysicsComponent::FORCE_RESET)
			{
				physicscomponent._flags &= ~SoftBodyPhysicsComponent::FORCE_RESET;
				physicscomponent.physicsobject = nullptr;
			}
			if (physicscomponent._flags & SoftBodyPhysicsComponent::SAFE_TO_REGISTER && physicscomponent.physicsobject == nullptr)
			{
				physicsLock.lock();
				AddSoftBody(scene, entity, physicscomponent, mesh);
				physicsLock.unlock();
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				SoftBody& physicsobject = GetSoftBody(physicscomponent);

				physicsobject.body->SetFriction(physicscomponent.friction);
				physicsobject.body->SetRestitution(physicscomponent.restitution);

				// This is different from rigid bodies, because soft body is a per mesh component (no TransformComponent). World matrix is propagated down from single mesh instance (ObjectUpdateSystem).
				XMMATRIX worldMatrix = XMLoadFloat4x4(&physicscomponent.worldMatrix);

				// System controls zero weight soft body nodes:
				for (size_t ind = 0; ind < physicscomponent.weights.size(); ++ind)
				{
					float weight = physicscomponent.weights[ind];

					//if (weight == 0)
					//{
					//	btSoftBody::Node& node = softbody->m_nodes[(uint32_t)ind];
					//	uint32_t graphicsInd = physicscomponent.physicsToGraphicsVertexMapping[ind];
					//	XMFLOAT3 position = mesh.vertex_positions[graphicsInd];
					//	XMVECTOR P = armature == nullptr ? XMLoadFloat3(&position) : wi::scene::SkinVertex(mesh, *armature, graphicsInd);
					//	P = XMVector3Transform(P, worldMatrix);
					//	XMStoreFloat3(&position, P);
					//	node.m_x = btVector3(position.x, position.y, position.z);
					//}
				}
			}
		});

		wi::jobsystem::Wait(ctx);

		//physics_scene.physics_system.OptimizeBroadPhase();
		
		// Perform internal simulation step:
		if (IsSimulationEnabled())
		{
			static TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
			static JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
			const int steps = ::clamp(int(dt / TIMESTEP), 1, ACCURACY);
			physics_scene.physics_system.Update(dt, steps, &temp_allocator, &job_system);
		}

		// Feedback physics objects to system:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), 64, [&](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			if (physicscomponent.physicsobject == nullptr || physicscomponent.IsKinematic())
				return;

			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);
			TransformComponent& transform = *scene.transforms.GetComponent(entity);

			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			Body* body = physicsobject.body;
			if (body == nullptr)
				return;

			RMat44 world = body->GetWorldTransform();
			world = world * physicsobject.additionalTransformInverse;
			RVec3 position = world.GetTranslation();
			Quat rotation = world.GetQuaternion();

			transform.translation_local = XMFLOAT3(position.GetX(), position.GetY(), position.GetZ());
			transform.rotation_local = XMFLOAT4(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
			transform.SetDirty();
		});
		
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.softbodies.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {

			SoftBodyPhysicsComponent& physicscomponent = scene.softbodies[args.jobIndex];
			if (physicscomponent.physicsobject == nullptr)
				return;

			Entity entity = scene.softbodies.GetEntity(args.jobIndex);

			SoftBody& physicsobject = GetSoftBody(physicscomponent);
			Body* body = physicsobject.body;
			if (body == nullptr)
				return;

			MeshComponent& mesh = *scene.meshes.GetComponent(entity);

			physicscomponent.aabb = wi::primitive::AABB();

			const SoftBodyMotionProperties* motion = (const SoftBodyMotionProperties*)physicsobject.body->GetMotionProperties();

			// Soft body simulation nodes will update graphics mesh:
			for (size_t ind = 0; ind < mesh.vertex_positions.size(); ++ind)
			{
				uint32_t physicsInd = physicscomponent.graphicsToPhysicsVertexMapping[ind];

				Vec3 position = motion->GetVertex(physicsInd).mPosition;
				XMFLOAT3 pos = XMFLOAT3(position.GetX(), position.GetY(), position.GetZ());

				physicscomponent.vertex_positions_simulation[ind].FromFULL(pos);
				//physicscomponent.vertex_normals_simulation[ind].FromFULL(XMFLOAT3(-node.m_n.getX(), -node.m_n.getY(), -node.m_n.getZ()));

				physicscomponent.aabb._min = wi::math::Min(physicscomponent.aabb._min, pos);
				physicscomponent.aabb._max = wi::math::Max(physicscomponent.aabb._max, pos);
			}

			// Update tangent vectors:
			if (!mesh.vertex_uvset_0.empty() && !physicscomponent.vertex_normals_simulation.empty())
			{
				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh.GetLODSubsetRange(0, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
					for (size_t i = 0; i < subset.indexCount; i += 3)
					{
						const uint32_t i0 = mesh.indices[i + 0];
						const uint32_t i1 = mesh.indices[i + 1];
						const uint32_t i2 = mesh.indices[i + 2];

						const XMFLOAT3 v0 = physicscomponent.vertex_positions_simulation[i0].GetPOS();
						const XMFLOAT3 v1 = physicscomponent.vertex_positions_simulation[i1].GetPOS();
						const XMFLOAT3 v2 = physicscomponent.vertex_positions_simulation[i2].GetPOS();

						const XMFLOAT2 u0 = mesh.vertex_uvset_0[i0];
						const XMFLOAT2 u1 = mesh.vertex_uvset_0[i1];
						const XMFLOAT2 u2 = mesh.vertex_uvset_0[i2];

						const XMVECTOR nor0 = physicscomponent.vertex_normals_simulation[i0].LoadNOR();
						const XMVECTOR nor1 = physicscomponent.vertex_normals_simulation[i1].LoadNOR();
						const XMVECTOR nor2 = physicscomponent.vertex_normals_simulation[i2].LoadNOR();

						const XMVECTOR facenormal = XMVector3Normalize(XMVectorAdd(XMVectorAdd(nor0, nor1), nor2));

						const float x1 = v1.x - v0.x;
						const float x2 = v2.x - v0.x;
						const float y1 = v1.y - v0.y;
						const float y2 = v2.y - v0.y;
						const float z1 = v1.z - v0.z;
						const float z2 = v2.z - v0.z;

						const float s1 = u1.x - u0.x;
						const float s2 = u2.x - u0.x;
						const float t1 = u1.y - u0.y;
						const float t2 = u2.y - u0.y;

						const float r = 1.0f / (s1 * t2 - s2 * t1);
						const XMVECTOR sdir = XMVectorSet((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
							(t2 * z1 - t1 * z2) * r, 0);
						const XMVECTOR tdir = XMVectorSet((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
							(s1 * z2 - s2 * z1) * r, 0);

						XMVECTOR tangent;
						tangent = XMVector3Normalize(XMVectorSubtract(sdir, XMVectorMultiply(facenormal, XMVector3Dot(facenormal, sdir))));
						float sign = XMVectorGetX(XMVector3Dot(XMVector3Cross(tangent, facenormal), tdir)) < 0.0f ? -1.0f : 1.0f;

						XMFLOAT3 t;
						XMStoreFloat3(&t, tangent);

						physicscomponent.vertex_tangents_tmp[i0].x += t.x;
						physicscomponent.vertex_tangents_tmp[i0].y += t.y;
						physicscomponent.vertex_tangents_tmp[i0].z += t.z;
						physicscomponent.vertex_tangents_tmp[i0].w = sign;

						physicscomponent.vertex_tangents_tmp[i1].x += t.x;
						physicscomponent.vertex_tangents_tmp[i1].y += t.y;
						physicscomponent.vertex_tangents_tmp[i1].z += t.z;
						physicscomponent.vertex_tangents_tmp[i1].w = sign;

						physicscomponent.vertex_tangents_tmp[i2].x += t.x;
						physicscomponent.vertex_tangents_tmp[i2].y += t.y;
						physicscomponent.vertex_tangents_tmp[i2].z += t.z;
						physicscomponent.vertex_tangents_tmp[i2].w = sign;
					}
				}

				for (size_t i = 0; i < physicscomponent.vertex_tangents_simulation.size(); ++i)
				{
					physicscomponent.vertex_tangents_simulation[i].FromFULL(physicscomponent.vertex_tangents_tmp[i]);
				}
			}

			mesh.aabb = physicscomponent.aabb;
		});

#ifdef JPH_DEBUG_RENDERER
		if (IsDebugDrawEnabled())
		{
			class JoltDebugRenderer : public DebugRendererSimple
			{
				void DrawLine(RVec3Arg inFrom, RVec3Arg inTo, ColorArg inColor) override
				{
					wi::renderer::RenderableLine line;
					line.start = XMFLOAT3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
					line.end = XMFLOAT3(inTo.GetX(), inTo.GetY(), inTo.GetZ());
					line.color_start = line.color_end = wi::Color(inColor.r, inColor.g, inColor.b, inColor.a);
					wi::renderer::DrawLine(line);
				}
				void DrawTriangle(RVec3Arg inV1, RVec3Arg inV2, RVec3Arg inV3, ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override
				{
					// Not needed if we only want to draw wireframes
				}
				void DrawText3D(RVec3Arg inPosition, const string_view& inString, ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f) override
				{
					wi::renderer::DebugTextParams params;
					params.position.x = inPosition.GetX();
					params.position.y = inPosition.GetY();
					params.position.z = inPosition.GetZ();
					params.scaling = 0.6f;
					params.flags |= wi::renderer::DebugTextParams::CAMERA_FACING;
					params.flags |= wi::renderer::DebugTextParams::CAMERA_SCALING;
					wi::renderer::DrawDebugText(inString.data(), params);
				}
			};
			static JoltDebugRenderer debug_renderer;
			BodyManager::DrawSettings settings;
			settings.mDrawCenterOfMassTransform = true;
			settings.mDrawShape = true;
			settings.mDrawShapeWireframe = true;
			settings.mDrawShapeColor = BodyManager::EShapeColor::ShapeTypeColor;
			physics_scene.physics_system.DrawBodies(settings, &debug_renderer);

			for(size_t i=0;i<scene.softbodies.GetCount();++i)
			{
				SoftBodyPhysicsComponent& physicscomponent = scene.softbodies[i];
				if (physicscomponent.physicsobject == nullptr)
					return;

				SoftBody& physicsobject = GetSoftBody(physicscomponent);
				Body* body = physicsobject.body;
				if (body == nullptr)
					return;

				const SoftBodyMotionProperties* motion = (const SoftBodyMotionProperties*)physicsobject.body->GetMotionProperties();
				motion->DrawVertices(&debug_renderer, Mat44::sIdentity());
			}
		}
#endif // JPH_DEBUG_RENDERER

		wi::profiler::EndRange(range); // Physics
	}

	void SetLinearVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).body->SetLinearVelocity(Vec3Arg(velocity.x, velocity.y, velocity.z));
		}
	}
	void SetAngularVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).body->SetAngularVelocity(Vec3Arg(velocity.x, velocity.y, velocity.z));
		}
	}

	void ApplyForce(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).body->AddForce(Vec3Arg(force.x, force.y, force.z));
		}
	}
	void ApplyForceAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force,
		const XMFLOAT3& at
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).body->AddForce(Vec3Arg(force.x, force.y, force.z), Vec3Arg(at.x, at.y, at.z));
		}
	}

	void ApplyImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).body->AddImpulse(Vec3Arg(impulse.x, impulse.y, impulse.z));
		}
	}
	void ApplyImpulse(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse
	)
	{

	}
	void ApplyImpulseAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			Vec3 at_world = physicsobject.body->GetCenterOfMassTransform().Inversed() * Vec3(at.x, at.y, at.z);
			physicsobject.body->AddImpulse(Vec3Arg(impulse.x, impulse.y, impulse.z), at_world);
		}
	}
	void ApplyImpulseAt(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at
	)
	{

	}

	void ApplyTorque(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).body->AddTorque(Vec3Arg(torque.x, torque.y, torque.z));
		}
	}
	void ApplyTorqueImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			GetRigidBody(physicscomponent).body->AddTorque(Vec3Arg(torque.x, torque.y, torque.z));
		}
	}

	void SetActivationState(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		ActivationState state
	)
	{
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		switch (state)
		{
		case wi::physics::ActivationState::Active:
			body_interface.ActivateBody(physicsobject.body->GetID());
			break;
		case wi::physics::ActivationState::Inactive:
			body_interface.DeactivateBody(physicsobject.body->GetID());
			break;
		default:
			break;
		}
	}
	void SetActivationState(
		wi::scene::SoftBodyPhysicsComponent& physicscomponent,
		ActivationState state
	)
	{
		SoftBody& physicsobject = GetSoftBody(physicscomponent);
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		switch (state)
		{
		case wi::physics::ActivationState::Active:
			body_interface.ActivateBody(physicsobject.body->GetID());
			break;
		case wi::physics::ActivationState::Inactive:
			body_interface.DeactivateBody(physicsobject.body->GetID());
			break;
		default:
			break;
		}
	}

	RayIntersectionResult Intersects(
		const wi::scene::Scene& scene,
		wi::primitive::Ray ray
	)
	{
		RayIntersectionResult result;
		if (scene.physics_scene == nullptr)
			return result;

		PhysicsScene& physics_scene = *(PhysicsScene*)scene.physics_scene.get();

		const float tmin = wi::math::Clamp(ray.TMin, 0, 1000000);
		const float tmax = wi::math::Clamp(ray.TMax, 0, 1000000);
		const float range = tmax - tmin;

		RRayCast inray{
			Vec3(ray.origin.x + ray.direction.x * tmin, ray.origin.y + ray.direction.y * tmin, ray.origin.z + ray.direction.z * tmin),
			Vec3(ray.direction.x * range, ray.direction.y * range, ray.direction.z * range)
		};

		RayCastSettings settings;
		settings.mBackFaceMode = EBackFaceMode::CollideWithBackFaces;
		settings.mTreatConvexAsSolid = true;

		ClosestHitCollisionCollector<CastRayCollector> collector;

		physics_scene.physics_system.GetNarrowPhaseQuery().CastRay(inray, settings, collector);
		if (!collector.HadHit())
			return result;

		if (collector.mHit.mBodyID.IsInvalid())
			return result;

		BodyLockRead lock(physics_scene.physics_system.GetBodyLockInterfaceNoLock(), collector.mHit.mBodyID);
		if (!lock.Succeeded())
			return result;

		const Body& body = lock.GetBody();
		const uint64_t userdata = body.GetUserData();
		const RigidBody* physicsobject = (RigidBody*)userdata;

		const Vec3 position = inray.GetPointOnRay(collector.mHit.mFraction);
		const Vec3 position_local = body.GetCenterOfMassTransform().Inversed() * position;
		const Vec3 normal = body.GetWorldSpaceSurfaceNormal(collector.mHit.mSubShapeID2, position);

		result.entity = physicsobject->entity;
		result.position = XMFLOAT3(position.GetX(), position.GetY(), position.GetZ());
		result.position_local = XMFLOAT3(position_local.GetX(), position_local.GetY(), position_local.GetZ());
		result.normal = XMFLOAT3(normal.GetX(), normal.GetY(), normal.GetZ());
		result.physicsobject = &body;

		return result;
	}

	struct PickDragOperation_Jolt
	{
		std::shared_ptr<void> physics_scene;
		Ref<TwoBodyConstraint> constraint;
		float pick_distance = 0;
		Body* bodyA = nullptr;
		Body* bodyB = nullptr;
		~PickDragOperation_Jolt()
		{
			if (physics_scene == nullptr)
				return;
			PhysicsScene& physics_scene = *((PhysicsScene*)this->physics_scene.get());
			physics_scene.physics_system.RemoveConstraint(constraint);
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			body_interface.RemoveBody(bodyA->GetID());
			body_interface.DestroyBody(bodyA->GetID());
		}
	};
	void PickDrag(
		const wi::scene::Scene& scene,
		wi::primitive::Ray ray,
		PickDragOperation& op
	)
	{
		if (scene.physics_scene == nullptr)
			return;
		PhysicsScene& physics_scene = *((PhysicsScene*)scene.physics_scene.get());
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();

		if (op.IsValid())
		{
			// Continue dragging:
			PickDragOperation_Jolt* internal_state = (PickDragOperation_Jolt*)op.internal_state.get();
			const float dist = internal_state->pick_distance;
			Vec3 pos = Vec3(ray.origin.x + ray.direction.x * dist, ray.origin.y + ray.direction.y * dist, ray.origin.z + ray.direction.z * dist);
			body_interface.MoveKinematic(internal_state->bodyA->GetID(), pos, Quat::sIdentity(), scene.dt);
		}
		else
		{
			// Begin picking:
			RayIntersectionResult result = Intersects(scene, ray);
			if (!result.IsValid())
				return;
			Body* body = (Body*)result.physicsobject;
			if (!body->IsRigidBody())
				return;

			auto internal_state = std::make_shared<PickDragOperation_Jolt>();
			internal_state->physics_scene = scene.physics_scene;
			internal_state->pick_distance = wi::math::Distance(ray.origin, result.position);
			internal_state->bodyB = body;

			Vec3 pos = Vec3(result.position.x, result.position.y, result.position.z);

			internal_state->bodyA = body_interface.CreateBody(BodyCreationSettings(new SphereShape(0.01f), pos, Quat::sIdentity(), EMotionType::Kinematic, Layers::MOVING));
			body_interface.AddBody(internal_state->bodyA->GetID(), EActivation::Activate);

#if 0
			DistanceConstraintSettings settings;
			settings.mPoint1 = settings.mPoint2 = pos;
#else
			SixDOFConstraintSettings settings;
			settings.mPosition1 = settings.mPosition2 = pos;
			for (int i = 0; i < SixDOFConstraintSettings::EAxis::Num; ++i)
			{
				settings.mLimitMin[i] = 0;
				settings.mLimitMax[i] = 0;
			}
#endif

			internal_state->constraint = settings.Create(*internal_state->bodyA, *internal_state->bodyB);
			physics_scene.physics_system.AddConstraint(internal_state->constraint);

			op.internal_state = internal_state;
		}
	}

}

#endif // PHYSICSENGINE
