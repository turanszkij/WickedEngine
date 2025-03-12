#pragma once

#include "wiPhysics_Jolt.h"
#include "wiScene.h"

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
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>

using namespace JPH;
using namespace wi::ecs;
using namespace wi::scene;

namespace wi::physics
{
	namespace jolt
	{
		class Ragdoll
		{
		public:	
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

				// Optional parts
				/*
				BODYPART_LEFT_FOOT,
				BODYPART_RIGHT_FOOT, 

				BODYPART_LEFT_HAND,
				BODYPART_RIGHT_HAND	
				*/	
				BODYPART_COUNT
			};

			std::shared_ptr<void> physics_scene;
			std::vector<RigidBody> rigidbodies;

			std::vector<Entity> saved_parents;
			
			Skeleton skeleton;
			RagdollSettings settings;
			Ref<JPH::Ragdoll> ragdoll;
			bool state_active = false;
			float scale = 1;
			
			//Vec3 prev_capsule_position[BODYPART_COUNT];
			//Quat prev_capsule_rotation[BODYPART_COUNT];
			std::vector<Vec3> prev_capsule_position;
			std::vector<Quat> prev_capsule_rotation;

			//float masses[BODYPART_COUNT] = {};
			//Vec3 positions[BODYPART_COUNT] = {};
			//Vec3 constraint_positions[BODYPART_COUNT] = {};
			
			std::vector<float> masses;
			std::vector<Vec3> positions;
			std::vector<Vec3> constraint_positions;
			std::vector<Mat44> final_transforms;

			Ragdoll(Scene& scene, HumanoidComponent& humanoid, Entity humanoidEntity, float scale,
			        unsigned const BodyPartCount = 11);
			~Ragdoll();
			unsigned getSize() const { return _bodyPartCount; }
			/// Activates ragdoll as dynamic physics object:
			void Activate(Scene& scene, Entity humanoidEntity);
			/// Disables dynamic ragdoll and reattaches loose parts as they were:
			void Deactivate(Scene& scene);
		protected:
		private:
			void initializeRagdoll();
			unsigned const _bodyPartCount;
			Scene& _scene;
			HumanoidComponent& _humanoid;
			Entity& _humanoidEntity;
			float _scale;
		};
	}
}	