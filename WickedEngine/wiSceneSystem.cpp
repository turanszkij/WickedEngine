#include "wiSceneSystem.h"
#include "wiMath.h"
#include "wiTextureHelper.h"

using namespace wiECS;
using namespace wiGraphicsTypes;

namespace wiSceneSystem
{

	void TransformComponent::UpdateTransform()
	{
		if (dirty)
		{
			dirty = false;

			XMVECTOR S_local = XMLoadFloat3(&scale_local);
			XMVECTOR R_local = XMLoadFloat4(&rotation_local);
			XMVECTOR T_local = XMLoadFloat3(&translation_local);
			XMMATRIX W =
				XMMatrixScalingFromVector(S_local) *
				XMMatrixRotationQuaternion(R_local) *
				XMMatrixTranslationFromVector(T_local);

			scale = scale_local;
			rotation = rotation_local;
			translation = translation_local;

			world_prev = world;
			XMStoreFloat4x4(&world, W);
		}
	}
	void TransformComponent::ApplyTransform()
	{
		dirty = true;

		scale_local = scale;
		rotation_local = rotation;
		translation_local = translation;
	}
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

		scale_local.x *= value.x;
		scale_local.y *= value.y;
		scale_local.z *= value.z;
	}
	void TransformComponent::MatrixTransform(const XMFLOAT4X4& matrix)
	{
		MatrixTransform(XMLoadFloat4x4(&matrix));
	}
	void TransformComponent::MatrixTransform(const XMMATRIX& matrix)
	{
		dirty = true;

		XMVECTOR S_local = XMLoadFloat3(&scale_local);
		XMVECTOR R_local = XMLoadFloat4(&rotation_local);
		XMVECTOR T_local = XMLoadFloat3(&translation_local);
		XMMATRIX W =
			XMMatrixScalingFromVector(S_local) *
			XMMatrixRotationQuaternion(R_local) *
			XMMatrixTranslationFromVector(T_local);

		W = W * matrix;

		XMMatrixDecompose(&S_local, &R_local, &T_local, W);
		XMStoreFloat3(&scale_local, S_local);
		XMStoreFloat4(&rotation_local, R_local);
		XMStoreFloat3(&translation_local, T_local);
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

	void CameraComponent::CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV)
	{
		zNearP = newNear;
		zFarP = newFar;
		width = newWidth;
		height = newHeight;
		fov = newFOV;
		Eye = XMFLOAT3(0, 0, 0);
		At = XMFLOAT3(0, 0, 1);
		Up = XMFLOAT3(0, 1, 0);

		UpdateProjection();
		UpdateCamera();
	}
	void CameraComponent::UpdateProjection()
	{
		XMStoreFloat4x4(&Projection, XMMatrixPerspectiveFovLH(fov, width / height, zFarP, zNearP)); // reverse zbuffer!
		XMStoreFloat4x4(&realProjection, XMMatrixPerspectiveFovLH(fov, width / height, zNearP, zFarP)); // normal zbuffer!
	}
	void CameraComponent::UpdateCamera(const TransformComponent* transform)
	{
		XMVECTOR _Eye;
		XMVECTOR _At;
		XMVECTOR _Up;

		if (transform != nullptr)
		{
			_Eye = XMLoadFloat3(&transform->translation);
			_At = XMVectorSet(0, 0, 1, 0);
			_Up = XMVectorSet(0, 1, 0, 0);

			XMMATRIX _Rot = XMMatrixRotationQuaternion(XMLoadFloat4(&transform->rotation));
			_At = XMVector3TransformNormal(_At, _Rot);
			_Up = XMVector3TransformNormal(_Up, _Rot);
		}
		else
		{
			_Eye = XMLoadFloat3(&Eye);
			_At = XMLoadFloat3(&At);
			_Up = XMLoadFloat3(&Up);
		}

		XMMATRIX _P = XMLoadFloat4x4(&Projection);
		XMMATRIX _InvP = XMMatrixInverse(nullptr, _P);
		XMStoreFloat4x4(&InvProjection, _InvP);

		XMMATRIX _V = XMMatrixLookToLH(_Eye, _At, _Up);
		XMMATRIX _VP = XMMatrixMultiply(_V, _P);
		XMStoreFloat4x4(&VP, _VP);
		XMStoreFloat4x4(&InvView, XMMatrixInverse(nullptr, _V));
		XMStoreFloat4x4(&InvVP, XMMatrixInverse(nullptr, _VP));
		XMStoreFloat4x4(&Projection, _P);
		XMStoreFloat4x4(&InvProjection, XMMatrixInverse(nullptr, _P));

		XMStoreFloat3(&Eye, _Eye);
		XMStoreFloat3(&At, _At);
		XMStoreFloat3(&Up, _Up);

		frustum.ConstructFrustum(zFarP, realProjection, View);
	}
	void CameraComponent::Reflect(const XMFLOAT4& plane)
	{
		XMVECTOR _Eye = XMLoadFloat3(&Eye);
		XMVECTOR _At = XMLoadFloat3(&At);
		XMVECTOR _Up = XMLoadFloat3(&Up);
		XMMATRIX _Ref = XMMatrixReflect(XMLoadFloat4(&plane));

		_Eye = XMVector3Transform(_Eye, _Ref);
		_At = XMVector3TransformNormal(_At, _Ref);
		_Up = XMVector3TransformNormal(_Up, _Ref);

		XMStoreFloat3(&Eye, _Eye);
		XMStoreFloat3(&At, _At);
		XMStoreFloat3(&Up, _Up);

		UpdateCamera();
	}


	void Scene::Update(float dt)
	{
		// Update Transform components:
		for (size_t i = 0; i < transforms.GetCount(); ++i)
		{
			TransformComponent& transform = transforms[i];
			transform.UpdateTransform();
		}

		// Update Transform hierarchy:
		for (size_t i = 0; i < parents.GetCount(); ++i)
		{
			ParentComponent& parentcomponent = parents[i];

			Entity entity = parents.GetEntity(i);
			TransformComponent* transform_child = transforms.GetComponent(entity);
			TransformComponent* transform_parent = transforms.GetComponent(parentcomponent.parentID);
			assert(transform_child != nullptr);
			assert(transform_parent != nullptr);

			XMVECTOR S_local = XMLoadFloat3(&transform_child->scale_local);
			XMVECTOR R_local = XMLoadFloat4(&transform_child->rotation_local);
			XMVECTOR T_local = XMLoadFloat3(&transform_child->translation_local);
			XMMATRIX W =
				XMMatrixScalingFromVector(S_local) *
				XMMatrixRotationQuaternion(R_local) *
				XMMatrixTranslationFromVector(T_local);

			XMMATRIX W_parent = XMLoadFloat4x4(&transform_parent->world);
			XMMATRIX B = XMLoadFloat4x4(&parentcomponent.world_parent_bind);
			W = W * B * W_parent;

			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);
			XMStoreFloat3(&transform_child->scale, S);
			XMStoreFloat4(&transform_child->rotation, R);
			XMStoreFloat3(&transform_child->translation, T);

			transform_child->world_prev = transform_child->world;
			XMStoreFloat4x4(&transform_child->world, W);
		}

		// Update Bone components:
		for (size_t i = 0; i < bones.GetCount(); ++i)
		{
			BoneComponent& bone = bones[i];
			Entity entity = bones.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			XMMATRIX inverseBindPoseMatrix = XMLoadFloat4x4(&bone.inverseBindPoseMatrix);
			XMMATRIX world = XMLoadFloat4x4(&transform.world);
			XMMATRIX skinningMatrix = inverseBindPoseMatrix * world;
			XMStoreFloat4x4(&bone.skinningMatrix, skinningMatrix);
		}

		// Update Light components:
		for (size_t i = 0; i < lights.GetCount(); ++i)
		{
			auto& light = lights[i];
			Entity entity = bones.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			XMMATRIX world = XMLoadFloat4x4(&transform.world);

			if (light.type == LightComponent::DIRECTIONAL)
			{
				XMStoreFloat3(&sunDirection, -XMVector3TransformNormal(XMVectorSet(0, -1, 0, 1), world));
				sunColor = light.color;
			}
		}

		// Update camera components:
		for (size_t i = 0; i < cameras.GetCount(); ++i)
		{
			CameraComponent& camera = cameras[i];
			Entity entity = cameras.GetEntity(i);
			const TransformComponent* transform = transforms.GetComponent(entity);
			camera.UpdateCamera(transform);
		}

	}
	void Scene::Remove(Entity entity)
	{
		owned_entities.erase(entity);

		names.Remove(entity);
		layers.Remove(entity);
		transforms.Remove(entity);
		parents.Remove(entity);
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

	Entity Scene::Component_FindName(const std::string& name)
	{
		for (size_t i = 0; i < names.GetCount(); ++i)
		{
			if (strcmp(names[i].name, name.c_str()) == 0)
			{
				return names.GetEntity(i);
			}
		}
		return INVALID_ENTITY;
	}
	void Scene::Component_Attach(Entity entity, Entity parent)
	{
		assert(entity != parent);

		auto it = parents.Find(entity);

		if (!it)
		{
			it = parents.Create(entity);
		}

		ParentComponent& parentcomponent = parents.GetComponent(it);
		TransformComponent* transformcomponent = transforms.GetComponent(parent);
		assert(transformcomponent != nullptr);

		parentcomponent.parentID = parent;
		XMStoreFloat4x4(&parentcomponent.world_parent_bind, XMMatrixInverse(nullptr, XMLoadFloat4x4(&transformcomponent->world)));
	}
	void Scene::Component_Detach(wiECS::Entity entity)
	{
		parents.Remove(entity);
	}

}
