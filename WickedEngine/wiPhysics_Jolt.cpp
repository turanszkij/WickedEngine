#include "wiPhysics.h"

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
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <Jolt/Physics/SoftBody/SoftBodyShape.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif // JPH_DEBUG_RENDERER

#include <thread>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

using namespace wi::ecs;
using namespace wi::scene;

namespace wi::physics
{
	inline XMMATRIX GetOrientation(XMVECTOR P0, XMVECTOR P1, XMVECTOR P2)
	{
		XMVECTOR T = P2 - P1;
		XMVECTOR B = P1 - P0;
		XMVECTOR N = XMVector3Cross(B, T);
		T = XMVector3Cross(B, N);
		T = XMVector3Normalize(T);
		B = XMVector3Normalize(B);
		N = XMVector3Normalize(N);
		return XMMATRIX(T, B, N, XMVectorSetW(P0, 1));
	}

	namespace jolt
	{
		bool ENABLED = true;
		bool SIMULATION_ENABLED = true;
		bool DEBUGDRAW_ENABLED = false;
		int ACCURACY = 4;
		int softbodyIterationCount = 6;
		float TIMESTEP = 1.0f / 60.0f;
		bool INTERPOLATION = true;

		const uint cMaxBodies = 65536;
		const uint cNumBodyMutexes = 0;
		const uint cMaxBodyPairs = 65536;
		const uint cMaxContactConstraints = 65536;
		const EMotionQuality cMotionQuality = EMotionQuality::LinearCast;

		inline Vec3 cast(const Float3& v) { return Vec3(v.x, v.y, v.z); }
		inline Vec3 cast(const XMFLOAT3& v) { return Vec3(v.x, v.y, v.z); }
		inline Quat cast(const XMFLOAT4& v) { return Quat(v.x, v.y, v.z, v.w); }
		inline Mat44 cast(const XMFLOAT4X4& v)
		{
			return Mat44(
				Vec4(v._11, v._12, v._13, v._14),
				Vec4(v._21, v._22, v._23, v._24),
				Vec4(v._31, v._32, v._33, v._34),
				Vec4(v._41, v._42, v._43, v._44)
			);
		}
		inline XMFLOAT3 cast(Vec3Arg v) { return XMFLOAT3(v.GetX(), v.GetY(), v.GetZ()); }
		inline XMFLOAT4 cast(QuatArg v) { return XMFLOAT4(v.GetX(), v.GetY(), v.GetZ(), v.GetW()); }
		inline XMFLOAT4X4 cast(Mat44 v)
		{
			XMFLOAT4X4 ret;
			v.StoreFloat4x4((Float4*)&ret);
			return ret;
		}

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
			float accumulator = 0;
			float alpha = 0;
			float GetKinematicDT(float dt) const
			{
				return clamp(accumulator + dt, 0.0f, TIMESTEP * ACCURACY);
			}
		};
		PhysicsScene& GetPhysicsScene(Scene& scene)
		{
			if (scene.physics_scene == nullptr)
			{
				auto physics_scene = std::make_shared<PhysicsScene>();

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
			BodyID bodyID;
			Entity entity = INVALID_ENTITY;

			// Interpolation state:
			Vec3 prev_position = Vec3::sZero();
			Quat prev_rotation = Quat::sIdentity();

			// Local body offset:
			Mat44 additionalTransform = Mat44::sIdentity();
			Mat44 additionalTransformInverse = Mat44::sIdentity();

			// This is to fixup ragdolls that are applied to skeletons from different kinds of model imports
			Mat44 restBasis = Mat44::sIdentity();
			Mat44 restBasisInverse = Mat44::sIdentity();

			// for trace hit reporting:
			wi::ecs::Entity humanoid_ragdoll_entity = wi::ecs::INVALID_ENTITY;
			wi::scene::HumanoidComponent::HumanoidBone humanoid_bone = wi::scene::HumanoidComponent::HumanoidBone::Count;

			~RigidBody()
			{
				if (physics_scene == nullptr || bodyID.IsInvalid())
					return;
				BodyInterface& body_interface = ((PhysicsScene*)physics_scene.get())->physics_system.GetBodyInterface(); // locking version because destructor can be called from any thread
				body_interface.RemoveBody(bodyID);
				body_interface.DestroyBody(bodyID);
			}
		};
		struct SoftBody
		{
			std::shared_ptr<void> physics_scene;
			BodyID bodyID;
			Entity entity = INVALID_ENTITY;

			SoftBodySharedSettings shared_settings;
			wi::vector<XMFLOAT4X4> inverseBindMatrices;
			struct Neighbors
			{
				uint32_t left = 0;
				uint32_t right = 0;
				constexpr void set(uint32_t l, uint32_t r)
				{
					left = l;
					right = r;
				}
			};
			wi::vector<Neighbors> physicsNeighbors;

			~SoftBody()
			{
				if (physics_scene == nullptr || bodyID.IsInvalid())
					return;
				BodyInterface& body_interface = ((PhysicsScene*)physics_scene.get())->physics_system.GetBodyInterface(); // locking version because destructor can be called from any thread
				body_interface.RemoveBody(bodyID);
				body_interface.DestroyBody(bodyID);
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

			// The default convex radius caused issues when creating small box shape, etc, so I decrease it:
			const float convexRadius = 0.001f;

			switch (physicscomponent.shape)
			{
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
			{
				BoxShapeSettings settings(Vec3(physicscomponent.box.halfextents.x * transform.scale_local.x, physicscomponent.box.halfextents.y * transform.scale_local.y, physicscomponent.box.halfextents.z * transform.scale_local.z), convexRadius);
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
				CylinderShapeSettings settings(physicscomponent.capsule.height * transform.scale_local.y, physicscomponent.capsule.radius * transform.scale_local.x, convexRadius);
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
				ConvexHullShapeSettings settings(points, convexRadius);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			else
			{
				wi::backlog::post("AddRigidBody failed: convex hull physics requested, but no MeshComponent provided!", wi::backlog::LogLevel::Error);
				return;
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
						triangle.mV[2] = Float3(mesh->vertex_positions[indices[i + 1]].x * transform.scale_local.x, mesh->vertex_positions[indices[i + 1]].y * transform.scale_local.y, mesh->vertex_positions[indices[i + 1]].z * transform.scale_local.z);
						triangle.mV[1] = Float3(mesh->vertex_positions[indices[i + 2]].x * transform.scale_local.x, mesh->vertex_positions[indices[i + 2]].y * transform.scale_local.y, mesh->vertex_positions[indices[i + 2]].z * transform.scale_local.z);
						trianglelist.push_back(triangle);
					}
				}

				MeshShapeSettings settings(trianglelist);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			else
			{
				wi::backlog::post("AddRigidBody failed: triangle mesh physics requested, but no MeshComponent provided!", wi::backlog::LogLevel::Error);
				return;
			}
			break;
			}

			if (!shape_result.IsValid())
			{
				char text[1024] = {};
				snprintf(text, arraysize(text), "AddRigidBody failed, shape_result: %s", shape_result.GetError().c_str());
				wi::backlog::post(text, wi::backlog::LogLevel::Error);
				return;
			}
			else
			{
				RigidBody& physicsobject = GetRigidBody(physicscomponent);
				physicsobject.physics_scene = scene.physics_scene;
				physicsobject.entity = entity;
				PhysicsScene& physics_scene = GetPhysicsScene(scene);

				physicsobject.shape = shape_result.Get();

				Mat44 mat = cast(transform.world);
				Vec3 local_offset = cast(physicscomponent.local_offset);

				physicsobject.additionalTransform.SetTranslation(local_offset);
				physicsobject.additionalTransformInverse = physicsobject.additionalTransform.Inversed();

				physicsobject.prev_position = mat.GetTranslation();
				physicsobject.prev_rotation = mat.GetQuaternion().Normalized();

				const EMotionType motionType = physicscomponent.mass == 0 ? EMotionType::Static : (physicscomponent.IsKinematic() ? EMotionType::Kinematic : EMotionType::Dynamic);

				BodyCreationSettings settings(
					physicsobject.shape.GetPtr(),
					local_offset + physicsobject.prev_position,
					physicsobject.prev_rotation,
					motionType,
					Layers::MOVING
				);
				settings.mRestitution = physicscomponent.restitution;
				settings.mFriction = physicscomponent.friction;
				settings.mLinearDamping = physicscomponent.damping_linear;
				settings.mAngularDamping = physicscomponent.damping_angular;
				settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
				if (motionType == EMotionType::Dynamic)
				{
					settings.mMassPropertiesOverride.mMass = physicscomponent.mass;
				}
				else
				{
					settings.mMassPropertiesOverride.mMass = 1;
				}
				settings.mAllowSleeping = !physicscomponent.IsDisableDeactivation();
				settings.mMotionQuality = cMotionQuality;
				settings.mUserData = (uint64_t)&physicsobject;

				BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking version because this is called from job system!

				const EActivation activation = physicscomponent.IsStartDeactivated() ? EActivation::DontActivate : EActivation::Activate;

				physicsobject.bodyID = body_interface.CreateAndAddBody(settings, activation);
				if (physicsobject.bodyID.IsInvalid())
				{
					wi::backlog::post("AddRigidBody failed: body couldn't be created! This could mean that there are too many physics objects.", wi::backlog::LogLevel::Error);
					return;
				}

				if (motionType == EMotionType::Dynamic)
				{
					// We must detach dynamic objects, because their physics object is created in world space
					//	and attachment would apply double transformation to the transform
					scene.locker.lock();
					scene.Component_Detach(entity);
					scene.locker.unlock();
				}
			}
		}
		void AddSoftBody(
			wi::scene::Scene& scene,
			Entity entity,
			wi::scene::SoftBodyPhysicsComponent& physicscomponent,
			wi::scene::MeshComponent& mesh
		)
		{
			SoftBody& physicsobject = GetSoftBody(physicscomponent);
			physicsobject.physics_scene = scene.physics_scene;
			physicsobject.entity = entity;
			PhysicsScene& physics_scene = GetPhysicsScene(scene);

			physicsobject.shared_settings.SetEmbedded();
			physicsobject.shared_settings.mVertexRadius = physicscomponent.vertex_radius;

			physicscomponent.CreateFromMesh(mesh);
			if (physicscomponent.physicsIndices.empty())
			{
				wi::backlog::post("AddSoftBody failed: physics faces are empty, this means generating physics mesh has failed, try to change settings.", wi::backlog::LogLevel::Error);
				return;
			}
			const size_t vertexCount = physicscomponent.physicsToGraphicsVertexMapping.size();

			physicsobject.physicsNeighbors.resize(vertexCount);

			for (size_t i = 0; i < physicscomponent.physicsIndices.size(); i += 3)
			{
				const uint32_t physicsInd0 = physicscomponent.physicsIndices[i + 0];
				const uint32_t physicsInd1 = physicscomponent.physicsIndices[i + 1];
				const uint32_t physicsInd2 = physicscomponent.physicsIndices[i + 2];

				SoftBodySharedSettings::Face& face = physicsobject.shared_settings.mFaces.emplace_back();
				face.mVertex[0] = physicsInd0;
				face.mVertex[2] = physicsInd1;
				face.mVertex[1] = physicsInd2;

				physicsobject.physicsNeighbors[physicsInd0].set(physicsInd2, physicsInd1);
				physicsobject.physicsNeighbors[physicsInd1].set(physicsInd0, physicsInd2);
				physicsobject.physicsNeighbors[physicsInd2].set(physicsInd1, physicsInd0);
			}

			const XMMATRIX worldMatrix = XMLoadFloat4x4(&physicscomponent.worldMatrix);

			physicsobject.shared_settings.mVertices.resize(vertexCount);
			physicsobject.inverseBindMatrices.resize(vertexCount);
			physicscomponent.boneData.resize(vertexCount);
			const float distributed_mass = physicscomponent.mass / vertexCount;
			for (size_t i = 0; i < vertexCount; ++i)
			{
				uint32_t graphicsInd = physicscomponent.physicsToGraphicsVertexMapping[i];

				XMFLOAT3 position = mesh.vertex_positions[graphicsInd];
				XMVECTOR P = XMLoadFloat3(&position);
				P = XMVector3Transform(P, worldMatrix);
				XMStoreFloat3(&position, P);
				physicsobject.shared_settings.mVertices[i].mPosition = Float3(position.x, position.y, position.z);

				float weight = physicscomponent.weights[graphicsInd] * distributed_mass;
				physicsobject.shared_settings.mVertices[i].mInvMass = weight == 0 ? 0 : 1.0f / (1.0f + weight);

				// The soft body node will have a bind matrix similar to an armature bone:
				XMVECTOR P0 = XMLoadFloat3(&mesh.vertex_positions[graphicsInd]);
				XMVECTOR P1 = XMLoadFloat3(&mesh.vertex_positions[physicscomponent.physicsToGraphicsVertexMapping[physicsobject.physicsNeighbors[i].left]]);
				XMVECTOR P2 = XMLoadFloat3(&mesh.vertex_positions[physicscomponent.physicsToGraphicsVertexMapping[physicsobject.physicsNeighbors[i].right]]);
				XMMATRIX B = GetOrientation(P0, P1, P2);
				B = XMMatrixInverse(nullptr, B);
				XMStoreFloat4x4(&physicsobject.inverseBindMatrices[i], B);
			}

			SoftBodySharedSettings::VertexAttributes vertexAttributes = { 1.0e-5f, 1.0e-5f, 1.0e-5f };
			physicsobject.shared_settings.CreateConstraints(&vertexAttributes, 1);
			physicsobject.shared_settings.Optimize();

			SoftBodyCreationSettings settings(&physicsobject.shared_settings, Vec3::sZero(), Quat::sIdentity(), Layers::MOVING);
			settings.mNumIterations = (uint32)softbodyIterationCount;
			settings.mFriction = physicscomponent.friction;
			settings.mRestitution = physicscomponent.restitution;
			settings.mPressure = physicscomponent.pressure;
			settings.mUpdatePosition = false;
			settings.mAllowSleeping = !physicscomponent.IsDisableDeactivation();
			settings.mUserData = (uint64_t)&physicsobject;

			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking version because this is called from job system!

			physicsobject.bodyID = body_interface.CreateAndAddSoftBody(settings, EActivation::Activate);

			if (physicsobject.bodyID.IsInvalid())
			{
				wi::backlog::post("AddSoftBody failed: body couldn't be created! This could mean that there are too many physics objects.", wi::backlog::LogLevel::Error);
				return;
			}
		}

		struct Ragdoll
		{
			enum BODYPART
			{
				BODYPART_PELVIS = 0,
				BODYPART_SPINE,
				BODYPART_HEAD,

				BODYPART_LEFT_UPPER_LEG,
				BODYPART_LEFT_LOWER_LEG,

				BODYPART_RIGHT_UPPER_LEG,
				BODYPART_RIGHT_LOWER_LEG,

				BODYPART_LEFT_UPPER_ARM,
				BODYPART_LEFT_LOWER_ARM,

				BODYPART_RIGHT_UPPER_ARM,
				BODYPART_RIGHT_LOWER_ARM,

				BODYPART_COUNT
			};

			std::shared_ptr<void> physics_scene;
			RigidBody rigidbodies[BODYPART_COUNT];
			Entity saved_parents[BODYPART_COUNT] = {};
			Skeleton skeleton;
			RagdollSettings settings;
			Ref<JPH::Ragdoll> ragdoll;
			bool state_active = false;
			float scale = 1;

			Ragdoll(Scene& scene, HumanoidComponent& humanoid, Entity humanoidEntity, float scale)
			{
				physics_scene = scene.physics_scene;
				PhysicsSystem& physics_system = ((PhysicsScene*)physics_scene.get())->physics_system;
				BodyInterface& body_interface = physics_system.GetBodyInterface(); // locking version because this is called from job system!

				float masses[BODYPART_COUNT] = {};
				Vec3 positions[BODYPART_COUNT] = {};
				Vec3 constraint_positions[BODYPART_COUNT] = {};
				Mat44 final_transforms[BODYPART_COUNT] = {};
#if 0
				// slow speed and visualizer to aid debugging:
				wi::renderer::SetGameSpeed(0.1f);
				SetDebugDrawEnabled(true);
#endif

				// Detect which way humanoid is facing in rest pose:
				const float facing = scene.GetHumanoidDefaultFacing(humanoid, humanoidEntity);

				// Whole ragdoll will take a uniform scaling:
				const XMMATRIX scaleMatrix = XMMatrixScaling(scale, scale, scale);
				this->scale = scale;

				// Calculate the bone lengths and radiuses in armature local space and create rigid bodies for bones:
				for (int c = 0; c < BODYPART_COUNT; ++c)
				{
					HumanoidComponent::HumanoidBone humanoid_bone = HumanoidComponent::HumanoidBone::Count;
					Entity entityA = INVALID_ENTITY;
					Entity entityB = INVALID_ENTITY;
					switch (c)
					{
					case BODYPART_PELVIS:
						humanoid_bone = HumanoidComponent::HumanoidBone::Hips;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Hips];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Spine];
						break;
					case BODYPART_SPINE:
						humanoid_bone = HumanoidComponent::HumanoidBone::Spine;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Spine];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Neck]; // prefer neck instead of head
						if (entityB == INVALID_ENTITY)
						{
							entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Head]; // fall back to head if neck not available
						}
						break;
					case BODYPART_HEAD:
						humanoid_bone = HumanoidComponent::HumanoidBone::Neck;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Neck]; // prefer neck instead of head
						if (entityA == INVALID_ENTITY)
						{
							humanoid_bone = HumanoidComponent::HumanoidBone::Head;
							entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Head]; // fall back to head if neck not available
						}
						break;
					case BODYPART_LEFT_UPPER_LEG:
						humanoid_bone = HumanoidComponent::HumanoidBone::LeftUpperLeg;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperLeg];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerLeg];
						break;
					case BODYPART_LEFT_LOWER_LEG:
						humanoid_bone = HumanoidComponent::HumanoidBone::LeftLowerLeg;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerLeg];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftFoot];
						break;
					case BODYPART_RIGHT_UPPER_LEG:
						humanoid_bone = HumanoidComponent::HumanoidBone::RightUpperLeg;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperLeg];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerLeg];
						break;
					case BODYPART_RIGHT_LOWER_LEG:
						humanoid_bone = HumanoidComponent::HumanoidBone::RightLowerLeg;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerLeg];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightFoot];
						break;
					case BODYPART_LEFT_UPPER_ARM:
						humanoid_bone = HumanoidComponent::HumanoidBone::LeftUpperArm;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperArm];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerArm];
						break;
					case BODYPART_LEFT_LOWER_ARM:
						humanoid_bone = HumanoidComponent::HumanoidBone::LeftLowerArm;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerArm];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftHand];
						break;
					case BODYPART_RIGHT_UPPER_ARM:
						humanoid_bone = HumanoidComponent::HumanoidBone::RightUpperArm;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperArm];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerArm];
						break;
					case BODYPART_RIGHT_LOWER_ARM:
						humanoid_bone = HumanoidComponent::HumanoidBone::RightLowerArm;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerArm];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightHand];
						break;
					}
					assert(entityA != INVALID_ENTITY);

					// Calculations here will be done in armature local space.
					//	Unfortunately since humanoid can be separate from armature, we use a "find" utility to find bone rest matrix in armature
					//	Note that current scaling of character is applied here separately from rest pose
					XMMATRIX restA = scene.GetRestPose(entityA) * scaleMatrix;
					XMMATRIX restB = scene.GetRestPose(entityB) * scaleMatrix;
					XMVECTOR rootA = restA.r[3];
					XMVECTOR rootB = restB.r[3];

					// Every bone will be a rigid body:
					RigidBody& physicsobject = rigidbodies[c];
					physicsobject.entity = entityA;

					float mass = scale;
					float capsule_height = scale;
					float capsule_radius = scale * humanoid.ragdoll_fatness;

					if (c == BODYPART_HEAD)
					{
						// Head doesn't necessarily have a child, so make up something reasonable:
						capsule_height = 0.05f * scale;
						capsule_radius = 0.1f * scale * humanoid.ragdoll_headsize;
					}
					else
					{
						// bone length:
						XMVECTOR len = XMVector3Length(XMVectorSubtract(rootB, rootA));
						capsule_height = XMVectorGetX(len);

						// capsule radius and length is tweaked per body part:
						switch (c)
						{
						case BODYPART_PELVIS:
							capsule_radius = 0.1f * scale * humanoid.ragdoll_fatness;
							break;
						case BODYPART_SPINE:
							capsule_radius = 0.1f * scale * humanoid.ragdoll_fatness;
							capsule_height -= capsule_radius * 2;
							break;
						case BODYPART_LEFT_LOWER_ARM:
						case BODYPART_RIGHT_LOWER_ARM:
							capsule_radius = capsule_height * 0.15f * humanoid.ragdoll_fatness;
							capsule_height += capsule_radius;
							break;
						case BODYPART_LEFT_UPPER_LEG:
						case BODYPART_RIGHT_UPPER_LEG:
							capsule_radius = capsule_height * 0.15f * humanoid.ragdoll_fatness;
							capsule_height -= capsule_radius * 2;
							break;
						case BODYPART_LEFT_LOWER_LEG:
						case BODYPART_RIGHT_LOWER_LEG:
							capsule_radius = capsule_height * 0.15f * humanoid.ragdoll_fatness;
							capsule_height -= capsule_radius;
							break;
						default:
							capsule_radius = capsule_height * 0.2f * humanoid.ragdoll_fatness;
							capsule_height -= capsule_radius * 2;
							break;
						}
					}

					ShapeSettings::ShapeResult shape_result;
					CapsuleShapeSettings shape_settings(capsule_height * 0.5f, capsule_radius);
					shape_settings.SetEmbedded();
					shape_result = shape_settings.Create();

					RotatedTranslatedShapeSettings rtshape_settings;
					rtshape_settings.SetEmbedded();
					rtshape_settings.mInnerShapePtr = shape_result.Get();
					rtshape_settings.mPosition = Vec3::sZero();
					rtshape_settings.mRotation = Quat::sIdentity();

					switch (c)
					{
					case BODYPART_LEFT_UPPER_ARM:
					case BODYPART_LEFT_LOWER_ARM:
					case BODYPART_RIGHT_UPPER_ARM:
					case BODYPART_RIGHT_LOWER_ARM:
						rtshape_settings.mRotation = Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI).Normalized();
						break;
					default:
						break;
					}

					shape_result = rtshape_settings.Create();
					physicsobject.shape = shape_result.Get();

					// capsule offset on axis is performed because otherwise capsule center would be in the bone root position
					//	which is not what we want. Instead the bone is moved on its axis so it resides between root and tail
					const float offset = capsule_height * 0.5f + capsule_radius;

					Vec3 local_offset = Vec3::sZero();
					switch (c)
					{
					case BODYPART_PELVIS:
						break;
					case BODYPART_SPINE:
					case BODYPART_HEAD:
						local_offset = Vec3(0, offset, 0);
						break;
					case BODYPART_LEFT_UPPER_LEG:
					case BODYPART_LEFT_LOWER_LEG:
					case BODYPART_RIGHT_UPPER_LEG:
					case BODYPART_RIGHT_LOWER_LEG:
						local_offset = Vec3(0, -offset, 0);
						break;
					case BODYPART_LEFT_UPPER_ARM:
					case BODYPART_LEFT_LOWER_ARM:
						local_offset = Vec3(-offset * facing, 0, 0);
						break;
					case BODYPART_RIGHT_UPPER_ARM:
					case BODYPART_RIGHT_LOWER_ARM:
						local_offset = Vec3(offset * facing, 0, 0);
						break;
					default:
						break;
					}

					physicsobject.additionalTransform.SetTranslation(local_offset);
					physicsobject.additionalTransformInverse = physicsobject.additionalTransform.Inversed();

					// Get the translation and rotation part of rest matrix:
					XMVECTOR SCA = {};
					XMVECTOR ROT = {};
					XMVECTOR TRA = {};
					XMMatrixDecompose(&SCA, &ROT, &TRA, restA);
					XMFLOAT4 rot = {};
					XMFLOAT3 tra = {};
					XMStoreFloat4(&rot, ROT);
					XMStoreFloat3(&tra, TRA);

					Vec3 root = cast(tra);

					Mat44 mat = Mat44::sTranslation(root);
					mat = mat * physicsobject.additionalTransform;

					physicsobject.restBasis = Mat44::sRotation(cast(rot));
					physicsobject.restBasisInverse = physicsobject.restBasis.Inversed();

					physicsobject.humanoid_ragdoll_entity = humanoidEntity;
					physicsobject.humanoid_bone = humanoid_bone;

					physicsobject.physics_scene = scene.physics_scene;

					masses[c] = mass;
					positions[c] = mat.GetTranslation();
					constraint_positions[c] = root;
					final_transforms[c] = Mat44::sTranslation(cast(tra)) * Mat44::sRotation(cast(rot));
					physicsobject.prev_position = final_transforms[c].GetTranslation();
					physicsobject.prev_rotation = final_transforms[c].GetQuaternion().Normalized();
					final_transforms[c] = final_transforms[c] * physicsobject.restBasisInverse;
					final_transforms[c] = final_transforms[c] * physicsobject.additionalTransform;
				}

				// For constraint setup, see examples in Jolt/Samples/Utils/RagdollLoader.cpp

				skeleton.SetEmbedded();

				uint bodyparts[BODYPART_COUNT] = {};
				bodyparts[BODYPART_PELVIS] = skeleton.AddJoint("LowerBody");
				bodyparts[BODYPART_SPINE] = skeleton.AddJoint("UpperBody", bodyparts[BODYPART_PELVIS]);
				bodyparts[BODYPART_HEAD] = skeleton.AddJoint("Head", bodyparts[BODYPART_SPINE]);
				bodyparts[BODYPART_LEFT_UPPER_LEG] = skeleton.AddJoint("UpperLegL", bodyparts[BODYPART_PELVIS]);
				bodyparts[BODYPART_LEFT_LOWER_LEG] = skeleton.AddJoint("LowerLegL", bodyparts[BODYPART_LEFT_UPPER_LEG]);
				bodyparts[BODYPART_RIGHT_UPPER_LEG] = skeleton.AddJoint("UpperLegR", bodyparts[BODYPART_PELVIS]);
				bodyparts[BODYPART_RIGHT_LOWER_LEG] = skeleton.AddJoint("LowerLegR", bodyparts[BODYPART_RIGHT_UPPER_LEG]);
				bodyparts[BODYPART_LEFT_UPPER_ARM] = skeleton.AddJoint("UpperArmL", bodyparts[BODYPART_SPINE]);
				bodyparts[BODYPART_LEFT_LOWER_ARM] = skeleton.AddJoint("LowerArmL", bodyparts[BODYPART_LEFT_UPPER_ARM]);
				bodyparts[BODYPART_RIGHT_UPPER_ARM] = skeleton.AddJoint("UpperArmR", bodyparts[BODYPART_SPINE]);
				bodyparts[BODYPART_RIGHT_LOWER_ARM] = skeleton.AddJoint("LowerArmR", bodyparts[BODYPART_RIGHT_UPPER_ARM]);

				// Constraint limits
				const float twist_angle[] = {
					0.0f,		// Lower Body (unused, there's no parent)
					5.0f,		// Upper Body
					90.0f,		// Head
					45.0f,		// Upper Leg L
					45.0f,		// Lower Leg L
					45.0f,		// Upper Leg R
					45.0f,		// Lower Leg R
					45.0f,		// Upper Arm L
					45.0f,		// Lower Arm L
					45.0f,		// Upper Arm R
					45.0f,		// Lower Arm R
				};

				const float normal_angle[] = {
					0.0f,		// Lower Body (unused, there's no parent)
					40.0f,		// Upper Body
					45.0f,		// Head
					45.0f,		// Upper Leg L
					0.0f,		// Lower Leg L
					45.0f,		// Upper Leg R
					0.0f,		// Lower Leg R
					90.0f,		// Upper Arm L
					0.0f,		// Lower Arm L
					90.0f,		// Upper Arm R
					0.0f,		// Lower Arm R
				};

				const float plane_angle[] = {
					0.0f,		// Lower Body (unused, there's no parent)
					40.0f,		// Upper Body
					45.0f,		// Head
					45.0f,		// Upper Leg L
					60.0f,		// Lower Leg L (cheating here, a knee is not symmetric, we should have rotated the twist axis)
					45.0f,		// Upper Leg R
					60.0f,		// Lower Leg R
					45.0f,		// Upper Arm L
					90.0f,		// Lower Arm L
					45.0f,		// Upper Arm R
					90.0f,		// Lower Arm R
				};

				static float constraint_dbg = 0.1f;
				static bool fixpose = false; // enable to fix the pose to rest pose, useful for debugging

				settings.SetEmbedded();
				settings.mSkeleton = &skeleton;
				settings.mParts.resize(skeleton.GetJointCount());
				for (int p = 0; p < skeleton.GetJointCount(); ++p)
				{
					RagdollSettings::Part& part = settings.mParts[p];
					part.SetShape(rigidbodies[p].shape);
					part.mPosition = positions[p];
					part.mRotation = Quat::sIdentity();
					part.mMotionType = EMotionType::Kinematic;
					part.mMotionQuality = cMotionQuality;
					part.mObjectLayer = Layers::MOVING;
					part.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
					part.mMassPropertiesOverride.mMass = masses[p];
					
					// First part is the root, doesn't have a parent and doesn't have a constraint
					if (p > 0)
					{
						if (p == BODYPART_LEFT_LOWER_LEG || p == BODYPART_RIGHT_LOWER_LEG)
						{
							Ref<HingeConstraintSettings> constraint = new HingeConstraintSettings;
							constraint->mDrawConstraintSize = constraint_dbg;
							constraint->mPoint1 = constraint->mPoint2 = constraint_positions[p];
							constraint->mHingeAxis1 = constraint->mHingeAxis2 = Vec3::sAxisX() * facing;
							constraint->mNormalAxis1 = constraint->mNormalAxis2 = -Vec3::sAxisY();
							if (fixpose)
							{
								constraint->mLimitsMin = constraint->mLimitsMax = 0;
							}
							else
							{
								constraint->mLimitsMin = 0;
								constraint->mLimitsMax = JPH_PI * 0.8f;
							}
							part.mToParent = constraint;
						}
						else if (p == BODYPART_LEFT_LOWER_ARM)
						{
							Ref<HingeConstraintSettings> constraint = new HingeConstraintSettings;
							constraint->mDrawConstraintSize = constraint_dbg;
							constraint->mPoint1 = constraint->mPoint2 = constraint_positions[p];
							constraint->mHingeAxis1 = constraint->mHingeAxis2 = Vec3::sAxisY();
							constraint->mNormalAxis1 = constraint->mNormalAxis2 = (constraint_positions[p] - constraint_positions[p - 1]).Normalized();
							if (fixpose)
							{
								constraint->mLimitsMin = constraint->mLimitsMax = 0;
							}
							else
							{
								constraint->mLimitsMin = 0;
								constraint->mLimitsMax = JPH_PI * 0.6f;
							}
							part.mToParent = constraint;
						}
						else if (p == BODYPART_RIGHT_LOWER_ARM)
						{
							Ref<HingeConstraintSettings> constraint = new HingeConstraintSettings;
							constraint->mDrawConstraintSize = constraint_dbg;
							constraint->mPoint1 = constraint->mPoint2 = constraint_positions[p];
							constraint->mHingeAxis1 = constraint->mHingeAxis2 = -Vec3::sAxisY();
							constraint->mNormalAxis1 = constraint->mNormalAxis2 = (constraint_positions[p] - constraint_positions[p - 1]).Normalized();
							if (fixpose)
							{
								constraint->mLimitsMin = constraint->mLimitsMax = 0;
							}
							else
							{
								constraint->mLimitsMin = 0;
								constraint->mLimitsMax = JPH_PI * 0.6f;
							}
							part.mToParent = constraint;
						}
						else
						{
							Ref<SwingTwistConstraintSettings> constraint = new SwingTwistConstraintSettings;
							constraint->mDrawConstraintSize = constraint_dbg;
							constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
							constraint->mTwistAxis1 = constraint->mTwistAxis2 = (positions[p] - constraint_positions[p]).Normalized();
							constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = Vec3::sAxisZ() * facing;
							if (fixpose)
							{
								constraint->mTwistMinAngle = constraint->mTwistMaxAngle = 0;
								constraint->mNormalHalfConeAngle = 0;
								constraint->mPlaneHalfConeAngle = 0;
							}
							else
							{
								constraint->mTwistMinAngle = -DegreesToRadians(twist_angle[p]);
								constraint->mTwistMaxAngle = DegreesToRadians(twist_angle[p]);
								constraint->mNormalHalfConeAngle = DegreesToRadians(normal_angle[p]);
								constraint->mPlaneHalfConeAngle = DegreesToRadians(plane_angle[p]);
							}
							part.mToParent = constraint;
						}
					}
				}

				settings.Stabilize();
				settings.DisableParentChildCollisions();
				settings.CalculateBodyIndexToConstraintIndex();

				static std::atomic<uint32_t> collisionGroupID{}; // generate unique collision group for each ragdoll to enable collision between them
				ragdoll = settings.CreateRagdoll(collisionGroupID.fetch_add(1), 0, &physics_system);
				ragdoll->SetPose(Vec3::sZero(), final_transforms);
				ragdoll->AddToPhysicsSystem(EActivation::Activate);

				const int count = (int)ragdoll->GetBodyCount();
				for (int index = 0; index < count; ++index)
				{
					rigidbodies[index].bodyID = ragdoll->GetBodyID(index);
					body_interface.SetUserData(rigidbodies[index].bodyID, (uint64_t)&rigidbodies[index]);
				}
			}
			~Ragdoll()
			{
				if (physics_scene == nullptr)
					return;
				PhysicsSystem& physics_system = ((PhysicsScene*)physics_scene.get())->physics_system;

				const int count = (int)ragdoll->GetBodyCount();
				for (int index = 0; index < count; ++index)
				{
					rigidbodies[index].bodyID = {}; // will be removed by ragdoll
				}

				ragdoll->RemoveFromPhysicsSystem();
			}

			// Activates ragdoll as dynamic physics object:
			void Activate(
				Scene& scene,
				Entity humanoidEntity
			)
			{
				if (state_active)
					return;
				state_active = true;

				const HumanoidComponent* humanoid = scene.humanoids.GetComponent(humanoidEntity);
				if (humanoid == nullptr)
					return;

				PhysicsSystem& physics_system = ((PhysicsScene*)physics_scene.get())->physics_system;
				BodyInterface& body_interface = physics_system.GetBodyInterface(); // locking version because this is called from job system!

				int c = 0;
				for (auto& x : rigidbodies)
				{
					body_interface.SetMotionType(x.bodyID, EMotionType::Dynamic, EActivation::Activate);

					// Save parenting information to be able to restore it:
					const HierarchyComponent* hier = scene.hierarchy.GetComponent(x.entity);
					if (hier != nullptr)
					{
						saved_parents[c] = hier->parentID;
					}
					else
					{
						saved_parents[c] = INVALID_ENTITY;
					}

					// detach bone because it will be simulated in world space:
					scene.Component_Detach(x.entity);

					c++;
				}

				// Stop all anims that are children of humanoid:
				for (size_t i = 0; i < scene.animations.GetCount(); ++i)
				{
					Entity entity = scene.animations.GetEntity(i);
					if (!scene.Entity_IsDescendant(entity, humanoidEntity))
						continue;
					AnimationComponent& animation = scene.animations[i];
					animation.Stop();
				}
			}

			// Disables dynamic ragdoll and reattaches loose parts as they were:
			void Deactivate(
				Scene& scene
			)
			{
				if (!state_active)
					return;
				state_active = false;

				PhysicsSystem& physics_system = ((PhysicsScene*)physics_scene.get())->physics_system;
				BodyInterface& body_interface = physics_system.GetBodyInterface(); // locking version because this is called from job system!

				int c = 0;
				for (auto& x : rigidbodies)
				{
					body_interface.SetMotionType(x.bodyID, EMotionType::Kinematic, EActivation::Activate);

					if (saved_parents[c] != INVALID_ENTITY)
					{
						scene.Component_Attach(x.entity, saved_parents[c]);
					}
					c++;
				}
			}
		};

	}
	using namespace jolt;

	void Initialize()
	{
		wi::Timer timer;

		RegisterDefaultAllocator();

		Factory::sInstance = new Factory();

		RegisterTypes();

		char text[256] = {};
		snprintf(text, arraysize(text), "wi::physics Initialized [Jolt Physics %d.%d.%d] (%d ms)", JPH_VERSION_MAJOR, JPH_VERSION_MINOR, JPH_VERSION_PATCH, (int)std::round(timer.elapsed()));
		wi::backlog::post(text);
	}

	bool IsEnabled() { return ENABLED; }
	void SetEnabled(bool value) { ENABLED = value; }

	bool IsSimulationEnabled() { return ENABLED && SIMULATION_ENABLED; }
	void SetSimulationEnabled(bool value) { SIMULATION_ENABLED = value; }

	bool IsInterpolationEnabled() { return INTERPOLATION; }
	void SetInterpolationEnabled(bool value) { INTERPOLATION = value; }

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

		wi::jobsystem::Wait(ctx);

		auto range = wi::profiler::BeginRangeCPU("Physics");

		PhysicsScene& physics_scene = GetPhysicsScene(scene);
		physics_scene.physics_system.SetGravity(cast(scene.weather.gravity));

		// System will register rigidbodies to objects:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), 64, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);

			if (physicscomponent.physicsobject == nullptr && scene.transforms.Contains(entity))
			{
				TransformComponent* transform = scene.transforms.GetComponent(entity);
				if (transform == nullptr)
					return;
				const ObjectComponent* object = scene.objects.GetComponent(entity);
				const MeshComponent* mesh = nullptr;
				if (object != nullptr)
				{
					mesh = scene.meshes.GetComponent(object->meshID);
				}
				AddRigidBody(scene, entity, physicscomponent, *transform, mesh);
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				RigidBody& physicsobject = GetRigidBody(physicscomponent);
				if (physicsobject.bodyID.IsInvalid())
					return;

				BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking, these jobs can be adding bodies
				body_interface.SetFriction(physicsobject.bodyID, physicscomponent.friction);
				body_interface.SetRestitution(physicsobject.bodyID, physicscomponent.restitution);

				const EMotionType prevMotionType = body_interface.GetMotionType(physicsobject.bodyID);
				const EMotionType currentMotionType = physicscomponent.mass == 0 ? EMotionType::Static : (physicscomponent.IsKinematic() ? EMotionType::Kinematic : EMotionType::Dynamic);

				if (prevMotionType != currentMotionType)
				{
					// Changed motion type:
					body_interface.SetMotionType(physicsobject.bodyID, currentMotionType, EActivation::Activate);

					if (currentMotionType == EMotionType::Dynamic)
					{
						// We must detach dynamic objects, because their physics object is created in world space
						//	and attachment would apply double transformation to the transform
						scene.locker.lock();
						scene.Component_Detach(entity);
						scene.locker.unlock();
					}
				}

				TransformComponent* transform = scene.transforms.GetComponent(entity);
				if (transform == nullptr)
					return;

				if (currentMotionType == EMotionType::Dynamic)
				{
					// Apply effects on dynamics if needed:
					if (scene.weather.IsOceanEnabled())
					{
						const Vec3 com = body_interface.GetCenterOfMassPosition(physicsobject.bodyID);
						const Vec3 surface_position = cast(scene.GetOceanPosAt(cast(com)));

						if (com.GetY() <= surface_position.GetY())
						{
							const Vec3 p2 = cast(scene.GetOceanPosAt(cast(com + Vec3(0, 0, 0.1f))));
							const Vec3 p3 = cast(scene.GetOceanPosAt(cast(com + Vec3(0.1f, 0, 0))));
							const Vec3 surface_normal = Vec3(p2 - surface_position).Cross(Vec3(p3 - surface_position)).Normalized();

							body_interface.ApplyBuoyancyImpulse(
								physicsobject.bodyID,
								surface_position,
								surface_normal,
								4.0f,
								0.8f,
								0.6f,
								Vec3::sZero(),
								physics_scene.physics_system.GetGravity(),
								scene.dt
							);
						}
					}
				}

				if (IsSimulationEnabled())
				{
					// Feedback system transform to kinematic and static physics objects:
					const Vec3 position = cast(transform->GetPosition());
					const Quat rotation = cast(transform->GetRotation());
					Mat44 m = Mat44::sTranslation(position) * Mat44::sRotation(rotation);
					m = m * physicsobject.additionalTransform;

					if (currentMotionType == EMotionType::Kinematic)
					{
						body_interface.MoveKinematic(
							physicsobject.bodyID,
							m.GetTranslation(),
							m.GetQuaternion().Normalized(),
							physics_scene.GetKinematicDT(scene.dt)
						);
					}
					else if (currentMotionType == EMotionType::Static)
					{
						body_interface.SetPositionAndRotation(
							physicsobject.bodyID,
							m.GetTranslation(),
							m.GetQuaternion().Normalized(),
							EActivation::DontActivate
						);
					}
				}
				else
				{
					// Simulation is disabled, update physics state immediately:
					physicsobject.prev_position = cast(transform->GetPosition());
					physicsobject.prev_rotation = cast(transform->GetRotation());
					body_interface.SetPositionAndRotation(
						physicsobject.bodyID,
						physicsobject.prev_position,
						physicsobject.prev_rotation,
						EActivation::Activate
					);
				}
			}
		});

		// System will register softbodies to meshes and update physics engine state:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.softbodies.GetCount(), 1, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			SoftBodyPhysicsComponent& physicscomponent = scene.softbodies[args.jobIndex];
			Entity entity = scene.softbodies.GetEntity(args.jobIndex);
			if (!scene.meshes.Contains(entity))
				return;
			MeshComponent* mesh = scene.meshes.GetComponent(entity);
			if (mesh == nullptr)
				return;
			const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;
			mesh->SetDynamic(true);

			if (physicscomponent._flags & SoftBodyPhysicsComponent::SAFE_TO_REGISTER && physicscomponent.physicsobject == nullptr)
			{
				AddSoftBody(scene, entity, physicscomponent, *mesh);
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				SoftBody& physicsobject = GetSoftBody(physicscomponent);
				if (physicsobject.bodyID.IsInvalid())
					return;

				BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking, these jobs can be adding bodies
				body_interface.SetFriction(physicsobject.bodyID, physicscomponent.friction);
				body_interface.SetRestitution(physicsobject.bodyID, physicscomponent.restitution);

				if (IsSimulationEnabled() && physicscomponent.IsWindEnabled())
				{
					// Add wind:
					const Vec3 wind = cast(scene.weather.windDirection);
					if (!wind.IsNearZero())
					{
						body_interface.AddForce(physicsobject.bodyID, wind, EActivation::Activate);
					}
				}

				// This is different from rigid bodies, because soft body is a per mesh component (no TransformComponent). World matrix is propagated down from single mesh instance (ObjectUpdateSystem).
				XMMATRIX worldMatrix = XMLoadFloat4x4(&physicscomponent.worldMatrix);

				BodyLockRead lock(physics_scene.physics_system.GetBodyLockInterfaceNoLock(), physicsobject.bodyID);
				if (!lock.Succeeded())
					return;
				const Body& body = lock.GetBody();
				SoftBodyMotionProperties* motion = (SoftBodyMotionProperties*)body.GetMotionProperties();

				// System controls zero weight soft body nodes:
				for (size_t ind = 0; ind < physicscomponent.physicsToGraphicsVertexMapping.size(); ++ind)
				{
					uint32_t graphicsInd = physicscomponent.physicsToGraphicsVertexMapping[ind];
					float weight = physicscomponent.weights[graphicsInd];

					if (weight == 0)
					{
						XMFLOAT3 position = mesh->vertex_positions[graphicsInd];
						XMVECTOR P = armature == nullptr ? XMLoadFloat3(&position) : wi::scene::SkinVertex(*mesh, *armature, graphicsInd);
						P = XMVector3Transform(P, worldMatrix);
						XMStoreFloat3(&position, P);

						SoftBodyMotionProperties::Vertex& node = motion->GetVertex((uint)ind);
						node.mPosition = cast(position);
					}
				}
			}
		});

		// Ragdoll management:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.humanoids.GetCount(), 1, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {
			HumanoidComponent& humanoid = scene.humanoids[args.jobIndex];
			Entity humanoidEntity = scene.humanoids.GetEntity(args.jobIndex);
			float scale = 1;
			if (scene.transforms.Contains(humanoidEntity))
			{
				scale = scene.transforms.GetComponent(humanoidEntity)->scale_local.x;
			}
			if (humanoid.ragdoll != nullptr)
			{
				Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
				if (!wi::math::float_equal(ragdoll.scale, scale))
				{
					humanoid.SetRagdollPhysicsEnabled(false); // while scaling ragdoll, it will be kinematic
					ragdoll.Deactivate(scene); // recreate attached skeleton hierarchy structure
					humanoid.ragdoll = {}; // delete ragdoll if scale changed, it will be recreated
				}
			}
			if (humanoid.ragdoll == nullptr)
			{
				humanoid.ragdoll = std::make_shared<Ragdoll>(scene, humanoid, humanoidEntity, scale);
			}
			Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();

			if (IsSimulationEnabled())
			{
				if (humanoid.IsRagdollPhysicsEnabled())
				{
					ragdoll.Activate(scene, humanoidEntity);
				}
				else
				{
					ragdoll.Deactivate(scene);

					BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking, these jobs can be adding bodies
					for (auto& rb : ragdoll.rigidbodies)
					{
						TransformComponent* transform = scene.transforms.GetComponent(rb.entity);
						if (transform == nullptr)
							return;

						const Vec3 position = cast(transform->GetPosition());
						const Quat rotation = cast(transform->GetRotation());

						Mat44 m = Mat44::sTranslation(position) * Mat44::sRotation(rotation);
						m = m * rb.restBasisInverse;
						m = m * rb.additionalTransform;

						body_interface.MoveKinematic(
							rb.bodyID,
							m.GetTranslation(),
							m.GetQuaternion().Normalized(),
							physics_scene.GetKinematicDT(scene.dt)
						);
					}
				}
			}
			else if(!humanoid.IsRagdollPhysicsEnabled())
			{
				// Simulation is disabled, update physics state immediately:
				BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking, these jobs can be adding bodies
				for (auto& rb : ragdoll.rigidbodies)
				{
					TransformComponent* transform = scene.transforms.GetComponent(rb.entity);
					if (transform == nullptr)
						return;

					const Vec3 position = cast(transform->GetPosition());
					const Quat rotation = cast(transform->GetRotation());

					Mat44 m = Mat44::sTranslation(position) * Mat44::sRotation(rotation);
					m = m * rb.restBasisInverse;
					m = m * rb.additionalTransform;

					rb.prev_position = m.GetTranslation();
					rb.prev_rotation = m.GetQuaternion().Normalized();

					body_interface.SetPositionAndRotation(
						rb.bodyID,
						rb.prev_position,
						rb.prev_rotation,
						EActivation::DontActivate
					);
				}
			}
		});

		wi::jobsystem::Wait(ctx);

		static wi::jobsystem::context broadphase_optimization_ctx;
		wi::jobsystem::Wait(broadphase_optimization_ctx);
		
		// Perform internal simulation step:
		bool simulation_happened = false;
		if (IsSimulationEnabled())
		{
			//static TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
			static TempAllocatorMalloc temp_allocator; // 10-100 MB was not enough for large simulation, I don't want to reserve more memory up front
			static JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

			physics_scene.accumulator += dt;
			physics_scene.accumulator = clamp(physics_scene.accumulator, 0.0f, TIMESTEP * ACCURACY);
			while (physics_scene.accumulator >= TIMESTEP)
			{
				const float next_accumulator = physics_scene.accumulator - TIMESTEP;
				if (IsInterpolationEnabled() && next_accumulator < TIMESTEP)
				{
					// On the last step, save previous locations, this is only needed for interpolation:
					//	We don't only save it for dynamic objects that will be interpolated, because on the next frame maybe simulation doesn't run
					//	but object types can change!
					wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), 64, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {
						RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
						if (physicscomponent.physicsobject == nullptr)
							return;
						RigidBody& rb = GetRigidBody(physicscomponent);
						BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
						Mat44 mat = body_interface.GetWorldTransform(rb.bodyID);
						mat = mat * rb.additionalTransformInverse;
						rb.prev_position = mat.GetTranslation();
						rb.prev_rotation = mat.GetQuaternion().Normalized();
					});
					wi::jobsystem::Dispatch(ctx, (uint32_t)scene.humanoids.GetCount(), 1, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {
						HumanoidComponent& humanoid = scene.humanoids[args.jobIndex];
						if (humanoid.ragdoll == nullptr)
							return;
						Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
						BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
						for (auto& rb : ragdoll.rigidbodies)
						{
							TransformComponent* transform = scene.transforms.GetComponent(rb.entity);
							if (transform == nullptr)
								continue;
							Mat44 mat = body_interface.GetWorldTransform(rb.bodyID);
							mat = mat * rb.additionalTransformInverse;
							mat = mat * rb.restBasis;
							rb.prev_position = mat.GetTranslation();
							rb.prev_rotation = mat.GetQuaternion().Normalized();
						}
					});
					wi::jobsystem::Wait(ctx);
				}

				simulation_happened = true;
				physics_scene.physics_system.Update(TIMESTEP, 1, &temp_allocator, &job_system);
				physics_scene.accumulator = next_accumulator;
			}
			physics_scene.alpha = physics_scene.accumulator / TIMESTEP;
		}

		if (simulation_happened)
		{
			broadphase_optimization_ctx.priority = wi::jobsystem::Priority::Streaming;
			wi::jobsystem::Execute(broadphase_optimization_ctx, [&](wi::jobsystem::JobArgs args) {
				physics_scene.physics_system.OptimizeBroadPhase();
			});
		}

		// Feedback physics objects to system:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), 64, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			if (physicscomponent.physicsobject == nullptr)
				return;
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			if (physicsobject.bodyID.IsInvalid())
				return;
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			if (body_interface.GetMotionType(physicsobject.bodyID) != EMotionType::Dynamic)
				return;

			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);
			TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (transform == nullptr)
				return;

			Mat44 mat = body_interface.GetWorldTransform(physicsobject.bodyID);
			mat = mat * physicsobject.additionalTransformInverse;
			Vec3 position = mat.GetTranslation();
			Quat rotation = mat.GetQuaternion().Normalized();

			if (IsInterpolationEnabled())
			{
				position = position * physics_scene.alpha + physicsobject.prev_position * (1 - physics_scene.alpha);
				rotation = physicsobject.prev_rotation.SLERP(rotation, physics_scene.alpha);
			}

			transform->translation_local = cast(position);
			transform->rotation_local = cast(rotation);
			transform->SetDirty();
		});
		
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.softbodies.GetCount(), 1, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			SoftBodyPhysicsComponent& physicscomponent = scene.softbodies[args.jobIndex];
			if (physicscomponent.physicsobject == nullptr)
				return;

			Entity entity = scene.softbodies.GetEntity(args.jobIndex);

			SoftBody& physicsobject = GetSoftBody(physicscomponent);
			if (physicsobject.bodyID.IsInvalid())
				return;

			BodyLockRead lock(physics_scene.physics_system.GetBodyLockInterfaceNoLock(), physicsobject.bodyID);
			if (!lock.Succeeded())
				return;

			const Body& body = lock.GetBody();

			MeshComponent* mesh = scene.meshes.GetComponent(entity);
			if (mesh == nullptr)
				return;

			physicscomponent.aabb = wi::primitive::AABB();

			const SoftBodyMotionProperties* motion = (const SoftBodyMotionProperties*)body.GetMotionProperties();
			const Array<SoftBodyMotionProperties::Vertex>& soft_vertices = motion->GetVertices();

			// Update bone matrices from physics vertices:
			for (size_t i = 0; i < soft_vertices.size(); ++i)
			{
				XMFLOAT3 p0 = cast(soft_vertices[i].mPosition);
				XMFLOAT3 p1 = cast(soft_vertices[physicsobject.physicsNeighbors[i].left].mPosition);
				XMFLOAT3 p2 = cast(soft_vertices[physicsobject.physicsNeighbors[i].right].mPosition);
				XMVECTOR P0 = XMLoadFloat3(&p0);
				XMVECTOR P1 = XMLoadFloat3(&p1);
				XMVECTOR P2 = XMLoadFloat3(&p2);
				XMMATRIX W = GetOrientation(P0, P1, P2);
				XMMATRIX B = XMLoadFloat4x4(&physicsobject.inverseBindMatrices[i]);
				XMMATRIX M = B * W;
				XMFLOAT4X4 boneData;
				XMStoreFloat4x4(&boneData, M);
				physicscomponent.boneData[i].Create(boneData);

#if 0
				scene.locker.lock();
				wi::renderer::DrawAxis(W, 0.05f, false);
				scene.locker.unlock();
#endif

				physicscomponent.aabb._min = wi::math::Min(physicscomponent.aabb._min, p0);
				physicscomponent.aabb._max = wi::math::Max(physicscomponent.aabb._max, p0);
			}

			scene.skinningAllocator.fetch_add(uint32_t(physicscomponent.boneData.size() * sizeof(ShaderTransform)));
		});

		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.humanoids.GetCount(), 1, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {
			HumanoidComponent& humanoid = scene.humanoids[args.jobIndex];
			if (humanoid.ragdoll == nullptr)
				return;
			Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
			if (humanoid.IsRagdollPhysicsEnabled())
			{
				BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
				for (auto& rb : ragdoll.rigidbodies)
				{
					TransformComponent* transform = scene.transforms.GetComponent(rb.entity);
					if (transform == nullptr)
						continue;
					Mat44 mat = body_interface.GetWorldTransform(rb.bodyID);
					mat = mat * rb.additionalTransformInverse;
					mat = mat * rb.restBasis;
					Vec3 position = mat.GetTranslation();
					Quat rotation = mat.GetQuaternion().Normalized();

					if (IsInterpolationEnabled())
					{
						position = position * physics_scene.alpha + rb.prev_position * (1 - physics_scene.alpha);
						rotation = rb.prev_rotation.SLERP(rotation, physics_scene.alpha);
					}

					transform->translation_local = cast(position);
					transform->rotation_local = cast(rotation);
					transform->SetDirty();
				}
			}
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
			settings.mDrawCenterOfMassTransform = false;
			settings.mDrawShape = true;
			settings.mDrawSoftBodyVertices = true;
			settings.mDrawSoftBodyEdgeConstraints = true;
			settings.mDrawShapeWireframe = true;
			settings.mDrawShapeColor = BodyManager::EShapeColor::ShapeTypeColor;
			physics_scene.physics_system.DrawBodies(settings, &debug_renderer);
			physics_scene.physics_system.DrawConstraints(&debug_renderer);
		}
#endif // JPH_DEBUG_RENDERER

		wi::jobsystem::Wait(ctx);

		wi::profiler::EndRange(range); // Physics
	}

	void SetLinearVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			body_interface.SetLinearVelocity(physicsobject.bodyID, cast(velocity));
		}
	}
	void SetAngularVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			body_interface.SetAngularVelocity(physicsobject.bodyID, cast(velocity));
		}
	}

	void ApplyForce(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			body_interface.AddForce(physicsobject.bodyID, cast(force));
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
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			Vec3 at_world = body_interface.GetCenterOfMassTransform(physicsobject.bodyID).Inversed() * cast(at);
			body_interface.AddForce(physicsobject.bodyID, cast(force), at_world);
		}
	}

	void ApplyImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			body_interface.AddImpulse(physicsobject.bodyID, cast(impulse));
		}
	}
	void ApplyImpulse(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse
	)
	{
		if (humanoid.ragdoll == nullptr)
			return;

		Ragdoll::BODYPART bodypart = Ragdoll::BODYPART_COUNT;
		switch (bone)
		{
		case HumanoidComponent::HumanoidBone::Hips:
			bodypart = Ragdoll::BODYPART_PELVIS;
			break;
		case HumanoidComponent::HumanoidBone::Spine:
			bodypart = Ragdoll::BODYPART_SPINE;
			break;
		case HumanoidComponent::HumanoidBone::Head:
		case HumanoidComponent::HumanoidBone::Neck:
			bodypart = Ragdoll::BODYPART_HEAD;
			break;
		case HumanoidComponent::HumanoidBone::RightUpperArm:
			bodypart = Ragdoll::BODYPART_RIGHT_UPPER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::RightLowerArm:
			bodypart = Ragdoll::BODYPART_RIGHT_LOWER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::LeftUpperArm:
			bodypart = Ragdoll::BODYPART_LEFT_UPPER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::LeftLowerArm:
			bodypart = Ragdoll::BODYPART_LEFT_LOWER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::RightUpperLeg:
			bodypart = Ragdoll::BODYPART_RIGHT_UPPER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::RightLowerLeg:
			bodypart = Ragdoll::BODYPART_RIGHT_LOWER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::LeftUpperLeg:
			bodypart = Ragdoll::BODYPART_LEFT_UPPER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::LeftLowerLeg:
			bodypart = Ragdoll::BODYPART_LEFT_LOWER_LEG;
			break;
		}
		if (bodypart == Ragdoll::BODYPART_COUNT)
			return;

		Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
		if (ragdoll.rigidbodies[bodypart].bodyID.IsInvalid())
			return;
		RigidBody& physicsobject = ragdoll.rigidbodies[bodypart];
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		body_interface.SetMotionType(physicsobject.bodyID, EMotionType::Dynamic, EActivation::Activate);
		body_interface.AddImpulse(physicsobject.bodyID, cast(impulse));
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
			PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			Vec3 at_world = body_interface.GetCenterOfMassTransform(physicsobject.bodyID) * cast(at);
			body_interface.AddImpulse(physicsobject.bodyID, cast(impulse), at_world);
		}
	}
	void ApplyImpulseAt(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at
	)
	{
		if (humanoid.ragdoll == nullptr)
			return;

		Ragdoll::BODYPART bodypart = Ragdoll::BODYPART_COUNT;
		switch (bone)
		{
		case HumanoidComponent::HumanoidBone::Hips:
			bodypart = Ragdoll::BODYPART_PELVIS;
			break;
		case HumanoidComponent::HumanoidBone::Spine:
			bodypart = Ragdoll::BODYPART_SPINE;
			break;
		case HumanoidComponent::HumanoidBone::Head:
		case HumanoidComponent::HumanoidBone::Neck:
			bodypart = Ragdoll::BODYPART_HEAD;
			break;
		case HumanoidComponent::HumanoidBone::RightUpperArm:
			bodypart = Ragdoll::BODYPART_RIGHT_UPPER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::RightLowerArm:
			bodypart = Ragdoll::BODYPART_RIGHT_LOWER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::LeftUpperArm:
			bodypart = Ragdoll::BODYPART_LEFT_UPPER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::LeftLowerArm:
			bodypart = Ragdoll::BODYPART_LEFT_LOWER_ARM;
			break;
		case HumanoidComponent::HumanoidBone::RightUpperLeg:
			bodypart = Ragdoll::BODYPART_RIGHT_UPPER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::RightLowerLeg:
			bodypart = Ragdoll::BODYPART_RIGHT_LOWER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::LeftUpperLeg:
			bodypart = Ragdoll::BODYPART_LEFT_UPPER_LEG;
			break;
		case HumanoidComponent::HumanoidBone::LeftLowerLeg:
			bodypart = Ragdoll::BODYPART_LEFT_LOWER_LEG;
			break;
		}
		if (bodypart == Ragdoll::BODYPART_COUNT)
			return;

		Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
		if (ragdoll.rigidbodies[bodypart].bodyID.IsInvalid())
			return;
		RigidBody& physicsobject = ragdoll.rigidbodies[bodypart];
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		Vec3 at_world = body_interface.GetCenterOfMassTransform(physicsobject.bodyID) * cast(at);
		body_interface.SetMotionType(physicsobject.bodyID, EMotionType::Dynamic, EActivation::Activate);
		body_interface.AddImpulse(physicsobject.bodyID, cast(impulse), at_world);
	}

	void ApplyTorque(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	)
	{
		if (physicscomponent.physicsobject != nullptr)
		{
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			body_interface.AddTorque(physicsobject.bodyID, cast(torque), EActivation::Activate);
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
			body_interface.ActivateBody(physicsobject.bodyID);
			break;
		case wi::physics::ActivationState::Inactive:
			body_interface.DeactivateBody(physicsobject.bodyID);
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
			body_interface.ActivateBody(physicsobject.bodyID);
			break;
		case wi::physics::ActivationState::Inactive:
			body_interface.DeactivateBody(physicsobject.bodyID);
			break;
		default:
			break;
		}
	}

	XMFLOAT3 GetSoftBodyNodePosition(
		wi::scene::SoftBodyPhysicsComponent& physicscomponent,
		uint32_t physicsIndex
	)
	{
		SoftBody& physicsobject = GetSoftBody(physicscomponent);
		if (physicsobject.bodyID.IsInvalid() || physicsobject.physics_scene == nullptr)
			return XMFLOAT3(0, 0, 0);

		const PhysicsScene& physics_scene = *(const PhysicsScene*)physicsobject.physics_scene.get();
		BodyLockRead lock(physics_scene.physics_system.GetBodyLockInterfaceNoLock(), physicsobject.bodyID);
		if (!lock.Succeeded())
			return XMFLOAT3(0, 0, 0);

		const Body& body = lock.GetBody();
		const SoftBodyMotionProperties* motion = (const SoftBodyMotionProperties*)body.GetMotionProperties();
		const Array<SoftBodyMotionProperties::Vertex>& soft_vertices = motion->GetVertices();
		return cast(soft_vertices[physicsIndex].mPosition);
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

		const float tmin = clamp(ray.TMin, 0.0f, 1000000.0f);
		const float tmax = clamp(ray.TMax, 0.0f, 1000000.0f);
		const float range = tmax - tmin;

		RRayCast inray{
			cast(ray.origin),
			cast(ray.direction).Normalized()
		};

		inray.mOrigin = inray.mOrigin + inray.mDirection * tmin;
		inray.mDirection = inray.mDirection * range;

		RayCastSettings settings;
		settings.mBackFaceMode = EBackFaceMode::IgnoreBackFaces;
		settings.mTreatConvexAsSolid = false;

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

		const Vec3 position = inray.GetPointOnRay(collector.mHit.mFraction);
		const Vec3 position_local = body.GetCenterOfMassTransform().Inversed() * position;
		const Vec3 normal = body.GetWorldSpaceSurfaceNormal(collector.mHit.mSubShapeID2, position);

		if (body.IsRigidBody())
		{
			const RigidBody* physicsobject = (RigidBody*)userdata;
			result.entity = physicsobject->entity;
			result.position = cast(position);
			result.position_local = cast(position_local);
			result.normal = cast(normal);
			result.physicsobject = &body;
			result.humanoid_ragdoll_entity = physicsobject->humanoid_ragdoll_entity;
			result.humanoid_bone = physicsobject->humanoid_bone;
		}
		else // soft body
		{
			const SoftBody* physicsobject = (SoftBody*)userdata;
			result.entity = physicsobject->entity;
			result.position = cast(position);
			result.position_local = cast(position_local);
			result.normal = cast(normal);
			result.physicsobject = &body;
			const SoftBodyShape* shape = (const SoftBodyShape*)body.GetShape();
			result.softbody_triangleID = (int)shape->GetFaceIndex(collector.mHit.mSubShapeID2);
		}

		return result;
	}

	struct PickDragOperation_Jolt
	{
		std::shared_ptr<void> physics_scene;
		Ref<TwoBodyConstraint> constraint;
		float pick_distance = 0;
		Body* bodyA = nullptr;
		Body* bodyB = nullptr;
		int softBodyVertex = -1;
		Vec3 softBodyVertexOffset = Vec3::sZero();
		float prevInvMass = 0;
		~PickDragOperation_Jolt()
		{
			if (physics_scene == nullptr || bodyB == nullptr)
				return;
			PhysicsScene& physics_scene = *((PhysicsScene*)this->physics_scene.get());
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			if (bodyA != nullptr)
			{
				// Rigid body constraint removal
				physics_scene.physics_system.RemoveConstraint(constraint);
				body_interface.RemoveBody(bodyA->GetID());
				body_interface.DestroyBody(bodyA->GetID());
			}
			if (bodyB->IsSoftBody() && softBodyVertex >= 0)
			{
				SoftBodyMotionProperties* motion = (SoftBodyMotionProperties*)bodyB->GetMotionProperties();
				SoftBodyMotionProperties::Vertex& node = motion->GetVertex((uint)softBodyVertex);
				node.mInvMass = prevInvMass;
			}
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
			if (internal_state->softBodyVertex >= 0)
			{
				// Soft body vertex:
				SoftBodyMotionProperties* motion = (SoftBodyMotionProperties*)internal_state->bodyB->GetMotionProperties();
				SoftBodyMotionProperties::Vertex& node = motion->GetVertex((uint)internal_state->softBodyVertex);
				node.mPosition = pos + internal_state->softBodyVertexOffset;
			}
			else
			{
				// Rigid body constraint:
				//body_interface.SetPosition(internal_state->bodyA->GetID(), pos, EActivation::Activate);
				body_interface.MoveKinematic(internal_state->bodyA->GetID(), pos, Quat::sIdentity(), physics_scene.GetKinematicDT(scene.dt));
			}
		}
		else
		{
			// Begin picking:
			RayIntersectionResult result = Intersects(scene, ray);
			if (!result.IsValid())
				return;
			Body* body = (Body*)result.physicsobject;

			auto internal_state = std::make_shared<PickDragOperation_Jolt>();
			internal_state->physics_scene = scene.physics_scene;
			internal_state->pick_distance = wi::math::Distance(ray.origin, result.position);
			internal_state->bodyB = body;

			if (body->IsRigidBody())
			{
				Vec3 pos = cast(result.position);

				internal_state->bodyA = body_interface.CreateBody(BodyCreationSettings(new SphereShape(0.01f), pos, Quat::sIdentity(), EMotionType::Kinematic, Layers::MOVING));
				body_interface.AddBody(internal_state->bodyA->GetID(), EActivation::Activate);

#if 0
				DistanceConstraintSettings settings;
				settings.SetEmbedded();
				settings.mPoint1 = settings.mPoint2 = pos;
#else
				FixedConstraintSettings settings;
				settings.SetEmbedded();
				settings.mAutoDetectPoint = true;
#endif

				internal_state->constraint = settings.Create(*internal_state->bodyA, *internal_state->bodyB);
				physics_scene.physics_system.AddConstraint(internal_state->constraint);
			}
			else if (body->IsSoftBody())
			{
				SoftBodyMotionProperties* motion = (SoftBodyMotionProperties*)body->GetMotionProperties();
				internal_state->softBodyVertex = (int)motion->GetFace((uint)result.softbody_triangleID).mVertex[0];
				SoftBodyVertex& vertex = motion->GetVertex((uint)internal_state->softBodyVertex);
				internal_state->prevInvMass = vertex.mInvMass;
				vertex.mInvMass = 0;
				internal_state->softBodyVertexOffset = vertex.mPosition - cast(result.position);
			}

			op.internal_state = internal_state;
		}
	}

}
