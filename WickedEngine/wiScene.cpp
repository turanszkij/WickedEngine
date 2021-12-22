#include "wiScene.h"
#include "wiTextureHelper.h"
#include "wiResourceManager.h"
#include "wiPhysics.h"
#include "wiArchive.h"
#include "wiRenderer.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBacklog.h"
#include "wiTimer.h"
#include "wiUnorderedMap.h"

#include "shaders/ShaderInterop_SurfelGI.h"

using namespace wi::ecs;
using namespace wi::enums;
using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::scene
{

	XMFLOAT3 TransformComponent::GetPosition() const
	{
		return *((XMFLOAT3*)&world._41);
	}
	XMFLOAT4 TransformComponent::GetRotation() const
	{
		XMFLOAT4 rotation;
		XMStoreFloat4(&rotation, GetRotationV());
		return rotation;
	}
	XMFLOAT3 TransformComponent::GetScale() const
	{
		XMFLOAT3 scale;
		XMStoreFloat3(&scale, GetScaleV());
		return scale;
	}
	XMVECTOR TransformComponent::GetPositionV() const
	{
		return XMLoadFloat3((XMFLOAT3*)&world._41);
	}
	XMVECTOR TransformComponent::GetRotationV() const
	{
		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
		return R;
	}
	XMVECTOR TransformComponent::GetScaleV() const
	{
		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
		return S;
	}
	XMMATRIX TransformComponent::GetLocalMatrix() const
	{
		XMVECTOR S_local = XMLoadFloat3(&scale_local);
		XMVECTOR R_local = XMLoadFloat4(&rotation_local);
		XMVECTOR T_local = XMLoadFloat3(&translation_local);
		return
			XMMatrixScalingFromVector(S_local) *
			XMMatrixRotationQuaternion(R_local) *
			XMMatrixTranslationFromVector(T_local);
	}
	void TransformComponent::UpdateTransform()
	{
		if (IsDirty())
		{
			SetDirty(false);

			XMStoreFloat4x4(&world, GetLocalMatrix());
		}
	}
	void TransformComponent::UpdateTransform_Parented(const TransformComponent& parent)
	{
		XMMATRIX W = GetLocalMatrix();
		XMMATRIX W_parent = XMLoadFloat4x4(&parent.world);
		W = W * W_parent;

		XMStoreFloat4x4(&world, W);
	}
	void TransformComponent::ApplyTransform()
	{
		SetDirty();

		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
		XMStoreFloat3(&scale_local, S);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&translation_local, T);
	}
	void TransformComponent::ClearTransform()
	{
		SetDirty();
		scale_local = XMFLOAT3(1, 1, 1);
		rotation_local = XMFLOAT4(0, 0, 0, 1);
		translation_local = XMFLOAT3(0, 0, 0);
	}
	void TransformComponent::Translate(const XMFLOAT3& value)
	{
		SetDirty();
		translation_local.x += value.x;
		translation_local.y += value.y;
		translation_local.z += value.z;
	}
	void TransformComponent::Translate(const XMVECTOR& value)
	{
		XMFLOAT3 translation;
		XMStoreFloat3(&translation, value);
		Translate(translation);
	}
	void TransformComponent::RotateRollPitchYaw(const XMFLOAT3& value)
	{
		SetDirty();

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
		SetDirty();

		XMVECTOR result = XMQuaternionMultiply(XMLoadFloat4(&rotation_local), XMLoadFloat4(&quaternion));
		result = XMQuaternionNormalize(result);
		XMStoreFloat4(&rotation_local, result);
	}
	void TransformComponent::Rotate(const XMVECTOR& quaternion)
	{
		XMFLOAT4 rotation;
		XMStoreFloat4(&rotation, quaternion);
		Rotate(rotation);
	}
	void TransformComponent::Scale(const XMFLOAT3& value)
	{
		SetDirty();
		scale_local.x *= value.x;
		scale_local.y *= value.y;
		scale_local.z *= value.z;
	}
	void TransformComponent::Scale(const XMVECTOR& value)
	{
		XMFLOAT3 scale;
		XMStoreFloat3(&scale, value);
		Scale(scale);
	}
	void TransformComponent::MatrixTransform(const XMFLOAT4X4& matrix)
	{
		MatrixTransform(XMLoadFloat4x4(&matrix));
	}
	void TransformComponent::MatrixTransform(const XMMATRIX& matrix)
	{
		SetDirty();

		XMVECTOR S;
		XMVECTOR R;
		XMVECTOR T;
		XMMatrixDecompose(&S, &R, &T, GetLocalMatrix() * matrix);

		XMStoreFloat3(&scale_local, S);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&translation_local, T);
	}
	void TransformComponent::Lerp(const TransformComponent& a, const TransformComponent& b, float t)
	{
		SetDirty();

		XMVECTOR aS, aR, aT;
		XMMatrixDecompose(&aS, &aR, &aT, XMLoadFloat4x4(&a.world));

		XMVECTOR bS, bR, bT;
		XMMatrixDecompose(&bS, &bR, &bT, XMLoadFloat4x4(&b.world));

		XMVECTOR S = XMVectorLerp(aS, bS, t);
		XMVECTOR R = XMQuaternionSlerp(aR, bR, t);
		XMVECTOR T = XMVectorLerp(aT, bT, t);

		XMStoreFloat3(&scale_local, S);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&translation_local, T);
	}
	void TransformComponent::CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t)
	{
		SetDirty();

		XMVECTOR aS, aR, aT;
		XMMatrixDecompose(&aS, &aR, &aT, XMLoadFloat4x4(&a.world));

		XMVECTOR bS, bR, bT;
		XMMatrixDecompose(&bS, &bR, &bT, XMLoadFloat4x4(&b.world));

		XMVECTOR cS, cR, cT;
		XMMatrixDecompose(&cS, &cR, &cT, XMLoadFloat4x4(&c.world));

		XMVECTOR dS, dR, dT;
		XMMatrixDecompose(&dS, &dR, &dT, XMLoadFloat4x4(&d.world));

		XMVECTOR T = XMVectorCatmullRom(aT, bT, cT, dT, t);

		XMVECTOR setupA;
		XMVECTOR setupB;
		XMVECTOR setupC;

		aR = XMQuaternionNormalize(aR);
		bR = XMQuaternionNormalize(bR);
		cR = XMQuaternionNormalize(cR);
		dR = XMQuaternionNormalize(dR);

		XMQuaternionSquadSetup(&setupA, &setupB, &setupC, aR, bR, cR, dR);
		XMVECTOR R = XMQuaternionSquad(bR, setupA, setupB, setupC, t);

		XMVECTOR S = XMVectorCatmullRom(aS, bS, cS, dS, t);

		XMStoreFloat3(&translation_local, T);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&scale_local, S);
	}

	void MaterialComponent::WriteShaderMaterial(ShaderMaterial* dest) const
	{
		dest->baseColor = baseColor;
		dest->emissive_r11g11b10 = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(emissiveColor.x * emissiveColor.w, emissiveColor.y * emissiveColor.w, emissiveColor.z * emissiveColor.w));
		dest->specular_r11g11b10 = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(specularColor.x * specularColor.w, specularColor.y * specularColor.w, specularColor.z * specularColor.w));
		dest->texMulAdd = texMulAdd;
		dest->roughness = roughness;
		dest->reflectance = reflectance;
		dest->metalness = metalness;
		dest->refraction = refraction;
		dest->normalMapStrength = (textures[NORMALMAP].resource.IsValid() ? normalMapStrength : 0);
		dest->parallaxOcclusionMapping = parallaxOcclusionMapping;
		dest->displacementMapping = displacementMapping;
		XMFLOAT4 sss = subsurfaceScattering;
		sss.x *= sss.w;
		sss.y *= sss.w;
		sss.z *= sss.w;
		XMFLOAT4 sss_inv = XMFLOAT4(
			sss_inv.x = 1.0f / ((1 + sss.x) * (1 + sss.x)),
			sss_inv.y = 1.0f / ((1 + sss.y) * (1 + sss.y)),
			sss_inv.z = 1.0f / ((1 + sss.z) * (1 + sss.z)),
			sss_inv.w = 1.0f / ((1 + sss.w) * (1 + sss.w))
		);
		dest->subsurfaceScattering = sss;
		dest->subsurfaceScattering_inv = sss_inv;
		dest->uvset_baseColorMap = textures[BASECOLORMAP].GetUVSet();
		dest->uvset_surfaceMap = textures[SURFACEMAP].GetUVSet();
		dest->uvset_normalMap = textures[NORMALMAP].GetUVSet();
		dest->uvset_displacementMap = textures[DISPLACEMENTMAP].GetUVSet();
		dest->uvset_emissiveMap = textures[EMISSIVEMAP].GetUVSet();
		dest->uvset_occlusionMap = textures[OCCLUSIONMAP].GetUVSet();
		dest->uvset_transmissionMap = textures[TRANSMISSIONMAP].GetUVSet();
		dest->uvset_sheenColorMap = textures[SHEENCOLORMAP].GetUVSet();
		dest->uvset_sheenRoughnessMap = textures[SHEENROUGHNESSMAP].GetUVSet();
		dest->uvset_clearcoatMap = textures[CLEARCOATMAP].GetUVSet();
		dest->uvset_clearcoatRoughnessMap = textures[CLEARCOATROUGHNESSMAP].GetUVSet();
		dest->uvset_clearcoatNormalMap = textures[CLEARCOATNORMALMAP].GetUVSet();
		dest->uvset_specularMap = textures[SPECULARMAP].GetUVSet();
		dest->sheenColor_r11g11b10 = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(sheenColor.x, sheenColor.y, sheenColor.z));
		dest->sheenRoughness = sheenRoughness;
		dest->clearcoat = clearcoat;
		dest->clearcoatRoughness = clearcoatRoughness;
		dest->alphaTest = 1 - alphaRef;
		dest->layerMask = layerMask;
		dest->transmission = transmission;

		uint32_t options = 0;
		if (IsUsingVertexColors())
		{
			options |= SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS;
		}
		if (IsUsingSpecularGlossinessWorkflow())
		{
			options |= SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW;
		}
		if (IsOcclusionEnabled_Primary())
		{
			options |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY;
		}
		if (IsOcclusionEnabled_Secondary())
		{
			options |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY;
		}
		if (IsUsingWind())
		{
			options |= SHADERMATERIAL_OPTION_BIT_USE_WIND;
		}
		if (IsReceiveShadow())
		{
			options |= SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW;
		}
		if (IsCastingShadow())
		{
			options |= SHADERMATERIAL_OPTION_BIT_CAST_SHADOW;
		}
		if (IsDoubleSided())
		{
			options |= SHADERMATERIAL_OPTION_BIT_DOUBLE_SIDED;
		}
		if (GetRenderTypes() & RENDERTYPE_TRANSPARENT)
		{
			options |= SHADERMATERIAL_OPTION_BIT_TRANSPARENT;
		}
		if (userBlendMode == BLENDMODE_ADDITIVE)
		{
			options |= SHADERMATERIAL_OPTION_BIT_ADDITIVE;
		}
		if (shaderType == SHADERTYPE_UNLIT)
		{
			options |= SHADERMATERIAL_OPTION_BIT_UNLIT;
		}
		dest->options = options; // ensure that this memory is not read, so bitwise ORs also not performed with it!

		GraphicsDevice* device = wi::graphics::GetDevice();
		dest->texture_basecolormap_index = device->GetDescriptorIndex(textures[BASECOLORMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_surfacemap_index = device->GetDescriptorIndex(textures[SURFACEMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_emissivemap_index = device->GetDescriptorIndex(textures[EMISSIVEMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_normalmap_index = device->GetDescriptorIndex(textures[NORMALMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_displacementmap_index = device->GetDescriptorIndex(textures[DISPLACEMENTMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_occlusionmap_index = device->GetDescriptorIndex(textures[OCCLUSIONMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_transmissionmap_index = device->GetDescriptorIndex(textures[TRANSMISSIONMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_sheencolormap_index = device->GetDescriptorIndex(textures[SHEENCOLORMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_sheenroughnessmap_index = device->GetDescriptorIndex(textures[SHEENROUGHNESSMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_clearcoatmap_index = device->GetDescriptorIndex(textures[CLEARCOATMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_clearcoatroughnessmap_index = device->GetDescriptorIndex(textures[CLEARCOATROUGHNESSMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_clearcoatnormalmap_index = device->GetDescriptorIndex(textures[CLEARCOATNORMALMAP].GetGPUResource(), SubresourceType::SRV);
		dest->texture_specularmap_index = device->GetDescriptorIndex(textures[SPECULARMAP].GetGPUResource(), SubresourceType::SRV);

	}
	void MaterialComponent::WriteTextures(const wi::graphics::GPUResource** dest, int count) const
	{
		count = std::min(count, (int)TEXTURESLOT_COUNT);
		for (int i = 0; i < count; ++i)
		{
			dest[i] = textures[i].GetGPUResource();
		}
	}
	uint32_t MaterialComponent::GetRenderTypes() const
	{
		if (IsCustomShader() && customShaderID < (int)wi::renderer::GetCustomShaders().size())
		{
			auto& customShader = wi::renderer::GetCustomShaders()[customShaderID];
			return customShader.renderTypeFlags;
		}
		if (shaderType == SHADERTYPE_WATER)
		{
			return RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER;
		}
		if (transmission > 0)
		{
			return RENDERTYPE_TRANSPARENT;
		}
		if (userBlendMode == BLENDMODE_OPAQUE)
		{
			return RENDERTYPE_OPAQUE;
		}
		return RENDERTYPE_TRANSPARENT;
	}
	void MaterialComponent::CreateRenderData()
	{
		for (auto& x : textures)
		{
			if (!x.name.empty())
			{
				x.resource = wi::resourcemanager::Load(x.name, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
			}
		}
	}
	uint32_t MaterialComponent::GetStencilRef() const
	{
		return wi::renderer::CombineStencilrefs(engineStencilRef, userStencilRef);
	}

	void MeshComponent::CreateRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		vertex_subsets.resize(vertex_positions.size());
		uint32_t subsetCounter = 0;
		for (auto& subset : subsets)
		{
			for (uint32_t i = 0; i < subset.indexCount; ++i)
			{
				uint32_t index = indices[subset.indexOffset + i];
				vertex_subsets[index] = subsetCounter;
			}
			subsetCounter++;
		}

		// Create index buffer GPU data:
		{
			GPUBufferDesc bd;
			bd.bind_flags = BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE;
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
			}

			if (GetIndexFormat() == IndexBufferFormat::UINT32)
			{
				bd.stride = sizeof(uint32_t);
				bd.format = Format::R32_UINT;
				bd.size = uint32_t(sizeof(uint32_t) * indices.size());

				// Use indices directly since vector is in correct format
				static_assert(std::is_same<decltype(indices)::value_type, uint32_t>::value, "indices not in IndexBufferFormat::UINT32");

				device->CreateBuffer(&bd, indices.data(), &indexBuffer);
				device->SetName(&indexBuffer, "indexBuffer_32bit");
			}
			else
			{
				bd.stride = sizeof(uint16_t);
				bd.format = Format::R16_UINT;
				bd.size = uint32_t(sizeof(uint16_t) * indices.size());

				wi::vector<uint16_t> gpuIndexData(indices.size());
				std::copy(indices.begin(), indices.end(), gpuIndexData.begin());

				device->CreateBuffer(&bd, gpuIndexData.data(), &indexBuffer);
				device->SetName(&indexBuffer, "indexBuffer_16bit");
			}
		}


		XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

		// vertexBuffer - POSITION + NORMAL + WIND:
		{
		    if (!targets.empty())
		    {
				vertex_positions_morphed.resize(vertex_positions.size());
				dirty_morph = true;
		    }

			wi::vector<Vertex_POS> vertices(vertex_positions.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				const XMFLOAT3& pos = vertex_positions[i];
			    XMFLOAT3 nor = vertex_normals.empty() ? XMFLOAT3(1, 1, 1) : vertex_normals[i];
			    XMStoreFloat3(&nor, XMVector3Normalize(XMLoadFloat3(&nor)));
				const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
				vertices[i].FromFULL(pos, nor, wind);

				_min = wi::math::Min(_min, pos);
				_max = wi::math::Max(_max, pos);
			}

			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
			}
			bd.size = (uint32_t)(sizeof(Vertex_POS) * vertices.size());

			device->CreateBuffer(&bd, vertices.data(), &vertexBuffer_POS);
			device->SetName(&vertexBuffer_POS, "vertexBuffer_POS");
		}

		// vertexBuffer - TANGENTS
		if(!vertex_uvset_0.empty())
		{
			if (vertex_tangents.empty())
			{
				// Generate tangents if not found:
				vertex_tangents.resize(vertex_positions.size());

				for (size_t i = 0; i < indices.size(); i += 3)
				{
					const uint32_t i0 = indices[i + 0];
					const uint32_t i1 = indices[i + 1];
					const uint32_t i2 = indices[i + 2];

					const XMFLOAT3 v0 = vertex_positions[i0];
					const XMFLOAT3 v1 = vertex_positions[i1];
					const XMFLOAT3 v2 = vertex_positions[i2];

					const XMFLOAT2 u0 = vertex_uvset_0[i0];
					const XMFLOAT2 u1 = vertex_uvset_0[i1];
					const XMFLOAT2 u2 = vertex_uvset_0[i2];

					const XMFLOAT3 n0 = vertex_normals[i0];
					const XMFLOAT3 n1 = vertex_normals[i1];
					const XMFLOAT3 n2 = vertex_normals[i2];

					const XMVECTOR nor0 = XMLoadFloat3(&n0);
					const XMVECTOR nor1 = XMLoadFloat3(&n1);
					const XMVECTOR nor2 = XMLoadFloat3(&n2);

					const XMVECTOR facenormal = XMVector3Normalize(nor0 + nor1 + nor2);

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
					tangent = XMVector3Normalize(sdir - facenormal * XMVector3Dot(facenormal, sdir));
					float sign = XMVectorGetX(XMVector3Dot(XMVector3Cross(tangent, facenormal), tdir)) < 0.0f ? -1.0f : 1.0f;

					XMFLOAT3 t;
					XMStoreFloat3(&t, tangent);

					vertex_tangents[i0].x += t.x;
					vertex_tangents[i0].y += t.y;
					vertex_tangents[i0].z += t.z;
					vertex_tangents[i0].w = sign;

					vertex_tangents[i1].x += t.x;
					vertex_tangents[i1].y += t.y;
					vertex_tangents[i1].z += t.z;
					vertex_tangents[i1].w = sign;

					vertex_tangents[i2].x += t.x;
					vertex_tangents[i2].y += t.y;
					vertex_tangents[i2].z += t.z;
					vertex_tangents[i2].w = sign;
				}

			}

			wi::vector<Vertex_TAN> vertices(vertex_tangents.size());
			for (size_t i = 0; i < vertex_tangents.size(); ++i)
			{
				vertices[i].FromFULL(vertex_tangents[i]);
			}

			GPUBufferDesc bd;
			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(Vertex_TAN);
			bd.size = (uint32_t)(bd.stride * vertices.size());

			device->CreateBuffer(&bd, vertices.data(), &vertexBuffer_TAN);
			device->SetName(&vertexBuffer_TAN, "vertexBuffer_TAN");
		}

		aabb = AABB(_min, _max);

		// skinning buffers:
		if (!vertex_boneindices.empty())
		{
			wi::vector<Vertex_BON> vertices(vertex_boneindices.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				XMFLOAT4& wei = vertex_boneweights[i];
				// normalize bone weights
				float len = wei.x + wei.y + wei.z + wei.w;
				if (len > 0)
				{
					wei.x /= len;
					wei.y /= len;
					wei.z /= len;
					wei.w /= len;
				}
				vertices[i].FromFULL(vertex_boneindices[i], wei);
			}

			GPUBufferDesc bd;
			bd.bind_flags = BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.size = (uint32_t)(sizeof(Vertex_BON) * vertices.size());

			device->CreateBuffer(&bd, vertices.data(), &vertexBuffer_BON);

			bd.usage = Usage::DEFAULT;
			bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;

			if (!vertex_tangents.empty())
			{
				bd.size = (uint32_t)(sizeof(Vertex_TAN) * vertex_tangents.size());
				device->CreateBuffer(&bd, nullptr, &streamoutBuffer_TAN);
				device->SetName(&streamoutBuffer_TAN, "streamoutBuffer_TAN");
			}

			bd.size = (uint32_t)(sizeof(Vertex_POS) * vertex_positions.size());
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
			}
			device->CreateBuffer(&bd, nullptr, &streamoutBuffer_POS);
			device->SetName(&streamoutBuffer_POS, "streamoutBuffer_POS");
		}

		// vertexBuffer - UV SET 0
		if(!vertex_uvset_0.empty())
		{
			wi::vector<Vertex_TEX> vertices(vertex_uvset_0.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertices[i].FromFULL(vertex_uvset_0[i]);
			}

			GPUBufferDesc bd;
			bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(Vertex_TEX);
			bd.size = (uint32_t)(bd.stride * vertices.size());

			device->CreateBuffer(&bd, vertices.data(), &vertexBuffer_UV0);
			device->SetName(&vertexBuffer_UV0, "vertexBuffer_UV0");
		}

		// vertexBuffer - UV SET 1
		if (!vertex_uvset_1.empty())
		{
			wi::vector<Vertex_TEX> vertices(vertex_uvset_1.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertices[i].FromFULL(vertex_uvset_1[i]);
			}

			GPUBufferDesc bd;
			bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(Vertex_TEX);
			bd.size = (uint32_t)(bd.stride * vertices.size());

			device->CreateBuffer(&bd, vertices.data(), &vertexBuffer_UV1);
			device->SetName(&vertexBuffer_UV1, "vertexBuffer_UV1");
		}

		// vertexBuffer - COLORS
		if (!vertex_colors.empty())
		{
			GPUBufferDesc bd;
			bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(Vertex_COL);
			bd.size = (uint32_t)(bd.stride * vertex_colors.size());

			device->CreateBuffer(&bd, vertex_colors.data(), &vertexBuffer_COL);
			device->SetName(&vertexBuffer_COL, "vertexBuffer_COL");
		}

		// vertexBuffer - ATLAS
		if (!vertex_atlas.empty())
		{
			wi::vector<Vertex_TEX> vertices(vertex_atlas.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertices[i].FromFULL(vertex_atlas[i]);
			}

			GPUBufferDesc bd;
			bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE;
			bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bd.stride = sizeof(Vertex_TEX);
			bd.size = (uint32_t)(bd.stride * vertices.size());

			device->CreateBuffer(&bd, vertices.data(), &vertexBuffer_ATL);
			device->SetName(&vertexBuffer_ATL, "vertexBuffer_ATL");
		}

		// vertexBuffer_PRE will be created on demand later!
		vertexBuffer_PRE = GPUBuffer();

		GPUBufferDesc desc;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
		desc.stride = sizeof(ShaderMeshSubset);
		desc.size = desc.stride * (uint32_t)subsets.size();
		bool success = device->CreateBuffer(&desc, nullptr, &subsetBuffer);
		assert(success);
		dirty_subsets = true;


		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;

			RaytracingAccelerationStructureDesc desc;
			desc.type = RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL;

			if (streamoutBuffer_POS.IsValid())
			{
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE;
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;
			}
			else
			{
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE;
			}

			for (auto& subset : subsets)
			{
				desc.bottom_level.geometries.emplace_back();
				auto& geometry = desc.bottom_level.geometries.back();
				geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES;
				geometry.triangles.vertex_buffer = streamoutBuffer_POS.IsValid() ? streamoutBuffer_POS : vertexBuffer_POS;
				geometry.triangles.index_buffer = indexBuffer;
				geometry.triangles.index_format = GetIndexFormat();
				geometry.triangles.index_count = subset.indexCount;
				geometry.triangles.index_offset = subset.indexOffset;
				geometry.triangles.vertex_count = (uint32_t)vertex_positions.size();
				geometry.triangles.vertex_format = Format::R32G32B32_FLOAT;
				geometry.triangles.vertex_stride = sizeof(MeshComponent::Vertex_POS);
			}

			bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLAS);
			assert(success);
			device->SetName(&BLAS, "BLAS");
		}
	}
	void MeshComponent::WriteShaderMesh(ShaderMesh* dest) const
	{
		dest->init();
		GraphicsDevice* device = wi::graphics::GetDevice();
		dest->ib = device->GetDescriptorIndex(&indexBuffer, SubresourceType::SRV);
		if (streamoutBuffer_POS.IsValid())
		{
			dest->vb_pos_nor_wind = device->GetDescriptorIndex(&streamoutBuffer_POS, SubresourceType::SRV);
		}
		else
		{
			dest->vb_pos_nor_wind = device->GetDescriptorIndex(&vertexBuffer_POS, SubresourceType::SRV);
		}
		if (streamoutBuffer_TAN.IsValid())
		{
			dest->vb_tan = device->GetDescriptorIndex(&streamoutBuffer_TAN, SubresourceType::SRV);
		}
		else
		{
			dest->vb_tan = device->GetDescriptorIndex(&vertexBuffer_TAN, SubresourceType::SRV);
		}
		dest->vb_col = device->GetDescriptorIndex(&vertexBuffer_COL, SubresourceType::SRV);
		dest->vb_uv0 = device->GetDescriptorIndex(&vertexBuffer_UV0, SubresourceType::SRV);
		dest->vb_uv1 = device->GetDescriptorIndex(&vertexBuffer_UV1, SubresourceType::SRV);
		dest->vb_atl = device->GetDescriptorIndex(&vertexBuffer_ATL, SubresourceType::SRV);
		dest->vb_pre = device->GetDescriptorIndex(&vertexBuffer_PRE, SubresourceType::SRV);
		dest->blendmaterial1 = terrain_material1_index;
		dest->blendmaterial2 = terrain_material2_index;
		dest->blendmaterial3 = terrain_material3_index;
		dest->subsetbuffer = device->GetDescriptorIndex(&subsetBuffer, SubresourceType::SRV);
		dest->aabb_min = aabb._min;
		dest->aabb_max = aabb._max;
		dest->tessellation_factor = tessellationFactor;

		uint flags = 0;
		if (IsDoubleSided())
		{
			flags |= SHADERMESH_FLAG_DOUBLE_SIDED;
		}
		dest->flags = flags; // ensure that this memory is not read, so bitwise ORs also not performed with it!

	}
	void MeshComponent::ComputeNormals(COMPUTE_NORMALS compute)
	{
		// Start recalculating normals:

		if(compute != COMPUTE_NORMALS_SMOOTH_FAST)
		{
			// Compute hard surface normals:

			// Right now they are always computed even before smooth setting

			wi::vector<uint32_t> newIndexBuffer;
			wi::vector<XMFLOAT3> newPositionsBuffer;
			wi::vector<XMFLOAT3> newNormalsBuffer;
			wi::vector<XMFLOAT2> newUV0Buffer;
			wi::vector<XMFLOAT2> newUV1Buffer;
			wi::vector<XMFLOAT2> newAtlasBuffer;
			wi::vector<XMUINT4> newBoneIndicesBuffer;
			wi::vector<XMFLOAT4> newBoneWeightsBuffer;
			wi::vector<uint32_t> newColorsBuffer;

			for (size_t face = 0; face < indices.size() / 3; face++)
			{
				uint32_t i0 = indices[face * 3 + 0];
				uint32_t i1 = indices[face * 3 + 1];
				uint32_t i2 = indices[face * 3 + 2];

				XMFLOAT3& p0 = vertex_positions[i0];
				XMFLOAT3& p1 = vertex_positions[i1];
				XMFLOAT3& p2 = vertex_positions[i2];

				XMVECTOR U = XMLoadFloat3(&p2) - XMLoadFloat3(&p0);
				XMVECTOR V = XMLoadFloat3(&p1) - XMLoadFloat3(&p0);

				XMVECTOR N = XMVector3Cross(U, V);
				N = XMVector3Normalize(N);

				XMFLOAT3 normal;
				XMStoreFloat3(&normal, N);

				newPositionsBuffer.push_back(p0);
				newPositionsBuffer.push_back(p1);
				newPositionsBuffer.push_back(p2);

				newNormalsBuffer.push_back(normal);
				newNormalsBuffer.push_back(normal);
				newNormalsBuffer.push_back(normal);

				if (!vertex_uvset_0.empty())
				{
					newUV0Buffer.push_back(vertex_uvset_0[i0]);
					newUV0Buffer.push_back(vertex_uvset_0[i1]);
					newUV0Buffer.push_back(vertex_uvset_0[i2]);
				}

				if (!vertex_uvset_1.empty())
				{
					newUV1Buffer.push_back(vertex_uvset_1[i0]);
					newUV1Buffer.push_back(vertex_uvset_1[i1]);
					newUV1Buffer.push_back(vertex_uvset_1[i2]);
				}

				if (!vertex_atlas.empty())
				{
					newAtlasBuffer.push_back(vertex_atlas[i0]);
					newAtlasBuffer.push_back(vertex_atlas[i1]);
					newAtlasBuffer.push_back(vertex_atlas[i2]);
				}

				if (!vertex_boneindices.empty())
				{
					newBoneIndicesBuffer.push_back(vertex_boneindices[i0]);
					newBoneIndicesBuffer.push_back(vertex_boneindices[i1]);
					newBoneIndicesBuffer.push_back(vertex_boneindices[i2]);
				}

				if (!vertex_boneweights.empty())
				{
					newBoneWeightsBuffer.push_back(vertex_boneweights[i0]);
					newBoneWeightsBuffer.push_back(vertex_boneweights[i1]);
					newBoneWeightsBuffer.push_back(vertex_boneweights[i2]);
				}

				if (!vertex_colors.empty())
				{
					newColorsBuffer.push_back(vertex_colors[i0]);
					newColorsBuffer.push_back(vertex_colors[i1]);
					newColorsBuffer.push_back(vertex_colors[i2]);
				}

				newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
				newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
				newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
			}

			// For hard surface normals, we created a new mesh in the previous loop through faces, so swap data:
			vertex_positions = newPositionsBuffer;
			vertex_normals = newNormalsBuffer;
			vertex_uvset_0 = newUV0Buffer;
			vertex_uvset_1 = newUV1Buffer;
			vertex_atlas = newAtlasBuffer;
			vertex_colors = newColorsBuffer;
			if (!vertex_boneindices.empty())
			{
				vertex_boneindices = newBoneIndicesBuffer;
			}
			if (!vertex_boneweights.empty())
			{
				vertex_boneweights = newBoneWeightsBuffer;
			}
			indices = newIndexBuffer;
		}

		switch (compute)
		{
		case MeshComponent::COMPUTE_NORMALS_HARD: 
		break;

		case MeshComponent::COMPUTE_NORMALS_SMOOTH:
		{
			// Compute smooth surface normals:

			// 1.) Zero normals, they will be averaged later
			for (size_t i = 0; i < vertex_normals.size(); i++)
			{
				vertex_normals[i] = XMFLOAT3(0, 0, 0);
			}

			// 2.) Find identical vertices by POSITION, accumulate face normals
			for (size_t i = 0; i < vertex_positions.size(); i++)
			{
				XMFLOAT3& v_search_pos = vertex_positions[i];

				for (size_t ind = 0; ind < indices.size() / 3; ++ind)
				{
					uint32_t i0 = indices[ind * 3 + 0];
					uint32_t i1 = indices[ind * 3 + 1];
					uint32_t i2 = indices[ind * 3 + 2];

					XMFLOAT3& v0 = vertex_positions[i0];
					XMFLOAT3& v1 = vertex_positions[i1];
					XMFLOAT3& v2 = vertex_positions[i2];

					bool match_pos0 =
						fabs(v_search_pos.x - v0.x) < FLT_EPSILON &&
						fabs(v_search_pos.y - v0.y) < FLT_EPSILON &&
						fabs(v_search_pos.z - v0.z) < FLT_EPSILON;

					bool match_pos1 =
						fabs(v_search_pos.x - v1.x) < FLT_EPSILON &&
						fabs(v_search_pos.y - v1.y) < FLT_EPSILON &&
						fabs(v_search_pos.z - v1.z) < FLT_EPSILON;

					bool match_pos2 =
						fabs(v_search_pos.x - v2.x) < FLT_EPSILON &&
						fabs(v_search_pos.y - v2.y) < FLT_EPSILON &&
						fabs(v_search_pos.z - v2.z) < FLT_EPSILON;

					if (match_pos0 || match_pos1 || match_pos2)
					{
						XMVECTOR U = XMLoadFloat3(&v2) - XMLoadFloat3(&v0);
						XMVECTOR V = XMLoadFloat3(&v1) - XMLoadFloat3(&v0);

						XMVECTOR N = XMVector3Cross(U, V);
						N = XMVector3Normalize(N);

						XMFLOAT3 normal;
						XMStoreFloat3(&normal, N);

						vertex_normals[i].x += normal.x;
						vertex_normals[i].y += normal.y;
						vertex_normals[i].z += normal.z;
					}

				}
			}

			// 3.) Find duplicated vertices by POSITION and UV0 and UV1 and ATLAS and SUBSET and remove them:
			for (auto& subset : subsets)
			{
				for (uint32_t i = 0; i < subset.indexCount - 1; i++)
				{
					uint32_t ind0 = indices[subset.indexOffset + (uint32_t)i];
					const XMFLOAT3& p0 = vertex_positions[ind0];
					const XMFLOAT2& u00 = vertex_uvset_0.empty() ? XMFLOAT2(0, 0) : vertex_uvset_0[ind0];
					const XMFLOAT2& u10 = vertex_uvset_1.empty() ? XMFLOAT2(0, 0) : vertex_uvset_1[ind0];
					const XMFLOAT2& at0 = vertex_atlas.empty() ? XMFLOAT2(0, 0) : vertex_atlas[ind0];

					for (uint32_t j = i + 1; j < subset.indexCount; j++)
					{
						uint32_t ind1 = indices[subset.indexOffset + (uint32_t)j];

						if (ind1 == ind0)
						{
							continue;
						}

						const XMFLOAT3& p1 = vertex_positions[ind1];
						const XMFLOAT2& u01 = vertex_uvset_0.empty() ? XMFLOAT2(0, 0) : vertex_uvset_0[ind1];
						const XMFLOAT2& u11 = vertex_uvset_1.empty() ? XMFLOAT2(0, 0) : vertex_uvset_1[ind1];
						const XMFLOAT2& at1 = vertex_atlas.empty() ? XMFLOAT2(0, 0) : vertex_atlas[ind1];

						const bool duplicated_pos =
							fabs(p0.x - p1.x) < FLT_EPSILON &&
							fabs(p0.y - p1.y) < FLT_EPSILON &&
							fabs(p0.z - p1.z) < FLT_EPSILON;

						const bool duplicated_uv0 =
							fabs(u00.x - u01.x) < FLT_EPSILON &&
							fabs(u00.y - u01.y) < FLT_EPSILON;

						const bool duplicated_uv1 =
							fabs(u10.x - u11.x) < FLT_EPSILON &&
							fabs(u10.y - u11.y) < FLT_EPSILON;

						const bool duplicated_atl =
							fabs(at0.x - at1.x) < FLT_EPSILON &&
							fabs(at0.y - at1.y) < FLT_EPSILON;

						if (duplicated_pos && duplicated_uv0 && duplicated_uv1 && duplicated_atl)
						{
							// Erase vertices[ind1] because it is a duplicate:
							if (ind1 < vertex_positions.size())
							{
								vertex_positions.erase(vertex_positions.begin() + ind1);
							}
							if (ind1 < vertex_normals.size())
							{
								vertex_normals.erase(vertex_normals.begin() + ind1);
							}
							if (ind1 < vertex_uvset_0.size())
							{
								vertex_uvset_0.erase(vertex_uvset_0.begin() + ind1);
							}
							if (ind1 < vertex_uvset_1.size())
							{
								vertex_uvset_1.erase(vertex_uvset_1.begin() + ind1);
							}
							if (ind1 < vertex_atlas.size())
							{
								vertex_atlas.erase(vertex_atlas.begin() + ind1);
							}
							if (ind1 < vertex_boneindices.size())
							{
								vertex_boneindices.erase(vertex_boneindices.begin() + ind1);
							}
							if (ind1 < vertex_boneweights.size())
							{
								vertex_boneweights.erase(vertex_boneweights.begin() + ind1);
							}

							// The vertices[ind1] was removed, so each index after that needs to be updated:
							for (auto& index : indices)
							{
								if (index > ind1 && index > 0)
								{
									index--;
								}
								else if (index == ind1)
								{
									index = ind0;
								}
							}

						}

					}
				}

			}

		}
		break;

		case MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST:
		{
			for (size_t i = 0; i < vertex_normals.size(); i++)
			{
				vertex_normals[i] = XMFLOAT3(0, 0, 0);
			}
			for (size_t i = 0; i < indices.size() / 3; ++i)
			{
				uint32_t index1 = indices[i * 3];
				uint32_t index2 = indices[i * 3 + 1];
				uint32_t index3 = indices[i * 3 + 2];

				XMVECTOR side1 = XMLoadFloat3(&vertex_positions[index1]) - XMLoadFloat3(&vertex_positions[index3]);
				XMVECTOR side2 = XMLoadFloat3(&vertex_positions[index1]) - XMLoadFloat3(&vertex_positions[index2]);
				XMVECTOR N = XMVector3Normalize(XMVector3Cross(side1, side2));
				XMFLOAT3 normal;
				XMStoreFloat3(&normal, N);

				vertex_normals[index1].x += normal.x;
				vertex_normals[index1].y += normal.y;
				vertex_normals[index1].z += normal.z;

				vertex_normals[index2].x += normal.x;
				vertex_normals[index2].y += normal.y;
				vertex_normals[index2].z += normal.z;

				vertex_normals[index3].x += normal.x;
				vertex_normals[index3].y += normal.y;
				vertex_normals[index3].z += normal.z;
			}
		}
		break;

		}

		vertex_tangents.clear(); // <- will be recomputed

		CreateRenderData(); // <- normals will be normalized here!
	}
	void MeshComponent::FlipCulling()
	{
		for (size_t face = 0; face < indices.size() / 3; face++)
		{
			uint32_t i0 = indices[face * 3 + 0];
			uint32_t i1 = indices[face * 3 + 1];
			uint32_t i2 = indices[face * 3 + 2];

			indices[face * 3 + 0] = i0;
			indices[face * 3 + 1] = i2;
			indices[face * 3 + 2] = i1;
		}

		CreateRenderData();
	}
	void MeshComponent::FlipNormals()
	{
		for (auto& normal : vertex_normals)
		{
			normal.x *= -1;
			normal.y *= -1;
			normal.z *= -1;
		}

		CreateRenderData();
	}
	void MeshComponent::Recenter()
	{
		XMFLOAT3 center = aabb.getCenter();

		for (auto& pos : vertex_positions)
		{
			pos.x -= center.x;
			pos.y -= center.y;
			pos.z -= center.z;
		}

		CreateRenderData();
	}
	void MeshComponent::RecenterToBottom()
	{
		XMFLOAT3 center = aabb.getCenter();
		center.y -= aabb.getHalfWidth().y;

		for (auto& pos : vertex_positions)
		{
			pos.x -= center.x;
			pos.y -= center.y;
			pos.z -= center.z;
		}

		CreateRenderData();
	}
	Sphere MeshComponent::GetBoundingSphere() const
	{
		XMFLOAT3 halfwidth = aabb.getHalfWidth();

		Sphere sphere;
		sphere.center = aabb.getCenter();
		sphere.radius = std::max(halfwidth.x, std::max(halfwidth.y, halfwidth.z));

		return sphere;
	}

	void ObjectComponent::ClearLightmap()
	{
		lightmap = Texture();
		lightmapWidth = 0;
		lightmapHeight = 0;
		lightmapIterationCount = 0; 
		lightmapTextureData.clear();
		SetLightmapRenderRequest(false);
	}

#if __has_include("OpenImageDenoise/oidn.hpp")
#define OPEN_IMAGE_DENOISE
#include "OpenImageDenoise/oidn.hpp"
#pragma comment(lib,"OpenImageDenoise.lib")
#pragma comment(lib,"tbb.lib")
// Also provide OpenImageDenoise.dll and tbb.dll near the exe!
#endif
	void ObjectComponent::SaveLightmap()
	{
		if (lightmap.IsValid() && has_flag(lightmap.desc.bind_flags, BindFlag::RENDER_TARGET))
		{
			SetLightmapRenderRequest(false);

			bool success = wi::helper::saveTextureToMemory(lightmap, lightmapTextureData);
			assert(success);

#ifdef OPEN_IMAGE_DENOISE
			if (success)
			{
				wi::vector<uint8_t> texturedata_dst(lightmapTextureData.size());

				size_t width = (size_t)lightmapWidth;
				size_t height = (size_t)lightmapHeight;
				{
					// https://github.com/OpenImageDenoise/oidn#c11-api-example

					// Create an Intel Open Image Denoise device
					static oidn::DeviceRef device = oidn::newDevice();
					static bool init = false;
					if (!init)
					{
						device.commit();
						init = true;
					}

					// Create a denoising filter
					oidn::FilterRef filter = device.newFilter("RTLightmap");
					filter.setImage("color", lightmapTextureData.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
					filter.setImage("output", texturedata_dst.data(), oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
					filter.commit();

					// Filter the image
					filter.execute();

					// Check for errors
					const char* errorMessage;
					auto error = device.getError(errorMessage);
					if (error != oidn::Error::None && error != oidn::Error::Cancelled)
					{
						wi::backlog::post(std::string("[OpenImageDenoise error] ") + errorMessage);
					}
				}

				lightmapTextureData = std::move(texturedata_dst); // replace old (raw) data with denoised data
			}
#endif // OPEN_IMAGE_DENOISE

			CompressLightmap();

			wi::texturehelper::CreateTexture(lightmap, lightmapTextureData.data(), lightmapWidth, lightmapHeight, lightmap.desc.format);
			wi::graphics::GetDevice()->SetName(&lightmap, "lightmap");
		}
	}
	void ObjectComponent::CompressLightmap()
	{

		// BC6 Block compression code that uses DirectXTex library, but it's not cross platform, so disabled:
#if 0
		wi::Timer timer;
		wi::backlog::post("compressing lightmap...");

		lightmap.desc.Format = lightmap_block_format;
		lightmap.desc.BindFlags = BindFlag::SHADER_RESOURCE;

		static constexpr wi::graphics::FORMAT lightmap_block_format = wi::graphics::FORMAT_BC6H_UF16;
		static constexpr uint32_t lightmap_blocksize = wi::graphics::GetFormatBlockSize(lightmap_block_format);
		static_assert(lightmap_blocksize == 4u);
		const uint32_t bc6_width = lightmapWidth / lightmap_blocksize;
		const uint32_t bc6_height = lightmapHeight / lightmap_blocksize;
		wi::vector<uint8_t> bc6_data;
		bc6_data.resize(sizeof(XMFLOAT4) * bc6_width * bc6_height);
		const XMFLOAT4* raw_data = (const XMFLOAT4*)lightmapTextureData.data();

		for (uint32_t x = 0; x < bc6_width; ++x)
		{
			for (uint32_t y = 0; y < bc6_height; ++y)
			{
				uint32_t bc6_idx = x + y * bc6_width;
				uint8_t* ptr = (uint8_t*)((XMFLOAT4*)bc6_data.data() + bc6_idx);

				XMVECTOR raw_vec[lightmap_blocksize * lightmap_blocksize];
				for (uint32_t i = 0; i < lightmap_blocksize; ++i)
				{
					for (uint32_t j = 0; j < lightmap_blocksize; ++j)
					{
						uint32_t raw_idx = (x * lightmap_blocksize + i) + (y * lightmap_blocksize + j) * lightmapWidth;
						uint32_t block_idx = i + j * lightmap_blocksize;
						raw_vec[block_idx] = XMLoadFloat4(raw_data + raw_idx);
					}
				}
				static_assert(arraysize(raw_vec) == 16); // it will work only for a certain block size!
				D3DXEncodeBC6HU(ptr, raw_vec, 0);
			}
		}

		lightmapTextureData = std::move(bc6_data); // replace old (raw) data with compressed data

		wi::backlog::post(
			"compressing lightmap [" +
			std::to_string(lightmapWidth) +
			"x" +
			std::to_string(lightmapHeight) +
			"] finished in " +
			std::to_string(timer.elapsed_seconds()) +
			" seconds"
		);
#else

		// Simple compression to R11G11B10_FLOAT format:
		using namespace PackedVector;
		wi::vector<uint8_t> packed_data;
		packed_data.resize(sizeof(XMFLOAT3PK) * lightmapWidth * lightmapHeight);
		XMFLOAT3PK* packed_ptr = (XMFLOAT3PK*)packed_data.data();
		XMFLOAT4* raw_ptr = (XMFLOAT4*)lightmapTextureData.data();

		uint32_t texelcount = lightmapWidth * lightmapHeight;
		for (uint32_t i = 0; i < texelcount; ++i)
		{
			XMStoreFloat3PK(packed_ptr + i, XMLoadFloat4(raw_ptr + i));
		}

		lightmapTextureData = std::move(packed_data);
		lightmap.desc.format = Format::R11G11B10_FLOAT;
		lightmap.desc.bind_flags = BindFlag::SHADER_RESOURCE;

#endif
	}

	void ArmatureComponent::CreateRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		GPUBufferDesc bd;
		bd.size = sizeof(ShaderTransform) * (uint32_t)boneCollection.size();
		bd.bind_flags = BindFlag::SHADER_RESOURCE;
		bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
		bd.stride = sizeof(ShaderTransform);

		device->CreateBuffer(&bd, nullptr, &boneBuffer);
	}

	void SoftBodyPhysicsComponent::CreateFromMesh(const MeshComponent& mesh)
	{
		vertex_positions_simulation.resize(mesh.vertex_positions.size());
		vertex_tangents_tmp.resize(mesh.vertex_tangents.size());
		vertex_tangents_simulation.resize(mesh.vertex_tangents.size());

		XMMATRIX W = XMLoadFloat4x4(&worldMatrix);
		XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
		for (size_t i = 0; i < vertex_positions_simulation.size(); ++i)
		{
			XMFLOAT3 pos = mesh.vertex_positions[i];
			XMStoreFloat3(&pos, XMVector3Transform(XMLoadFloat3(&pos), W));
			XMFLOAT3 nor = mesh.vertex_normals.empty() ? XMFLOAT3(1, 1, 1) : mesh.vertex_normals[i];
			XMStoreFloat3(&nor, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&nor), W)));
			const uint8_t wind = mesh.vertex_windweights.empty() ? 0xFF : mesh.vertex_windweights[i];
			vertex_positions_simulation[i].FromFULL(pos, nor, wind);
			_min = wi::math::Min(_min, pos);
			_max = wi::math::Max(_max, pos);
		}
		aabb = AABB(_min, _max);

		if(physicsToGraphicsVertexMapping.empty())
		{
			// Create a mapping that maps unique vertex positions to all vertex indices that share that. Unique vertex positions will make up the physics mesh:
			wi::unordered_map<size_t, uint32_t> uniquePositions;
			graphicsToPhysicsVertexMapping.resize(mesh.vertex_positions.size());
			physicsToGraphicsVertexMapping.clear();
			weights.clear();

			for (size_t i = 0; i < mesh.vertex_positions.size(); ++i)
			{
				const XMFLOAT3& position = mesh.vertex_positions[i];

				size_t hashes[] = {
					std::hash<float>{}(position.x),
					std::hash<float>{}(position.y),
					std::hash<float>{}(position.z),
				};
				size_t vertexHash = (((hashes[0] ^ (hashes[1] << 1) >> 1) ^ (hashes[2] << 1)) >> 1);

				if (uniquePositions.count(vertexHash) == 0)
				{
					uniquePositions[vertexHash] = (uint32_t)physicsToGraphicsVertexMapping.size();
					physicsToGraphicsVertexMapping.push_back((uint32_t)i);
				}
				graphicsToPhysicsVertexMapping[i] = uniquePositions[vertexHash];
			}

			weights.resize(physicsToGraphicsVertexMapping.size());
			std::fill(weights.begin(), weights.end(), 1.0f);
		}
	}
	
	void CameraComponent::CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV)
	{
		zNearP = newNear;
		zFarP = newFar;
		width = newWidth;
		height = newHeight;
		fov = newFOV;

		SetCustomProjectionEnabled(false);

		UpdateCamera();
	}
	void CameraComponent::UpdateCamera()
	{
		if (!IsCustomProjectionEnabled())
		{
			XMStoreFloat4x4(&Projection, XMMatrixPerspectiveFovLH(fov, width / height, zFarP, zNearP)); // reverse zbuffer!
			Projection.m[2][0] = jitter.x;
			Projection.m[2][1] = jitter.y;
		}

		XMVECTOR _Eye = XMLoadFloat3(&Eye);
		XMVECTOR _At = XMLoadFloat3(&At);
		XMVECTOR _Up = XMLoadFloat3(&Up);

		XMMATRIX _V = XMMatrixLookToLH(_Eye, _At, _Up);
		XMStoreFloat4x4(&View, _V);

		XMMATRIX _P = XMLoadFloat4x4(&Projection);
		XMMATRIX _InvP = XMMatrixInverse(nullptr, _P);
		XMStoreFloat4x4(&InvProjection, _InvP);

		XMMATRIX _VP = XMMatrixMultiply(_V, _P);
		XMStoreFloat4x4(&View, _V);
		XMStoreFloat4x4(&VP, _VP);
		XMStoreFloat4x4(&InvView, XMMatrixInverse(nullptr, _V));
		XMStoreFloat4x4(&InvVP, XMMatrixInverse(nullptr, _VP));
		XMStoreFloat4x4(&Projection, _P);
		XMStoreFloat4x4(&InvProjection, XMMatrixInverse(nullptr, _P));

		frustum.Create(_VP);
	}
	void CameraComponent::TransformCamera(const TransformComponent& transform)
	{
		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&transform.world));

		XMVECTOR _Eye = T;
		XMVECTOR _At = XMVectorSet(0, 0, 1, 0);
		XMVECTOR _Up = XMVectorSet(0, 1, 0, 0);

		XMMATRIX _Rot = XMMatrixRotationQuaternion(R);
		_At = XMVector3TransformNormal(_At, _Rot);
		_Up = XMVector3TransformNormal(_Up, _Rot);
		XMStoreFloat3x3(&rotationMatrix, _Rot);

		XMMATRIX _V = XMMatrixLookToLH(_Eye, _At, _Up);
		XMStoreFloat4x4(&View, _V);

		XMStoreFloat3(&Eye, _Eye);
		XMStoreFloat3(&At, _At);
		XMStoreFloat3(&Up, _Up);
	}
	void CameraComponent::Reflect(const XMFLOAT4& plane)
	{
		XMVECTOR _Eye = XMLoadFloat3(&Eye);
		XMVECTOR _At = XMLoadFloat3(&At);
		XMVECTOR _Up = XMLoadFloat3(&Up);
		XMMATRIX _Ref = XMMatrixReflect(XMLoadFloat4(&plane));

		// reverse clipping if behind clip plane ("if underwater")
		clipPlane = plane;
		float d = XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&clipPlane), _Eye));
		if (d < 0)
		{
			clipPlane.x *= -1;
			clipPlane.y *= -1;
			clipPlane.z *= -1;
			clipPlane.w *= -1;
		}

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
		this->dt = dt;

		GraphicsDevice* device = wi::graphics::GetDevice();

		instanceArraySize = objects.GetCount() + hairs.GetCount() + emitters.GetCount();
		if (instanceBuffer.desc.size < (instanceArraySize * sizeof(ShaderMeshInstance)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMeshInstance);
			desc.size = desc.stride * instanceArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&desc, nullptr, &instanceBuffer);
			device->SetName(&instanceBuffer, "instanceBuffer");

			desc.usage = Usage::UPLOAD;
			desc.bind_flags = BindFlag::NONE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			for (int i = 0; i < arraysize(instanceUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &instanceUploadBuffer[i]);
				device->SetName(&instanceUploadBuffer[i], "instanceUploadBuffer");
			}
		}
		instanceArrayMapped = (ShaderMeshInstance*)instanceUploadBuffer[device->GetBufferIndex()].mapped_data;

		meshArraySize = meshes.GetCount() + hairs.GetCount() + emitters.GetCount();
		if (meshBuffer.desc.size < (meshArraySize * sizeof(ShaderMesh)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMesh);
			desc.size = desc.stride * meshArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&desc, nullptr, &meshBuffer);
			device->SetName(&meshBuffer, "meshBuffer");

			desc.usage = Usage::UPLOAD;
			desc.bind_flags = BindFlag::NONE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			for (int i = 0; i < arraysize(meshUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &meshUploadBuffer[i]);
				device->SetName(&meshUploadBuffer[i], "meshUploadBuffer");
			}
		}
		meshArrayMapped = (ShaderMesh*)meshUploadBuffer[device->GetBufferIndex()].mapped_data;

		materialArraySize = materials.GetCount();
		if (materialBuffer.desc.size < (materialArraySize * sizeof(ShaderMaterial)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMaterial);
			desc.size = desc.stride * materialArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&desc, nullptr, &materialBuffer);
			device->SetName(&materialBuffer, "materialBuffer");

			desc.usage = Usage::UPLOAD;
			desc.bind_flags = BindFlag::NONE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			for (int i = 0; i < arraysize(materialUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &materialUploadBuffer[i]);
				device->SetName(&materialUploadBuffer[i], "materialUploadBuffer");
			}
		}
		materialArrayMapped = (ShaderMaterial*)materialUploadBuffer[device->GetBufferIndex()].mapped_data;

		TLAS_instancesMapped = nullptr;
		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			GPUBufferDesc desc;
			desc.stride = (uint32_t)device->GetTopLevelAccelerationStructureInstanceSize();
			desc.size = desc.stride * instanceArraySize * 2; // *2 to grow fast
			desc.usage = Usage::UPLOAD;
			if (TLAS_instancesUpload->desc.size < desc.size)
			{
				for (int i = 0; i < arraysize(TLAS_instancesUpload); ++i)
				{
					device->CreateBuffer(&desc, nullptr, &TLAS_instancesUpload[i]);
					device->SetName(&TLAS_instancesUpload[i], "TLAS_instancesUpload");
				}
			}
			TLAS_instancesMapped = TLAS_instancesUpload[device->GetBufferIndex()].mapped_data;
		}

		// Occlusion culling read:
		if(wi::renderer::GetOcclusionCullingEnabled() && !wi::renderer::GetFreezeCullingCameraEnabled())
		{
			uint32_t minQueryCount = uint32_t(objects.GetCount() + lights.GetCount());
			if (queryHeap.desc.query_count < minQueryCount)
			{
				GPUQueryHeapDesc desc;
				desc.type = GpuQueryType::OCCLUSION_BINARY;
				desc.query_count = minQueryCount;
				bool success = device->CreateQueryHeap(&desc, &queryHeap);
				assert(success);

				GPUBufferDesc bd;
				bd.usage = Usage::READBACK;
				bd.size = desc.query_count * sizeof(uint64_t);

				for (int i = 0; i < arraysize(queryResultBuffer); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &queryResultBuffer[i]);
					assert(success);
				}

				if (device->CheckCapability(GraphicsDeviceCapability::PREDICATION))
				{
					bd.usage = Usage::DEFAULT;
					bd.misc_flags |= ResourceMiscFlag::PREDICATION;
					success = device->CreateBuffer(&bd, nullptr, &queryPredicationBuffer);
					assert(success);
				}
			}

			// Advance to next query result buffer to use (this will be the oldest one that was written)
			queryheap_idx = (queryheap_idx + 1) % arraysize(queryResultBuffer);

			// Clear query allocation state:
			queryAllocator.store(0);
		}

		wi::jobsystem::context ctx;

		wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
			// Must not keep inactive TLAS instances, so zero them out for safety:
			std::memset(TLAS_instancesMapped, 0, TLAS_instancesUpload->desc.size);
		});

		RunPreviousFrameTransformUpdateSystem(ctx);

		RunAnimationUpdateSystem(ctx);

		RunTransformUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunHierarchyUpdateSystem(ctx);

		RunMeshUpdateSystem(ctx);

		RunMaterialUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunSpringUpdateSystem(ctx);

		RunInverseKinematicsUpdateSystem(ctx);

		RunArmatureUpdateSystem(ctx);

		RunImpostorUpdateSystem(ctx);

		RunWeatherUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		wi::physics::RunPhysicsUpdateSystem(ctx, *this, dt);

		RunObjectUpdateSystem(ctx);

		RunCameraUpdateSystem(ctx);

		RunDecalUpdateSystem(ctx);

		RunProbeUpdateSystem(ctx);

		RunForceUpdateSystem(ctx);

		RunLightUpdateSystem(ctx);

		RunParticleUpdateSystem(ctx);

		RunSoundUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		// Merge parallel bounds computation (depends on object update system):
		bounds = AABB();
		for (auto& group_bound : parallel_bounds)
		{
			bounds = AABB::Merge(bounds, group_bound);
		}

		if (lightmap_refresh_needed.load())
		{
			SetAccelerationStructureUpdateRequested(true);
		}

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			// Recreate top level acceleration structure if the object count changed:
			if (TLAS.desc.top_level.count < instanceArraySize)
			{
				RaytracingAccelerationStructureDesc desc;
				desc.flags = RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;
				desc.type = RaytracingAccelerationStructureDesc::Type::TOPLEVEL;
				desc.top_level.count = (uint32_t)instanceArraySize * 2; // *2 to grow fast
				GPUBufferDesc bufdesc;
				bufdesc.misc_flags |= ResourceMiscFlag::RAY_TRACING;
				bufdesc.stride = (uint32_t)device->GetTopLevelAccelerationStructureInstanceSize();
				bufdesc.size = bufdesc.stride * desc.top_level.count;
				bool success = device->CreateBuffer(&bufdesc, nullptr, &desc.top_level.instance_buffer);
				assert(success);
				device->SetName(&desc.top_level.instance_buffer, "TLAS.instanceBuffer");
				success = device->CreateRaytracingAccelerationStructure(&desc, &TLAS);
				assert(success);
				device->SetName(&TLAS, "TLAS");
			}
		}

		if (!device->CheckCapability(GraphicsDeviceCapability::RAYTRACING) && IsAccelerationStructureUpdateRequested())
		{
			BVH.Update(*this);
		}

		// Update water ripples:
		for (size_t i = 0; i < waterRipples.size(); ++i)
		{
			auto& ripple = waterRipples[i];
			ripple.Update(dt * 60);

			// Remove inactive ripples:
			if (ripple.params.opacity <= 0 + FLT_EPSILON || ripple.params.fade >= 1 - FLT_EPSILON)
			{
				ripple = waterRipples.back();
				waterRipples.pop_back();
				i--;
			}
		}

		if (wi::renderer::GetSurfelGIEnabled())
		{
			if (!surfelBuffer.IsValid())
			{
				GPUBufferDesc desc;
				desc.stride = sizeof(Surfel);
				desc.size = desc.stride * SURFEL_CAPACITY;
				desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
				device->CreateBuffer(&desc, nullptr, &surfelBuffer);
				device->SetName(&surfelBuffer, "surfelBuffer");

				desc.stride = sizeof(SurfelData);
				desc.size = desc.stride * SURFEL_CAPACITY;
				desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&desc, nullptr, &surfelDataBuffer);
				device->SetName(&surfelDataBuffer, "surfelDataBuffer");

				desc.stride = sizeof(uint);
				desc.size = desc.stride * SURFEL_CAPACITY;
				desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&desc, nullptr, &surfelAliveBuffer[0]);
				device->SetName(&surfelAliveBuffer[0], "surfelAliveBuffer[0]");
				device->CreateBuffer(&desc, nullptr, &surfelAliveBuffer[1]);
				device->SetName(&surfelAliveBuffer[1], "surfelAliveBuffer[1]");

				wi::vector<uint32_t> dead_indices(SURFEL_CAPACITY);
				for (uint32_t i = 0; i < dead_indices.size(); ++i)
				{
					dead_indices[i] = uint32_t(dead_indices.size() - 1 - i);
				}
				device->CreateBuffer(&desc, dead_indices.data(), &surfelDeadBuffer);
				device->SetName(&surfelDeadBuffer, "surfelDeadBuffer");

				desc.stride = sizeof(uint);
				desc.size = desc.stride * 9; // count (1 uint), nextCount (1 uint), deadCount (1 uint), cellAllocator (1 uint), IndirectDispatchArgs (3 uints), raycount (1 uint), shortage (1 uint)
				desc.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::INDIRECT_ARGS;
				uint stats_data[] = { 0,0,SURFEL_CAPACITY,0,0,0,0,0,0 };
				device->CreateBuffer(&desc, &stats_data, &surfelStatsBuffer);
				device->SetName(&surfelStatsBuffer, "surfelStatsBuffer");

				desc.stride = sizeof(SurfelGridCell);
				desc.size = desc.stride * SURFEL_TABLE_SIZE;
				desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&desc, nullptr, &surfelGridBuffer);
				device->SetName(&surfelGridBuffer, "surfelGridBuffer");

				desc.stride = sizeof(uint);
				desc.size = desc.stride * SURFEL_CAPACITY * 27; // each surfel can be in 3x3x3=27 cells
				desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&desc, nullptr, &surfelCellBuffer);
				device->SetName(&surfelCellBuffer, "surfelCellBuffer");

				TextureDesc tex;
				tex.width = SURFEL_MOMENT_ATLAS_TEXELS;
				tex.height = SURFEL_MOMENT_ATLAS_TEXELS;
				tex.format = Format::R16G16_FLOAT;
				tex.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				device->CreateTexture(&tex, nullptr, &surfelMomentsTexture[0]);
				device->SetName(&surfelMomentsTexture[0], "surfelMomentsTexture[0]");
				device->CreateTexture(&tex, nullptr, &surfelMomentsTexture[1]);
				device->SetName(&surfelMomentsTexture[1], "surfelMomentsTexture[1]");
			}
			std::swap(surfelAliveBuffer[0], surfelAliveBuffer[1]);
			std::swap(surfelMomentsTexture[0], surfelMomentsTexture[1]);
		}


		// Shader scene resources:
		shaderscene.instancebuffer = device->GetDescriptorIndex(&instanceBuffer, SubresourceType::SRV);
		shaderscene.meshbuffer = device->GetDescriptorIndex(&meshBuffer, SubresourceType::SRV);
		shaderscene.materialbuffer = device->GetDescriptorIndex(&materialBuffer, SubresourceType::SRV);
		shaderscene.envmaparray = device->GetDescriptorIndex(&envmapArray, SubresourceType::SRV);
		if (weather.skyMap.IsValid())
		{
			shaderscene.globalenvmap = device->GetDescriptorIndex(&weather.skyMap.GetTexture(), SubresourceType::SRV);
		}
		else
		{
			shaderscene.globalenvmap = -1;
		}
		shaderscene.TLAS = device->GetDescriptorIndex(&TLAS, SubresourceType::SRV);
		shaderscene.BVH_counter = device->GetDescriptorIndex(&BVH.primitiveCounterBuffer, SubresourceType::SRV);
		shaderscene.BVH_nodes = device->GetDescriptorIndex(&BVH.bvhNodeBuffer, SubresourceType::SRV);
		shaderscene.BVH_primitives = device->GetDescriptorIndex(&BVH.primitiveBuffer, SubresourceType::SRV);

		shaderscene.aabb_min = bounds.getMin();
		shaderscene.aabb_max = bounds.getMax();
		shaderscene.aabb_extents.x = abs(shaderscene.aabb_max.x - shaderscene.aabb_min.x);
		shaderscene.aabb_extents.y = abs(shaderscene.aabb_max.y - shaderscene.aabb_min.y);
		shaderscene.aabb_extents.z = abs(shaderscene.aabb_max.z - shaderscene.aabb_min.z);
		shaderscene.aabb_extents_rcp.x = 1.0f / shaderscene.aabb_extents.x;
		shaderscene.aabb_extents_rcp.y = 1.0f / shaderscene.aabb_extents.y;
		shaderscene.aabb_extents_rcp.z = 1.0f / shaderscene.aabb_extents.z;

		shaderscene.weather.sun_color = weather.sunColor;
		shaderscene.weather.sun_direction = weather.sunDirection;
		shaderscene.weather.sun_energy = weather.sunEnergy;
		shaderscene.weather.ambient = weather.ambient;
		shaderscene.weather.cloudiness = weather.cloudiness;
		shaderscene.weather.cloud_scale = weather.cloudScale;
		shaderscene.weather.cloud_speed = weather.cloudSpeed;
		shaderscene.weather.fog.start = weather.fogStart;
		shaderscene.weather.fog.end = weather.fogEnd;
		shaderscene.weather.fog.height_start = weather.fogHeightStart;
		shaderscene.weather.fog.height_end = weather.fogHeightEnd;
		shaderscene.weather.fog.height_sky = weather.fogHeightSky;
		shaderscene.weather.horizon = weather.horizon;
		shaderscene.weather.zenith = weather.zenith;
		shaderscene.weather.sky_exposure = weather.skyExposure;
		shaderscene.weather.wind.speed = weather.windSpeed;
		shaderscene.weather.wind.randomness = weather.windRandomness;
		shaderscene.weather.wind.wavesize = weather.windWaveSize;
		shaderscene.weather.wind.direction = weather.windDirection;
		shaderscene.weather.atmosphere = weather.atmosphereParameters;
		shaderscene.weather.volumetric_clouds = weather.volumetricCloudParameters;
	}
	void Scene::Clear()
	{
		names.Clear();
		layers.Clear();
		transforms.Clear();
		prev_transforms.Clear();
		hierarchy.Clear();
		materials.Clear();
		meshes.Clear();
		impostors.Clear();
		objects.Clear();
		aabb_objects.Clear();
		rigidbodies.Clear();
		softbodies.Clear();
		armatures.Clear();
		lights.Clear();
		aabb_lights.Clear();
		cameras.Clear();
		probes.Clear();
		aabb_probes.Clear();
		forces.Clear();
		decals.Clear();
		aabb_decals.Clear();
		animations.Clear();
		animation_datas.Clear();
		emitters.Clear();
		hairs.Clear();
		weathers.Clear();
		sounds.Clear();
		inverse_kinematics.Clear();
		springs.Clear();

		TLAS = RaytracingAccelerationStructure();
		BVH.Clear();
		waterRipples.clear();

		surfelBuffer = {};
		surfelDataBuffer = {};
		surfelAliveBuffer[0] = {};
		surfelAliveBuffer[1] = {};
		surfelDeadBuffer = {};
		surfelStatsBuffer = {};
		surfelGridBuffer = {};
		surfelCellBuffer = {};
	}
	void Scene::Merge(Scene& other)
	{
		names.Merge(other.names);
		layers.Merge(other.layers);
		transforms.Merge(other.transforms);
		prev_transforms.Merge(other.prev_transforms);
		hierarchy.Merge(other.hierarchy);
		materials.Merge(other.materials);
		meshes.Merge(other.meshes);
		impostors.Merge(other.impostors);
		objects.Merge(other.objects);
		aabb_objects.Merge(other.aabb_objects);
		rigidbodies.Merge(other.rigidbodies);
		softbodies.Merge(other.softbodies);
		armatures.Merge(other.armatures);
		lights.Merge(other.lights);
		aabb_lights.Merge(other.aabb_lights);
		cameras.Merge(other.cameras);
		probes.Merge(other.probes);
		aabb_probes.Merge(other.aabb_probes);
		forces.Merge(other.forces);
		decals.Merge(other.decals);
		aabb_decals.Merge(other.aabb_decals);
		animations.Merge(other.animations);
		animation_datas.Merge(other.animation_datas);
		emitters.Merge(other.emitters);
		hairs.Merge(other.hairs);
		weathers.Merge(other.weathers);
		sounds.Merge(other.sounds);
		inverse_kinematics.Merge(other.inverse_kinematics);
		springs.Merge(other.springs);

		bounds = AABB::Merge(bounds, other.bounds);
	}

	void Scene::Entity_Remove(Entity entity)
	{
		Component_Detach(entity); // special case, this will also remove entity from hierarchy but also do more!

		names.Remove(entity);
		layers.Remove(entity);
		transforms.Remove(entity);
		prev_transforms.Remove(entity);
		materials.Remove(entity);
		meshes.Remove(entity);
		impostors.Remove(entity);
		objects.Remove(entity);
		aabb_objects.Remove(entity);
		rigidbodies.Remove(entity);
		softbodies.Remove(entity);
		armatures.Remove(entity);
		lights.Remove(entity);
		aabb_lights.Remove(entity);
		cameras.Remove(entity);
		probes.Remove(entity);
		aabb_probes.Remove(entity);
		forces.Remove(entity);
		decals.Remove(entity);
		aabb_decals.Remove(entity);
		animations.Remove(entity);
		animation_datas.Remove(entity);
		emitters.Remove(entity);
		hairs.Remove(entity);
		weathers.Remove(entity);
		sounds.Remove(entity);
		inverse_kinematics.Remove(entity);
		springs.Remove(entity);
	}
	Entity Scene::Entity_FindByName(const std::string& name)
	{
		for (size_t i = 0; i < names.GetCount(); ++i)
		{
			if (names[i] == name)
			{
				return names.GetEntity(i);
			}
		}
		return INVALID_ENTITY;
	}
	Entity Scene::Entity_Duplicate(Entity entity)
	{
		wi::Archive archive;

		// First write the root entity to staging area:
		archive.SetReadModeAndResetPos(false);
		Entity_Serialize(archive, entity);

		// Then deserialize root:
		archive.SetReadModeAndResetPos(true);
		Entity root = Entity_Serialize(archive);

		return root;
	}
	Entity Scene::Entity_CreateMaterial(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		materials.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateObject(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		prev_transforms.Create(entity);

		aabb_objects.Create(entity);

		objects.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateMesh(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		meshes.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateLight(
		const std::string& name,
		const XMFLOAT3& position,
		const XMFLOAT3& color,
		float energy,
		float range,
		LightComponent::LightType type)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		aabb_lights.Create(entity).createFromHalfWidth(position, XMFLOAT3(range, range, range));

		LightComponent& light = lights.Create(entity);
		light.energy = energy;
		light.range_local = range;
		light.fov = XM_PIDIV4;
		light.color = color;
		light.SetType(type);

		return entity;
	}
	Entity Scene::Entity_CreateForce(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		ForceFieldComponent& force = forces.Create(entity);
		force.gravity = 0;
		force.range_local = 0;
		force.type = ENTITY_TYPE_FORCEFIELD_POINT;

		return entity;
	}
	Entity Scene::Entity_CreateEnvironmentProbe(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		aabb_probes.Create(entity);

		probes.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateDecal(
		const std::string& name,
		const std::string& textureName,
		const std::string& normalMapName
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		aabb_decals.Create(entity);

		decals.Create(entity);

		MaterialComponent& material = materials.Create(entity);
		material.textures[MaterialComponent::BASECOLORMAP].name = textureName;
		material.textures[MaterialComponent::NORMALMAP].name = normalMapName;
		material.CreateRenderData();

		return entity;
	}
	Entity Scene::Entity_CreateCamera(
		const std::string& name,
		float width, float height, float nearPlane, float farPlane, float fov
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		CameraComponent& camera = cameras.Create(entity);
		camera.CreatePerspective(width, height, nearPlane, farPlane, fov);

		return entity;
	}
	Entity Scene::Entity_CreateEmitter(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		emitters.Create(entity).count = 10;

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		materials.Create(entity).userBlendMode = BLENDMODE_ALPHA;

		return entity;
	}
	Entity Scene::Entity_CreateHair(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		hairs.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		materials.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateSound(
		const std::string& name,
		const std::string& filename,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		SoundComponent& sound = sounds.Create(entity);
		sound.filename = filename;
		sound.soundResource = wi::resourcemanager::Load(filename, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
		wi::audio::CreateSoundInstance(&sound.soundResource.GetSound(), &sound.soundinstance);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		return entity;
	}

	void Scene::Component_Attach(Entity entity, Entity parent, bool child_already_in_local_space)
	{
		assert(entity != parent);

		if (hierarchy.Contains(entity))
		{
			Component_Detach(entity);
		}

		HierarchyComponent& parentcomponent = hierarchy.Create(entity);
		parentcomponent.parentID = parent;

		TransformComponent* transform_parent = transforms.GetComponent(parent);
		if (transform_parent == nullptr)
		{
			transform_parent = &transforms.Create(parent);
		}

		TransformComponent* transform_child = transforms.GetComponent(entity);
		if (transform_child == nullptr)
		{
			transform_child = &transforms.Create(entity);
			transform_parent = transforms.GetComponent(parent); // after transforms.Create(), transform_parent pointer could have become invalidated!
		}
		if (!child_already_in_local_space)
		{
			XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world));
			transform_child->MatrixTransform(B);
			transform_child->UpdateTransform();
		}
		transform_child->UpdateTransform_Parented(*transform_parent);

		LayerComponent* layer_parent = layers.GetComponent(parent);
		if (layer_parent == nullptr)
		{
			layer_parent = &layers.Create(parent);
		}
		LayerComponent* layer_child = layers.GetComponent(entity);
		if (layer_child == nullptr)
		{
			layer_child = &layers.Create(entity);
		}
	}
	void Scene::Component_Detach(Entity entity)
	{
		const HierarchyComponent* parent = hierarchy.GetComponent(entity);

		if (parent != nullptr)
		{
			TransformComponent* transform = transforms.GetComponent(entity);
			if (transform != nullptr)
			{
				transform->ApplyTransform();
			}

			LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				layer->propagationMask = ~0;
			}

			hierarchy.Remove(entity);
		}
	}
	void Scene::Component_DetachChildren(Entity parent)
	{
		for (size_t i = 0; i < hierarchy.GetCount(); )
		{
			if (hierarchy[i].parentID == parent)
			{
				Entity entity = hierarchy.GetEntity(i);
				Component_Detach(entity);
			}
			else
			{
				++i;
			}
		}
	}


	const uint32_t small_subtask_groupsize = 64;

	void Scene::RunPreviousFrameTransformUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)prev_transforms.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			PreviousFrameTransformComponent& prev_transform = prev_transforms[args.jobIndex];
			Entity entity = prev_transforms.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			prev_transform.world_prev = transform.world;
		});
	}
	void Scene::RunAnimationUpdateSystem(wi::jobsystem::context& ctx)
	{
		for (size_t i = 0; i < animations.GetCount(); ++i)
		{
			AnimationComponent& animation = animations[i];
			if (!animation.IsPlaying() && animation.timer == 0.0f)
			{
				continue;
			}

			for (const AnimationComponent::AnimationChannel& channel : animation.channels)
			{
				assert(channel.samplerIndex < (int)animation.samplers.size());
				AnimationComponent::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
				if (sampler.data == INVALID_ENTITY)
				{
					// backwards-compatibility mode
					sampler.data = CreateEntity();
					animation_datas.Create(sampler.data) = sampler.backwards_compatibility_data;
					sampler.backwards_compatibility_data.keyframe_times.clear();
					sampler.backwards_compatibility_data.keyframe_data.clear();
				}
				const AnimationDataComponent* animationdata = animation_datas.GetComponent(sampler.data);
				if (animationdata == nullptr)
				{
					continue;
				}

				int keyLeft = 0;
				int keyRight = 0;

				if (animationdata->keyframe_times.back() < animation.timer)
				{
					// Rightmost keyframe is already outside animation, so just snap to last keyframe:
					keyLeft = keyRight = (int)animationdata->keyframe_times.size() - 1;
				}
				else
				{
					// Search for the right keyframe (greater/equal to anim time):
					while (animationdata->keyframe_times[keyRight++] < animation.timer) {}
					keyRight--;

					// Left keyframe is just near right:
					keyLeft = std::max(0, keyRight - 1);
				}

				float left = animationdata->keyframe_times[keyLeft];

				TransformComponent transform;

				TransformComponent* target_transform = nullptr;
				MeshComponent* target_mesh = nullptr;

				if (channel.path == AnimationComponent::AnimationChannel::Path::WEIGHTS)
				{
					ObjectComponent* object = objects.GetComponent(channel.target);
					assert(object != nullptr);
					if (object == nullptr)
						continue;
					target_mesh = meshes.GetComponent(object->meshID);
					assert(target_mesh != nullptr);
					if (target_mesh == nullptr)
						continue;
					animation.morph_weights_temp.resize(target_mesh->targets.size());
				}
				else
				{
					target_transform = transforms.GetComponent(channel.target);
					assert(target_transform != nullptr);
					if (target_transform == nullptr)
						continue;
					transform = *target_transform;
				}

				switch (sampler.mode)
				{
				default:
				case AnimationComponent::AnimationSampler::Mode::STEP:
				{
					// Nearest neighbor method (snap to left):
					switch (channel.path)
					{
					default:
					case AnimationComponent::AnimationChannel::Path::TRANSLATION:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
						transform.translation_local = ((const XMFLOAT3*)animationdata->keyframe_data.data())[keyLeft];
					}
					break;
					case AnimationComponent::AnimationChannel::Path::ROTATION:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4);
						transform.rotation_local = ((const XMFLOAT4*)animationdata->keyframe_data.data())[keyLeft];
					}
					break;
					case AnimationComponent::AnimationChannel::Path::SCALE:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
						transform.scale_local = ((const XMFLOAT3*)animationdata->keyframe_data.data())[keyLeft];
					}
					break;
					case AnimationComponent::AnimationChannel::Path::WEIGHTS:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size());
						for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
						{
							animation.morph_weights_temp[j] = animationdata->keyframe_data[keyLeft * animation.morph_weights_temp.size() + j];
						}
					}
					break;
					}
				}
				break;
				case AnimationComponent::AnimationSampler::Mode::LINEAR:
				{
					// Linear interpolation method:
					float t;
					if (keyLeft == keyRight)
					{
						t = 0;
					}
					else
					{
						float right = animationdata->keyframe_times[keyRight];
						t = (animation.timer - left) / (right - left);
					}

					switch (channel.path)
					{
					default:
					case AnimationComponent::AnimationChannel::Path::TRANSLATION:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
						const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft]);
						XMVECTOR vRight = XMLoadFloat3(&data[keyRight]);
						XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
						XMStoreFloat3(&transform.translation_local, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::ROTATION:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4);
						const XMFLOAT4* data = (const XMFLOAT4*)animationdata->keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat4(&data[keyLeft]);
						XMVECTOR vRight = XMLoadFloat4(&data[keyRight]);
						XMVECTOR vAnim = XMQuaternionSlerp(vLeft, vRight, t);
						vAnim = XMQuaternionNormalize(vAnim);
						XMStoreFloat4(&transform.rotation_local, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::SCALE:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
						const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft]);
						XMVECTOR vRight = XMLoadFloat3(&data[keyRight]);
						XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
						XMStoreFloat3(&transform.scale_local, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::WEIGHTS:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size());
						for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
						{
							float vLeft = animationdata->keyframe_data[keyLeft * animation.morph_weights_temp.size() + j];
							float vRight = animationdata->keyframe_data[keyLeft * animation.morph_weights_temp.size() + j];
							float vAnim = wi::math::Lerp(vLeft, vRight, t);
							animation.morph_weights_temp[j] = vAnim;
						}
					}
					break;
					}
				}
				break;
				case AnimationComponent::AnimationSampler::Mode::CUBICSPLINE:
				{
					// Cubic Spline interpolation method:
					float t;
					if (keyLeft == keyRight)
					{
						t = 0;
					}
					else
					{
						float right = animationdata->keyframe_times[keyRight];
						t = (animation.timer - left) / (right - left);
					}

					const float t2 = t * t;
					const float t3 = t2 * t;

					switch (channel.path)
					{
					default:
					case AnimationComponent::AnimationChannel::Path::TRANSLATION:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3 * 3);
						const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft * 3 + 1]);
						XMVECTOR vLeftTanOut = dt * XMLoadFloat3(&data[keyLeft * 3 + 2]);
						XMVECTOR vRightTanIn = dt * XMLoadFloat3(&data[keyRight * 3 + 0]);
						XMVECTOR vRight = XMLoadFloat3(&data[keyRight * 3 + 1]);
						XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
						XMStoreFloat3(&transform.translation_local, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::ROTATION:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4 * 3);
						const XMFLOAT4* data = (const XMFLOAT4*)animationdata->keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat4(&data[keyLeft * 3 + 1]);
						XMVECTOR vLeftTanOut = dt * XMLoadFloat4(&data[keyLeft * 3 + 2]);
						XMVECTOR vRightTanIn = dt * XMLoadFloat4(&data[keyRight * 3 + 0]);
						XMVECTOR vRight = XMLoadFloat4(&data[keyRight * 3 + 1]);
						XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
						vAnim = XMQuaternionNormalize(vAnim);
						XMStoreFloat4(&transform.rotation_local, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::SCALE:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3 * 3);
						const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft * 3 + 1]);
						XMVECTOR vLeftTanOut = dt * XMLoadFloat3(&data[keyLeft * 3 + 2]);
						XMVECTOR vRightTanIn = dt * XMLoadFloat3(&data[keyRight * 3 + 0]);
						XMVECTOR vRight = XMLoadFloat3(&data[keyRight * 3 + 1]);
						XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
						XMStoreFloat3(&transform.scale_local, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::WEIGHTS:
					{
						assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size() * 3);
						for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
						{
							float vLeft = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 1];
							float vLeftTanOut = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 2];
							float vRightTanIn = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 0];
							float vRight = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 1];
							float vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
							animation.morph_weights_temp[j] = vAnim;
						}
					}
					break;
					}
				}
				break;
				}

				if (target_transform != nullptr)
				{
					target_transform->SetDirty();

					const float t = animation.amount;

					const XMVECTOR aS = XMLoadFloat3(&target_transform->scale_local);
					const XMVECTOR aR = XMLoadFloat4(&target_transform->rotation_local);
					const XMVECTOR aT = XMLoadFloat3(&target_transform->translation_local);

					const XMVECTOR bS = XMLoadFloat3(&transform.scale_local);
					const XMVECTOR bR = XMLoadFloat4(&transform.rotation_local);
					const XMVECTOR bT = XMLoadFloat3(&transform.translation_local);

					const XMVECTOR S = XMVectorLerp(aS, bS, t);
					const XMVECTOR R = XMQuaternionSlerp(aR, bR, t);
					const XMVECTOR T = XMVectorLerp(aT, bT, t);

					XMStoreFloat3(&target_transform->scale_local, S);
					XMStoreFloat4(&target_transform->rotation_local, R);
					XMStoreFloat3(&target_transform->translation_local, T);
				}

				if (target_mesh != nullptr)
				{
					const float t = animation.amount;

					for (size_t j = 0; j < target_mesh->targets.size(); ++j)
					{
						target_mesh->targets[j].weight = wi::math::Lerp(target_mesh->targets[j].weight, animation.morph_weights_temp[j], t);
					}

					target_mesh->dirty_morph = true;
				}

			}

			if (animation.IsPlaying())
			{
				animation.timer += dt * animation.speed;
			}

			if (animation.IsLooped() && animation.timer > animation.end)
			{
				animation.timer = animation.start;
			}
		}
	}
	void Scene::RunTransformUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)transforms.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			TransformComponent& transform = transforms[args.jobIndex];
			transform.UpdateTransform();
		});
	}
	void Scene::RunHierarchyUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)hierarchy.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			HierarchyComponent& hier = hierarchy[args.jobIndex];
			Entity entity = hierarchy.GetEntity(args.jobIndex);

			TransformComponent* transform_child = transforms.GetComponent(entity);
			XMMATRIX worldmatrix;
			if (transform_child != nullptr)
			{
				worldmatrix = transform_child->GetLocalMatrix();
			}

			LayerComponent* layer_child = layers.GetComponent(entity);
			if (layer_child != nullptr)
			{
				layer_child->propagationMask = ~0u; // clear propagation mask to full
			}

			if (transform_child == nullptr && layer_child == nullptr)
				return;

			Entity parentID = hier.parentID;
			while (parentID != INVALID_ENTITY)
			{
				TransformComponent* transform_parent = transforms.GetComponent(parentID);
				if (transform_child != nullptr && transform_parent != nullptr)
				{
					worldmatrix *= transform_parent->GetLocalMatrix();
				}

				LayerComponent* layer_parent = layers.GetComponent(parentID);
				if (layer_child != nullptr && layer_parent != nullptr)
				{
					layer_child->propagationMask &= layer_parent->layerMask;
				}

				const HierarchyComponent* hier_recursive = hierarchy.GetComponent(parentID);
				if (hier_recursive != nullptr)
				{
					parentID = hier_recursive->parentID;
				}
				else
				{
					parentID = INVALID_ENTITY;
				}
			}

			if (transform_child != nullptr)
			{
				XMStoreFloat4x4(&transform_child->world, worldmatrix);
			}

		});
	}
	void Scene::RunSpringUpdateSystem(wi::jobsystem::context& ctx)
	{
		static float time = 0;
		time += dt;
		const XMVECTOR windDir = XMLoadFloat3(&weather.windDirection);
		const XMVECTOR gravity = XMVectorSet(0, -9.8f, 0, 0);

		for (size_t i = 0; i < springs.GetCount(); ++i)
		{
			SpringComponent& spring = springs[i];
			if (spring.IsDisabled())
			{
				continue;
			}
			Entity entity = springs.GetEntity(i);
			TransformComponent* transform = transforms.GetComponent(entity);
			if (transform == nullptr)
			{
				assert(0);
				continue;
			}

			if (spring.IsResetting())
			{
				spring.Reset(false);
				spring.center_of_mass = transform->GetPosition();
				spring.velocity = XMFLOAT3(0, 0, 0);
			}

			const HierarchyComponent* hier = hierarchy.GetComponent(entity);
			TransformComponent* parent_transform = hier == nullptr ? nullptr : transforms.GetComponent(hier->parentID);
			if (parent_transform != nullptr)
			{
				// Spring hierarchy resolve depends on spring component order!
				//	It works best when parent spring is located before child spring!
				//	It will work the other way, but results will be less convincing
				transform->UpdateTransform_Parented(*parent_transform);
			}

			const XMVECTOR position_current = transform->GetPositionV();
			XMVECTOR position_prev = XMLoadFloat3(&spring.center_of_mass);
			XMVECTOR force = (position_current - position_prev) * spring.stiffness;

			if (spring.wind_affection > 0)
			{
				force += std::sin(time * weather.windSpeed + XMVectorGetX(XMVector3Dot(position_current, windDir))) * windDir * spring.wind_affection;
			}
			if (spring.IsGravityEnabled())
			{
				force += gravity;
			}

			XMVECTOR velocity = XMLoadFloat3(&spring.velocity);
			velocity += force * dt;
			XMVECTOR position_target = position_prev + velocity * dt;

			if (parent_transform != nullptr)
			{
				const XMVECTOR position_parent = parent_transform->GetPositionV();
				const XMVECTOR parent_to_child = position_current - position_parent;
				const XMVECTOR parent_to_target = position_target - position_parent;

				if (!spring.IsStretchEnabled())
				{
					// Limit offset to keep distance from parent:
					const XMVECTOR len = XMVector3Length(parent_to_child);
					position_target = position_parent + XMVector3Normalize(parent_to_target) * len;
				}

				// Parent rotation to point to new child position:
				const XMVECTOR dir_parent_to_child = XMVector3Normalize(parent_to_child);
				const XMVECTOR dir_parent_to_target = XMVector3Normalize(parent_to_target);
				const XMVECTOR axis = XMVector3Normalize(XMVector3Cross(dir_parent_to_child, dir_parent_to_target));
				const float angle = XMScalarACos(XMVectorGetX(XMVector3Dot(dir_parent_to_child, dir_parent_to_target))); // don't use std::acos!
				const XMVECTOR Q = XMQuaternionNormalize(XMQuaternionRotationNormal(axis, angle));
				TransformComponent saved_parent = *parent_transform;
				saved_parent.ApplyTransform();
				saved_parent.Rotate(Q);
				saved_parent.UpdateTransform();
				std::swap(saved_parent.world, parent_transform->world); // only store temporary result, not modifying actual local space!
			}

			XMStoreFloat3(&spring.center_of_mass, position_target);
			velocity *= spring.damping;
			XMStoreFloat3(&spring.velocity, velocity);
			*((XMFLOAT3*)&transform->world._41) = spring.center_of_mass;
		}
	}
	void Scene::RunInverseKinematicsUpdateSystem(wi::jobsystem::context& ctx)
	{
		bool recompute_hierarchy = false;
		for (size_t i = 0; i < inverse_kinematics.GetCount(); ++i)
		{
			const InverseKinematicsComponent& ik = inverse_kinematics[i];
			if (ik.IsDisabled())
			{
				continue;
			}
			Entity entity = inverse_kinematics.GetEntity(i);
			TransformComponent* transform = transforms.GetComponent(entity);
			TransformComponent* target = transforms.GetComponent(ik.target);
			const HierarchyComponent* hier = hierarchy.GetComponent(entity);
			if (transform == nullptr || target == nullptr || hier == nullptr)
			{
				continue;
			}

			const XMVECTOR target_pos = target->GetPositionV();
			for (uint32_t iteration = 0; iteration < ik.iteration_count; ++iteration)
			{
				TransformComponent* stack[32] = {};
				Entity parent_entity = hier->parentID;
				TransformComponent* child_transform = transform;
				for (uint32_t chain = 0; chain < std::min(ik.chain_length, (uint32_t)arraysize(stack)); ++chain)
				{
					recompute_hierarchy = true; // any IK will trigger a full transform hierarchy recompute step at the end(**)

					// stack stores all traversed chain links so far:
					stack[chain] = child_transform;

					// Compute required parent rotation that moves ik transform closer to target transform:
					TransformComponent* parent_transform = transforms.GetComponent(parent_entity);
					const XMVECTOR parent_pos = parent_transform->GetPositionV();
					const XMVECTOR dir_parent_to_ik = XMVector3Normalize(transform->GetPositionV() - parent_pos);
					const XMVECTOR dir_parent_to_target = XMVector3Normalize(target_pos - parent_pos);
					const XMVECTOR axis = XMVector3Normalize(XMVector3Cross(dir_parent_to_ik, dir_parent_to_target));
					const float angle = XMScalarACos(XMVectorGetX(XMVector3Dot(dir_parent_to_ik, dir_parent_to_target)));
					const XMVECTOR Q = XMQuaternionNormalize(XMQuaternionRotationNormal(axis, angle));

					// parent to world space:
					parent_transform->ApplyTransform();
					// rotate parent:
					parent_transform->Rotate(Q);
					parent_transform->UpdateTransform();
					// parent back to local space (if parent has parent):
					const HierarchyComponent* hier_parent = hierarchy.GetComponent(parent_entity);
					if (hier_parent != nullptr)
					{
						Entity parent_of_parent_entity = hier_parent->parentID;
						const TransformComponent* transform_parent_of_parent = transforms.GetComponent(parent_of_parent_entity);
						XMMATRIX parent_of_parent_inverse = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent_of_parent->world));
						parent_transform->MatrixTransform(parent_of_parent_inverse);
						// Do not call UpdateTransform() here, to keep parent world matrix in world space!
					}

					// update chain from parent to children:
					const TransformComponent* recurse_parent = parent_transform;
					for (int recurse_chain = (int)chain; recurse_chain >= 0; --recurse_chain)
					{
						stack[recurse_chain]->UpdateTransform_Parented(*recurse_parent);
						recurse_parent = stack[recurse_chain];
					}

					if (hier_parent == nullptr)
					{
						// chain root reached, exit
						break;
					}

					// move up in the chain by one:
					child_transform = parent_transform;
					parent_entity = hier_parent->parentID;
					assert(chain < (uint32_t)arraysize(stack) - 1); // if this is encountered, just extend stack array size

				}
			}
		}

		if (recompute_hierarchy)
		{
			// (**)If there was IK, we need to recompute transform hierarchy. This is only necessary for transforms that have parent
			//	transforms that are IK. Because the IK chain is computed from child to parent upwards, IK that have child would not update
			//	its transform properly in some cases (such as if animation writes to that child)
			for (size_t i = 0; i < hierarchy.GetCount(); ++i)
			{
				const HierarchyComponent& parentcomponent = hierarchy[i];
				Entity entity = hierarchy.GetEntity(i);

				TransformComponent* transform_child = transforms.GetComponent(entity);
				TransformComponent* transform_parent = transforms.GetComponent(parentcomponent.parentID);
				if (transform_child != nullptr && transform_parent != nullptr)
				{
					transform_child->UpdateTransform_Parented(*transform_parent);
				}
			}
		}
	}
	void Scene::RunArmatureUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)armatures.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {

			ArmatureComponent& armature = armatures[args.jobIndex];
			Entity entity = armatures.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			// The transform world matrices are in world space, but skinning needs them in armature-local space, 
			//	so that the skin is reusable for instanced meshes.
			//	We remove the armature's world matrix from the bone world matrix to obtain the bone local transform
			//	These local bone matrices will only be used for skinning, the actual transform components for the bones
			//	remain unchanged.
			//
			//	This is useful for an other thing too:
			//	If a whole transform tree is transformed by some parent (even gltf import does that to convert from RH to LH space)
			//	then the inverseBindMatrices are not reflected in that because they are not contained in the hierarchy system. 
			//	But this will correct them too.
			XMMATRIX R = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform.world));

			if (armature.boneData.size() != armature.boneCollection.size())
			{
				armature.boneData.resize(armature.boneCollection.size());
			}

			XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
			XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

			int boneIndex = 0;
			for (Entity boneEntity : armature.boneCollection)
			{
				const TransformComponent& bone = *transforms.GetComponent(boneEntity);

				XMMATRIX B = XMLoadFloat4x4(&armature.inverseBindMatrices[boneIndex]);
				XMMATRIX W = XMLoadFloat4x4(&bone.world);
				XMMATRIX M = B * W * R;

				XMFLOAT4X4 mat;
				XMStoreFloat4x4(&mat, M);
				armature.boneData[boneIndex++].Create(mat);

				const float bone_radius = 1;
				XMFLOAT3 bonepos = bone.GetPosition();
				AABB boneAABB;
				boneAABB.createFromHalfWidth(bonepos, XMFLOAT3(bone_radius, bone_radius, bone_radius));
				_min = wi::math::Min(_min, boneAABB._min);
				_max = wi::math::Max(_max, boneAABB._max);
			}

			armature.aabb = AABB(_min, _max);

			if (!armature.boneBuffer.IsValid() || armature.boneBuffer.desc.size != armature.boneData.size() * sizeof(ShaderTransform))
			{
				armature.CreateRenderData();
			}
		});
	}
	void Scene::RunMeshUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)meshes.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			Entity entity = meshes.GetEntity(args.jobIndex);
			MeshComponent& mesh = meshes[args.jobIndex];
			GraphicsDevice* device = wi::graphics::GetDevice();

			if (!mesh.vertexBuffer_PRE.IsValid())
			{
				const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(entity);
				if (softbody != nullptr && wi::physics::IsSimulationEnabled())
				{
					device->CreateBuffer(&mesh.vertexBuffer_POS.desc, nullptr, &mesh.streamoutBuffer_POS);
					device->CreateBuffer(&mesh.vertexBuffer_POS.desc, nullptr, &mesh.vertexBuffer_PRE);
					device->CreateBuffer(&mesh.vertexBuffer_TAN.desc, nullptr, &mesh.streamoutBuffer_TAN);
				}
				else if (mesh.IsSkinned() && armatures.Contains(mesh.armatureID))
				{
					if (softbody == nullptr || softbody->vertex_positions_simulation.empty())
					{
						device->CreateBuffer(&mesh.streamoutBuffer_POS.GetDesc(), nullptr, &mesh.vertexBuffer_PRE);
					}
				}
			}

			if (mesh.streamoutBuffer_POS.IsValid() && mesh.vertexBuffer_PRE.IsValid())
			{
				std::swap(mesh.streamoutBuffer_POS, mesh.vertexBuffer_PRE);
			}

			uint32_t subsetIndex = 0;
			for (auto& subset : mesh.subsets)
			{
				const MaterialComponent* material = materials.GetComponent(subset.materialID);
				if (material != nullptr)
				{
					subset.materialIndex = (uint32_t)materials.GetIndex(subset.materialID);
					if (mesh.BLAS.IsValid())
					{
						auto& geometry = mesh.BLAS.desc.bottom_level.geometries[subsetIndex];
						uint32_t flags = geometry.flags;
						if (material->IsAlphaTestEnabled() || (material->GetRenderTypes() & RENDERTYPE_TRANSPARENT) || !material->IsCastingShadow())
						{
							geometry.flags &= ~RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE;
						}
						else
						{
							geometry.flags = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE;
						}
						if (flags != geometry.flags)
						{
							mesh.BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;
						}
						if (mesh.streamoutBuffer_POS.IsValid())
						{
							mesh.BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;
							geometry.triangles.vertex_buffer = mesh.streamoutBuffer_POS;
						}
						if (material->IsDoubleSided())
						{
							mesh._flags |= MeshComponent::TLAS_FORCE_DOUBLE_SIDED;
						}
					}
				}
				else
				{
					subset.materialIndex = 0;
				}
				subsetIndex++;
			}

			if (mesh.BLAS.IsValid())
			{
				if (mesh.dirty_morph)
				{
					mesh.BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;
				}
			}

			mesh.terrain_material1_index = (uint32_t)materials.GetIndex(mesh.terrain_material1);
			mesh.terrain_material2_index = (uint32_t)materials.GetIndex(mesh.terrain_material2);
			mesh.terrain_material3_index = (uint32_t)materials.GetIndex(mesh.terrain_material3);

			// Update morph targets if needed:
			if (mesh.dirty_morph && !mesh.targets.empty())
			{
			    XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
			    XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

			    for (size_t i = 0; i < mesh.vertex_positions.size(); ++i)
			    {
					XMFLOAT3 pos = mesh.vertex_positions[i];
					XMFLOAT3 nor = mesh.vertex_normals.empty() ? XMFLOAT3(1, 1, 1) : mesh.vertex_normals[i];
					const uint8_t wind = mesh.vertex_windweights.empty() ? 0xFF : mesh.vertex_windweights[i];

					for (const MeshComponent::MeshMorphTarget& target : mesh.targets)
					{
						pos.x += target.weight * target.vertex_positions[i].x;
						pos.y += target.weight * target.vertex_positions[i].y;
						pos.z += target.weight * target.vertex_positions[i].z;

						if (!target.vertex_normals.empty())
						{
							nor.x += target.weight * target.vertex_normals[i].x;
							nor.y += target.weight * target.vertex_normals[i].y;
							nor.z += target.weight * target.vertex_normals[i].z;
						}
					}

					XMStoreFloat3(&nor, XMVector3Normalize(XMLoadFloat3(&nor)));
					mesh.vertex_positions_morphed[i].FromFULL(pos, nor, wind);

					_min = wi::math::Min(_min, pos);
					_max = wi::math::Max(_max, pos);
			    }

			    mesh.aabb = AABB(_min, _max);
			}

			mesh.WriteShaderMesh(meshArrayMapped + args.jobIndex);

		});
	}
	void Scene::RunMaterialUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)materials.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			MaterialComponent& material = materials[args.jobIndex];
			Entity entity = materials.GetEntity(args.jobIndex);
			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				material.layerMask = layer->layerMask;
			}

			material.texAnimElapsedTime += dt * material.texAnimFrameRate;
			if (material.texAnimElapsedTime >= 1.0f)
			{
				material.texMulAdd.z = fmodf(material.texMulAdd.z + material.texAnimDirection.x, 1);
				material.texMulAdd.w = fmodf(material.texMulAdd.w + material.texAnimDirection.y, 1);
				material.texAnimElapsedTime = 0.0f;

				material.SetDirty(); // will trigger constant buffer update later on
			}

			material.engineStencilRef = STENCILREF_DEFAULT;
			if (material.IsCustomShader())
			{
				material.engineStencilRef = STENCILREF_CUSTOMSHADER;
			}

			if (material.IsDirty())
			{
				material.SetDirty(false);
			}

			material.WriteShaderMaterial(materialArrayMapped + args.jobIndex);

		});
	}
	void Scene::RunImpostorUpdateSystem(wi::jobsystem::context& ctx)
	{
		if (impostors.GetCount() > 0 && !impostorArray.IsValid())
		{
			GraphicsDevice* device = wi::graphics::GetDevice();

			TextureDesc desc;
			desc.width = impostorTextureDim;
			desc.height = impostorTextureDim;

			desc.bind_flags = BindFlag::DEPTH_STENCIL;
			desc.array_size = 1;
			desc.format = Format::D16_UNORM;
			desc.layout = ResourceState::DEPTHSTENCIL;
			device->CreateTexture(&desc, nullptr, &impostorDepthStencil);
			device->SetName(&impostorDepthStencil, "impostorDepthStencil");

			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.array_size = maxImpostorCount * impostorCaptureAngles * 3;
			desc.format = Format::R8G8B8A8_UNORM;
			desc.layout = ResourceState::SHADER_RESOURCE;

			device->CreateTexture(&desc, nullptr, &impostorArray);
			device->SetName(&impostorArray, "impostorArray");

			renderpasses_impostor.resize(desc.array_size);

			for (uint32_t i = 0; i < desc.array_size; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&impostorArray, SubresourceType::RTV, i, 1, 0, 1);
				assert(subresource_index == i);

				RenderPassDesc renderpassdesc;
				renderpassdesc.attachments.push_back(
					RenderPassAttachment::RenderTarget(
						&impostorArray,
						RenderPassAttachment::LoadOp::CLEAR
					)
				);
				renderpassdesc.attachments.back().subresource = subresource_index;

				renderpassdesc.attachments.push_back(
					RenderPassAttachment::DepthStencil(
						&impostorDepthStencil,
						RenderPassAttachment::LoadOp::CLEAR,
						RenderPassAttachment::StoreOp::DONTCARE
					)
				);

				device->CreateRenderPass(&renderpassdesc, &renderpasses_impostor[subresource_index]);
			}
		}

		wi::jobsystem::Dispatch(ctx, (uint32_t)impostors.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {

			ImpostorComponent& impostor = impostors[args.jobIndex];
			impostor.aabb = AABB();
			impostor.instances.clear();

			if (impostor.IsDirty())
			{
				impostor.SetDirty(false);
				impostor.render_dirty = true;
			}
		});
	}
	void Scene::RunObjectUpdateSystem(wi::jobsystem::context& ctx)
	{
		assert(objects.GetCount() == aabb_objects.GetCount());

		parallel_bounds.clear();
		parallel_bounds.resize((size_t)wi::jobsystem::DispatchGroupCount((uint32_t)objects.GetCount(), small_subtask_groupsize));
		
		wi::jobsystem::Dispatch(ctx, (uint32_t)objects.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			Entity entity = objects.GetEntity(args.jobIndex);
			ObjectComponent& object = objects[args.jobIndex];
			AABB& aabb = aabb_objects[args.jobIndex];

			// Update occlusion culling status:
			if (!wi::renderer::GetFreezeCullingCameraEnabled())
			{
				object.occlusionHistory <<= 1; // advance history by 1 frame
				int query_id = object.occlusionQueries[queryheap_idx];
				if (queryResultBuffer[queryheap_idx].mapped_data != nullptr && query_id >= 0)
				{
					uint64_t visible = ((uint64_t*)queryResultBuffer[queryheap_idx].mapped_data)[query_id];
					if (visible)
					{
						object.occlusionHistory |= 1; // visible
					}
				}
				else
				{
					object.occlusionHistory |= 1; // visible
				}
			}
			object.occlusionQueries[queryheap_idx] = -1; // invalidate query

			const LayerComponent* layer = layers.GetComponent(entity);
			uint32_t layerMask;
			if (layer == nullptr)
			{
				layerMask = ~0;
			}
			else
			{
				layerMask = layer->GetLayerMask();
			}

			aabb = AABB();
			object.rendertypeMask = 0;
			object.SetDynamic(false);
			object.SetImpostorPlacement(false);
			object.SetRequestPlanarReflection(false);

			if (object.meshID != INVALID_ENTITY)
			{

				const MeshComponent* mesh = meshes.GetComponent(object.meshID);

				// These will only be valid for a single frame:
				object.transform_index = (int)transforms.GetIndex(entity);
				object.prev_transform_index = (int)prev_transforms.GetIndex(entity);

				const TransformComponent& transform = transforms[object.transform_index];

				if (mesh != nullptr)
				{
					XMMATRIX W = XMLoadFloat4x4(&transform.world);
					aabb = mesh->aabb.transform(W);

					// This is instance bounding box matrix:
					XMFLOAT4X4 meshMatrix;
					XMStoreFloat4x4(&meshMatrix, mesh->aabb.getAsBoxMatrix() * W);

					// We need sometimes the center of the instance bounding box, not the transform position (which can be outside the bounding box)
					object.center = *((XMFLOAT3*)&meshMatrix._41);

					if (mesh->IsSkinned() || mesh->IsDynamic())
					{
						object.SetDynamic(true);
						const ArmatureComponent* armature = armatures.GetComponent(mesh->armatureID);
						if (armature != nullptr)
						{
							aabb = AABB::Merge(aabb, armature->aabb);
						}
					}

					for (auto& subset : mesh->subsets)
					{
						const MaterialComponent* material = materials.GetComponent(subset.materialID);

						if (material != nullptr)
						{
							object.rendertypeMask |= material->GetRenderTypes();

							if (material->HasPlanarReflection())
							{
								object.SetRequestPlanarReflection(true);
							}
						}
					}

					ImpostorComponent* impostor = impostors.GetComponent(object.meshID);
					if (impostor != nullptr)
					{
						object.SetImpostorPlacement(true);
						object.impostorSwapDistance = impostor->swapInDistance;
						object.impostorFadeThresholdRadius = aabb.getRadius();

						impostor->aabb = AABB::Merge(impostor->aabb, aabb);
						impostor->color = object.color;
						impostor->fadeThresholdRadius = object.impostorFadeThresholdRadius;

						const Sphere boundingsphere = mesh->GetBoundingSphere();

						locker.lock();
						impostor->instances.push_back(args.jobIndex);
						locker.unlock();
					}

					SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
					if (softbody != nullptr)
					{
						// this will be registered as soft body in the next physics update
						softbody->_flags |= SoftBodyPhysicsComponent::SAFE_TO_REGISTER;

						// soft body manipulated with the object matrix
						softbody->worldMatrix = transform.world;

						if (softbody->graphicsToPhysicsVertexMapping.empty())
						{
							softbody->CreateFromMesh(*mesh);
						}

						// simulation aabb will be used for soft bodies
						aabb = softbody->aabb;

						// soft bodies have no transform, their vertices are simulated in world space
						object.transform_index = -1;
						object.prev_transform_index = -1;
					}

					// Create GPU instance data:
					const XMFLOAT4X4& worldMatrix = object.transform_index >= 0 ? transforms[object.transform_index].world : wi::math::IDENTITY_MATRIX;
					const XMFLOAT4X4& worldMatrixPrev = object.prev_transform_index >= 0 ? prev_transforms[object.prev_transform_index].world_prev : wi::math::IDENTITY_MATRIX;

					XMMATRIX worldMatrixInverseTranspose = XMLoadFloat4x4(&worldMatrix);
					worldMatrixInverseTranspose = XMMatrixInverse(nullptr, worldMatrixInverseTranspose);
					worldMatrixInverseTranspose = XMMatrixTranspose(worldMatrixInverseTranspose);
					XMFLOAT4X4 transformIT;
					XMStoreFloat4x4(&transformIT, worldMatrixInverseTranspose);

					GraphicsDevice* device = wi::graphics::GetDevice();
					ShaderMeshInstance& inst = instanceArrayMapped[args.jobIndex];
					inst.init();
					inst.transform.Create(worldMatrix);
					inst.transformInverseTranspose.Create(transformIT);
					inst.transformPrev.Create(worldMatrixPrev);
					if (object.lightmap.IsValid())
					{
						inst.lightmap = device->GetDescriptorIndex(&object.lightmap, SubresourceType::SRV);
					}
					inst.uid = entity;
					inst.layerMask = layerMask;
					inst.color = wi::math::CompressColor(object.color);
					inst.emissive = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(object.emissiveColor.x * object.emissiveColor.w, object.emissiveColor.y * object.emissiveColor.w, object.emissiveColor.z * object.emissiveColor.w));
					inst.meshIndex = (uint)meshes.GetIndex(object.meshID);

					if (TLAS_instancesMapped != nullptr)
					{
						// TLAS instance data:
						RaytracingAccelerationStructureDesc::TopLevel::Instance instance = {};
						for (int i = 0; i < arraysize(instance.transform); ++i)
						{
							for (int j = 0; j < arraysize(instance.transform[i]); ++j)
							{
								instance.transform[i][j] = worldMatrix.m[j][i];
							}
						}
						instance.instance_id = args.jobIndex;
						instance.instance_mask = layerMask & 0xFF;
						instance.bottom_level = mesh->BLAS;

						if (mesh->IsDoubleSided() || mesh->_flags & MeshComponent::TLAS_FORCE_DOUBLE_SIDED)
						{
							instance.flags |= RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;
						}

						if (XMVectorGetX(XMMatrixDeterminant(W)) > 0)
						{
							// There is a mismatch between object space winding and BLAS winding:
							//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_raytracing_instance_flags
							instance.flags |= RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
						}

						void* dest = (void*)((size_t)TLAS_instancesMapped + (size_t)args.jobIndex * device->GetTopLevelAccelerationStructureInstanceSize());
						device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
					}

					// lightmap things:
					if (object.IsLightmapRenderRequested() && dt > 0)
					{
						if (!object.lightmap.IsValid())
						{
							object.lightmapWidth = wi::math::GetNextPowerOfTwo(object.lightmapWidth + 1) / 2;
							object.lightmapHeight = wi::math::GetNextPowerOfTwo(object.lightmapHeight + 1) / 2;

							TextureDesc desc;
							desc.width = object.lightmapWidth;
							desc.height = object.lightmapHeight;
							desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
							// Note: we need the full precision format to achieve correct accumulative blending! 
							//	But the final lightmap will be compressed into an optimal format when the rendering is finished
							desc.format = Format::R32G32B32A32_FLOAT;

							device->CreateTexture(&desc, nullptr, &object.lightmap);
							device->SetName(&object.lightmap, "lightmap_renderable");

							RenderPassDesc renderpassdesc;

							renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&object.lightmap, RenderPassAttachment::LoadOp::CLEAR));

							device->CreateRenderPass(&renderpassdesc, &object.renderpass_lightmap_clear);

							renderpassdesc.attachments.back().loadop = RenderPassAttachment::LoadOp::LOAD;
							device->CreateRenderPass(&renderpassdesc, &object.renderpass_lightmap_accumulate);

							object.lightmapIterationCount = 0; // reset accumulation
						}
						lightmap_refresh_needed.store(true);
					}

					if (!object.lightmapTextureData.empty() && !object.lightmap.IsValid())
					{
						// Create a GPU-side per object lighmap if there is none yet, but the data exists already:
						object.lightmap.desc.format = Format::R11G11B10_FLOAT;
						wi::texturehelper::CreateTexture(object.lightmap, object.lightmapTextureData.data(), object.lightmapWidth, object.lightmapHeight, object.lightmap.desc.format);
						device->SetName(&object.lightmap, "lightmap");
					}
				}

				aabb.layerMask = layerMask;

				// parallel bounds computation using shared memory:
				AABB* shared_bounds = (AABB*)args.sharedmemory;
				if (args.isFirstJobInGroup)
				{
					*shared_bounds = aabb_objects[args.jobIndex];
				}
				else
				{
					*shared_bounds = AABB::Merge(*shared_bounds, aabb_objects[args.jobIndex]);
				}
				if (args.isLastJobInGroup)
				{
					parallel_bounds[args.groupID] = *shared_bounds;
				}
			}

		}, sizeof(AABB));
	}
	void Scene::RunCameraUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)cameras.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			CameraComponent& camera = cameras[args.jobIndex];
			Entity entity = cameras.GetEntity(args.jobIndex);
			const TransformComponent* transform = transforms.GetComponent(entity);
			if (transform != nullptr)
			{
				camera.TransformCamera(*transform);
			}
			camera.UpdateCamera();
		});
	}
	void Scene::RunDecalUpdateSystem(wi::jobsystem::context& ctx)
	{
		assert(decals.GetCount() == aabb_decals.GetCount());

		for (size_t i = 0; i < decals.GetCount(); ++i)
		{
			DecalComponent& decal = decals[i];
			Entity entity = decals.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);
			decal.world = transform.world;

			XMMATRIX W = XMLoadFloat4x4(&decal.world);
			XMVECTOR front = XMVectorSet(0, 0, 1, 0);
			front = XMVector3TransformNormal(front, W);
			XMStoreFloat3(&decal.front, front);

			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);
			XMStoreFloat3(&decal.position, T);
			XMFLOAT3 scale;
			XMStoreFloat3(&scale, S);
			decal.range = std::max(scale.x, std::max(scale.y, scale.z)) * 2;

			AABB& aabb = aabb_decals[i];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.transform(transform.world);

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer == nullptr)
			{
				aabb.layerMask = ~0;
			}
			else
			{
				aabb.layerMask = layer->GetLayerMask();
			}

			const MaterialComponent& material = *materials.GetComponent(entity);
			decal.color = material.baseColor;
			decal.emissive = material.GetEmissiveStrength();
			decal.texture = material.textures[MaterialComponent::BASECOLORMAP].resource;
			decal.normal = material.textures[MaterialComponent::NORMALMAP].resource;
		}
	}
	void Scene::RunProbeUpdateSystem(wi::jobsystem::context& ctx)
	{
		assert(probes.GetCount() == aabb_probes.GetCount());

		if (!envmapArray.IsValid()) // even when zero probes, this will be created, since sometimes only the sky will be rendered into it
		{
			GraphicsDevice* device = wi::graphics::GetDevice();

			TextureDesc desc;
			desc.array_size = 6;
			desc.bind_flags = BindFlag::DEPTH_STENCIL;
			desc.format = Format::D16_UNORM;
			desc.height = envmapRes;
			desc.width = envmapRes;
			desc.mip_levels = 1;
			desc.misc_flags = ResourceMiscFlag::TEXTURECUBE;
			desc.usage = Usage::DEFAULT;
			desc.layout = ResourceState::DEPTHSTENCIL;

			device->CreateTexture(&desc, nullptr, &envrenderingDepthBuffer);
			device->SetName(&envrenderingDepthBuffer, "envrenderingDepthBuffer");

			desc.array_size = envmapCount * 6;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET | BindFlag::UNORDERED_ACCESS;
			desc.format = Format::R11G11B10_FLOAT;
			desc.height = envmapRes;
			desc.width = envmapRes;
			desc.mip_levels = envmapMIPs;
			desc.misc_flags = ResourceMiscFlag::TEXTURECUBE;
			desc.usage = Usage::DEFAULT;
			desc.layout = ResourceState::SHADER_RESOURCE;

			device->CreateTexture(&desc, nullptr, &envmapArray);
			device->SetName(&envmapArray, "envmapArray");

			renderpasses_envmap.resize(envmapCount);

			for (uint32_t i = 0; i < envmapCount; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::RTV, i * 6, 6, 0, 1);
				assert(subresource_index == i);

				RenderPassDesc renderpassdesc;
				renderpassdesc.attachments.push_back(
					RenderPassAttachment::RenderTarget(&envmapArray,
						RenderPassAttachment::LoadOp::DONTCARE
					)
				);
				renderpassdesc.attachments.back().subresource = subresource_index;

				renderpassdesc.attachments.push_back(
					RenderPassAttachment::DepthStencil(
						&envrenderingDepthBuffer,
						RenderPassAttachment::LoadOp::CLEAR,
						RenderPassAttachment::StoreOp::DONTCARE
					)
				);

				device->CreateRenderPass(&renderpassdesc, &renderpasses_envmap[subresource_index]);
			}
			for (uint32_t i = 0; i < envmapArray.desc.mip_levels; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::SRV, 0, desc.array_size, i, 1);
				assert(subresource_index == i);
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::UAV, 0, desc.array_size, i, 1);
				assert(subresource_index == i);
			}

			// debug probe views, individual cubes:
			for (uint32_t i = 0; i < envmapCount; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::SRV, i * 6, 6, 0, -1);
				assert(subresource_index == envmapArray.desc.mip_levels + i);
			}
		}

		// reconstruct envmap array status:
		bool envmapTaken[envmapCount] = {};
		for (size_t i = 0; i < probes.GetCount(); ++i)
		{
			EnvironmentProbeComponent& probe = probes[i];
			if (probe.textureIndex >= 0 && probe.textureIndex < envmapCount)
			{
				envmapTaken[probe.textureIndex] = true;
			}
			else
			{
				probe.textureIndex = -1;
			}
		}

		for (size_t probeIndex = 0; probeIndex < probes.GetCount(); ++probeIndex)
		{
			EnvironmentProbeComponent& probe = probes[probeIndex];
			Entity entity = probes.GetEntity(probeIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			probe.position = transform.GetPosition();

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMStoreFloat4x4(&probe.inverseMatrix, XMMatrixInverse(nullptr, W));

			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);
			XMFLOAT3 scale;
			XMStoreFloat3(&scale, S);
			probe.range = std::max(scale.x, std::max(scale.y, scale.z)) * 2;

			AABB& aabb = aabb_probes[probeIndex];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.transform(transform.world);

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer == nullptr)
			{
				aabb.layerMask = ~0;
			}
			else
			{
				aabb.layerMask = layer->GetLayerMask();
			}

			if (probe.IsDirty() || probe.IsRealTime())
			{
				probe.SetDirty(false);
				probe.render_dirty = true;
			}

			if (probe.render_dirty && probe.textureIndex < 0)
			{
				// need to take a free envmap texture slot:
				bool found = false;
				for (int i = 0; i < arraysize(envmapTaken); ++i)
				{
					if (envmapTaken[i] == false)
					{
						envmapTaken[i] = true;
						probe.textureIndex = i;
						found = true;
						break;
					}
				}
			}
		}
	}
	void Scene::RunForceUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)forces.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			ForceFieldComponent& force = forces[args.jobIndex];
			Entity entity = forces.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);

			XMStoreFloat3(&force.position, T);
			XMStoreFloat3(&force.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, -1, 0, 0), W)));

			force.range_global = force.range_local * std::max(XMVectorGetX(S), std::max(XMVectorGetY(S), XMVectorGetZ(S)));
		});
	}
	void Scene::RunLightUpdateSystem(wi::jobsystem::context& ctx)
	{
		assert(lights.GetCount() == aabb_lights.GetCount());

		wi::jobsystem::Dispatch(ctx, (uint32_t)lights.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			LightComponent& light = lights[args.jobIndex];
			Entity entity = lights.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);
			AABB& aabb = aabb_lights[args.jobIndex];

			light.occlusionquery = -1;

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer == nullptr)
			{
				aabb.layerMask = ~0;
			}
			else
			{
				aabb.layerMask = layer->GetLayerMask();
			}

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);

			XMStoreFloat3(&light.position, T);
			XMStoreFloat4(&light.rotation, R);
			XMStoreFloat3(&light.scale, S);
			XMStoreFloat3(&light.direction, XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), W));

			light.range_global = light.range_local * std::max(XMVectorGetX(S), std::max(XMVectorGetY(S), XMVectorGetZ(S)));

			switch (light.type)
			{
			default:
			case LightComponent::DIRECTIONAL:
				aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
				locker.lock();
				if (args.jobIndex < weather.most_important_light_index)
				{
					weather.most_important_light_index = args.jobIndex;
					weather.sunColor = light.color;
					weather.sunDirection = light.direction;
					weather.sunEnergy = light.energy;
				}
				locker.unlock();
				break;
			case LightComponent::SPOT:
				aabb.createFromHalfWidth(light.position, XMFLOAT3(light.GetRange(), light.GetRange(), light.GetRange()));
				break;
			case LightComponent::POINT:
				aabb.createFromHalfWidth(light.position, XMFLOAT3(light.GetRange(), light.GetRange(), light.GetRange()));
				break;
			}

		});
	}
	void Scene::RunParticleUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)hairs.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			HairParticleSystem& hair = hairs[args.jobIndex];
			Entity entity = hairs.GetEntity(args.jobIndex);

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				hair.layerMask = layer->GetLayerMask();
			}

			if (hair.meshID != INVALID_ENTITY)
			{
				const MeshComponent* mesh = meshes.GetComponent(hair.meshID);

				if (mesh != nullptr)
				{
					const TransformComponent& transform = *transforms.GetComponent(entity);

					hair.UpdateCPU(transform, *mesh, dt);

					GraphicsDevice* device = wi::graphics::GetDevice();

					size_t meshIndex = meshes.GetCount() + args.jobIndex;
					ShaderMesh& mesh = meshArrayMapped[meshIndex];
					mesh.init();
					mesh.ib = device->GetDescriptorIndex(&hair.primitiveBuffer, SubresourceType::SRV);
					mesh.vb_pos_nor_wind = device->GetDescriptorIndex(&hair.vertexBuffer_POS[0], SubresourceType::SRV);
					mesh.vb_pre = device->GetDescriptorIndex(&hair.vertexBuffer_POS[1], SubresourceType::SRV);
					mesh.vb_uv0 = device->GetDescriptorIndex(&hair.vertexBuffer_TEX, SubresourceType::SRV);
					mesh.subsetbuffer = device->GetDescriptorIndex(&hair.subsetBuffer, SubresourceType::SRV);
					mesh.flags = SHADERMESH_FLAG_DOUBLE_SIDED | SHADERMESH_FLAG_HAIRPARTICLE;

					size_t instanceIndex = objects.GetCount() + args.jobIndex;
					ShaderMeshInstance& inst = instanceArrayMapped[instanceIndex];
					inst.init();
					inst.uid = entity;
					inst.layerMask = hair.layerMask;
					// every vertex is pretransformed and simulated in worldspace for hair particle:
					inst.transform.Create(wi::math::IDENTITY_MATRIX);
					inst.transformPrev.Create(wi::math::IDENTITY_MATRIX);
					inst.meshIndex = (uint)meshIndex;

					if (TLAS_instancesMapped != nullptr && hair.BLAS.IsValid())
					{
						// TLAS instance data:
						RaytracingAccelerationStructureDesc::TopLevel::Instance instance = {};
						for (int i = 0; i < arraysize(instance.transform); ++i)
						{
							for (int j = 0; j < arraysize(instance.transform[i]); ++j)
							{
								instance.transform[i][j] = wi::math::IDENTITY_MATRIX.m[j][i];
							}
						}
						instance.instance_id = (uint32_t)instanceIndex;
						instance.instance_mask = hair.layerMask & 0xFF;
						instance.bottom_level = hair.BLAS;
						instance.flags = RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;

						void* dest = (void*)((size_t)TLAS_instancesMapped + instanceIndex * device->GetTopLevelAccelerationStructureInstanceSize());
						device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
					}
				}
			}

		});

		wi::jobsystem::Dispatch(ctx, (uint32_t)emitters.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			EmittedParticleSystem& emitter = emitters[args.jobIndex];
			Entity entity = emitters.GetEntity(args.jobIndex);

			MaterialComponent* material = materials.GetComponent(entity);
			if (material != nullptr)
			{
				if (!material->IsUsingVertexColors())
				{
					material->SetUseVertexColors(true);
				}
				if (emitter.shaderType == EmittedParticleSystem::PARTICLESHADERTYPE::SOFT_LIGHTING)
				{
					material->shaderType = MaterialComponent::SHADERTYPE_PBR;
				}
				else
				{
					material->shaderType = MaterialComponent::SHADERTYPE_UNLIT;
				}
			}

			const LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				emitter.layerMask = layer->GetLayerMask();
			}

			const TransformComponent& transform = *transforms.GetComponent(entity);
			emitter.UpdateCPU(transform, dt);

			GraphicsDevice* device = wi::graphics::GetDevice();

			size_t meshIndex = meshes.GetCount() + hairs.GetCount() + args.jobIndex;
			ShaderMesh& mesh = meshArrayMapped[meshIndex];
			mesh.init();
			mesh.ib = device->GetDescriptorIndex(&emitter.primitiveBuffer, SubresourceType::SRV);
			mesh.vb_pos_nor_wind = device->GetDescriptorIndex(&emitter.vertexBuffer_POS, SubresourceType::SRV);
			mesh.vb_uv0 = device->GetDescriptorIndex(&emitter.vertexBuffer_TEX, SubresourceType::SRV);
			mesh.vb_uv1 = device->GetDescriptorIndex(&emitter.vertexBuffer_TEX2, SubresourceType::SRV);
			mesh.vb_col = device->GetDescriptorIndex(&emitter.vertexBuffer_COL, SubresourceType::SRV);
			mesh.subsetbuffer = device->GetDescriptorIndex(&emitter.subsetBuffer, SubresourceType::SRV);
			mesh.flags = SHADERMESH_FLAG_DOUBLE_SIDED | SHADERMESH_FLAG_EMITTEDPARTICLE;

			size_t instanceIndex = objects.GetCount() + hairs.GetCount() + args.jobIndex;
			ShaderMeshInstance& inst = instanceArrayMapped[instanceIndex];
			inst.init();
			inst.uid = entity;
			inst.layerMask = emitter.layerMask;
			// every vertex is pretransformed and simulated in worldspace for emitted particle:
			inst.transform.Create(wi::math::IDENTITY_MATRIX);
			inst.transformPrev.Create(wi::math::IDENTITY_MATRIX);
			inst.meshIndex = (uint)meshIndex;

			if (TLAS_instancesMapped != nullptr && emitter.BLAS.IsValid())
			{
				// TLAS instance data:
				RaytracingAccelerationStructureDesc::TopLevel::Instance instance = {};
				for (int i = 0; i < arraysize(instance.transform); ++i)
				{
					for (int j = 0; j < arraysize(instance.transform[i]); ++j)
					{
						instance.transform[i][j] = wi::math::IDENTITY_MATRIX.m[j][i];
					}
				}
				instance.instance_id = (uint32_t)instanceIndex;
				instance.instance_mask = emitter.layerMask & 0xFF;
				instance.bottom_level = emitter.BLAS;
				instance.flags = RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;

				void* dest = (void*)((size_t)TLAS_instancesMapped + instanceIndex * device->GetTopLevelAccelerationStructureInstanceSize());
				device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
			}

		});
	}
	void Scene::RunWeatherUpdateSystem(wi::jobsystem::context& ctx)
	{
		if (weathers.GetCount() > 0)
		{
			weather = weathers[0];
			weather.most_important_light_index = ~0;

			if (weather.IsOceanEnabled() && !ocean.IsValid())
			{
				OceanRegenerate();
			}
		}
	}
	void Scene::RunSoundUpdateSystem(wi::jobsystem::context& ctx)
	{
		const CameraComponent& camera = GetCamera();
		wi::audio::SoundInstance3D instance3D;
		instance3D.listenerPos = camera.Eye;
		instance3D.listenerUp = camera.Up;
		instance3D.listenerFront = camera.At;

		for (size_t i = 0; i < sounds.GetCount(); ++i)
		{
			SoundComponent& sound = sounds[i];

			if (!sound.IsDisable3D())
			{
				Entity entity = sounds.GetEntity(i);
				const TransformComponent* transform = transforms.GetComponent(entity);
				if (transform != nullptr)
				{
					instance3D.emitterPos = transform->GetPosition();
					wi::audio::Update3D(&sound.soundinstance, instance3D);
				}
			}
			if (sound.IsPlaying())
			{
				wi::audio::Play(&sound.soundinstance);
			}
			else
			{
				wi::audio::Stop(&sound.soundinstance);
			}
			if (!sound.IsLooped())
			{
				wi::audio::ExitLoop(&sound.soundinstance);
			}
			wi::audio::SetVolume(sound.volume, &sound.soundinstance);
		}
	}

	void Scene::PutWaterRipple(const std::string& image, const XMFLOAT3& pos)
	{
		wi::Sprite img(image);
		img.params.enableExtractNormalMap();
		img.params.blendFlag = BLENDMODE_ADDITIVE;
		img.anim.fad = 0.01f;
		img.anim.scaleX = 0.2f;
		img.anim.scaleY = 0.2f;
		img.params.pos = pos;
		img.params.rotation = (wi::random::GetRandom(0, 1000) * 0.001f) * 2 * 3.1415f;
		img.params.siz = XMFLOAT2(1, 1);
		img.params.quality = wi::image::QUALITY_ANISOTROPIC;
		img.params.pivot = XMFLOAT2(0.5f, 0.5f);
		waterRipples.push_back(img);
	}

	XMVECTOR SkinVertex(const MeshComponent& mesh, const ArmatureComponent& armature, uint32_t index, XMVECTOR* N)
	{
		XMVECTOR P;
		if (mesh.vertex_positions_morphed.empty())
		{
		    P = XMLoadFloat3(&mesh.vertex_positions[index]);
		}
		else
		{
		    P = mesh.vertex_positions_morphed[index].LoadPOS();
		}
		const XMUINT4& ind = mesh.vertex_boneindices[index];
		const XMFLOAT4& wei = mesh.vertex_boneweights[index];

		const XMFLOAT4X4 mat[] = {
			armature.boneData[ind.x].GetMatrix(),
			armature.boneData[ind.y].GetMatrix(),
			armature.boneData[ind.z].GetMatrix(),
			armature.boneData[ind.w].GetMatrix(),
		};
		const XMMATRIX M[] = {
			XMMatrixTranspose(XMLoadFloat4x4(&mat[0])),
			XMMatrixTranspose(XMLoadFloat4x4(&mat[1])),
			XMMatrixTranspose(XMLoadFloat4x4(&mat[2])),
			XMMatrixTranspose(XMLoadFloat4x4(&mat[3])),
		};

		XMVECTOR skinned;
		skinned =  XMVector3Transform(P, M[0]) * wei.x;
		skinned += XMVector3Transform(P, M[1]) * wei.y;
		skinned += XMVector3Transform(P, M[2]) * wei.z;
		skinned += XMVector3Transform(P, M[3]) * wei.w;
		P = skinned;

		if (N != nullptr)
		{
			*N = XMLoadFloat3(&mesh.vertex_normals[index]);
			skinned =  XMVector3TransformNormal(*N, M[0]) * wei.x;
			skinned += XMVector3TransformNormal(*N, M[1]) * wei.y;
			skinned += XMVector3TransformNormal(*N, M[2]) * wei.z;
			skinned += XMVector3TransformNormal(*N, M[3]) * wei.w;
			*N = XMVector3Normalize(skinned);
		}

		return P;
	}




	Entity LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix, bool attached)
	{
		Scene scene;
		Entity root = LoadModel(scene, fileName, transformMatrix, attached);
		GetScene().Merge(scene);
		return root;
	}

	Entity LoadModel(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix, bool attached)
	{
		wi::Archive archive(fileName, true);
		if (archive.IsOpen())
		{
			// Serialize it from file:
			scene.Serialize(archive);

			// First, create new root:
			Entity root = CreateEntity();
			scene.transforms.Create(root);
			scene.layers.Create(root).layerMask = ~0;

			{
				// Apply the optional transformation matrix to the new scene:

				// Parent all unparented transforms to new root entity
				for (size_t i = 0; i < scene.transforms.GetCount() - 1; ++i) // GetCount() - 1 because the last added was the "root"
				{
					Entity entity = scene.transforms.GetEntity(i);
					if (!scene.hierarchy.Contains(entity))
					{
						scene.Component_Attach(entity, root);
					}
				}

				// The root component is transformed, scene is updated:
				TransformComponent* root_transform = scene.transforms.GetComponent(root);
				root_transform->MatrixTransform(transformMatrix);

				scene.Update(0);
			}

			if (!attached)
			{
				// In this case, we don't care about the root anymore, so delete it. This will simplify overall hierarchy
				scene.Component_DetachChildren(root);
				scene.Entity_Remove(root);
				root = INVALID_ENTITY;
			}

			return root;
		}

		return INVALID_ENTITY;
	}

	PickResult Pick(const Ray& ray, uint32_t renderTypeMask, uint32_t layerMask, const Scene& scene)
	{
		PickResult result;

		if (scene.objects.GetCount() > 0)
		{
			const XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
			const XMVECTOR rayDirection = XMVector3Normalize(XMLoadFloat3(&ray.direction));

			for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
			{
				const AABB& aabb = scene.aabb_objects[i];
				if (!ray.intersects(aabb))
				{
					continue;
				}

				const ObjectComponent& object = scene.objects[i];
				if (object.meshID == INVALID_ENTITY)
				{
					continue;
				}
				if (!(renderTypeMask & object.GetRenderTypes()))
				{
					continue;
				}

				Entity entity = scene.aabb_objects.GetEntity(i);
				const LayerComponent* layer = scene.layers.GetComponent(entity);
				if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
				{
					continue;
				}

				const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);
				const SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object.meshID);
				const bool softbody_active = softbody != nullptr && !softbody->vertex_positions_simulation.empty();

				const XMMATRIX objectMat = object.transform_index >= 0 ? XMLoadFloat4x4(&scene.transforms[object.transform_index].world) : XMMatrixIdentity();
				const XMMATRIX objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);

				const XMVECTOR rayOrigin_local = XMVector3Transform(rayOrigin, objectMat_Inverse);
				const XMVECTOR rayDirection_local = XMVector3Normalize(XMVector3TransformNormal(rayDirection, objectMat_Inverse));

				const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;

				int subsetCounter = 0;
				for (auto& subset : mesh.subsets)
				{
					for (size_t i = 0; i < subset.indexCount; i += 3)
					{
						const uint32_t i0 = mesh.indices[subset.indexOffset + i + 0];
						const uint32_t i1 = mesh.indices[subset.indexOffset + i + 1];
						const uint32_t i2 = mesh.indices[subset.indexOffset + i + 2];

						XMVECTOR p0;
						XMVECTOR p1;
						XMVECTOR p2;

						if (softbody_active)
						{
							p0 = softbody->vertex_positions_simulation[i0].LoadPOS();
							p1 = softbody->vertex_positions_simulation[i1].LoadPOS();
							p2 = softbody->vertex_positions_simulation[i2].LoadPOS();
						}
						else
						{
							if (armature == nullptr)
							{
								if (mesh.vertex_positions_morphed.empty())
							    {
									p0 = XMLoadFloat3(&mesh.vertex_positions[i0]);
									p1 = XMLoadFloat3(&mesh.vertex_positions[i1]);
									p2 = XMLoadFloat3(&mesh.vertex_positions[i2]);
								}
								else
								{
								    p0 = mesh.vertex_positions_morphed[i0].LoadPOS();
								    p1 = mesh.vertex_positions_morphed[i1].LoadPOS();
								    p2 = mesh.vertex_positions_morphed[i2].LoadPOS();
								}
							}
							else
							{
								p0 = SkinVertex(mesh, *armature, i0);
								p1 = SkinVertex(mesh, *armature, i1);
								p2 = SkinVertex(mesh, *armature, i2);
							}
						}

						float distance;
						XMFLOAT2 bary;
						if (wi::math::RayTriangleIntersects(rayOrigin_local, rayDirection_local, p0, p1, p2, distance, bary))
						{
							const XMVECTOR pos = XMVector3Transform(XMVectorAdd(rayOrigin_local, rayDirection_local*distance), objectMat);
							distance = wi::math::Distance(pos, rayOrigin);

							if (distance < result.distance)
							{
								const XMVECTOR nor = XMVector3Normalize(XMVector3TransformNormal(XMVector3Cross(XMVectorSubtract(p2, p1), XMVectorSubtract(p1, p0)), objectMat));

								result.entity = entity;
								XMStoreFloat3(&result.position, pos);
								XMStoreFloat3(&result.normal, nor);
								result.distance = distance;
								result.subsetIndex = subsetCounter;
								result.vertexID0 = (int)i0;
								result.vertexID1 = (int)i1;
								result.vertexID2 = (int)i2;
								result.bary = bary;
							}
						}
					}
					subsetCounter++;
				}

			}
		}

		// Construct a matrix that will orient to position (P) according to surface normal (N):
		XMVECTOR N = XMLoadFloat3(&result.normal);
		XMVECTOR P = XMLoadFloat3(&result.position);
		XMVECTOR E = XMLoadFloat3(&ray.origin);
		XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, P - E));
		XMVECTOR B = XMVector3Normalize(XMVector3Cross(T, N));
		XMMATRIX M = { T, N, B, P };
		XMStoreFloat4x4(&result.orientation, M);

		return result;
	}

	SceneIntersectSphereResult SceneIntersectSphere(const Sphere& sphere, uint32_t renderTypeMask, uint32_t layerMask, const Scene& scene)
	{
		SceneIntersectSphereResult result;
		XMVECTOR Center = XMLoadFloat3(&sphere.center);
		XMVECTOR Radius = XMVectorReplicate(sphere.radius);
		XMVECTOR RadiusSq = XMVectorMultiply(Radius, Radius);

		if (scene.objects.GetCount() > 0)
		{

			for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
			{
				const AABB& aabb = scene.aabb_objects[i];
				if (!sphere.intersects(aabb))
				{
					continue;
				}

				const ObjectComponent& object = scene.objects[i];
				if (object.meshID == INVALID_ENTITY)
				{
					continue;
				}
				if (!(renderTypeMask & object.GetRenderTypes()))
				{
					continue;
				}

				Entity entity = scene.aabb_objects.GetEntity(i);
				const LayerComponent* layer = scene.layers.GetComponent(entity);
				if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
				{
					continue;
				}

				const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);
				const SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object.meshID);
				const bool softbody_active = softbody != nullptr && !softbody->vertex_positions_simulation.empty();

				const XMMATRIX objectMat = object.transform_index >= 0 ? XMLoadFloat4x4(&scene.transforms[object.transform_index].world) : XMMatrixIdentity();

				const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;

				int subsetCounter = 0;
				for (auto& subset : mesh.subsets)
				{
					for (size_t i = 0; i < subset.indexCount; i += 3)
					{
						const uint32_t i0 = mesh.indices[subset.indexOffset + i + 0];
						const uint32_t i1 = mesh.indices[subset.indexOffset + i + 1];
						const uint32_t i2 = mesh.indices[subset.indexOffset + i + 2];

						XMVECTOR p0;
						XMVECTOR p1;
						XMVECTOR p2;

						if (softbody_active)
						{
							p0 = softbody->vertex_positions_simulation[i0].LoadPOS();
							p1 = softbody->vertex_positions_simulation[i1].LoadPOS();
							p2 = softbody->vertex_positions_simulation[i2].LoadPOS();
						}
						else
						{
							if (armature == nullptr)
							{
								p0 = XMLoadFloat3(&mesh.vertex_positions[i0]);
								p1 = XMLoadFloat3(&mesh.vertex_positions[i1]);
								p2 = XMLoadFloat3(&mesh.vertex_positions[i2]);
							}
							else
							{
								p0 = SkinVertex(mesh, *armature, i0);
								p1 = SkinVertex(mesh, *armature, i1);
								p2 = SkinVertex(mesh, *armature, i2);
							}
						}

						p0 = XMVector3Transform(p0, objectMat);
						p1 = XMVector3Transform(p1, objectMat);
						p2 = XMVector3Transform(p2, objectMat);

						XMFLOAT3 min, max;
						XMStoreFloat3(&min, XMVectorMin(p0, XMVectorMin(p1, p2)));
						XMStoreFloat3(&max, XMVectorMax(p0, XMVectorMax(p1, p2)));
						AABB aabb_triangle(min, max);
						if (sphere.intersects(aabb_triangle) == AABB::OUTSIDE)
						{
							continue;
						}

						// Compute the plane of the triangle (has to be normalized).
						XMVECTOR N = XMVector3Normalize(XMVector3Cross(XMVectorSubtract(p1, p0), XMVectorSubtract(p2, p0)));

						// Assert that the triangle is not degenerate.
						assert(!XMVector3Equal(N, XMVectorZero()));

						// Find the nearest feature on the triangle to the sphere.
						XMVECTOR Dist = XMVector3Dot(XMVectorSubtract(Center, p0), N);

						if (!mesh.IsDoubleSided() && XMVectorGetX(Dist) > 0)
						{
							continue; // pass through back faces
						}

						// If the center of the sphere is farther from the plane of the triangle than
						// the radius of the sphere, then there cannot be an intersection.
						XMVECTOR NoIntersection = XMVectorLess(Dist, XMVectorNegate(Radius));
						NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Dist, Radius));

						// Project the center of the sphere onto the plane of the triangle.
						XMVECTOR Point0 = XMVectorNegativeMultiplySubtract(N, Dist, Center);

						// Is it inside all the edges? If so we intersect because the distance 
						// to the plane is less than the radius.
						//XMVECTOR Intersection = DirectX::Internal::PointOnPlaneInsideTriangle(Point0, p0, p1, p2);

						// Compute the cross products of the vector from the base of each edge to 
						// the point with each edge vector.
						XMVECTOR C0 = XMVector3Cross(XMVectorSubtract(Point0, p0), XMVectorSubtract(p1, p0));
						XMVECTOR C1 = XMVector3Cross(XMVectorSubtract(Point0, p1), XMVectorSubtract(p2, p1));
						XMVECTOR C2 = XMVector3Cross(XMVectorSubtract(Point0, p2), XMVectorSubtract(p0, p2));

						// If the cross product points in the same direction as the normal the the
						// point is inside the edge (it is zero if is on the edge).
						XMVECTOR Zero = XMVectorZero();
						XMVECTOR Inside0 = XMVectorLessOrEqual(XMVector3Dot(C0, N), Zero);
						XMVECTOR Inside1 = XMVectorLessOrEqual(XMVector3Dot(C1, N), Zero);
						XMVECTOR Inside2 = XMVectorLessOrEqual(XMVector3Dot(C2, N), Zero);

						// If the point inside all of the edges it is inside.
						XMVECTOR Intersection = XMVectorAndInt(XMVectorAndInt(Inside0, Inside1), Inside2);

						bool inside = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

						// Find the nearest point on each edge.

						// Edge 0,1
						XMVECTOR Point1 = DirectX::Internal::PointOnLineSegmentNearestPoint(p0, p1, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point1)), RadiusSq));

						// Edge 1,2
						XMVECTOR Point2 = DirectX::Internal::PointOnLineSegmentNearestPoint(p1, p2, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point2)), RadiusSq));

						// Edge 2,0
						XMVECTOR Point3 = DirectX::Internal::PointOnLineSegmentNearestPoint(p2, p0, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point3)), RadiusSq));

						bool intersects = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

						if (intersects)
						{
							XMVECTOR bestPoint = Point0;
							if (!inside)
							{
								// If the sphere center's projection on the triangle plane is not within the triangle,
								//	determine the closest point on triangle to the sphere center
								float bestDist = XMVectorGetX(XMVector3LengthSq(Point1 - Center));
								bestPoint = Point1;

								float d = XMVectorGetX(XMVector3LengthSq(Point2 - Center));
								if (d < bestDist)
								{
									bestDist = d;
									bestPoint = Point2;
								}
								d = XMVectorGetX(XMVector3LengthSq(Point3 - Center));
								if (d < bestDist)
								{
									bestDist = d;
									bestPoint = Point3;
								}
							}
							XMVECTOR intersectionVec = Center - bestPoint;
							XMVECTOR intersectionVecLen = XMVector3Length(intersectionVec);

							result.entity = entity;
							result.depth = sphere.radius - XMVectorGetX(intersectionVecLen);
							XMStoreFloat3(&result.position, bestPoint);
							XMStoreFloat3(&result.normal, intersectionVec / intersectionVecLen);
							return result;
						}
					}
					subsetCounter++;
				}

			}
		}

		return result;
	}
	SceneIntersectSphereResult SceneIntersectCapsule(const Capsule& capsule, uint32_t renderTypeMask, uint32_t layerMask, const Scene& scene)
	{
		SceneIntersectSphereResult result;
		XMVECTOR Base = XMLoadFloat3(&capsule.base);
		XMVECTOR Tip = XMLoadFloat3(&capsule.tip);
		XMVECTOR Radius = XMVectorReplicate(capsule.radius);
		XMVECTOR LineEndOffset = XMVector3Normalize(Tip - Base) * Radius;
		XMVECTOR A = Base + LineEndOffset;
		XMVECTOR B = Tip - LineEndOffset;
		XMVECTOR RadiusSq = XMVectorMultiply(Radius, Radius);
		AABB capsule_aabb = capsule.getAABB();

		if (scene.objects.GetCount() > 0)
		{

			for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
			{
				const AABB& aabb = scene.aabb_objects[i];
				if (capsule_aabb.intersects(aabb) == AABB::INTERSECTION_TYPE::OUTSIDE)
				{
					continue;
				}

				const ObjectComponent& object = scene.objects[i];
				if (object.meshID == INVALID_ENTITY)
				{
					continue;
				}
				if (!(renderTypeMask & object.GetRenderTypes()))
				{
					continue;
				}

				Entity entity = scene.aabb_objects.GetEntity(i);
				const LayerComponent* layer = scene.layers.GetComponent(entity);
				if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
				{
					continue;
				}

				const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);
				const SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object.meshID);
				const bool softbody_active = softbody != nullptr && !softbody->vertex_positions_simulation.empty();

				const XMMATRIX objectMat = object.transform_index >= 0 ? XMLoadFloat4x4(&scene.transforms[object.transform_index].world) : XMMatrixIdentity();

				const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;

				int subsetCounter = 0;
				for (auto& subset : mesh.subsets)
				{
					for (size_t i = 0; i < subset.indexCount; i += 3)
					{
						const uint32_t i0 = mesh.indices[subset.indexOffset + i + 0];
						const uint32_t i1 = mesh.indices[subset.indexOffset + i + 1];
						const uint32_t i2 = mesh.indices[subset.indexOffset + i + 2];

						XMVECTOR p0;
						XMVECTOR p1;
						XMVECTOR p2;

						if (softbody_active)
						{
							p0 = softbody->vertex_positions_simulation[i0].LoadPOS();
							p1 = softbody->vertex_positions_simulation[i1].LoadPOS();
							p2 = softbody->vertex_positions_simulation[i2].LoadPOS();
						}
						else
						{
							if (armature == nullptr || armature->boneData.empty())
							{
								p0 = XMLoadFloat3(&mesh.vertex_positions[i0]);
								p1 = XMLoadFloat3(&mesh.vertex_positions[i1]);
								p2 = XMLoadFloat3(&mesh.vertex_positions[i2]);
							}
							else
							{
								p0 = SkinVertex(mesh, *armature, i0);
								p1 = SkinVertex(mesh, *armature, i1);
								p2 = SkinVertex(mesh, *armature, i2);
							}
						}
						
						p0 = XMVector3Transform(p0, objectMat);
						p1 = XMVector3Transform(p1, objectMat);
						p2 = XMVector3Transform(p2, objectMat);

						XMFLOAT3 min, max;
						XMStoreFloat3(&min, XMVectorMin(p0, XMVectorMin(p1, p2)));
						XMStoreFloat3(&max, XMVectorMax(p0, XMVectorMax(p1, p2)));
						AABB aabb_triangle(min, max);
						if (capsule_aabb.intersects(aabb_triangle) == AABB::OUTSIDE)
						{
							continue;
						}

						// Compute the plane of the triangle (has to be normalized).
						XMVECTOR N = XMVector3Normalize(XMVector3Cross(XMVectorSubtract(p1, p0), XMVectorSubtract(p2, p0)));
						
						XMVECTOR ReferencePoint;
						XMVECTOR d = XMVector3Normalize(B - A);
						if (abs(XMVectorGetX(XMVector3Dot(N, d))) < FLT_EPSILON)
						{
							// Capsule line cannot be intersected with triangle plane (they are parallel)
							//	In this case, just take a point from triangle
							ReferencePoint = p0;
						}
						else
						{
							// Intersect capsule line with triangle plane:
							XMVECTOR t = XMVector3Dot(N, (Base - p0) / XMVectorAbs(XMVector3Dot(N, d)));
							XMVECTOR LinePlaneIntersection = Base + d * t;

							// Compute the cross products of the vector from the base of each edge to 
							// the point with each edge vector.
							XMVECTOR C0 = XMVector3Cross(XMVectorSubtract(LinePlaneIntersection, p0), XMVectorSubtract(p1, p0));
							XMVECTOR C1 = XMVector3Cross(XMVectorSubtract(LinePlaneIntersection, p1), XMVectorSubtract(p2, p1));
							XMVECTOR C2 = XMVector3Cross(XMVectorSubtract(LinePlaneIntersection, p2), XMVectorSubtract(p0, p2));

							// If the cross product points in the same direction as the normal the the
							// point is inside the edge (it is zero if is on the edge).
							XMVECTOR Zero = XMVectorZero();
							XMVECTOR Inside0 = XMVectorLessOrEqual(XMVector3Dot(C0, N), Zero);
							XMVECTOR Inside1 = XMVectorLessOrEqual(XMVector3Dot(C1, N), Zero);
							XMVECTOR Inside2 = XMVectorLessOrEqual(XMVector3Dot(C2, N), Zero);

							// If the point inside all of the edges it is inside.
							XMVECTOR Intersection = XMVectorAndInt(XMVectorAndInt(Inside0, Inside1), Inside2);

							bool inside = XMVectorGetIntX(Intersection) != 0;

							if (inside)
							{
								ReferencePoint = LinePlaneIntersection;
							}
							else
							{
								// Find the nearest point on each edge.

								// Edge 0,1
								XMVECTOR Point1 = wi::math::ClosestPointOnLineSegment(p0, p1, LinePlaneIntersection);

								// Edge 1,2
								XMVECTOR Point2 = wi::math::ClosestPointOnLineSegment(p1, p2, LinePlaneIntersection);

								// Edge 2,0
								XMVECTOR Point3 = wi::math::ClosestPointOnLineSegment(p2, p0, LinePlaneIntersection);

								ReferencePoint = Point1;
								float bestDist = XMVectorGetX(XMVector3LengthSq(Point1 - LinePlaneIntersection));
								float d = abs(XMVectorGetX(XMVector3LengthSq(Point2 - LinePlaneIntersection)));
								if (d < bestDist)
								{
									bestDist = d;
									ReferencePoint = Point2;
								}
								d = abs(XMVectorGetX(XMVector3LengthSq(Point3 - LinePlaneIntersection)));
								if (d < bestDist)
								{
									bestDist = d;
									ReferencePoint = Point3;
								}
							}


						}

						// Place a sphere on closest point on line segment to intersection:
						XMVECTOR Center = wi::math::ClosestPointOnLineSegment(A, B, ReferencePoint);

						// Assert that the triangle is not degenerate.
						assert(!XMVector3Equal(N, XMVectorZero()));

						// Find the nearest feature on the triangle to the sphere.
						XMVECTOR Dist = XMVector3Dot(XMVectorSubtract(Center, p0), N);

						if (!mesh.IsDoubleSided() && XMVectorGetX(Dist) > 0)
						{
							continue; // pass through back faces
						}

						// If the center of the sphere is farther from the plane of the triangle than
						// the radius of the sphere, then there cannot be an intersection.
						XMVECTOR NoIntersection = XMVectorLess(Dist, XMVectorNegate(Radius));
						NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Dist, Radius));

						// Project the center of the sphere onto the plane of the triangle.
						XMVECTOR Point0 = XMVectorNegativeMultiplySubtract(N, Dist, Center);

						// Is it inside all the edges? If so we intersect because the distance 
						// to the plane is less than the radius.
						//XMVECTOR Intersection = DirectX::Internal::PointOnPlaneInsideTriangle(Point0, p0, p1, p2);

						// Compute the cross products of the vector from the base of each edge to 
						// the point with each edge vector.
						XMVECTOR C0 = XMVector3Cross(XMVectorSubtract(Point0, p0), XMVectorSubtract(p1, p0));
						XMVECTOR C1 = XMVector3Cross(XMVectorSubtract(Point0, p1), XMVectorSubtract(p2, p1));
						XMVECTOR C2 = XMVector3Cross(XMVectorSubtract(Point0, p2), XMVectorSubtract(p0, p2));

						// If the cross product points in the same direction as the normal the the
						// point is inside the edge (it is zero if is on the edge).
						XMVECTOR Zero = XMVectorZero();
						XMVECTOR Inside0 = XMVectorLessOrEqual(XMVector3Dot(C0, N), Zero);
						XMVECTOR Inside1 = XMVectorLessOrEqual(XMVector3Dot(C1, N), Zero);
						XMVECTOR Inside2 = XMVectorLessOrEqual(XMVector3Dot(C2, N), Zero);

						// If the point inside all of the edges it is inside.
						XMVECTOR Intersection = XMVectorAndInt(XMVectorAndInt(Inside0, Inside1), Inside2);

						bool inside = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

						// Find the nearest point on each edge.

						// Edge 0,1
						XMVECTOR Point1 = wi::math::ClosestPointOnLineSegment(p0, p1, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point1)), RadiusSq));

						// Edge 1,2
						XMVECTOR Point2 = wi::math::ClosestPointOnLineSegment(p1, p2, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point2)), RadiusSq));

						// Edge 2,0
						XMVECTOR Point3 = wi::math::ClosestPointOnLineSegment(p2, p0, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point3)), RadiusSq));

						bool intersects = XMVector4EqualInt(XMVectorAndCInt(Intersection, NoIntersection), XMVectorTrueInt());

						if (intersects)
						{
							XMVECTOR bestPoint = Point0;
							if (!inside)
							{
								// If the sphere center's projection on the triangle plane is not within the triangle,
								//	determine the closest point on triangle to the sphere center
								float bestDist = XMVectorGetX(XMVector3LengthSq(Point1 - Center));
								bestPoint = Point1;

								float d = XMVectorGetX(XMVector3LengthSq(Point2 - Center));
								if (d < bestDist)
								{
									bestDist = d;
									bestPoint = Point2;
								}
								d = XMVectorGetX(XMVector3LengthSq(Point3 - Center));
								if (d < bestDist)
								{
									bestDist = d;
									bestPoint = Point3;
								}
							}
							XMVECTOR intersectionVec = Center - bestPoint;
							XMVECTOR intersectionVecLen = XMVector3Length(intersectionVec);

							result.entity = entity;
							result.depth = capsule.radius - XMVectorGetX(intersectionVecLen);
							XMStoreFloat3(&result.position, bestPoint);
							XMStoreFloat3(&result.normal, intersectionVec / intersectionVecLen);
							return result;
						}
					}
					subsetCounter++;
				}

			}
		}

		return result;
	}

}
