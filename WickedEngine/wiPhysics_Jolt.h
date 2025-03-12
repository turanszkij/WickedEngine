#pragma once 

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
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>

using namespace JPH;
using namespace wi::ecs;
using namespace wi::scene;

namespace wi::physics
{
	namespace jolt
	{
		static float TIMESTEP = 1.0f / 60.0f;
		static int ACCURACY = 4;

		const EMotionQuality cMotionQuality = EMotionQuality::LinearCast;

		const uint cMaxBodies = 65536;
		const uint cNumBodyMutexes = 0;
		const uint cMaxBodyPairs = 65536;
		const uint cMaxContactConstraints = 65536;

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
		
		namespace Layers
		{
			static constexpr ObjectLayer NON_MOVING = 0;
			static constexpr ObjectLayer MOVING = 1;
			static constexpr ObjectLayer NUM_LAYERS = 2;
		};
		// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
		// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
		// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
		// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
		// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.

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

		struct RigidBody
		{
			std::shared_ptr<void> physics_scene;
			ShapeRefC shape;
			BodyID bodyID;
			Entity entity = INVALID_ENTITY;
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

			// water ripple stuff:
			bool was_underwater = false;

			// vehicle:
			VehicleConstraint* vehicle_constraint = nullptr;
			Vec3 prev_wheel_positions[4] = { Vec3::sZero(), Vec3::sZero(), Vec3::sZero(), Vec3::sZero() };
			Quat prev_wheel_rotations[4] = { Quat::sIdentity(), Quat::sIdentity(), Quat::sIdentity(), Quat::sIdentity() };

			void Delete()
			{
				if (physics_scene == nullptr || bodyID.IsInvalid())
					return;
				PhysicsScene* jolt_physics_scene = (PhysicsScene*)physics_scene.get();
				BodyInterface& body_interface = jolt_physics_scene->physics_system.GetBodyInterface(); // locking version because destructor can be called from any thread
				body_interface.RemoveBody(bodyID);
				body_interface.DestroyBody(bodyID);
				bodyID = {};
				if (vehicle_constraint != nullptr)
				{
					jolt_physics_scene->physics_system.RemoveStepListener(vehicle_constraint);
					jolt_physics_scene->physics_system.RemoveConstraint(vehicle_constraint);
					vehicle_constraint = nullptr;
				}
				shape = nullptr;
			}

			~RigidBody()
			{
				Delete();
			}
		};
	}
}