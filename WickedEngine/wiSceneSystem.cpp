#include "wiSceneSystem.h"

namespace wiSceneSystem
{
	void Scene::Update(float dt)
	{
		// Update Transform components:
		for (size_t i = 0; i < transforms.GetCount(); ++i)
		{
			auto& transform = transforms[i];

			const bool parented = transforms.IsValid(transform.parent_ref);

			if (transform.dirty || parented)
			{
				transform.dirty = false;

				XMVECTOR scale_local = XMLoadFloat3(&transform.scale_local);
				XMVECTOR rotation_local = XMLoadFloat4(&transform.rotation_local);
				XMVECTOR translation_local = XMLoadFloat3(&transform.translation_local);
				XMMATRIX world =
					XMMatrixScalingFromVector(scale_local) *
					XMMatrixRotationQuaternion(rotation_local) *
					XMMatrixTranslationFromVector(translation_local);

				if (parented)
				{
					auto& parent = transforms.GetComponent(transform.parent_ref);
					XMMATRIX world_parent = XMLoadFloat4x4(&parent.world);
					XMMATRIX bindMatrix = XMLoadFloat4x4(&transform.world_parent_bind);
					world = world * bindMatrix * world_parent;
				}

				transform.world_prev = transform.world;
				XMStoreFloat4x4(&transform.world, world);
			}

		}

		// Update Bone components:
		for (size_t i = 0; i < bones.GetCount(); ++i)
		{
			auto& bone = bones[i];
			auto& transform = transforms.GetComponent(bone.transform_ref);

			XMMATRIX inverseBindPoseMatrix = XMLoadFloat4x4(&bone.inverseBindPoseMatrix);
			XMMATRIX world = XMLoadFloat4x4(&transform.world);
			XMMATRIX skinningMatrix = inverseBindPoseMatrix * world;
			XMStoreFloat4x4(&bone.skinningMatrix, skinningMatrix);
		}

	}
}
