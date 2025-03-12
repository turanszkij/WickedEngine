#include "wiRagdoll.h"

namespace wi::physics
{
	namespace jolt
	{
		Ragdoll::Ragdoll(Scene& scene, HumanoidComponent& humanoid, Entity humanoidEntity, 
		                 float scale, unsigned const BodyPartCount)
			: _bodyPartCount(BodyPartCount)
			, _scene(scene)
			, _humanoid(humanoid)
			, _humanoidEntity(humanoidEntity)
			, _scale(scale)
		{
			initializeRagdoll();
		}

		Ragdoll::~Ragdoll()
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

		void Ragdoll::initializeRagdoll()
		{
			physics_scene = _scene.physics_scene;
			PhysicsSystem& physics_system = ((PhysicsScene*)physics_scene.get())->physics_system;
			BodyInterface& body_interface = physics_system.GetBodyInterface(); // locking version because this is called from job system!
			
			masses.resize(_bodyPartCount);
			positions.resize(_bodyPartCount);
			constraint_positions.resize(_bodyPartCount);
			final_transforms.resize(_bodyPartCount);
			rigidbodies.resize(_bodyPartCount);
			saved_parents.resize(_bodyPartCount);
			prev_capsule_position.resize(_bodyPartCount);
			prev_capsule_rotation.resize(_bodyPartCount);
#if 0
			// slow speed and visualizer to aid debugging:
			wi::renderer::SetGameSpeed(0.1f);
			SetDebugDrawEnabled(true);
#endif

			// Detect which way humanoid is facing in rest pose:
			const float facing = _humanoid.default_facing;

			// Whole ragdoll will take a uniform scaling:
			const XMMATRIX scaleMatrix = XMMatrixScaling(scale, scale, scale);
			this->scale = scale;

			// Calculate the bone lengths and radiuses in armature local space and create rigid bodies for bones:
			for (int c = 0; c < _bodyPartCount; ++c)
			{
				HumanoidComponent::HumanoidBone humanoid_bone = HumanoidComponent::HumanoidBone::Count;
				Entity entityA = INVALID_ENTITY;
				Entity entityB = INVALID_ENTITY;
				switch (c)
				{
				case BODYPART_PELVIS:
					humanoid_bone = HumanoidComponent::HumanoidBone::Hips;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Hips];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Spine];
					break;
				case BODYPART_SPINE:
					humanoid_bone = HumanoidComponent::HumanoidBone::Spine;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Spine];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Neck]; // prefer neck instead of head
					if (entityB == INVALID_ENTITY)
					{
						entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Head]; // fall back to head if neck not available
					}
					break;
				case BODYPART_HEAD:
					humanoid_bone = HumanoidComponent::HumanoidBone::Neck;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Neck]; // prefer neck instead of head
					if (entityA == INVALID_ENTITY)
					{
						humanoid_bone = HumanoidComponent::HumanoidBone::Head;
						entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::Head]; // fall back to head if neck not available
					}
					break;
				case BODYPART_LEFT_UPPER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftUpperLeg;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperLeg];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerLeg];
					break;
				case BODYPART_LEFT_LOWER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftLowerLeg;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerLeg];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftFoot];
					break;
				case BODYPART_RIGHT_UPPER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightUpperLeg;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperLeg];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerLeg];
					break;
				case BODYPART_RIGHT_LOWER_LEG:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightLowerLeg;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerLeg];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightFoot];
					break;
				case BODYPART_LEFT_UPPER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftUpperArm;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftUpperArm];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerArm];
					break;
				case BODYPART_LEFT_LOWER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::LeftLowerArm;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftLowerArm];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::LeftHand];
					break;
				case BODYPART_RIGHT_UPPER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightUpperArm;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightUpperArm];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerArm];
					break;
				case BODYPART_RIGHT_LOWER_ARM:
					humanoid_bone = HumanoidComponent::HumanoidBone::RightLowerArm;
					entityA = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightLowerArm];
					entityB = _humanoid.bones[(size_t)HumanoidComponent::HumanoidBone::RightHand];
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
				XMMATRIX restA = _scene.GetRestPose(entityA) * scaleMatrix;
				XMMATRIX restB = _scene.GetRestPose(entityB) * scaleMatrix;
				XMVECTOR rootA = restA.r[3];
				XMVECTOR rootB = restB.r[3];

				// Every bone will be a rigid body:
				RigidBody& physicsobject = rigidbodies[c];
				physicsobject.entity = entityA;

				float mass = scale;
				float capsule_height = scale;
				float capsule_radius = scale * _humanoid.ragdoll_fatness;

				if (c == BODYPART_HEAD)
				{
					// Head doesn't necessarily have a child, so make up something reasonable:
					capsule_height = 0.05f * scale;
					capsule_radius = 0.1f * scale * _humanoid.ragdoll_headsize;
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
						capsule_radius = 0.1f * scale * _humanoid.ragdoll_fatness;
						break;
					case BODYPART_SPINE:
						capsule_radius = 0.1f * scale * _humanoid.ragdoll_fatness;
						capsule_height -= capsule_radius * 2;
						break;
					case BODYPART_LEFT_LOWER_ARM:
					case BODYPART_RIGHT_LOWER_ARM:
						capsule_radius = capsule_height * 0.15f * _humanoid.ragdoll_fatness;
						capsule_height += capsule_radius;
						break;
					case BODYPART_LEFT_UPPER_LEG:
					case BODYPART_RIGHT_UPPER_LEG:
						capsule_radius = capsule_height * 0.15f * _humanoid.ragdoll_fatness;
						capsule_height -= capsule_radius * 2;
						break;
					case BODYPART_LEFT_LOWER_LEG:
					case BODYPART_RIGHT_LOWER_LEG:
						capsule_radius = capsule_height * 0.15f * _humanoid.ragdoll_fatness;
						capsule_height -= capsule_radius;
						break;
					default:
						capsule_radius = capsule_height * 0.2f * _humanoid.ragdoll_fatness;
						capsule_height -= capsule_radius * 2;
						break;
					}
				}

				capsule_radius = std::abs(capsule_radius);
				capsule_height = std::abs(capsule_height);

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
					physicsobject.capsule = wi::primitive::Capsule(XMFLOAT3(-capsule_height * 0.5f - capsule_radius, 0, 0), XMFLOAT3(capsule_height * 0.5f + capsule_radius, 0, 0), capsule_radius);
					rtshape_settings.mRotation = Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI).Normalized();
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

				physicsobject.humanoid_ragdoll_entity = _humanoidEntity;
				physicsobject.humanoid_bone = humanoid_bone;

				physicsobject.physics_scene = _scene.physics_scene;

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

			uint bodyparts[_bodyPartCount] = {};
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
			Mat44* m44 = &final_transforms[0];
			ragdoll->SetPose(Vec3::sZero(), m44);
			ragdoll->AddToPhysicsSystem(EActivation::Activate);

			const int count = (int)ragdoll->GetBodyCount();
			for (int index = 0; index < count; ++index)
			{
				rigidbodies[index].bodyID = ragdoll->GetBodyID(index);
				body_interface.SetUserData(rigidbodies[index].bodyID, (uint64_t)&rigidbodies[index]);
			}
		}
		
		// Activates ragdoll as dynamic physics object:
		void Ragdoll::Activate(
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
		void Ragdoll::Deactivate(
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
	}

}	