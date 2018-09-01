#include "wiSceneSystem.h"
#include "wiMath.h"
#include "wiTextureHelper.h"
#include "wiResourceManager.h"

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
			XMStoreFloat3x3(&rotationMatrix, _Rot);
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
		XMStoreFloat4x4(&View, _V);
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

		// Update Physics components:
		for (size_t i = 0; i < physicscomponents.GetCount(); ++i)
		{
			PhysicsComponent& physicscomponent = physicscomponents[i];
		}

		// Update Material components:
		for (size_t i = 0; i < materials.GetCount(); ++i)
		{
			MaterialComponent& material = materials[i];

			material.texAnimSleep -= dt * material.texAnimFrameRate;
			if (material.texAnimSleep <= 0)
			{
				material.texMulAdd.z = fmodf(material.texMulAdd.z + material.texAnimDirection.x, 1);
				material.texMulAdd.w = fmodf(material.texMulAdd.w + material.texAnimDirection.y, 1);
				material.texAnimSleep = 1.0f;
			}

			material.engineStencilRef = STENCILREF_DEFAULT;
			if (material.subsurfaceScattering > 0)
			{
				material.engineStencilRef = STENCILREF_SKIN;
			}

		}

		// Update Object components:
		for (size_t i = 0; i < objects.GetCount(); ++i)
		{
			ObjectComponent& object = objects[i];
			Entity entity = objects.GetEntity(i);
			CullableComponent& cullable = *cullables.GetComponent(entity);

			cullable.aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0));

			if (object.meshID != INVALID_ENTITY)
			{
				const TransformComponent* transform = transforms.GetComponent(entity);
				const MeshComponent* mesh = meshes.GetComponent(object.meshID);

				if (mesh != nullptr && transform != nullptr)
				{
					cullable.aabb = mesh->aabb.get(transform->world);
				}
			}
		}

		// Update Camera components:
		for (size_t i = 0; i < cameras.GetCount(); ++i)
		{
			CameraComponent& camera = cameras[i];
			Entity entity = cameras.GetEntity(i);
			const TransformComponent* transform = transforms.GetComponent(entity);
			camera.UpdateCamera(transform);
		}

		// Update Decal components:
		for (size_t i = 0; i < decals.GetCount(); ++i)
		{
			DecalComponent& decal = decals[i];
			Entity entity = decals.GetEntity(i);
			const TransformComponent* transform = transforms.GetComponent(entity);
			decal.world = transform->world;
			XMVECTOR front = XMVectorSet(0, 0, 1, 0);
			front = XMVector3TransformNormal(front, XMLoadFloat4x4(&decal.world));
			XMStoreFloat3(&decal.front, front);
		}

		// Update Light components:
		for (size_t i = 0; i < lights.GetCount(); ++i)
		{
			LightComponent& light = lights[i];
			Entity entity = lights.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);
			CullableComponent& cullable = *cullables.GetComponent(entity);

			XMMATRIX world = XMLoadFloat4x4(&transform.world);
			XMVECTOR translation = XMLoadFloat3(&transform.translation);
			XMVECTOR rotation = XMLoadFloat4(&transform.rotation);

			switch (light.type)
			{
			case LightComponent::DIRECTIONAL:
			{
				XMStoreFloat3(&sunDirection, -XMVector3TransformNormal(XMVectorSet(0, -1, 0, 1), world));
				sunColor = light.color;

				if (light.shadow)
				{
					XMFLOAT2 screen = XMFLOAT2((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
					CameraComponent* camera = cameras.GetComponent(wiRenderer::getCameraID());
					float nearPlane = camera->zNearP;
					float farPlane = camera->zFarP;
					XMMATRIX view = camera->GetView();
					XMMATRIX projection = camera->GetRealProjection();
					XMMATRIX world = XMMatrixIdentity();

					// Set up three shadow cascades (far - mid - near):
					const float referenceFrustumDepth = 800.0f;									// this was the frustum depth used for reference
					const float currentFrustumDepth = farPlane - nearPlane;						// current frustum depth
					const float lerp0 = referenceFrustumDepth / currentFrustumDepth * 0.5f;		// third slice distance from cam (percentage)
					const float lerp1 = referenceFrustumDepth / currentFrustumDepth * 0.12f;	// second slice distance from cam (percentage)
					const float lerp2 = referenceFrustumDepth / currentFrustumDepth * 0.016f;	// first slice distance from cam (percentage)

					// Create shadow cascades for main camera:
					if (light.shadowCam_dirLight.empty())
					{
						light.shadowCam_dirLight.resize(3);
					}

					// Place the shadow cascades inside the viewport:
					if (!light.shadowCam_dirLight.empty())
					{
						// frustum top left - near
						XMVECTOR a0 = XMVector3Unproject(XMVectorSet(0, 0, 0, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
						// frustum top left - far
						XMVECTOR a1 = XMVector3Unproject(XMVectorSet(0, 0, 1, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
						// frustum bottom right - near
						XMVECTOR b0 = XMVector3Unproject(XMVectorSet(screen.x, screen.y, 0, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
						// frustum bottom right - far
						XMVECTOR b1 = XMVector3Unproject(XMVectorSet(screen.x, screen.y, 1, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);

						// calculate cascade projection sizes:
						float size0 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b1, lerp0), XMVectorLerp(a0, a1, lerp0))));
						float size1 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b1, lerp1), XMVectorLerp(a0, a1, lerp1))));
						float size2 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b1, lerp2), XMVectorLerp(a0, a1, lerp2))));

						XMVECTOR rotDefault = XMQuaternionIdentity();

						// create shadow cascade projections:
						light.shadowCam_dirLight[0] = LightComponent::SHCAM(size0, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);
						light.shadowCam_dirLight[1] = LightComponent::SHCAM(size1, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);
						light.shadowCam_dirLight[2] = LightComponent::SHCAM(size2, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);

						// frustum center - near
						XMVECTOR c = XMVector3Unproject(XMVectorSet(screen.x * 0.5f, screen.y * 0.5f, 0, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
						// frustum center - far
						XMVECTOR d = XMVector3Unproject(XMVectorSet(screen.x * 0.5f, screen.y * 0.5f, 1, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);

						// Avoid shadowmap texel swimming by aligning them to a discrete grid:
						float f0 = light.shadowCam_dirLight[0].size / (float)wiRenderer::SHADOWRES_2D;
						float f1 = light.shadowCam_dirLight[1].size / (float)wiRenderer::SHADOWRES_2D;
						float f2 = light.shadowCam_dirLight[2].size / (float)wiRenderer::SHADOWRES_2D;
						XMVECTOR e0 = XMVectorFloor(XMVectorLerp(c, d, lerp0) / f0) * f0;
						XMVECTOR e1 = XMVectorFloor(XMVectorLerp(c, d, lerp1) / f1) * f1;
						XMVECTOR e2 = XMVectorFloor(XMVectorLerp(c, d, lerp2) / f2) * f2;

						XMMATRIX rrr = XMMatrixRotationQuaternion(XMLoadFloat4(&transform.rotation));

						light.shadowCam_dirLight[0].Update(rrr, e0);
						light.shadowCam_dirLight[1].Update(rrr, e1);
						light.shadowCam_dirLight[2].Update(rrr, e2);
					}
				}

				cullable.aabb.createFromHalfWidth(wiRenderer::getCamera()->Eye, XMFLOAT3(10000, 10000, 10000));
			}
			break;
			case LightComponent::SPOT:
			{
				if (light.shadow)
				{
					if (light.shadowCam_spotLight.empty())
					{
						light.shadowCam_spotLight.push_back(LightComponent::SHCAM(XMFLOAT4(0, 0, 0, 1), 0.1f, 1000.0f, light.fov));
					}
					light.shadowCam_spotLight[0].Update(world);
					light.shadowCam_spotLight[0].farplane = light.range;
					light.shadowCam_spotLight[0].Create_Perspective(light.fov);
				}

				cullable.aabb.createFromHalfWidth(transform.translation, XMFLOAT3(light.range, light.range, light.range));
			}
			break;
			case LightComponent::POINT:
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
			{
				if (light.shadow)
				{
					if (light.shadowCam_pointLight.empty())
					{
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), 0.1f, 1000.0f, XM_PIDIV2)); //+x
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), 0.1f, 1000.0f, XM_PIDIV2)); //-x

						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(1, 0, 0, -0), 0.1f, 1000.0f, XM_PIDIV2)); //+y
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0, 0, 0, -1), 0.1f, 1000.0f, XM_PIDIV2)); //-y

						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0.707f, 0, 0, -0.707f), 0.1f, 1000.0f, XM_PIDIV2)); //+z
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0, 0.707f, 0.707f, 0), 0.1f, 1000.0f, XM_PIDIV2)); //-z
					}
					for (auto& x : light.shadowCam_pointLight) {
						x.Update(translation);
						x.farplane = max(1.0f, light.range);
						x.Create_Perspective(XM_PIDIV2);
					}
				}

				if (light.type == LightComponent::POINT)
				{
					cullable.aabb.createFromHalfWidth(transform.translation, XMFLOAT3(light.range, light.range, light.range));
				}
				else
				{
					// area lights have no bounds, just like directional lights (maybe todo)
					cullable.aabb.createFromHalfWidth(wiRenderer::getCamera()->Eye, XMFLOAT3(10000, 10000, 10000));
				}
			}
			break;
			}

		}

		// Iterate all cullables and recompute scene bounds:
		bounds.create(XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX), XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
		for (size_t i = 0; i < cullables.GetCount(); ++i)
		{
			const CullableComponent& cullable = cullables[i];

			bounds = AABB::Merge(bounds, cullable.aabb);
		}

	}
	void Scene::Clear()
	{
		for (Entity entity : owned_entities)
		{
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

		owned_entities.clear();
	}

	void Scene::Entity_Remove(Entity entity)
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
	Entity Scene::Entity_FindByName(const std::string& name)
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
	Entity Scene::Entity_CreateLight(
		const std::string& name,
		const XMFLOAT3& position,
		const XMFLOAT3& color,
		float energy,
		float range)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		transforms.Create(entity).Translate(position);

		cullables.Create(entity).aabb.createFromHalfWidth(position, XMFLOAT3(range, range, range));

		LightComponent& light = lights.Create(entity);
		light.energy = energy;
		light.range = range;
		light.fov = XM_PIDIV4;
		light.color = color;
		light.SetType(LightComponent::POINT);

		return entity;
	}
	wiECS::Entity Scene::Entity_CreateForce(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		transforms.Create(entity).Translate(position);

		ForceFieldComponent& force = forces.Create(entity);
		force.gravity = 0;
		force.range = 0;
		force.type = ENTITY_TYPE_FORCEFIELD_POINT;

		return entity;
	}
	wiECS::Entity Scene::Entity_CreateEnvironmentProbe(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		transforms.Create(entity).Translate(position);

		EnvironmentProbeComponent& probe = probes.Create(entity);
		probe.isUpToDate = false;
		probe.realTime = false;
		probe.textureIndex = -1;

		return entity;
	}
	wiECS::Entity Scene::Entity_CreateDecal(
		const std::string& name,
		const std::string& textureName,
		const std::string& normalMapName
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		transforms.Create(entity);

		cullables.Create(entity);

		DecalComponent& decal = decals.Create(entity);
		decal.textureName = textureName;
		decal.texture = (Texture2D*)wiResourceManager::GetGlobal()->add(decal.textureName);
		decal.normalMapName = normalMapName;
		decal.normal = (Texture2D*)wiResourceManager::GetGlobal()->add(decal.normalMapName);

		return entity;
	}

	void Scene::Component_Attach(Entity entity, Entity parent)
	{
		assert(entity != parent);

		ParentComponent* parentcomponent = parents.GetComponent(entity);

		if (parentcomponent == nullptr)
		{
			parentcomponent = &parents.Create(entity);
		}

		TransformComponent* transformcomponent = transforms.GetComponent(parent);
		assert(transformcomponent != nullptr);

		parentcomponent->parentID = parent;
		XMStoreFloat4x4(&parentcomponent->world_parent_bind, XMMatrixInverse(nullptr, XMLoadFloat4x4(&transformcomponent->world)));
	}
	void Scene::Component_Detach(wiECS::Entity entity)
	{
		TransformComponent* transform = transforms.GetComponent(entity);
		if (transform != nullptr) 
		{
			transform->ApplyTransform();
		}
		parents.Remove(entity);
	}
	void Scene::Component_DetachChildren(wiECS::Entity parent)
	{
		for (size_t i = 0; i < parents.GetCount(); )
		{
			if (parents[i].parentID == parent)
			{
				Entity entity = parents.GetEntity(i);
				Component_Detach(entity);
			}
			else
			{
				++i;
			}
		}
	}

}
