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
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <Jolt/Physics/SoftBody/SoftBodyShape.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Vehicle/MotorcycleController.h>
#include <Jolt/Physics/Character/Character.h>

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

	static constexpr uint32_t dispatchGroupSize = 256u;

	namespace jolt
	{
		bool ENABLED = true;
		bool SIMULATION_ENABLED = true;
		bool DEBUGDRAW_ENABLED = false;
		float CONSTRAINT_DEBUGSIZE = 1;
		int ACCURACY = 4;
		int softbodyIterationCount = 6;
		float TIMESTEP = 1.0f / 60.0f;
		bool INTERPOLATION = true;
		float CHARACTER_COLLISION_TOLERANCE = 0.05f;

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
		inline Mat44 cast(const XMMATRIX& v)
		{
			XMFLOAT4X4 m;
			XMStoreFloat4x4(&m, v);
			return cast(m);
		}
		inline XMFLOAT3 cast(Vec3Arg v) { return XMFLOAT3(v.GetX(), v.GetY(), v.GetZ()); }
		inline XMFLOAT4 cast(QuatArg v) { return XMFLOAT4(v.GetX(), v.GetY(), v.GetZ(), v.GetW()); }
		inline XMFLOAT4X4 cast(Mat44 v)
		{
			XMFLOAT4X4 ret;
			v.StoreFloat4x4((Float4*)&ret);
			return ret;
		}

		static std::atomic<uint32_t> collisionGroupID{}; // generate unique collision group for each ragdoll to enable collision between them

		enum Layers : ObjectLayer
		{
			GHOST = 0,
			NON_MOVING = 1,
			MOVING = 2,
		};

		/// Class that determines if two object layers can collide
		class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
		{
		public:
			bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
			{
				switch (inObject1)
				{
				case Layers::GHOST:
					return false; // collides with nothing
				case Layers::NON_MOVING:
					return inObject2 == Layers::MOVING; // Non moving only collides with moving
				case Layers::MOVING:
					return inObject2 == Layers::MOVING || inObject2 == Layers::NON_MOVING; // Moving collides with moving and non moving
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
			static constexpr BroadPhaseLayer STATIC(0);
			static constexpr BroadPhaseLayer DYNAMIC(1);
			static constexpr uint NUM_LAYERS(2);
		};

		// BroadPhaseLayerInterface implementation
		// This defines a mapping between object and broadphase layers.
		class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
		{
		public:
			BPLayerInterfaceImpl() {}

			virtual uint GetNumBroadPhaseLayers() const override
			{
				return BroadPhaseLayers::NUM_LAYERS;
			}

			virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
			{
				return inLayer == Layers::MOVING ? BroadPhaseLayers::DYNAMIC : BroadPhaseLayers::STATIC;
			}
		};

		/// Class that determines if an object layer can collide with a broadphase layer
		class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
		{
		public:
			virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
			{
				switch (inLayer1)
				{
				case Layers::GHOST:
					return false;
				case Layers::NON_MOVING:
					return inLayer2 == BroadPhaseLayers::DYNAMIC;
				case Layers::MOVING:
					return inLayer2 == BroadPhaseLayers::DYNAMIC || inLayer2 == BroadPhaseLayers::STATIC;
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
				// we don't call UnregisterTypes() here, this is intentional
				// see #945 and jrouwe/JoltPhysics#1458 for more information
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
			bool activate_all_rigid_bodies = false;
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
			Entity entity = INVALID_ENTITY;
			ShapeRefC shape;
			BodyID bodyID;

			// property tracking:
			float friction = 0;
			float restitution = 0;
			EMotionType motiontype = EMotionType::Static;
			uint8_t start_deactivated : 1;
			uint8_t was_underwater : 1;
			uint8_t was_active_prev_frame : 1;
			uint8_t teleporting : 1;
			Vec3 initial_position = Vec3::sZero();
			Quat initial_rotation = Quat::sIdentity();

			// Things for parented objects:
			XMFLOAT4X4 parentMatrix = wi::math::IDENTITY_MATRIX;
			XMFLOAT4X4 parentMatrixInverse = wi::math::IDENTITY_MATRIX;

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
			wi::primitive::Capsule capsule;

			// vehicle:
			VehicleConstraint* vehicle_constraint = nullptr;
			Vec3 prev_wheel_positions[4] = { Vec3::sZero(), Vec3::sZero(), Vec3::sZero(), Vec3::sZero() };
			Quat prev_wheel_rotations[4] = { Quat::sIdentity(), Quat::sIdentity(), Quat::sIdentity(), Quat::sIdentity() };

			// character:
			Ref<Character> character = nullptr;

			void Delete()
			{
				if (physics_scene == nullptr || bodyID.IsInvalid())
					return;
				PhysicsScene* jolt_physics_scene = (PhysicsScene*)physics_scene.get();
				BodyInterface& body_interface = jolt_physics_scene->physics_system.GetBodyInterface(); // locking version because destructor can be called from any thread
				body_interface.RemoveBody(bodyID);
				if (character != nullptr)
				{
					character = {};
				}
				else
				{
					body_interface.DestroyBody(bodyID);
				}
				bodyID = {};
				if (vehicle_constraint != nullptr)
				{
					jolt_physics_scene->physics_system.RemoveStepListener(vehicle_constraint);
					jolt_physics_scene->physics_system.RemoveConstraint(vehicle_constraint);
					vehicle_constraint = nullptr;
				}
				shape = nullptr;
			}

			RigidBody()
			{
				start_deactivated = 0;
				was_underwater = 0;
				was_active_prev_frame = 0;
				teleporting = 0;
			}
			~RigidBody()
			{
				Delete();
			}
		};
		struct SoftBody
		{
			std::shared_ptr<void> physics_scene;
			Entity entity = INVALID_ENTITY;
			BodyID bodyID;
			float friction = 0;
			float restitution = 0;

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
		struct Constraint
		{
			std::shared_ptr<void> physics_scene;
			Entity entity = INVALID_ENTITY;
			Ref<TwoBodyConstraint> constraint;
			BodyID body1_self;
			BodyID body2_self;

			// to detect the case when referenced rigidbody is deleted by someone:
			BodyID body1_ref;
			BodyID body2_ref;

			 // to detect constraint breaking:
			float bind_distance = 0;
			BodyID body1_id;
			BodyID body2_id;

			~Constraint()
			{
				if (physics_scene == nullptr)
					return;
				if (constraint != nullptr)
				{
					((PhysicsScene*)physics_scene.get())->physics_system.RemoveConstraint(constraint);
				}
				BodyInterface& body_interface = ((PhysicsScene*)physics_scene.get())->physics_system.GetBodyInterface(); // locking version because destructor can be called from any thread
				if (!body1_self.IsInvalid())
				{
					body_interface.RemoveBody(body1_self);
					body_interface.DestroyBody(body1_self);
				}
				if (!body2_self.IsInvalid())
				{
					body_interface.RemoveBody(body2_self);
					body_interface.DestroyBody(body2_self);
				}
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
		const RigidBody& GetRigidBody(const wi::scene::RigidBodyPhysicsComponent& physicscomponent)
		{
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
		const SoftBody& GetSoftBody(const wi::scene::SoftBodyPhysicsComponent& physicscomponent)
		{
			return *(SoftBody*)physicscomponent.physicsobject.get();
		}
		Constraint& GetConstraint(wi::scene::PhysicsConstraintComponent& physicscomponent)
		{
			if (physicscomponent.physicsobject == nullptr)
			{
				physicscomponent.physicsobject = std::make_shared<Constraint>();
			}
			return *(Constraint*)physicscomponent.physicsobject.get();
		}
		const Constraint& GetConstraint(const wi::scene::PhysicsConstraintComponent& physicscomponent)
		{
			return *(Constraint*)physicscomponent.physicsobject.get();
		}

		void AddRigidBody(
			wi::scene::Scene& scene,
			Entity entity,
			wi::scene::RigidBodyPhysicsComponent& physicscomponent,
			const wi::scene::TransformComponent& _transform,
			const wi::scene::MeshComponent* mesh
		)
		{
			RigidBody& physicsobject = GetRigidBody(physicscomponent);

			const bool refresh = physicsobject.bodyID.IsInvalid() == false;
			physicsobject.Delete(); // if it was already existing, we handle recreation

			TransformComponent transform = _transform;

			scene.locker.lock();
			XMMATRIX parentMatrix = scene.ComputeParentMatrixRecursive(entity);
			scene.locker.unlock();

			transform.ApplyTransform();

			if (mesh != nullptr && mesh->precomputed_rigidbody_physics_shape.physicsobject != nullptr)
			{
				// The shape comes from mesh's precomputed shape:
				const RigidBody& precomputed_rigidbody_with_shape = GetRigidBody(mesh->precomputed_rigidbody_physics_shape);
				physicsobject.shape = precomputed_rigidbody_with_shape.shape;
			}

			if (physicsobject.shape == nullptr) // shape creation can be called from outside as optimization from threads
			{
				CreateRigidBodyShape(physicscomponent, transform.scale_local, mesh);
			}

			if (physicsobject.shape != nullptr)
			{
				physicsobject.physics_scene = scene.physics_scene;
				physicsobject.entity = entity;
				XMStoreFloat4x4(&physicsobject.parentMatrix, parentMatrix);
				XMStoreFloat4x4(&physicsobject.parentMatrixInverse, XMMatrixInverse(nullptr, parentMatrix));
				PhysicsScene& physics_scene = GetPhysicsScene(scene);

				Mat44 mat = cast(transform.world);
				Vec3 local_offset = cast(physicscomponent.local_offset);

				physicsobject.additionalTransform.SetTranslation(local_offset);
				physicsobject.additionalTransformInverse = physicsobject.additionalTransform.Inversed();

				physicsobject.prev_position = mat.GetTranslation();
				physicsobject.prev_rotation = mat.GetQuaternion().Normalized();

				if (!refresh)
				{
					// only set initial orientation when created for the first time:
					physicsobject.initial_position = physicsobject.prev_position;
					physicsobject.initial_rotation = physicsobject.prev_rotation;
				}

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
				if (motionType == EMotionType::Static)
				{
					settings.mObjectLayer = Layers::NON_MOVING;
				}

				physicsobject.friction = settings.mFriction;
				physicsobject.restitution = settings.mRestitution;
				physicsobject.motiontype = settings.mMotionType;

				physicsobject.start_deactivated = physicscomponent.IsStartDeactivated();
				const EActivation activation = physicsobject.start_deactivated ? EActivation::DontActivate : EActivation::Activate;

				if (physicscomponent.IsCharacterPhysics())
				{
					CharacterSettings character_settings;
					character_settings.mLayer = Layers::MOVING;
					character_settings.mEnhancedInternalEdgeRemoval = true;
					character_settings.mFriction = physicscomponent.friction;
					character_settings.mMass = physicscomponent.mass;
					character_settings.mShape = physicsobject.shape;
					character_settings.mMaxSlopeAngle = physicscomponent.character.maxSlopeAngle;
					character_settings.mGravityFactor = physicscomponent.character.gravityFactor;
					switch (physicscomponent.shape)
					{
					case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
						character_settings.mSupportingVolume = Plane(Vec3::sAxisY(), -physicscomponent.sphere.radius); // Accept contacts that touch the lower sphere
						break;
					case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
						character_settings.mSupportingVolume = Plane(Vec3::sAxisY(), -physicscomponent.capsule.radius); // Accept contacts that touch the lower sphere of the capsule
						break;
					default:
						break;
					}
					physicsobject.character = new Character(
						&character_settings,
						physicsobject.initial_position,
						physicsobject.initial_rotation,
						(uint64_t)&physicsobject,
						&physics_scene.physics_system
					);
					physicsobject.bodyID = physicsobject.character->GetBodyID();
					physicsobject.character->AddToPhysicsSystem(activation);
					return;
				}

				BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking version because this is called from job system!
				Body* body = body_interface.CreateBody(settings);
				if (body == nullptr || body->GetID().IsInvalid())
				{
					wi::backlog::post("AddRigidBody failed: body couldn't be created! This could mean that there are too many physics objects.", wi::backlog::LogLevel::Error);
					return;
				}
				physicsobject.bodyID = body->GetID();
				body_interface.AddBody(physicsobject.bodyID, activation);

				body->SetUserData((uint64_t)&physicsobject);

				// Vehicle const settings:
				static constexpr bool	sAntiRollbar = true;
				static constexpr bool	sLimitedSlipDifferentials = true;
				static constexpr float	sFrontCasterAngle = 0.0f;
				static constexpr float	sFrontKingPinAngle = 0.0f;
				static constexpr float	sFrontCamber = 0.0f;
				static constexpr float	sFrontToe = 0.0f;
				static constexpr float	sFrontSuspensionForwardAngle = 0.0f;
				static constexpr float	sFrontSuspensionSidewaysAngle = 0.0f;
				static constexpr float	sRearSuspensionForwardAngle = 0.0f;
				static constexpr float	sRearSuspensionSidewaysAngle = 0.0f;
				static constexpr float	sRearCasterAngle = 0.0f;
				static constexpr float	sRearKingPinAngle = 0.0f;
				static constexpr float	sRearCamber = 0.0f;
				static constexpr float	sRearToe = 0.0f;

				if (physicscomponent.IsCar())
				{
					const float wheel_radius = physicscomponent.vehicle.wheel_radius;
					const float wheel_width = physicscomponent.vehicle.wheel_width;
					const float half_vehicle_length = physicscomponent.vehicle.chassis_half_length;
					const float half_vehicle_width = physicscomponent.vehicle.chassis_half_width;
					const float half_vehicle_height = physicscomponent.vehicle.chassis_half_height;
					const float front_wheel_offset = physicscomponent.vehicle.front_wheel_offset;
					const float rear_wheel_offset = physicscomponent.vehicle.rear_wheel_offset;
					const bool four_wheel_drive = physicscomponent.vehicle.car.four_wheel_drive;
					const float max_engine_torque = physicscomponent.vehicle.max_engine_torque;
					const float clutch_strength = physicscomponent.vehicle.clutch_strength;

					const float	sFrontSuspensionMinLength = physicscomponent.vehicle.front_suspension.min_length;
					const float	sFrontSuspensionMaxLength = physicscomponent.vehicle.front_suspension.max_length;
					const float	sFrontSuspensionFrequency = physicscomponent.vehicle.front_suspension.frequency;
					const float	sFrontSuspensionDamping = physicscomponent.vehicle.front_suspension.damping;

					const float	sRearSuspensionMinLength = physicscomponent.vehicle.rear_suspension.min_length;
					const float	sRearSuspensionMaxLength = physicscomponent.vehicle.rear_suspension.max_length;
					const float	sRearSuspensionFrequency = physicscomponent.vehicle.rear_suspension.frequency;
					const float	sRearSuspensionDamping = physicscomponent.vehicle.rear_suspension.damping;

					const float	sMaxRollAngle = physicscomponent.vehicle.max_roll_angle;
					const float	sMaxSteeringAngle = physicscomponent.vehicle.max_steering_angle;

					// Create collision testers
					VehicleCollisionTester* vehicle_tester = nullptr;
					switch (physicscomponent.vehicle.collision_mode)
					{
					default:
					case RigidBodyPhysicsComponent::Vehicle::CollisionMode::Ray:
						vehicle_tester = new VehicleCollisionTesterRay(Layers::MOVING);
						break;
					case RigidBodyPhysicsComponent::Vehicle::CollisionMode::Sphere:
						vehicle_tester = new VehicleCollisionTesterCastSphere(Layers::MOVING, 0.5f * wheel_width);
						break;
					case RigidBodyPhysicsComponent::Vehicle::CollisionMode::Cylinder:
						vehicle_tester = new VehicleCollisionTesterCastCylinder(Layers::MOVING);
						break;
					}

					// Create vehicle constraint
					VehicleConstraintSettings vehicle;
					vehicle.mDrawConstraintSize = 0.1f;
					vehicle.mMaxPitchRollAngle = sMaxRollAngle;

					// Suspension direction
					Vec3 front_suspension_dir = Vec3(Tan(sFrontSuspensionSidewaysAngle), -1, Tan(sFrontSuspensionForwardAngle)).Normalized();
					Vec3 front_steering_axis = Vec3(-Tan(sFrontKingPinAngle), 1, -Tan(sFrontCasterAngle)).Normalized();
					Vec3 front_wheel_up = Vec3(Sin(sFrontCamber), Cos(sFrontCamber), 0);
					Vec3 front_wheel_forward = Vec3(-Sin(sFrontToe), 0, Cos(sFrontToe));
					Vec3 rear_suspension_dir = Vec3(Tan(sRearSuspensionSidewaysAngle), -1, Tan(sRearSuspensionForwardAngle)).Normalized();
					Vec3 rear_steering_axis = Vec3(-Tan(sRearKingPinAngle), 1, -Tan(sRearCasterAngle)).Normalized();
					Vec3 rear_wheel_up = Vec3(Sin(sRearCamber), Cos(sRearCamber), 0);
					Vec3 rear_wheel_forward = Vec3(-Sin(sRearToe), 0, Cos(sRearToe));
					Vec3 flip_x(-1, 1, 1);

					// Wheels, left front
					WheelSettingsWV* w1 = new WheelSettingsWV;
					w1->mPosition = Vec3(-half_vehicle_width, half_vehicle_height, half_vehicle_length + front_wheel_offset);
					w1->mSuspensionDirection = front_suspension_dir;
					w1->mSteeringAxis = front_steering_axis;
					w1->mWheelUp = front_wheel_up;
					w1->mWheelForward = front_wheel_forward;
					w1->mSuspensionMinLength = sFrontSuspensionMinLength;
					w1->mSuspensionMaxLength = sFrontSuspensionMaxLength;
					w1->mSuspensionSpring.mFrequency = sFrontSuspensionFrequency;
					w1->mSuspensionSpring.mDamping = sFrontSuspensionDamping;
					w1->mMaxSteerAngle = sMaxSteeringAngle;
					w1->mMaxHandBrakeTorque = 0.0f; // Front wheel doesn't have hand brake

					// Right front
					WheelSettingsWV* w2 = new WheelSettingsWV;
					w2->mPosition = Vec3(half_vehicle_width, half_vehicle_height, half_vehicle_length + front_wheel_offset);
					w2->mSuspensionDirection = flip_x * front_suspension_dir;
					w2->mSteeringAxis = flip_x * front_steering_axis;
					w2->mWheelUp = flip_x * front_wheel_up;
					w2->mWheelForward = flip_x * front_wheel_forward;
					w2->mSuspensionMinLength = sFrontSuspensionMinLength;
					w2->mSuspensionMaxLength = sFrontSuspensionMaxLength;
					w2->mSuspensionSpring.mFrequency = sFrontSuspensionFrequency;
					w2->mSuspensionSpring.mDamping = sFrontSuspensionDamping;
					w2->mMaxSteerAngle = sMaxSteeringAngle;
					w2->mMaxHandBrakeTorque = 0.0f; // Front wheel doesn't have hand brake

					// Left rear
					WheelSettingsWV* w3 = new WheelSettingsWV;
					w3->mPosition = Vec3(-half_vehicle_width, half_vehicle_height, -half_vehicle_length + rear_wheel_offset);
					w3->mSuspensionDirection = rear_suspension_dir;
					w3->mSteeringAxis = rear_steering_axis;
					w3->mWheelUp = rear_wheel_up;
					w3->mWheelForward = rear_wheel_forward;
					w3->mSuspensionMinLength = sRearSuspensionMinLength;
					w3->mSuspensionMaxLength = sRearSuspensionMaxLength;
					w3->mSuspensionSpring.mFrequency = sRearSuspensionFrequency;
					w3->mSuspensionSpring.mDamping = sRearSuspensionDamping;
					w3->mMaxSteerAngle = 0.0f;

					// Right rear
					WheelSettingsWV* w4 = new WheelSettingsWV;
					w4->mPosition = Vec3(half_vehicle_width, half_vehicle_height, -half_vehicle_length + rear_wheel_offset);
					w4->mSuspensionDirection = flip_x * rear_suspension_dir;
					w4->mSteeringAxis = flip_x * rear_steering_axis;
					w4->mWheelUp = flip_x * rear_wheel_up;
					w4->mWheelForward = flip_x * rear_wheel_forward;
					w4->mSuspensionMinLength = sRearSuspensionMinLength;
					w4->mSuspensionMaxLength = sRearSuspensionMaxLength;
					w4->mSuspensionSpring.mFrequency = sRearSuspensionFrequency;
					w4->mSuspensionSpring.mDamping = sRearSuspensionDamping;
					w4->mMaxSteerAngle = 0.0f;

					vehicle.mWheels = { w1, w2, w3, w4 };

					for (WheelSettings* w : vehicle.mWheels)
					{
						w->mRadius = wheel_radius;
						w->mWidth = wheel_width;
					}

					WheeledVehicleControllerSettings* controller_settings = new WheeledVehicleControllerSettings;

					controller_settings->mEngine.mMaxTorque = max_engine_torque;
					controller_settings->mTransmission.mClutchStrength = clutch_strength;

					// Set slip ratios to the same for everything
					float limited_slip_ratio = sLimitedSlipDifferentials ? 1.4f : FLT_MAX;
					controller_settings->mDifferentialLimitedSlipRatio = limited_slip_ratio;
					for (auto& diff : controller_settings->mDifferentials)
					{
						diff.mLimitedSlipRatio = limited_slip_ratio;
					}

					vehicle.mController = controller_settings;

					// Differential
					controller_settings->mDifferentials.resize(four_wheel_drive ? 2 : 1);
					controller_settings->mDifferentials[0].mLeftWheel = 0;
					controller_settings->mDifferentials[0].mRightWheel = 1;
					if (four_wheel_drive)
					{
						controller_settings->mDifferentials[1].mLeftWheel = 2;
						controller_settings->mDifferentials[1].mRightWheel = 3;

						// Split engine torque
						controller_settings->mDifferentials[0].mEngineTorqueRatio = controller_settings->mDifferentials[1].mEngineTorqueRatio = 0.5f;
					}

					// Anti rollbars
					if (sAntiRollbar)
					{
						vehicle.mAntiRollBars.resize(2);
						vehicle.mAntiRollBars[0].mLeftWheel = 0;
						vehicle.mAntiRollBars[0].mRightWheel = 1;
						vehicle.mAntiRollBars[1].mLeftWheel = 2;
						vehicle.mAntiRollBars[1].mRightWheel = 3;
					}

					physicsobject.vehicle_constraint = new VehicleConstraint(*body, vehicle);
					physicsobject.vehicle_constraint->SetVehicleCollisionTester(vehicle_tester);

					// The vehicle settings were tweaked with a buggy implementation of the longitudinal tire impulses, this meant that PhysicsSettings::mNumVelocitySteps times more impulse
					// could be applied than intended. To keep the behavior of the vehicle the same we increase the max longitudinal impulse by the same factor. In a future version the vehicle
					// will be retweaked.
					static_cast<WheeledVehicleController*>(physicsobject.vehicle_constraint->GetController())->SetTireMaxImpulseCallback([](uint, float& outLongitudinalImpulse, float& outLateralImpulse, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float, float, float) {
						outLongitudinalImpulse = 10.0f * inLongitudinalFriction * inSuspensionImpulse;
						outLateralImpulse = inLateralFriction * inSuspensionImpulse;
					});

					physics_scene.physics_system.AddConstraint(physicsobject.vehicle_constraint);
					physics_scene.physics_system.AddStepListener(physicsobject.vehicle_constraint);
				}
				else if (physicscomponent.IsMotorcycle())
				{
					const float max_engine_torque = physicscomponent.vehicle.max_engine_torque;
					const float clutch_strength = physicscomponent.vehicle.clutch_strength;

					const float back_wheel_radius = physicscomponent.vehicle.wheel_radius;
					const float back_wheel_width = physicscomponent.vehicle.wheel_width;
					const float back_wheel_pos_z = -physicscomponent.vehicle.chassis_half_length + physicscomponent.vehicle.rear_wheel_offset;
					const float back_suspension_min_length = physicscomponent.vehicle.rear_suspension.min_length;
					const float back_suspension_max_length = physicscomponent.vehicle.rear_suspension.max_length;
					const float back_suspension_freq = physicscomponent.vehicle.rear_suspension.frequency;
					const float back_brake_torque = physicscomponent.vehicle.motorcycle.rear_brake_torque;

					const float front_wheel_radius = physicscomponent.vehicle.wheel_radius;
					const float front_wheel_width = physicscomponent.vehicle.wheel_width;
					const float front_wheel_pos_z = physicscomponent.vehicle.chassis_half_length + physicscomponent.vehicle.front_wheel_offset;
					const float front_suspension_min_length = physicscomponent.vehicle.front_suspension.min_length;
					const float front_suspension_max_length = physicscomponent.vehicle.front_suspension.max_length;
					const float front_suspension_freq = physicscomponent.vehicle.front_suspension.frequency;
					const float front_brake_torque = physicscomponent.vehicle.motorcycle.front_brake_torque;
					const float half_vehicle_height = physicscomponent.vehicle.chassis_half_height;

					const float max_steering_angle = physicscomponent.vehicle.max_steering_angle;

					const float caster_angle = physicscomponent.vehicle.motorcycle.front_suspension_angle;

					const float	sMaxRollAngle = physicscomponent.vehicle.max_roll_angle;

					const bool sOverrideFrontSuspensionForcePoint = false;	///< If true, the front suspension force point is overridden
					const bool sOverrideRearSuspensionForcePoint = false;	///< If true, the rear suspension force point is overridden

					// Create collision testers
					VehicleCollisionTester* vehicle_tester = nullptr;
					switch (physicscomponent.vehicle.collision_mode)
					{
					default:
					case RigidBodyPhysicsComponent::Vehicle::CollisionMode::Ray:
						vehicle_tester = new VehicleCollisionTesterRay(Layers::MOVING);
						break;
					case RigidBodyPhysicsComponent::Vehicle::CollisionMode::Sphere:
						vehicle_tester = new VehicleCollisionTesterCastSphere(Layers::MOVING, 0.5f * std::max(back_wheel_radius, front_wheel_radius));
						break;
					case RigidBodyPhysicsComponent::Vehicle::CollisionMode::Cylinder:
						vehicle_tester = new VehicleCollisionTesterCastCylinder(Layers::MOVING);
						break;
					}

					// Create vehicle constraint
					VehicleConstraintSettings vehicle;
					vehicle.mDrawConstraintSize = 0.1f;
					vehicle.mMaxPitchRollAngle = sMaxRollAngle;

					// Wheels
					WheelSettingsWV* front = new WheelSettingsWV;
					front->mPosition = Vec3(0.0f, half_vehicle_height, front_wheel_pos_z);
					front->mMaxSteerAngle = max_steering_angle;
					front->mSuspensionDirection = Vec3(0, -1, Tan(caster_angle)).Normalized();
					front->mSteeringAxis = -front->mSuspensionDirection;
					front->mRadius = front_wheel_radius;
					front->mWidth = front_wheel_width;
					front->mSuspensionMinLength = front_suspension_min_length;
					front->mSuspensionMaxLength = front_suspension_max_length;
					front->mSuspensionSpring.mFrequency = front_suspension_freq;
					front->mMaxBrakeTorque = front_brake_torque;

					WheelSettingsWV* back = new WheelSettingsWV;
					back->mPosition = Vec3(0.0f, half_vehicle_height, back_wheel_pos_z);
					back->mMaxSteerAngle = 0.0f;
					back->mRadius = back_wheel_radius;
					back->mWidth = back_wheel_width;
					back->mSuspensionMinLength = back_suspension_min_length;
					back->mSuspensionMaxLength = back_suspension_max_length;
					back->mSuspensionSpring.mFrequency = back_suspension_freq;
					back->mMaxBrakeTorque = back_brake_torque;

					if (sOverrideFrontSuspensionForcePoint)
					{
						front->mEnableSuspensionForcePoint = true;
						front->mSuspensionForcePoint = front->mPosition + front->mSuspensionDirection * front->mSuspensionMinLength;
					}

					if (sOverrideRearSuspensionForcePoint)
					{
						back->mEnableSuspensionForcePoint = true;
						back->mSuspensionForcePoint = back->mPosition + back->mSuspensionDirection * back->mSuspensionMinLength;
					}

					vehicle.mWheels = { front, back };

					MotorcycleControllerSettings* controller = new MotorcycleControllerSettings;
					controller->mEngine.mMaxTorque = max_engine_torque;
					controller->mEngine.mMinRPM = 1000.0f;
					controller->mEngine.mMaxRPM = 10000.0f;
					controller->mTransmission.mShiftDownRPM = 2000.0f;
					controller->mTransmission.mShiftUpRPM = 8000.0f;
					controller->mTransmission.mGearRatios = { 2.27f, 1.63f, 1.3f, 1.09f, 0.96f, 0.88f }; // From: https://www.blocklayer.com/rpm-gear-bikes
					controller->mTransmission.mReverseGearRatios = { -4.0f };
					controller->mTransmission.mClutchStrength = clutch_strength;
					//controller->mMaxLeanAngle = sMaxRollAngle;
					vehicle.mController = controller;

					// Differential (not really applicable to a motorcycle but we need one anyway to drive it)
					controller->mDifferentials.resize(1);
					controller->mDifferentials[0].mLeftWheel = -1;
					controller->mDifferentials[0].mRightWheel = 1;
					controller->mDifferentials[0].mDifferentialRatio = 1.93f * 40.0f / 16.0f; // Combining primary and final drive (back divided by front sprockets) from: https://www.blocklayer.com/rpm-gear-bikes

					physicsobject.vehicle_constraint = new VehicleConstraint(*body, vehicle);
					physicsobject.vehicle_constraint->SetVehicleCollisionTester(vehicle_tester);
					physics_scene.physics_system.AddConstraint(physicsobject.vehicle_constraint);
					physics_scene.physics_system.AddStepListener(physicsobject.vehicle_constraint);
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

			physicsobject.friction = settings.mFriction;
			physicsobject.restitution = settings.mRestitution;

			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface(); // locking version because this is called from job system!

			physicsobject.bodyID = body_interface.CreateAndAddSoftBody(settings, EActivation::Activate);

			if (physicsobject.bodyID.IsInvalid())
			{
				wi::backlog::post("AddSoftBody failed: body couldn't be created! This could mean that there are too many physics objects.", wi::backlog::LogLevel::Error);
				return;
			}
		}
		void AddConstraint(
			wi::scene::Scene& scene,
			Entity entity,
			wi::scene::PhysicsConstraintComponent& physicscomponent,
			const wi::scene::TransformComponent& transform
		)
		{
			Constraint& physicsobject = GetConstraint(physicscomponent);
			physicsobject.physics_scene = scene.physics_scene;
			PhysicsScene& physics_scene = GetPhysicsScene(scene);
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterface();

			Body* body1 = nullptr;
			Body* body2 = nullptr;

			const RigidBodyPhysicsComponent* rigidbodyA = scene.rigidbodies.GetComponent(physicscomponent.bodyA);
			if (rigidbodyA == nullptr || rigidbodyA->physicsobject == nullptr)
			{
				body1 = body_interface.CreateBody(BodyCreationSettings(new SphereShape(0.01f), cast(transform.GetPosition()), cast(transform.GetRotation()).Normalized(), EMotionType::Kinematic, Layers::GHOST));
				physicsobject.body1_self = body1->GetID();
				body_interface.AddBody(physicsobject.body1_self, EActivation::Activate);
				physicsobject.body1_id = physicsobject.body1_self;
			}
			else
			{
				const RigidBody& rb = GetRigidBody(*rigidbodyA);
				BodyLockWrite lock(physics_scene.physics_system.GetBodyLockInterface(), rb.bodyID);
				if (!lock.Succeeded())
				{
					wilog_error("AddConstraint error: bodyA lock did not succeed!");
					return;
				}
				Body& body = lock.GetBody();
				body1 = &body;
				physicsobject.body1_ref = rb.bodyID;
				physicsobject.body1_id = rb.bodyID;
			}

			const RigidBodyPhysicsComponent* rigidbodyB = scene.rigidbodies.GetComponent(physicscomponent.bodyB);
			if (rigidbodyB == nullptr || rigidbodyB->physicsobject == nullptr)
			{
				body2 = body_interface.CreateBody(BodyCreationSettings(new SphereShape(0.01f), cast(transform.GetPosition()), cast(transform.GetRotation()).Normalized(), EMotionType::Kinematic, Layers::GHOST));
				physicsobject.body2_self = body2->GetID();
				body_interface.AddBody(physicsobject.body2_self, EActivation::Activate);
				physicsobject.body2_id = physicsobject.body2_self;
			}
			else
			{
				const RigidBody& rb = GetRigidBody(*rigidbodyB);
				BodyLockWrite lock(physics_scene.physics_system.GetBodyLockInterface(), rb.bodyID);
				if (!lock.Succeeded())
				{
					wilog_error("AddConstraint error: bodyB lock did not succeed!");
					return;
				}
				Body& body = lock.GetBody();
				body2 = &body;
				physicsobject.body2_ref = rb.bodyID;
				physicsobject.body2_id = rb.bodyID;
			}

			if (physicscomponent.type == PhysicsConstraintComponent::Type::Fixed)
			{
				FixedConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPoint1 = settings.mPoint2 = cast(transform.GetPosition());
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else if (physicscomponent.type == PhysicsConstraintComponent::Type::Point)
			{
				PointConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPoint1 = settings.mPoint2 = cast(transform.GetPosition());
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else if (physicscomponent.type == PhysicsConstraintComponent::Type::Distance)
			{
				DistanceConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPoint1 = settings.mPoint2 = cast(transform.GetPosition());
				settings.mMinDistance = physicscomponent.distance_constraint.min_distance;
				settings.mMinDistance = physicscomponent.distance_constraint.max_distance;
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else if (physicscomponent.type == PhysicsConstraintComponent::Type::Hinge)
			{
				HingeConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPoint1 = settings.mPoint2 = cast(transform.GetPosition());
				settings.mHingeAxis1 = settings.mHingeAxis2 = cast(transform.GetUp()).Normalized();
				settings.mNormalAxis1 = settings.mNormalAxis2 = cast(transform.GetRight()).Normalized();
				settings.mLimitsMin = physicscomponent.hinge_constraint.min_angle;
				settings.mLimitsMax = physicscomponent.hinge_constraint.max_angle;
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else if (physicscomponent.type == PhysicsConstraintComponent::Type::Cone)
			{
				ConeConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPoint1 = settings.mPoint2 = cast(transform.GetPosition());
				settings.mTwistAxis1 = settings.mTwistAxis2 = cast(transform.GetRight()).Normalized();
				settings.mHalfConeAngle = physicscomponent.cone_constraint.half_cone_angle;
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else if (physicscomponent.type == PhysicsConstraintComponent::Type::SixDOF)
			{
				SixDOFConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPosition1 = settings.mPosition2 = cast(transform.GetPosition());
				settings.mAxisX1 = settings.mAxisX2 = cast(transform.GetRight()).Normalized();
				settings.mAxisY1 = settings.mAxisY2 = cast(transform.GetUp()).Normalized();
				settings.mLimitMin[SixDOFConstraintSettings::EAxis::TranslationX] = physicscomponent.six_dof.minTranslationAxes.x;
				settings.mLimitMin[SixDOFConstraintSettings::EAxis::TranslationY] = physicscomponent.six_dof.minTranslationAxes.y;
				settings.mLimitMin[SixDOFConstraintSettings::EAxis::TranslationZ] = physicscomponent.six_dof.minTranslationAxes.z;
				settings.mLimitMax[SixDOFConstraintSettings::EAxis::TranslationX] = physicscomponent.six_dof.maxTranslationAxes.x;
				settings.mLimitMax[SixDOFConstraintSettings::EAxis::TranslationY] = physicscomponent.six_dof.maxTranslationAxes.y;
				settings.mLimitMax[SixDOFConstraintSettings::EAxis::TranslationZ] = physicscomponent.six_dof.maxTranslationAxes.z;
				settings.mLimitMin[SixDOFConstraintSettings::EAxis::RotationX] = physicscomponent.six_dof.minRotationAxes.x;
				settings.mLimitMin[SixDOFConstraintSettings::EAxis::RotationY] = physicscomponent.six_dof.minRotationAxes.y;
				settings.mLimitMin[SixDOFConstraintSettings::EAxis::RotationZ] = physicscomponent.six_dof.minRotationAxes.z;
				settings.mLimitMax[SixDOFConstraintSettings::EAxis::RotationX] = physicscomponent.six_dof.maxRotationAxes.x;
				settings.mLimitMax[SixDOFConstraintSettings::EAxis::RotationY] = physicscomponent.six_dof.maxRotationAxes.y;
				settings.mLimitMax[SixDOFConstraintSettings::EAxis::RotationZ] = physicscomponent.six_dof.maxRotationAxes.z;
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else if (physicscomponent.type == PhysicsConstraintComponent::Type::SwingTwist)
			{
				SwingTwistConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPosition1 = settings.mPosition2 = cast(transform.GetPosition());
				settings.mTwistAxis1 = settings.mTwistAxis2 = cast(transform.GetRight()).Normalized();
				settings.mPlaneAxis1 = settings.mPlaneAxis2 = cast(transform.GetUp()).Normalized();
				settings.mNormalHalfConeAngle = physicscomponent.swing_twist.normal_half_cone_angle;
				settings.mPlaneHalfConeAngle = physicscomponent.swing_twist.plane_half_cone_angle;
				settings.mTwistMinAngle = physicscomponent.swing_twist.min_twist_angle;
				settings.mTwistMaxAngle = physicscomponent.swing_twist.max_twist_angle;
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else if (physicscomponent.type == PhysicsConstraintComponent::Type::Slider)
			{
				SliderConstraintSettings settings;
				settings.mSpace = EConstraintSpace::WorldSpace;
				settings.mPoint1 = settings.mPoint2 = cast(transform.GetPosition());
				settings.mSliderAxis1 = settings.mSliderAxis2 = cast(transform.GetRight()).Normalized();
				settings.mNormalAxis1 = settings.mNormalAxis2 = cast(transform.GetUp()).Normalized();
				physicsobject.constraint = settings.Create(*body1, *body2);
			}
			else
			{
				wilog("Constraint creation failed: constraint type is not valid!");
				return;
			}

			if (physicsobject.constraint == nullptr)
			{
				wilog("Constraint creation failed: constraint is not valid!");
				return;
			}

			if (physicscomponent.IsDisableSelfCollision())
			{
				uint32_t groupID = collisionGroupID.fetch_add(1);
				body1->GetCollisionGroup().SetGroupID(groupID);
				body1->GetCollisionGroup().SetSubGroupID(0);
				body2->GetCollisionGroup().SetGroupID(groupID);
				body2->GetCollisionGroup().SetSubGroupID(1);

				Ref<GroupFilterTable> group_filter = new GroupFilterTable(2);
				group_filter->DisableCollision(0, 1);
				body1->GetCollisionGroup().SetGroupFilter(group_filter);
				body2->GetCollisionGroup().SetGroupFilter(group_filter);
			}
			else
			{
				body1->GetCollisionGroup().SetGroupFilter(nullptr);
				body2->GetCollisionGroup().SetGroupFilter(nullptr);
			}

			physicsobject.bind_distance = (body1->GetCenterOfMassPosition() - body2->GetCenterOfMassPosition()).Length();

			physics_scene.physics_system.AddConstraint(physicsobject.constraint);
			physicscomponent.SetRefreshParametersNeeded(true); // motors will be refreshed
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

				BODYPART_LEFT_FOOT,
				BODYPART_RIGHT_FOOT,

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
			Vec3 prev_capsule_position[BODYPART_COUNT] = {};
			Quat prev_capsule_rotation[BODYPART_COUNT] = {};

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
				const float facing = humanoid.default_facing;

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
					case BODYPART_LEFT_FOOT:
						humanoid_bone = HumanoidComponent::HumanoidBone::LeftFoot;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftFoot];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftToes];
						break;
					case BODYPART_RIGHT_FOOT:
						humanoid_bone = HumanoidComponent::HumanoidBone::RightFoot;
						entityA = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightFoot];
						entityB = humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightToes];
						break;
					}
					if (entityA == INVALID_ENTITY)
					{
						wilog_warning("Ragdoll creation aborted because of a missing body part: %d", c);
						return;
					}

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
						case BODYPART_LEFT_FOOT:
						case BODYPART_RIGHT_FOOT:
							capsule_radius = capsule_height * 0.3f * humanoid.ragdoll_fatness;
							break;
						default:
							capsule_radius = capsule_height * 0.2f * humanoid.ragdoll_fatness;
							capsule_height -= capsule_radius * 2;
							break;
						}
					}

					capsule_radius = std::abs(capsule_radius);
					capsule_height = std::abs(capsule_height);

					ShapeSettings::ShapeResult shape_result;
					CapsuleShapeSettings capsule_settings(capsule_height * 0.5f, capsule_radius);
					capsule_settings.SetEmbedded();
					BoxShapeSettings box_settings(Vec3(capsule_radius, capsule_radius * 0.75f, capsule_height * 0.5f + capsule_radius), capsule_radius * 0.1f);
					box_settings.SetEmbedded();

					switch (c)
					{
					case BODYPART_LEFT_FOOT:
					case BODYPART_RIGHT_FOOT:
						shape_result = box_settings.Create();
						break;
					default:
						shape_result = capsule_settings.Create();
						break;
					}

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
						physicsobject.capsule = wi::primitive::Capsule(XMFLOAT3(-capsule_height * 0.5f - capsule_radius, 0, 0), XMFLOAT3(capsule_height * 0.5f + capsule_radius, 0, 0), capsule_radius);
						rtshape_settings.mRotation = Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI).Normalized();
						break;
					case BODYPART_LEFT_FOOT:
					case BODYPART_RIGHT_FOOT:
						physicsobject.capsule = wi::primitive::Capsule(XMFLOAT3(0, 0, -capsule_height * 0.5f - capsule_radius), XMFLOAT3(0, 0, capsule_height * 0.5f + capsule_radius), capsule_radius);
						break;
					default:
						physicsobject.capsule = wi::primitive::Capsule(XMFLOAT3(0, -capsule_height * 0.5f - capsule_radius, 0), XMFLOAT3(0, capsule_height * 0.5f + capsule_radius, 0), capsule_radius);
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
					case BODYPART_LEFT_FOOT:
					case BODYPART_RIGHT_FOOT:
						local_offset = Vec3(0, XMVectorGetY(rootB - rootA), capsule_height * 0.5f * facing);
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
					physicsobject.initial_position = physicsobject.prev_position;
					physicsobject.initial_rotation = physicsobject.prev_rotation;
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
				bodyparts[BODYPART_LEFT_FOOT] = skeleton.AddJoint("FootL", bodyparts[BODYPART_LEFT_LOWER_LEG]);
				bodyparts[BODYPART_RIGHT_FOOT] = skeleton.AddJoint("FootR", bodyparts[BODYPART_RIGHT_LOWER_LEG]);

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
					20.0f,		// Foot L
					20.0f,		// Foot R
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
					20.0f,		// Foot L
					20.0f,		// Foot R
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
					20.0f,		// Foot L
					20.0f,		// Foot R
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
					if (p == 0)
						continue;

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
						if (p == BODYPART_LEFT_FOOT || p == BODYPART_RIGHT_FOOT)
						{
							constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = Vec3::sAxisX() * facing;
						}
						else
						{
							constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = Vec3::sAxisZ() * facing;
						}
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

				settings.Stabilize();
				settings.DisableParentChildCollisions();
				settings.CalculateBodyIndexToConstraintIndex();

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

		wilog("wi::physics Initialized [Jolt Physics %d.%d.%d] (%d ms)", JPH_VERSION_MAJOR, JPH_VERSION_MINOR, JPH_VERSION_PATCH, (int)std::round(timer.elapsed()));
	}

	void CreateRigidBodyShape(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& scale_local,
		const wi::scene::MeshComponent* mesh
	)
	{
		ShapeSettings::ShapeResult shape_result;

		// The default convex radius caused issues when creating small box shape, etc, so I decrease it:
		const float convexRadius = 0.001f;

		Vec3 bottom_offset = Vec3::sZero();

		switch (physicscomponent.shape)
		{
		case RigidBodyPhysicsComponent::CollisionShape::BOX:
		{
			BoxShapeSettings settings(Vec3(physicscomponent.box.halfextents.x * scale_local.x, physicscomponent.box.halfextents.y * scale_local.y, physicscomponent.box.halfextents.z * scale_local.z), convexRadius);
			settings.SetEmbedded();
			shape_result = settings.Create();
			bottom_offset = Vec3(0, settings.mHalfExtent.GetY(), 0);
		}
		break;
		case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
		{
			SphereShapeSettings settings(physicscomponent.sphere.radius * scale_local.x);
			settings.SetEmbedded();
			shape_result = settings.Create();
			bottom_offset = Vec3(0, settings.mRadius, 0);
		}
		break;
		case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
		{
			CapsuleShapeSettings settings(physicscomponent.capsule.height * scale_local.y, physicscomponent.capsule.radius * scale_local.x);
			settings.SetEmbedded();
			shape_result = settings.Create();
			bottom_offset = Vec3(0, settings.mHalfHeightOfCylinder + settings.mRadius, 0);
		}
		break;
		case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
		{
			CylinderShapeSettings settings(physicscomponent.capsule.height * scale_local.y, physicscomponent.capsule.radius * scale_local.x, convexRadius);
			settings.SetEmbedded();
			shape_result = settings.Create();
			bottom_offset = Vec3(0, settings.mHalfHeight, 0);
		}
		break;

		case RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL:
			if (mesh != nullptr)
			{
				Array<Vec3> points;
				points.reserve(mesh->vertex_positions.size());
				for (auto& pos : mesh->vertex_positions)
				{
					points.push_back(Vec3(pos.x * scale_local.x, pos.y * scale_local.y, pos.z * scale_local.z));
				}
				ConvexHullShapeSettings settings(points, convexRadius);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			else
			{
				wi::backlog::post("CreateRigidBodyShape failed: convex hull physics requested, but no MeshComponent provided!", wi::backlog::LogLevel::Error);
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
						triangle.mV[0] = Float3(mesh->vertex_positions[indices[i + 0]].x * scale_local.x, mesh->vertex_positions[indices[i + 0]].y * scale_local.y, mesh->vertex_positions[indices[i + 0]].z * scale_local.z);
						triangle.mV[2] = Float3(mesh->vertex_positions[indices[i + 1]].x * scale_local.x, mesh->vertex_positions[indices[i + 1]].y * scale_local.y, mesh->vertex_positions[indices[i + 1]].z * scale_local.z);
						triangle.mV[1] = Float3(mesh->vertex_positions[indices[i + 2]].x * scale_local.x, mesh->vertex_positions[indices[i + 2]].y * scale_local.y, mesh->vertex_positions[indices[i + 2]].z * scale_local.z);
						trianglelist.push_back(triangle);
					}
				}

				MeshShapeSettings settings(trianglelist);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			else
			{
				wi::backlog::post("CreateRigidBodyShape failed: triangle mesh physics requested, but no MeshComponent provided!", wi::backlog::LogLevel::Error);
				return;
			}
			break;

		case RigidBodyPhysicsComponent::CollisionShape::HEIGHTFIELD:
			if (mesh != nullptr)
			{
				wi::vector<float> heights;
				heights.reserve(mesh->vertex_positions.size());

				wi::primitive::AABB aabb;

				for (XMFLOAT3 pos : mesh->vertex_positions)
				{
					pos.x *= scale_local.x;
					pos.y *= scale_local.y;
					pos.z *= scale_local.z;
					heights.push_back(pos.y);
					aabb.AddPoint(pos);
				}

				XMFLOAT3 min = aabb.getMin();
				min.y = 0;

				uint32_t dim = (uint32_t)std::sqrt((double)heights.size());
				wilog_assert(dim >= 2, "Height field shape dimension must be at least 2!");

				Vec3 scale = cast(aabb.getHalfWidth()) * 2 / (float)(dim - 1);
				scale.SetY(1);

				HeightFieldShapeSettings settings(heights.data(), cast(min), scale, dim);
				settings.SetEmbedded();
				shape_result = settings.Create();
			}
			else
			{
				wi::backlog::post("CreateRigidBodyShape failed: height field physics requested, but no MeshComponent provided!", wi::backlog::LogLevel::Error);
				return;
			}
			break;

		}

		if (!shape_result.IsValid())
		{
			wilog_error("CreateRigidBodyShape failed, shape_result: %s", shape_result.GetError().c_str());
			return;
		}

		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		physicsobject.shape = shape_result.Get();

		if (physicscomponent.IsVehicle())
		{
			// Vehicle center of mass will be offset to chassis height to improve handling:
			physicsobject.shape = OffsetCenterOfMassShapeSettings(Vec3(0, -physicsobject.shape->GetLocalBounds().GetExtent().GetY() + physicscomponent.vehicle.chassis_half_height, 0), physicsobject.shape).Create().Get();
		}

		if (physicscomponent.IsCharacterPhysics())
		{
			physicsobject.shape = RotatedTranslatedShapeSettings(bottom_offset, Quat::sIdentity(), physicsobject.shape).Create().Get();
		}
	}

	bool IsEnabled() { return ENABLED; }
	void SetEnabled(bool value) { ENABLED = value; }

	bool IsSimulationEnabled() { return ENABLED && SIMULATION_ENABLED; }
	void SetSimulationEnabled(bool value) { SIMULATION_ENABLED = value; }

	bool IsInterpolationEnabled() { return INTERPOLATION; }
	void SetInterpolationEnabled(bool value) { INTERPOLATION = value; }

	bool IsDebugDrawEnabled() { return DEBUGDRAW_ENABLED; }
	void SetDebugDrawEnabled(bool value) { DEBUGDRAW_ENABLED = value; }

	void SetConstraintDebugSize(float value) { CONSTRAINT_DEBUGSIZE = value; }
	float GetConstraintDebugSize() { return CONSTRAINT_DEBUGSIZE; }

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

		// TODO: without this there are bugs in terrain physics generation
		scene.RunHierarchyUpdateSystem(ctx);
		wi::jobsystem::Wait(ctx);

		auto range = wi::profiler::BeginRangeCPU("Physics");

		PhysicsScene& physics_scene = GetPhysicsScene(scene);
		physics_scene.physics_system.SetGravity(cast(scene.weather.gravity));

		// First, do the creations when needed (AddRigidBody, AddSoftBody, etc):
		//	These will be locking updates, but doesn't need to be performed frequently
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), dispatchGroupSize, [&scene](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);

			if ((physicscomponent.physicsobject == nullptr || physicscomponent.IsRefreshParametersNeeded()) && scene.transforms.Contains(entity))
			{
				physicscomponent.SetRefreshParametersNeeded(false);
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
		});
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.softbodies.GetCount(), 1, [&scene](wi::jobsystem::JobArgs args) {

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
		});
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.humanoids.GetCount(), 1, [&scene](wi::jobsystem::JobArgs args) {
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
		});

		wi::jobsystem::Wait(ctx); // wait for rigidbody creations
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.constraints.GetCount(), dispatchGroupSize, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			PhysicsConstraintComponent& physicscomponent = scene.constraints[args.jobIndex];
			if (physicscomponent.bodyA == INVALID_ENTITY && physicscomponent.bodyB == INVALID_ENTITY)
			{
				physicscomponent.physicsobject = nullptr;
				return;
			}
			if (!scene.rigidbodies.Contains(physicscomponent.bodyA) && !scene.rigidbodies.Contains(physicscomponent.bodyB))
			{
				physicscomponent.physicsobject = nullptr;
				return;
			}
			if (physicscomponent.physicsobject != nullptr)
			{
				// Detection of deleted rigidbody references:
				const Constraint& constraint = GetConstraint((const PhysicsConstraintComponent&)physicscomponent);
				if (!constraint.body1_ref.IsInvalid())
				{
					const RigidBodyPhysicsComponent* rb = scene.rigidbodies.GetComponent(physicscomponent.bodyA);
					if (rb != nullptr && rb->physicsobject != nullptr)
					{
						const RigidBody& body = GetRigidBody(*rb);
						if (body.bodyID != constraint.body1_ref)
						{
							// Rigidbody to constraint object mismatch!
							physicscomponent.physicsobject = nullptr;
							return;
						}
					}
				}
				if (!constraint.body2_ref.IsInvalid())
				{
					const RigidBodyPhysicsComponent* rb = scene.rigidbodies.GetComponent(physicscomponent.bodyB);
					if (rb != nullptr && rb->physicsobject != nullptr)
					{
						const RigidBody& body = GetRigidBody(*rb);
						if (body.bodyID != constraint.body2_ref)
						{
							// Rigidbody to constraint object mismatch!
							physicscomponent.physicsobject = nullptr;
							return;
						}
					}
				}
			}

			Entity entity = scene.constraints.GetEntity(args.jobIndex);
			if (physicscomponent.physicsobject == nullptr && scene.transforms.Contains(entity))
			{
				TransformComponent* transform = scene.transforms.GetComponent(entity);
				if (transform == nullptr)
					return;
				AddConstraint(scene, entity, physicscomponent, *transform);
			}

			if (physicscomponent.physicsobject != nullptr)
			{
				Constraint& constraint = GetConstraint(physicscomponent);
				if (physicscomponent.IsRefreshParametersNeeded())
				{
					physicscomponent.SetRefreshParametersNeeded(false);
					if (physicscomponent.type == PhysicsConstraintComponent::Type::Fixed)
					{
					}
					else if (physicscomponent.type == PhysicsConstraintComponent::Type::Point)
					{
					}
					else if (physicscomponent.type == PhysicsConstraintComponent::Type::Distance)
					{
						DistanceConstraint* ptr = ((DistanceConstraint*)constraint.constraint.GetPtr());
						ptr->SetDistance(physicscomponent.distance_constraint.min_distance, physicscomponent.distance_constraint.max_distance);
					}
					else if (physicscomponent.type == PhysicsConstraintComponent::Type::Hinge)
					{
						HingeConstraint* ptr = ((HingeConstraint*)constraint.constraint.GetPtr());
						ptr->SetLimits(physicscomponent.hinge_constraint.min_angle, physicscomponent.hinge_constraint.max_angle);

						if (physicscomponent.hinge_constraint.target_angular_velocity != 0.0f)
						{
							BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
							if (!constraint.body1_ref.IsInvalid())
							{
								body_interface.ActivateBody(constraint.body1_ref);
							}
							if (!constraint.body2_ref.IsInvalid())
							{
								body_interface.ActivateBody(constraint.body2_ref);
							}
							ptr->SetMotorState(EMotorState::Velocity);
							ptr->SetTargetAngularVelocity(physicscomponent.hinge_constraint.target_angular_velocity);
						}
						else
						{
							ptr->SetMotorState(EMotorState::Off);
						}
					}
					else if (physicscomponent.type == PhysicsConstraintComponent::Type::Cone)
					{
						ConeConstraint* ptr = ((ConeConstraint*)constraint.constraint.GetPtr());
						ptr->SetHalfConeAngle(physicscomponent.cone_constraint.half_cone_angle);
					}
					else if (physicscomponent.type == PhysicsConstraintComponent::Type::SixDOF)
					{
						SixDOFConstraint* ptr = ((SixDOFConstraint*)constraint.constraint.GetPtr());
						ptr->SetTranslationLimits(cast(physicscomponent.six_dof.minTranslationAxes), cast(physicscomponent.six_dof.maxTranslationAxes));
						ptr->SetRotationLimits(cast(physicscomponent.six_dof.minRotationAxes), cast(physicscomponent.six_dof.maxRotationAxes));
					}
					else if (physicscomponent.type == PhysicsConstraintComponent::Type::SwingTwist)
					{
						SwingTwistConstraint* ptr = ((SwingTwistConstraint*)constraint.constraint.GetPtr());
						ptr->SetNormalHalfConeAngle(physicscomponent.swing_twist.normal_half_cone_angle);
						ptr->SetPlaneHalfConeAngle(physicscomponent.swing_twist.plane_half_cone_angle);
						ptr->SetTwistMinAngle(physicscomponent.swing_twist.min_twist_angle);
						ptr->SetTwistMaxAngle(physicscomponent.swing_twist.max_twist_angle);
					}
					else if (physicscomponent.type == PhysicsConstraintComponent::Type::Slider)
					{
						SliderConstraint* ptr = ((SliderConstraint*)constraint.constraint.GetPtr());
						ptr->SetLimits(physicscomponent.slider_constraint.min_limit, physicscomponent.slider_constraint.max_limit);

						if (physicscomponent.slider_constraint.target_velocity != 0.0f)
						{
							BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
							if (!constraint.body1_ref.IsInvalid())
							{
								body_interface.ActivateBody(constraint.body1_ref);
							}
							if (!constraint.body2_ref.IsInvalid())
							{
								body_interface.ActivateBody(constraint.body2_ref);
							}
							ptr->SetMotorState(EMotorState(EMotorState::Velocity));
							ptr->SetTargetVelocity(physicscomponent.slider_constraint.target_velocity);
							auto& settings = ptr->GetMotorSettings();
							settings.mMaxForceLimit = physicscomponent.slider_constraint.max_force;
							settings.mMinForceLimit = -physicscomponent.slider_constraint.max_force;
						}
						else
						{
							ptr->SetMotorState(EMotorState::Off);
						}
					}
				}
				if (physicscomponent.break_distance < FLT_MAX)
				{
					BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
					const Vec3 pos1 = body_interface.GetCenterOfMassPosition(constraint.body1_id);
					const Vec3 pos2 = body_interface.GetCenterOfMassPosition(constraint.body2_id);
					float dist = (pos1 - pos2).Length();
					dist -= constraint.bind_distance;
					if (dist > physicscomponent.break_distance)
					{
						// break constraint:
						constraint.constraint->SetEnabled(false);
					}
				}
			}
		});

		wi::jobsystem::Wait(ctx); // wait for all creations

		// Now do the property updating
		//	These will be non-locking updates and perfromed potentially every frame
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), dispatchGroupSize, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			Entity entity = scene.rigidbodies.GetEntity(args.jobIndex);
			if (physicscomponent.physicsobject == nullptr)
				return;
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			if (physicsobject.bodyID.IsInvalid())
				return;

			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();

			if (physicsobject.friction != physicscomponent.friction)
			{
				physicsobject.friction = physicscomponent.friction;
				body_interface.SetFriction(physicsobject.bodyID, physicscomponent.friction);
			}
			if (physicsobject.restitution != physicscomponent.restitution)
			{
				physicsobject.restitution = physicscomponent.restitution;
				body_interface.SetRestitution(physicsobject.bodyID, physicscomponent.restitution);
			}

			const EMotionType prevMotionType = physicsobject.motiontype;
			const EMotionType currentMotionType = physicscomponent.mass == 0 ? EMotionType::Static : (physicscomponent.IsKinematic() ? EMotionType::Kinematic : EMotionType::Dynamic);

			if (prevMotionType != currentMotionType)
			{
				// Changed motion type:
				physicsobject.motiontype = currentMotionType;
				body_interface.SetMotionType(physicsobject.bodyID, currentMotionType, EActivation::DontActivate);

				if (currentMotionType == EMotionType::Dynamic)
				{
					// Changed to dynamic, remember attachment matrices at this point:
					scene.locker.lock();
					XMMATRIX parentMatrix = scene.ComputeParentMatrixRecursive(entity);
					scene.locker.unlock();
					XMStoreFloat4x4(&physicsobject.parentMatrix, parentMatrix);
					XMStoreFloat4x4(&physicsobject.parentMatrixInverse, XMMatrixInverse(nullptr, parentMatrix));
				}

				if (currentMotionType == EMotionType::Static)
				{
					body_interface.SetObjectLayer(physicsobject.bodyID, Layers::NON_MOVING);
				}
				else
				{
					body_interface.SetObjectLayer(physicsobject.bodyID, Layers::MOVING);
				}
			}

			TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (transform == nullptr)
				return;

			if (currentMotionType == EMotionType::Dynamic)
			{
				// Detaching object manually before the physics simulation:
				transform->MatrixTransform(physicsobject.parentMatrix);
			}

			if (physics_scene.activate_all_rigid_bodies)
			{
				body_interface.ActivateBody(physicsobject.bodyID);
			}

			const bool is_active = physicsobject.was_active_prev_frame;

			if (currentMotionType == EMotionType::Dynamic && is_active)
			{
				// Apply effects on dynamics if needed:
				if (scene.weather.IsOceanEnabled())
				{
					const Vec3 com = body_interface.GetCenterOfMassPosition(physicsobject.bodyID);
					const Vec3 surface_position = cast(scene.GetOceanPosAt(cast(com)));
					const float diff = com.GetY() - surface_position.GetY();
					if (diff < 0)
					{
						const Vec3 p2 = cast(scene.GetOceanPosAt(cast(com + Vec3(0, 0, 0.1f))));
						const Vec3 p3 = cast(scene.GetOceanPosAt(cast(com + Vec3(0.1f, 0, 0))));
						const Vec3 surface_normal = Vec3(p2 - surface_position).Cross(Vec3(p3 - surface_position)).Normalized();

						body_interface.ApplyBuoyancyImpulse(
							physicsobject.bodyID,
							surface_position,
							surface_normal,
							physicscomponent.buoyancy,
							0.8f,
							0.6f,
							Vec3::sZero(),
							physics_scene.physics_system.GetGravity(),
							scene.dt
						);

						if (!physicsobject.was_underwater)
						{
							physicsobject.was_underwater = true;
							scene.PutWaterRipple(cast(surface_position));
						}
					}
					else
					{
						physicsobject.was_underwater = false;
					}
				}
			}

			if (physicsobject.teleporting)
			{
				physicsobject.teleporting = false;
			}
			else
			{
				const Vec3 position = cast(transform->GetPosition());
				const Quat rotation = cast(transform->GetRotation());
				Mat44 m = Mat44::sTranslation(position) * Mat44::sRotation(rotation);
				m = m * physicsobject.additionalTransform;

				if (IsSimulationEnabled())
				{
					// Feedback system transform to kinematic and static physics objects:
					if (currentMotionType == EMotionType::Kinematic)
					{
						body_interface.MoveKinematic(
							physicsobject.bodyID,
							m.GetTranslation(),
							m.GetQuaternion().Normalized(),
							physics_scene.GetKinematicDT(scene.dt)
						);
					}
					else if (currentMotionType == EMotionType::Static || !is_active)
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
					physicsobject.prev_position = position;
					physicsobject.prev_rotation = rotation;
					body_interface.SetPositionAndRotation(
						physicsobject.bodyID,
						m.GetTranslation(),
						m.GetQuaternion().Normalized(),
						EActivation::DontActivate
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

			if (physicscomponent.physicsobject == nullptr)
				return;

			SoftBody& physicsobject = GetSoftBody(physicscomponent);
			if (physicsobject.bodyID.IsInvalid())
				return;

			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();

			if (physicsobject.friction != physicscomponent.friction)
			{
				physicsobject.friction = physicscomponent.friction;
				body_interface.SetFriction(physicsobject.bodyID, physicscomponent.friction);
			}
			if (physicsobject.restitution != physicscomponent.restitution)
			{
				physicsobject.restitution = physicscomponent.restitution;
				body_interface.SetRestitution(physicsobject.bodyID, physicscomponent.restitution);
			}

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
		});

		// Ragdoll management:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.humanoids.GetCount(), 1, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {
			HumanoidComponent& humanoid = scene.humanoids[args.jobIndex];
			if (humanoid.ragdoll == nullptr)
				return;
			Entity humanoidEntity = scene.humanoids.GetEntity(args.jobIndex);
			Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();

			if (humanoid.IsRagdollPhysicsEnabled())
			{
				ragdoll.Activate(scene, humanoidEntity);
			}

			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();

			if (IsSimulationEnabled())
			{
				if (humanoid.IsRagdollPhysicsEnabled())
				{
					// Apply effects on dynamics if needed:
					if (scene.weather.IsOceanEnabled())
					{
						static const Ragdoll::BODYPART floating_bodyparts[] = {
							Ragdoll::BODYPART_PELVIS,
							Ragdoll::BODYPART_SPINE,
						};
						for (auto& bodypart : floating_bodyparts)
						{
							auto& rb = ragdoll.rigidbodies[bodypart];
							const Vec3 com = body_interface.GetCenterOfMassPosition(rb.bodyID);
							const Vec3 surface_position = cast(scene.GetOceanPosAt(cast(com)));
							const float diff = com.GetY() - surface_position.GetY();
							if (diff < 0)
							{
								const Vec3 p2 = cast(scene.GetOceanPosAt(cast(com + Vec3(0, 0, 0.1f))));
								const Vec3 p3 = cast(scene.GetOceanPosAt(cast(com + Vec3(0.1f, 0, 0))));
								const Vec3 surface_normal = Vec3(p2 - surface_position).Cross(Vec3(p3 - surface_position)).Normalized();

								body_interface.ApplyBuoyancyImpulse(
									rb.bodyID,
									surface_position,
									surface_normal,
									6.0f,
									0.8f,
									0.6f,
									Vec3::sZero(),
									physics_scene.physics_system.GetGravity(),
									scene.dt
								);

								if (!rb.was_underwater)
								{
									rb.was_underwater = true;
									scene.PutWaterRipple(cast(surface_position));
								}
							}
							else
							{
								rb.was_underwater = false;
							}
						}
					}
				}
				else
				{
					ragdoll.Deactivate(scene);

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
				for (auto& rb : ragdoll.rigidbodies)
				{
					TransformComponent* transform = scene.transforms.GetComponent(rb.entity);
					if (transform == nullptr)
						return;

					const Vec3 position = cast(transform->GetPosition());
					const Quat rotation = cast(transform->GetRotation());
					Mat44 m = Mat44::sTranslation(position) * Mat44::sRotation(rotation);
					m = m * rb.additionalTransform;

					// Simulation is disabled, update physics state immediately:
					rb.prev_position = position;
					rb.prev_rotation = rotation;
					body_interface.SetPositionAndRotation(
						rb.bodyID,
						m.GetTranslation(),
						m.GetQuaternion().Normalized(),
						EActivation::Activate
					);
				}
			}
		});

		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.constraints.GetCount(), dispatchGroupSize, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			PhysicsConstraintComponent& physicscomponent = scene.constraints[args.jobIndex];
			if (physicscomponent.physicsobject == nullptr)
				return;
			Constraint& constraint = GetConstraint(physicscomponent);
			constraint.constraint->SetDrawConstraintSize(CONSTRAINT_DEBUGSIZE);
			if (constraint.body1_self.IsInvalid() && constraint.body2_self.IsInvalid())
				return;

			// Kinematic constraint body:
			Entity entity = scene.constraints.GetEntity(args.jobIndex);
			const TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (transform == nullptr)
				return;

			if (!constraint.body1_self.IsInvalid())
			{
				physics_scene.physics_system.GetBodyInterfaceNoLock().MoveKinematic(
					constraint.body1_self,
					cast(transform->GetPosition()),
					cast(transform->GetRotation()),
					physics_scene.GetKinematicDT(scene.dt)
				);
			}
			if (!constraint.body2_self.IsInvalid())
			{
				physics_scene.physics_system.GetBodyInterfaceNoLock().MoveKinematic(
					constraint.body2_self,
					cast(transform->GetPosition()),
					cast(transform->GetRotation()),
					physics_scene.GetKinematicDT(scene.dt)
				);
			}
		});

		wi::jobsystem::Wait(ctx);

		physics_scene.activate_all_rigid_bodies = false;
		
		// Perform internal simulation step:
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
					wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), dispatchGroupSize, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {
						RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
						if (physicscomponent.physicsobject == nullptr)
							return;
						RigidBody& rb = GetRigidBody(physicscomponent);
						if (rb.character != nullptr)
						{
							rb.character->PostSimulation(CHARACTER_COLLISION_TOLERANCE);
						}
						BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
						Mat44 mat = body_interface.GetWorldTransform(rb.bodyID);
						mat = mat * rb.additionalTransformInverse;
						rb.prev_position = mat.GetTranslation();
						rb.prev_rotation = mat.GetQuaternion().Normalized();

						if (rb.vehicle_constraint != nullptr)
						{
							const Entity car_wheel_entities[] = {
								physicscomponent.vehicle.wheel_entity_front_left,
								physicscomponent.vehicle.wheel_entity_front_right,
								physicscomponent.vehicle.wheel_entity_rear_left,
								physicscomponent.vehicle.wheel_entity_rear_right,
							};
							const Entity motor_wheel_entities[] = {
								physicscomponent.vehicle.wheel_entity_front_left,
								physicscomponent.vehicle.wheel_entity_rear_left,
							};
							const uint32_t count = physicscomponent.vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type::Car ? arraysize(car_wheel_entities) : arraysize(motor_wheel_entities);

							for (uint32_t i = 0; i < count; ++i)
							{
								Entity wheel_entity = physicscomponent.vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type::Car ? car_wheel_entities[i] : motor_wheel_entities[i];
								if (wheel_entity == INVALID_ENTITY)
									continue;

								TransformComponent* wheel_transform = scene.transforms.GetComponent(wheel_entity);
								if (wheel_transform != nullptr)
								{
									XMFLOAT4X4 localMatrix;
									XMStoreFloat4x4(&localMatrix, wheel_transform->GetLocalMatrix());
									Vec3 right = cast(wi::math::GetRight(localMatrix)).Normalized();
									Vec3 up = cast(wi::math::GetUp(localMatrix)).Normalized();
									Mat44 wheelmat = rb.vehicle_constraint->GetWheelWorldTransform(i, right, up);
									rb.prev_wheel_positions[i] = wheelmat.GetTranslation();
									rb.prev_wheel_rotations[i] = wheelmat.GetQuaternion();
								}
							}
						}
					});
					wi::jobsystem::Dispatch(ctx, (uint32_t)scene.humanoids.GetCount(), 1, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {
						HumanoidComponent& humanoid = scene.humanoids[args.jobIndex];
						if (humanoid.ragdoll == nullptr)
							return;
						Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
						BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
						int bodypart = 0;
						for (auto& rb : ragdoll.rigidbodies)
						{
							TransformComponent* transform = scene.transforms.GetComponent(rb.entity);
							if (transform == nullptr)
								continue;
							Mat44 mat = body_interface.GetWorldTransform(rb.bodyID);
							ragdoll.prev_capsule_position[bodypart] = mat.GetTranslation();
							ragdoll.prev_capsule_rotation[bodypart] = mat.GetQuaternion().Normalized();
							mat = mat * rb.additionalTransformInverse;
							mat = mat * rb.restBasis;
							rb.prev_position = mat.GetTranslation();
							rb.prev_rotation = mat.GetQuaternion().Normalized();
							bodypart++;
						}
					});
					wi::jobsystem::Wait(ctx);
				}

				physics_scene.physics_system.Update(TIMESTEP, 1, &temp_allocator, &job_system);
				physics_scene.accumulator = next_accumulator;
			}
			physics_scene.alpha = physics_scene.accumulator / TIMESTEP;
		}

		// Feedback physics objects to system:
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), dispatchGroupSize, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			if (physicscomponent.physicsobject == nullptr)
				return;
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			if (physicsobject.bodyID.IsInvalid())
				return;
			if (physicsobject.character != nullptr)
			{
				physicsobject.character->PostSimulation(CHARACTER_COLLISION_TOLERANCE);
			}
			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			physicsobject.was_active_prev_frame = body_interface.IsActive(physicsobject.bodyID);
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

			// Back to local space of parent:
			transform->MatrixTransform(physicsobject.parentMatrixInverse);
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

			if (humanoid.ragdoll_bodyparts.size() != Ragdoll::BODYPART_COUNT)
			{
				humanoid.ragdoll_bodyparts.resize(Ragdoll::BODYPART_COUNT);
			}
			humanoid.ragdoll_bounds = wi::primitive::AABB();
			int bodypart = 0;

			BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
			for (auto& rb : ragdoll.rigidbodies)
			{
				TransformComponent* transform = scene.transforms.GetComponent(rb.entity);
				if (transform == nullptr)
					continue;
				Mat44 mat = body_interface.GetWorldTransform(rb.bodyID);

				auto& bp = humanoid.ragdoll_bodyparts[bodypart];
				bp.bone = rb.humanoid_bone;
				bp.capsule.radius = rb.capsule.radius;

				Vec3 capsule_position = mat.GetTranslation();
				Quat capsule_rotation = mat.GetQuaternion().Normalized();
				if (IsInterpolationEnabled() && IsSimulationEnabled())
				{
					capsule_position = capsule_position * physics_scene.alpha + ragdoll.prev_capsule_position[bodypart] * (1 - physics_scene.alpha);
					capsule_rotation = ragdoll.prev_capsule_rotation[bodypart].SLERP(capsule_rotation, physics_scene.alpha);
				}
				Mat44 _capsulemat = Mat44::sRotationTranslation(capsule_rotation, capsule_position);
				XMFLOAT4X4 capsulemat = cast(_capsulemat);
				XMMATRIX M = XMLoadFloat4x4(&capsulemat);
				XMStoreFloat3(&bp.capsule.base, XMVector3Transform(XMLoadFloat3(&rb.capsule.base), M));
				XMStoreFloat3(&bp.capsule.tip, XMVector3Transform(XMLoadFloat3(&rb.capsule.tip), M));
				humanoid.ragdoll_bounds = wi::primitive::AABB::Merge(humanoid.ragdoll_bounds, bp.capsule.getAABB());

#if 0
				scene.locker.lock();
				wi::renderer::DrawCapsule(bp.capsule, XMFLOAT4(1, 1, 1, 1), false);
				scene.locker.unlock();
#endif

				if (humanoid.IsRagdollPhysicsEnabled())
				{
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

				bodypart++;
			}

#if 0
			scene.locker.lock();
			XMFLOAT4X4 mat;
			XMStoreFloat4x4(&mat, humanoid.ragdoll_bounds.getAsBoxMatrix());
			wi::renderer::DrawBox(mat, XMFLOAT4(1, 1, 1, 1), false);
			scene.locker.unlock();
#endif
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
			settings.mDrawMassAndInertia = false;
			settings.mDrawShape = true;
			settings.mDrawSoftBodyVertices = true;
			settings.mDrawSoftBodyEdgeConstraints = true;
			settings.mDrawShapeWireframe = true;
			settings.mDrawShapeColor = BodyManager::EShapeColor::ShapeTypeColor;
			debug_renderer.SetCameraPos(cast(scene.camera.Eye));
			physics_scene.physics_system.DrawBodies(settings, &debug_renderer);
			physics_scene.physics_system.DrawConstraints(&debug_renderer);
		}
#endif // JPH_DEBUG_RENDERER

		wi::jobsystem::Wait(ctx);

		wi::profiler::EndRange(range); // Physics
	}


	void SetPosition(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& position
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			physicsobject.character->SetPosition(cast(position));
			physicsobject.teleporting = true;
			return;
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		physicsobject.prev_position = cast(position);
		Mat44 m = Mat44::sTranslation(physicsobject.prev_position) * Mat44::sRotation(physicsobject.prev_rotation);
		m = m * physicsobject.additionalTransform;
		body_interface.SetPosition(
			physicsobject.bodyID,
			m.GetTranslation(),
			EActivation::DontActivate
		);
		physicsobject.teleporting = true;
	}

	void SetPositionAndRotation(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& position,
		const XMFLOAT4& rotation
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			physicsobject.character->SetPositionAndRotation(cast(position), cast(rotation));
			physicsobject.teleporting = true;
			return;
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		physicsobject.prev_position = cast(position);
		physicsobject.prev_rotation = cast(rotation).Normalized();
		Mat44 m = Mat44::sTranslation(physicsobject.prev_position) * Mat44::sRotation(physicsobject.prev_rotation);
		m = m * physicsobject.additionalTransform;
		body_interface.SetPositionAndRotation(
			physicsobject.bodyID,
			m.GetTranslation(),
			m.GetQuaternion().Normalized(),
			EActivation::DontActivate
		);
		physicsobject.teleporting = true;
	}

	void SetLinearVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			physicsobject.character->SetLinearVelocity(cast(velocity));
			return;
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		body_interface.SetLinearVelocity(physicsobject.bodyID, cast(velocity));
	}
	void SetAngularVelocity(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			physicsobject.character->SetLinearAndAngularVelocity(physicsobject.character->GetLinearVelocity(), cast(velocity));
			return;
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		body_interface.SetAngularVelocity(physicsobject.bodyID, cast(velocity));
	}

	XMFLOAT3 GetVelocity(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return XMFLOAT3(0, 0, 0);
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			return cast(physicsobject.character->GetLinearVelocity());
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		return cast(body_interface.GetLinearVelocity(physicsobject.bodyID));
	}
	XMFLOAT3 GetPosition(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return XMFLOAT3(0, 0, 0);
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			return cast(physicsobject.character->GetPosition());
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		return cast(body_interface.GetPosition(physicsobject.bodyID));
	}
	XMFLOAT4 GetRotation(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return XMFLOAT4(0, 0, 0, 1);
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			return cast(physicsobject.character->GetRotation());
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		return cast(body_interface.GetRotation(physicsobject.bodyID));
	}

	XMFLOAT3 GetCharacterGroundPosition(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return XMFLOAT3(0, 0, 0);
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			return cast(physicsobject.character->GetGroundPosition());
		}
		return XMFLOAT3(0, 0, 0);
	}
	XMFLOAT3 GetCharacterGroundNormal(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return XMFLOAT3(0, 0, 0);
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			return cast(physicsobject.character->GetGroundNormal());
		}
		return XMFLOAT3(0, 0, 0);
	}
	XMFLOAT3 GetCharacterGroundVelocity(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return XMFLOAT3(0, 0, 0);
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			return cast(physicsobject.character->GetGroundVelocity());
		}
		return XMFLOAT3(0, 0, 0);
	}
	bool IsCharacterGroundSupported(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return false;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			return physicsobject.character->IsSupported();
		}
		return false;
	}
	CharacterGroundStates GetCharacterGroundState(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return CharacterGroundStates::NotSupported;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			switch (physicsobject.character->GetGroundState())
			{
			case CharacterBase::EGroundState::OnGround:
				return CharacterGroundStates::OnGround;
			case CharacterBase::EGroundState::OnSteepGround:
				return CharacterGroundStates::OnSteepGround;
			case CharacterBase::EGroundState::NotSupported:
				return CharacterGroundStates::NotSupported;
			case CharacterBase::EGroundState::InAir:
				return CharacterGroundStates::InAir;
			default:
				assert(0);
				break;
			}
		}
		return CharacterGroundStates::NotSupported;
	}
	bool ChangeCharacterShape(RigidBodyPhysicsComponent& physicscomponent, const RigidBodyPhysicsComponent::CapsuleParams& capsule)
	{
		if (physicscomponent.physicsobject == nullptr)
			return false;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			ShapeSettings::ShapeResult shape_result;
			Vec3 bottom_offset = Vec3::sZero();
			CapsuleShapeSettings settings(capsule.height, capsule.radius);
			settings.SetEmbedded();
			shape_result = settings.Create();
			bottom_offset = Vec3(0, settings.mHalfHeightOfCylinder + settings.mRadius, 0);
			ShapeRefC newshape = RotatedTranslatedShapeSettings(bottom_offset, Quat::sIdentity(), shape_result.Get()).Create().Get();
			bool success = physicsobject.character->SetShape(newshape, 1.5f * 0.02f /*1.5 * PhysicsSystem::mPenetrationSlop*/);
			if (success)
			{
				physicsobject.shape = newshape;
				physicscomponent.shape = RigidBodyPhysicsComponent::CollisionShape::CAPSULE;
				physicscomponent.capsule = capsule;
			}
			return success;
		}
		return false;
	}
	void MoveCharacter(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& movement_direction_in,
		float movement_speed,
		float jump,
		bool controlMovementDuringJump
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character == nullptr)
			return;

		// This is the same logic as the Jolt physics character sample in CharacterTest::HandleInput

		// Cancel movement in opposite direction of normal when touching something we can't walk up
		Vec3 movement_direction = cast(movement_direction_in);
		Character::EGroundState ground_state = physicsobject.character->GetGroundState();
		if (ground_state == Character::EGroundState::OnSteepGround
			|| ground_state == Character::EGroundState::NotSupported)
		{
			Vec3 normal = physicsobject.character->GetGroundNormal();
			normal.SetY(0.0f);
			float dot = normal.Dot(movement_direction);
			if (dot < 0.0f)
				movement_direction -= (dot * normal) / normal.LengthSq();
		}

		if (controlMovementDuringJump || physicsobject.character->IsSupported())
		{
			// Update velocity
			Vec3 current_velocity = physicsobject.character->GetLinearVelocity();
			Vec3 desired_velocity = movement_speed * movement_direction;
			if (!desired_velocity.IsNearZero() || current_velocity.GetY() < 0.0f || !physicsobject.character->IsSupported())
				desired_velocity.SetY(current_velocity.GetY());
			Vec3 new_velocity = 0.75f * current_velocity + 0.25f * desired_velocity;

			// Jump
			if (jump > 0 && ground_state == Character::EGroundState::OnGround)
				new_velocity += Vec3(0, jump, 0);

			// Update the velocity
			physicsobject.character->SetLinearVelocity(new_velocity);
		}
	}

	void ApplyForce(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		body_interface.AddForce(physicsobject.bodyID, cast(force));
	}
	void ApplyForceAt(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force,
		const XMFLOAT3& at,
		bool at_local
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		Vec3 at_world = at_local ? body_interface.GetCenterOfMassTransform(physicsobject.bodyID).Inversed() * cast(at) : cast(at);
		body_interface.AddForce(physicsobject.bodyID, cast(force), at_world);
	}

	void ApplyImpulse(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			physicsobject.character->AddImpulse(cast(impulse));
			return;
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		body_interface.AddImpulse(physicsobject.bodyID, cast(impulse));
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
		default:
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
		const XMFLOAT3& at,
		bool at_local
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			physicsobject.character->AddImpulse(cast(impulse));
			return;
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		Vec3 at_world = at_local ? body_interface.GetCenterOfMassTransform(physicsobject.bodyID) * cast(at) : cast(at);
		body_interface.AddImpulse(physicsobject.bodyID, cast(impulse), at_world);
	}
	void ApplyImpulseAt(
		wi::scene::HumanoidComponent& humanoid,
		wi::scene::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at,
		bool at_local
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
		default:
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
		Vec3 at_world = at_local ? body_interface.GetCenterOfMassTransform(physicsobject.bodyID) * cast(at) : cast(at);
		body_interface.SetMotionType(physicsobject.bodyID, EMotionType::Dynamic, EActivation::Activate);
		body_interface.AddImpulse(physicsobject.bodyID, cast(impulse), at_world);
	}

	void ApplyTorque(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		body_interface.AddTorque(physicsobject.bodyID, cast(torque), EActivation::Activate);
	}

	void DriveVehicle(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		float forward,
		float right,
		float brake,
		float handbrake
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.vehicle_constraint == nullptr)
			return;

		if (physicscomponent.vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type::Car)
		{
			WheeledVehicleController* controller = static_cast<WheeledVehicleController*>(physicsobject.vehicle_constraint->GetController());
			controller->SetDriverInput(forward, -right, brake, handbrake);
		}
		else if (physicscomponent.vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type::Motorcycle)
		{
			MotorcycleController* controller = static_cast<MotorcycleController*>(physicsobject.vehicle_constraint->GetController());
			controller->SetDriverInput(forward, -right, brake, handbrake);
			controller->EnableLeanController(physicscomponent.vehicle.motorcycle.lean_control);
		}

		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		body_interface.ActivateBody(physicsobject.bodyID);
	}

	float GetVehicleForwardVelocity(wi::scene::RigidBodyPhysicsComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return 0;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.vehicle_constraint == nullptr)
			return 0;
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		float velocity = (body_interface.GetRotation(physicsobject.bodyID).Conjugated() * body_interface.GetLinearVelocity(physicsobject.bodyID)).GetZ();
		return velocity;
	}

	void OverrideWehicleWheelTransforms(wi::scene::Scene& scene)
	{
		if (scene.physics_scene == nullptr)
			return;
		if (!IsSimulationEnabled())
			return;
		PhysicsScene& physics_scene = GetPhysicsScene(scene);
		wi::jobsystem::context ctx;
		wi::jobsystem::Dispatch(ctx, (uint32_t)scene.rigidbodies.GetCount(), dispatchGroupSize, [&scene, &physics_scene](wi::jobsystem::JobArgs args) {

			RigidBodyPhysicsComponent& physicscomponent = scene.rigidbodies[args.jobIndex];
			if (physicscomponent.physicsobject == nullptr)
				return;
			RigidBody& physicsobject = GetRigidBody(physicscomponent);
			if (physicsobject.vehicle_constraint == nullptr)
				return;

			const Entity car_wheel_entities[] = {
				physicscomponent.vehicle.wheel_entity_front_left,
				physicscomponent.vehicle.wheel_entity_front_right,
				physicscomponent.vehicle.wheel_entity_rear_left,
				physicscomponent.vehicle.wheel_entity_rear_right,
			};
			const Entity motor_wheel_entities[] = {
				physicscomponent.vehicle.wheel_entity_front_left,
				physicscomponent.vehicle.wheel_entity_rear_left,
			};
			const uint32_t count = physicscomponent.vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type::Car ? arraysize(car_wheel_entities) : arraysize(motor_wheel_entities);

			for (uint32_t i = 0; i < count; ++i)
			{
				Entity wheel_entity = physicscomponent.vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type::Car ? car_wheel_entities[i] : motor_wheel_entities[i];
				if (wheel_entity == INVALID_ENTITY)
					continue;

				TransformComponent* wheel_transform = scene.transforms.GetComponent(wheel_entity);
				if (wheel_transform != nullptr)
				{
					XMFLOAT4X4 localMatrix;
					XMStoreFloat4x4(&localMatrix, wheel_transform->GetLocalMatrix());
					Vec3 right = cast(wi::math::GetRight(localMatrix)).Normalized();
					Vec3 up = cast(wi::math::GetUp(localMatrix)).Normalized();
					Mat44 wheelmat = physicsobject.vehicle_constraint->GetWheelWorldTransform(i, right, up);
					Vec3 wheelpos = wheelmat.GetTranslation();
					Quat wheelrot = wheelmat.GetQuaternion();
					if (IsInterpolationEnabled())
					{
						wheelpos = wheelpos * physics_scene.alpha + physicsobject.prev_wheel_positions[i] * (1 - physics_scene.alpha);
						wheelrot = physicsobject.prev_wheel_rotations[i].SLERP(wheelrot, physics_scene.alpha);
					}
					wheelmat = Mat44::sRotationTranslation(wheelrot, wheelpos) * Mat44::sScale(cast(wheel_transform->GetScale()));
					wheel_transform->world = cast(wheelmat);

					scene.RefreshHierarchyTopdownFromParent(wheel_entity);

#if 0
					// Debug draw:
					XMFLOAT4X4 tmp = cast(Mat44::sRotationTranslation(wheelrot, wheelpos));
					std::scoped_lock lck(scene.locker);
					wi::renderer::DrawAxis(XMLoadFloat4x4(&tmp), 1);
#endif
				}
			}
		});
		wi::jobsystem::Wait(ctx);
	}

	bool IsConstraintBroken(const wi::scene::PhysicsConstraintComponent& physicscomponent)
	{
		if (physicscomponent.physicsobject == nullptr)
			return false;
		const Constraint& constraint = GetConstraint(physicscomponent);
		return constraint.constraint->GetEnabled();
	}
	void SetConstraintBroken(wi::scene::PhysicsConstraintComponent& physicscomponent, bool broken)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		Constraint& constraint = GetConstraint(physicscomponent);
		constraint.constraint->SetEnabled(!broken);
	}

	void SetActivationState(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		ActivationState state
	)
	{
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr && state == ActivationState::Active)
		{
			physicsobject.character->Activate();
			return;
		}
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
	void ActivateAllRigidBodies(Scene& scene)
	{
		PhysicsScene& physics_scene = *(PhysicsScene*)scene.physics_scene.get();
		physics_scene.activate_all_rigid_bodies = true;
	}

	void ResetPhysicsObjects(Scene& scene)
	{
		PhysicsScene& physics_scene = *(PhysicsScene*)scene.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		BodyIDVector bodies;
		physics_scene.physics_system.GetBodies(bodies);
		for (BodyID& bodyID : bodies)
		{
			uint64_t userdata = body_interface.GetUserData(bodyID);
			if (userdata == 0)
				continue;

			if (body_interface.GetBodyType(bodyID) == EBodyType::RigidBody)
			{
				RigidBody* physicsobject = (RigidBody*)userdata;
				body_interface.SetPositionRotationAndVelocity(
					bodyID,
					physicsobject->additionalTransform.GetTranslation() + physicsobject->initial_position,
					physicsobject->initial_rotation,
					Vec3::sZero(),
					Vec3::sZero()
				);
				physicsobject->prev_position = physicsobject->initial_position;
				physicsobject->prev_rotation = physicsobject->initial_rotation;
				if (body_interface.GetMotionType(bodyID) != EMotionType::Static)
				{
					body_interface.ResetSleepTimer(bodyID);
				}
				if (physicsobject->start_deactivated)
				{
					body_interface.DeactivateBody(bodyID);
				}

				// Feedback dynamic bodies: physics -> system
				if (body_interface.GetMotionType(bodyID) == EMotionType::Dynamic)
				{
					TransformComponent* transform = scene.transforms.GetComponent(physicsobject->entity);
					if (transform != nullptr)
					{
						Mat44 mat = body_interface.GetWorldTransform(bodyID);
						mat = mat * physicsobject->additionalTransformInverse;
						Vec3 position = mat.GetTranslation();
						Quat rotation = mat.GetQuaternion().Normalized();

						transform->translation_local = cast(position);
						transform->rotation_local = cast(rotation);

						// Back to local space of parent:
						transform->MatrixTransform(physicsobject->parentMatrixInverse);
					}
				}
			}
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

	void SetGhostMode(
		wi::scene::RigidBodyPhysicsComponent& physicscomponent,
		bool value
	)
	{
		if (physicscomponent.physicsobject == nullptr)
			return;
		RigidBody& physicsobject = GetRigidBody(physicscomponent);
		if (physicsobject.character != nullptr)
		{
			physicsobject.character->SetLayer(value ? Layers::GHOST : Layers::MOVING);
			return;
		}
		PhysicsScene& physics_scene = *(PhysicsScene*)physicsobject.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		EMotionType motionType = body_interface.GetMotionType(physicsobject.bodyID);
		ObjectLayer layer = value ? Layers::GHOST : (motionType == EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING);
		body_interface.SetObjectLayer(physicsobject.bodyID, layer);
	}
	void SetGhostMode(
		wi::scene::HumanoidComponent& humanoid,
		bool value
	)
	{
		if (humanoid.ragdoll == nullptr)
			return;
		Ragdoll& ragdoll = *(Ragdoll*)humanoid.ragdoll.get();
		PhysicsScene& physics_scene = *(PhysicsScene*)ragdoll.physics_scene.get();
		BodyInterface& body_interface = physics_scene.physics_system.GetBodyInterfaceNoLock();
		ObjectLayer layer = value ? Layers::GHOST : Layers::MOVING;
		for (auto& rb : ragdoll.rigidbodies)
		{
			if (rb.bodyID.IsInvalid())
				continue;
			body_interface.SetObjectLayer(rb.bodyID, layer);
		}
	}
	void SetRagdollGhostMode(wi::scene::HumanoidComponent& humanoid, bool value)
	{
		SetGhostMode(humanoid, value);
	}

	template <class CollectorType>
	class WickedClosestHitCollector : public JPH::ClosestHitCollisionCollector<CollectorType>
	{
	public:
		const Scene* scene = nullptr;
		const PhysicsScene* physics_scene = nullptr;

		using ResultType = typename CollectorType::ResultType;
		void AddHit(const ResultType& inResult) override
		{
			BodyLockRead lock(physics_scene->physics_system.GetBodyLockInterfaceNoLock(), inResult.mBodyID);
			if (!lock.Succeeded())
				return;
			const Body& body = lock.GetBody();
			const uint64_t userdata = body.GetUserData();
			if (userdata == 0)
				return;

			if (body.IsRigidBody())
			{
				const RigidBody* physicsobject = (RigidBody*)userdata;
				if (physicsobject->humanoid_ragdoll_entity != INVALID_ENTITY)
				{
					const HumanoidComponent* humanoid = scene->humanoids.GetComponent(physicsobject->humanoid_ragdoll_entity);
					if (humanoid != nullptr && humanoid->IsIntersectionDisabled())
						return; // skip this from AddHit
				}
			}

			ClosestHitCollisionCollector<CollectorType>::AddHit(inResult);
		}
	};

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
		settings.mBackFaceModeTriangles = EBackFaceMode::IgnoreBackFaces;
		settings.mTreatConvexAsSolid = false;

		WickedClosestHitCollector<CastRayCollector> collector;
		collector.scene = &scene;
		collector.physics_scene = &physics_scene;

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
		if (userdata == 0)
			return result;

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
		float bind_distance = 0;
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
		PickDragOperation& op,
		ConstraintType constraint_type,
		float break_distance
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

				float dist = (internal_state->bodyA->GetCenterOfMassPosition() - internal_state->bodyB->GetCenterOfMassPosition()).Length();
				dist -= internal_state->bind_distance;
				if (dist > break_distance)
				{
					internal_state->constraint->SetEnabled(false);
				}
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

				internal_state->bodyA = body_interface.CreateBody(BodyCreationSettings(new SphereShape(0.01f), pos, Quat::sIdentity(), EMotionType::Kinematic, Layers::GHOST));
				body_interface.AddBody(internal_state->bodyA->GetID(), EActivation::Activate);

				if (constraint_type == ConstraintType::Fixed)
				{
					FixedConstraintSettings settings;
					settings.SetEmbedded();
					settings.mAutoDetectPoint = true;
					internal_state->constraint = settings.Create(*internal_state->bodyA, *internal_state->bodyB);
				}
				else if (constraint_type == ConstraintType::Point)
				{
					PointConstraintSettings settings;
					settings.SetEmbedded();
					settings.mPoint1 = settings.mPoint2 = pos;
					internal_state->constraint = settings.Create(*internal_state->bodyA, *internal_state->bodyB);
				}

				internal_state->bind_distance = (internal_state->bodyA->GetCenterOfMassPosition() - internal_state->bodyB->GetCenterOfMassPosition()).Length();

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
