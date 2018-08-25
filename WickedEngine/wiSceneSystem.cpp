#include "wiSceneSystem.h"
#include "wiMath.h"
#include "wiTextureHelper.h"

using namespace wiECS;
using namespace wiGraphicsTypes;

namespace wiSceneSystem
{

	void TransformComponent::ClearTransform()
	{
		dirty = true;
		scale_local = XMFLOAT3(1, 1, 1);
		rotation_local = XMFLOAT4(0, 0, 0, 1);
		translation_local = XMFLOAT3(0, 0, 0);
	}
	void TransformComponent::Translate(const XMFLOAT3& value)
	{
		dirty = true;
		translation_local.x += value.x;
		translation_local.y += value.y;
		translation_local.z += value.z;
	}
	void TransformComponent::RotateRollPitchYaw(const XMFLOAT3& value)
	{
		dirty = true;

		// This needs to be handled a bit differently
		XMVECTOR quat = XMLoadFloat4(&rotation_local);
		XMVECTOR x = XMQuaternionRotationRollPitchYaw(value.x, 0, 0);
		XMVECTOR y = XMQuaternionRotationRollPitchYaw(0, value.y, 0);
		XMVECTOR z = XMQuaternionRotationRollPitchYaw(0, 0, value.z);

		quat = XMQuaternionMultiply(x, quat);
		quat = XMQuaternionMultiply(quat, y);
		quat = XMQuaternionMultiply(z, quat);
		quat = XMQuaternionNormalize(quat);

		XMStoreFloat4(&rotation_local, quat);
	}
	void TransformComponent::Rotate(const XMFLOAT4& quaternion)
	{
		dirty = true;

		XMVECTOR result = XMQuaternionMultiply(XMLoadFloat4(&rotation_local), XMLoadFloat4(&quaternion));
		result = XMQuaternionNormalize(result);
		XMStoreFloat4(&rotation_local, result);
	}
	void TransformComponent::Scale(const XMFLOAT3& value)
	{
		dirty = true;

		scale_local.x += value.x;
		scale_local.y += value.y;
		scale_local.z += value.z;
	}
	void TransformComponent::Lerp(const TransformComponent& a, const TransformComponent& b, float t)
	{
		dirty = true;

		XMVECTOR aS = XMLoadFloat3(&a.scale);
		XMVECTOR aR = XMLoadFloat4(&a.rotation);
		XMVECTOR aT = XMLoadFloat3(&a.translation);

		XMVECTOR bS = XMLoadFloat3(&b.scale);
		XMVECTOR bR = XMLoadFloat4(&b.rotation);
		XMVECTOR bT = XMLoadFloat3(&b.translation);

		XMVECTOR S = XMVectorLerp(aS, bS, t);
		XMVECTOR R = XMQuaternionSlerp(aR, bR, t);
		XMVECTOR T = XMVectorLerp(aT, bT, t);

		XMStoreFloat3(&scale_local, S);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&translation_local, T);
	}
	void TransformComponent::CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t)
	{
		dirty = true;

		XMVECTOR aS = XMLoadFloat3(&a.scale);
		XMVECTOR aR = XMLoadFloat4(&a.rotation);
		XMVECTOR aT = XMLoadFloat3(&a.translation);

		XMVECTOR bS = XMLoadFloat3(&b.scale);
		XMVECTOR bR = XMLoadFloat4(&b.rotation);
		XMVECTOR bT = XMLoadFloat3(&b.translation);

		XMVECTOR cS = XMLoadFloat3(&c.scale);
		XMVECTOR cR = XMLoadFloat4(&c.rotation);
		XMVECTOR cT = XMLoadFloat3(&c.translation);

		XMVECTOR dS = XMLoadFloat3(&d.scale);
		XMVECTOR dR = XMLoadFloat4(&d.rotation);
		XMVECTOR dT = XMLoadFloat3(&d.translation);

		XMVECTOR T = XMVectorCatmullRom(aT, bT, cT, dT, t);

		// Catmull-rom has issues with full rotation for quaternions (todo):
		XMVECTOR R = XMVectorCatmullRom(aR, bR, cR, dR, t);
		R = XMQuaternionNormalize(R);

		XMVECTOR S = XMVectorCatmullRom(aS, bS, cS, dS, t);

		XMStoreFloat3(&translation_local, T);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&scale_local, S);
	}


	Texture2D* MaterialComponent::GetBaseColorMap() const
	{
		if (baseColorMap != nullptr)
		{
			return baseColorMap;
		}
		return wiTextureHelper::getInstance()->getWhite();
	}
	Texture2D* MaterialComponent::GetNormalMap() const
	{
		return normalMap;
		//if (normalMap != nullptr)
		//{
		//	return normalMap;
		//}
		//return wiTextureHelper::getInstance()->getNormalMapDefault();
	}
	Texture2D* MaterialComponent::GetSurfaceMap() const
	{
		if (surfaceMap != nullptr)
		{
			return surfaceMap;
		}
		return wiTextureHelper::getInstance()->getWhite();
	}
	Texture2D* MaterialComponent::GetDisplacementMap() const
	{
		if (displacementMap != nullptr)
		{
			return displacementMap;
		}
		return wiTextureHelper::getInstance()->getWhite();
	}

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

				XMVECTOR S, R, T;
				XMMatrixDecompose(&S, &R, &T, world);
				XMStoreFloat3(&transform.scale, S);
				XMStoreFloat4(&transform.rotation, R);
				XMStoreFloat3(&transform.translation, T);

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

		// Update Light components:
		for (size_t i = 0; i < lights.GetCount(); ++i)
		{
			auto& light = lights[i];
			auto& transform = transforms.GetComponent(light.transform_ref);

			XMMATRIX world = XMLoadFloat4x4(&transform.world);

			if (light.type == LightComponent::DIRECTIONAL)
			{
				XMStoreFloat3(&sunDirection, -XMVector3TransformNormal(XMVectorSet(0, -1, 0, 1), world));
				sunColor = light.color;
			}
		}

	}
	void Scene::Remove(Entity entity)
	{
		owned_entities.erase(entity);

		nodes.Remove(entity);
		transforms.Remove(entity);
		materials.Remove(entity);
		meshes.Remove(entity);
		objects.Remove(entity);
		physicscomponents.Remove(entity);
		cullables.Remove(entity);
		bones.Remove(entity);
		armatures.Remove(entity);
		lights.Remove(entity);
		cameras.Remove(entity);
		probes.Remove(entity);
		forces.Remove(entity);
		decals.Remove(entity);
		models.Remove(entity);
	}
	void Scene::Clear()
	{
		for (auto& x : owned_entities)
		{
			Remove(x);
		}
	}


}
