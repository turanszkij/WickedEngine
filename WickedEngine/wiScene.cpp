#include "wiScene.h"
#include "wiMath.h"
#include "wiTextureHelper.h"
#include "wiResourceManager.h"
#include "wiPhysicsEngine.h"
#include "wiArchive.h"
#include "wiRenderer.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiHelper.h"

#include <functional>
#include <unordered_map>

using namespace wiECS;
using namespace wiGraphics;

namespace wiScene
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

	const Texture* MaterialComponent::GetBaseColorMap() const
	{
		if (baseColorMap != nullptr)
		{
			return baseColorMap->texture;
		}
		return wiTextureHelper::getWhite();
	}
	const Texture* MaterialComponent::GetNormalMap() const
	{
		if (normalMap != nullptr)
		{
			return normalMap->texture;
		}
		return nullptr;
	}
	const Texture* MaterialComponent::GetSurfaceMap() const
	{
		if (surfaceMap != nullptr)
		{
			return surfaceMap->texture;
		}
		return wiTextureHelper::getWhite();
	}
	const Texture* MaterialComponent::GetDisplacementMap() const
	{
		if (displacementMap != nullptr)
		{
			return displacementMap->texture;
		}
		return wiTextureHelper::getWhite();
	}
	const Texture* MaterialComponent::GetEmissiveMap() const
	{
		if (emissiveMap != nullptr)
		{
			return emissiveMap->texture;
		}
		return wiTextureHelper::getWhite();
	}
	const Texture* MaterialComponent::GetOcclusionMap() const
	{
		if (occlusionMap != nullptr)
		{
			return occlusionMap->texture;
		}
		return wiTextureHelper::getWhite();
	}
	void MaterialComponent::WriteShaderMaterial(ShaderMaterial* dest) const
	{
		dest->baseColor = baseColor;
		dest->emissiveColor = emissiveColor;
		dest->texMulAdd = texMulAdd;
		dest->roughness = roughness;
		dest->reflectance = reflectance;
		dest->metalness = metalness;
		dest->refractionIndex = refractionIndex;
		dest->normalMapStrength = (normalMap == nullptr ? 0 : normalMapStrength);
		dest->parallaxOcclusionMapping = parallaxOcclusionMapping;
		dest->displacementMapping = displacementMapping;
		dest->uvset_baseColorMap = baseColorMap == nullptr ? -1 : (int)uvset_baseColorMap;
		dest->uvset_surfaceMap = surfaceMap == nullptr ? -1 : (int)uvset_surfaceMap;
		dest->uvset_normalMap = normalMap == nullptr ? -1 : (int)uvset_normalMap;
		dest->uvset_displacementMap = displacementMap == nullptr ? -1 : (int)uvset_displacementMap;
		dest->uvset_emissiveMap = emissiveMap == nullptr ? -1 : (int)uvset_emissiveMap;
		dest->uvset_occlusionMap = occlusionMap == nullptr ? -1 : (int)uvset_occlusionMap;
		dest->options = 0;
		if (IsUsingVertexColors())
		{
			dest->options |= SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS;
		}
		if (IsUsingSpecularGlossinessWorkflow())
		{
			dest->options |= SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW;
		}
		if (IsOcclusionEnabled_Primary())
		{
			dest->options |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY;
		}
		if (IsOcclusionEnabled_Secondary())
		{
			dest->options |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY;
		}
		if (IsUsingWind())
		{
			dest->options |= SHADERMATERIAL_OPTION_BIT_USE_WIND;
		}

		dest->baseColorAtlasMulAdd = XMFLOAT4(0, 0, 0, 0);
		dest->surfaceMapAtlasMulAdd = XMFLOAT4(0, 0, 0, 0);
		dest->emissiveMapAtlasMulAdd = XMFLOAT4(0, 0, 0, 0);
		dest->normalMapAtlasMulAdd = XMFLOAT4(0, 0, 0, 0);
	}
	uint32_t MaterialComponent::GetRenderTypes() const
	{
		if (IsCustomShader() && customShaderID < (int)wiRenderer::GetCustomShaders().size())
		{
			auto& customShader = wiRenderer::GetCustomShaders()[customShaderID];
			return customShader.renderTypeFlags;
		}
		if (shaderType == SHADERTYPE_WATER)
		{
			return RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER;
		}
		if (userBlendMode == BLENDMODE_OPAQUE)
		{
			return RENDERTYPE_OPAQUE;
		}
		return RENDERTYPE_TRANSPARENT;
	}
	void MaterialComponent::CreateRenderData(const std::string& content_dir)
	{
		GPUBufferDesc desc;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(MaterialCB);
		wiRenderer::GetDevice()->CreateBuffer(&desc, nullptr, &constantBuffer);

		if (!baseColorMapName.empty())
		{
			baseColorMap = wiResourceManager::Load(content_dir + baseColorMapName);
		}
		if (!surfaceMapName.empty())
		{
			surfaceMap = wiResourceManager::Load(content_dir + surfaceMapName);
		}
		if (!normalMapName.empty())
		{
			normalMap = wiResourceManager::Load(content_dir + normalMapName);
		}
		if (!displacementMapName.empty())
		{
			displacementMap = wiResourceManager::Load(content_dir + displacementMapName);
		}
		if (!emissiveMapName.empty())
		{
			emissiveMap = wiResourceManager::Load(content_dir + emissiveMapName);
		}
		if (!occlusionMapName.empty())
		{
			occlusionMap = wiResourceManager::Load(content_dir + occlusionMapName);
		}
	}

	void MeshComponent::CreateRenderData()
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		// Create index buffer GPU data:
		{
			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;

			SubresourceData initData;

			if (GetIndexFormat() == INDEXFORMAT_32BIT)
			{
				bd.StructureByteStride = sizeof(uint32_t);
				bd.Format = FORMAT_R32_UINT;
				bd.ByteWidth = uint32_t(sizeof(uint32_t) * indices.size());

				// Use indices directly since vector is in correct format
				static_assert(std::is_same<decltype(indices)::value_type, uint32_t>::value, "indices not in INDEXFORMAT_32BIT");
				initData.pSysMem = indices.data();

				device->CreateBuffer(&bd, &initData, &indexBuffer);
				device->SetName(&indexBuffer, "indexBuffer_32bit");
			}
			else
			{
				bd.StructureByteStride = sizeof(uint16_t);
				bd.Format = FORMAT_R16_UINT;
				bd.ByteWidth = uint32_t(sizeof(uint16_t) * indices.size());

				std::vector<uint16_t> gpuIndexData(indices.size());
				std::copy(indices.begin(), indices.end(), gpuIndexData.begin());
				initData.pSysMem = gpuIndexData.data();

				device->CreateBuffer(&bd, &initData, &indexBuffer);
				device->SetName(&indexBuffer, "indexBuffer_16bit");
			}

			for (MeshSubset& subset : subsets)
			{
				subset.indexBuffer_subresource = device->CreateSubresource(&indexBuffer, SRV, subset.indexOffset * GetIndexStride());
			}
		}


		XMFLOAT3 _min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
		XMFLOAT3 _max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		// vertexBuffer - POSITION + NORMAL + WIND:
		{
			std::vector<Vertex_POS> vertices(vertex_positions.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				const XMFLOAT3& pos = vertex_positions[i];
				XMFLOAT3 nor = vertex_normals.empty() ? XMFLOAT3(1, 1, 1) : vertex_normals[i];
				XMStoreFloat3(&nor, XMVector3Normalize(XMLoadFloat3(&nor)));
				const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
				vertices[i].FromFULL(pos, nor, wind);

				_min = wiMath::Min(_min, pos);
				_max = wiMath::Max(_max, pos);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			bd.ByteWidth = (uint32_t)(sizeof(Vertex_POS) * vertices.size());

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_POS);
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

			std::vector<Vertex_TAN> vertices(vertex_tangents.size());
			for (size_t i = 0; i < vertex_tangents.size(); ++i)
			{
				vertices[i].FromFULL(vertex_tangents[i]);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			bd.StructureByteStride = sizeof(Vertex_TAN);
			bd.ByteWidth = (uint32_t)(bd.StructureByteStride * vertices.size());

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_TAN);
			device->SetName(&vertexBuffer_TAN, "vertexBuffer_TAN");
		}

		aabb = AABB(_min, _max);

		// skinning buffers:
		if (!vertex_boneindices.empty())
		{
			std::vector<Vertex_BON> vertices(vertex_boneindices.size());
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
			bd.Usage = USAGE_IMMUTABLE;
			bd.BindFlags = BIND_SHADER_RESOURCE;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			bd.ByteWidth = (uint32_t)(sizeof(Vertex_BON) * vertices.size());

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_BON);

			bd.Usage = USAGE_DEFAULT;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

			bd.ByteWidth = (uint32_t)(sizeof(Vertex_POS) * vertex_positions.size());
			device->CreateBuffer(&bd, nullptr, &streamoutBuffer_POS);
			device->SetName(&streamoutBuffer_POS, "streamoutBuffer_POS");

			bd.ByteWidth = (uint32_t)(sizeof(Vertex_TAN) * vertex_tangents.size());
			device->CreateBuffer(&bd, nullptr, &streamoutBuffer_TAN);
			device->SetName(&streamoutBuffer_TAN, "streamoutBuffer_TAN");
		}

		// vertexBuffer - UV SET 0
		if(!vertex_uvset_0.empty())
		{
			std::vector<Vertex_TEX> vertices(vertex_uvset_0.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertices[i].FromFULL(vertex_uvset_0[i]);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = sizeof(Vertex_TEX);
			bd.ByteWidth = (uint32_t)(bd.StructureByteStride * vertices.size());
			bd.Format = Vertex_TEX::FORMAT;

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_UV0);
			device->SetName(&vertexBuffer_UV0, "vertexBuffer_UV0");
		}

		// vertexBuffer - UV SET 1
		if (!vertex_uvset_1.empty())
		{
			std::vector<Vertex_TEX> vertices(vertex_uvset_1.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertices[i].FromFULL(vertex_uvset_1[i]);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = sizeof(Vertex_TEX);
			bd.ByteWidth = (uint32_t)(bd.StructureByteStride * vertices.size());
			bd.Format = Vertex_TEX::FORMAT;

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_UV1);
			device->SetName(&vertexBuffer_UV1, "vertexBuffer_UV1");
		}

		// vertexBuffer - COLORS
		if (!vertex_colors.empty())
		{
			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = sizeof(Vertex_COL);
			bd.ByteWidth = (uint32_t)(bd.StructureByteStride * vertex_colors.size());
			bd.Format = FORMAT_R8G8B8A8_UNORM;

			SubresourceData InitData;
			InitData.pSysMem = vertex_colors.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_COL);
			device->SetName(&vertexBuffer_COL, "vertexBuffer_COL");
		}

		// vertexBuffer - ATLAS
		if (!vertex_atlas.empty())
		{
			std::vector<Vertex_TEX> vertices(vertex_atlas.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertices[i].FromFULL(vertex_atlas[i]);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = sizeof(Vertex_TEX);
			bd.ByteWidth = (uint32_t)(bd.StructureByteStride * vertices.size());
			bd.Format = Vertex_TEX::FORMAT;

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_ATL);
			device->SetName(&vertexBuffer_ATL, "vertexBuffer_ATL");
		}

		// vertexBuffer - SUBSETS
		{
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

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = sizeof(uint8_t);
			bd.ByteWidth = (uint32_t)(bd.StructureByteStride * vertex_subsets.size());
			bd.Format = FORMAT_R8_UINT;

			SubresourceData InitData;
			InitData.pSysMem = vertex_subsets.data();
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_SUB);
			device->SetName(&vertexBuffer_SUB, "vertexBuffer_SUB");
		}

		// vertexBuffer_PRE will be created on demand later!
		vertexBuffer_PRE = GPUBuffer();


		if (wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		{
			BLAS_build_pending = true;

			RaytracingAccelerationStructureDesc desc;
			desc.type = RaytracingAccelerationStructureDesc::BOTTOMLEVEL;

			if (streamoutBuffer_POS.IsValid())
			{
				desc._flags |= RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE;
				desc._flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;
			}
			else
			{
				desc._flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE;
			}

#if 0
			// Flattened subsets:
			desc.bottomlevel.geometries.emplace_back();
			auto& geometry = desc.bottomlevel.geometries.back();
			geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES;
			geometry.triangles.vertexBuffer = streamoutBuffer_POS.IsValid() ? streamoutBuffer_POS : vertexBuffer_POS;
			geometry.triangles.indexBuffer = indexBuffer;
			geometry.triangles.indexFormat = GetIndexFormat();
			geometry.triangles.indexCount = (uint32_t)indices.size();
			geometry.triangles.indexOffset = 0;
			geometry.triangles.vertexCount = (uint32_t)vertex_positions.size();
			geometry.triangles.vertexFormat = FORMAT_R32G32B32_FLOAT;
			geometry.triangles.vertexStride = sizeof(MeshComponent::Vertex_POS);
#else
			// One geometry per subset:
			for (auto& subset : subsets)
			{
				desc.bottomlevel.geometries.emplace_back();
				auto& geometry = desc.bottomlevel.geometries.back();
				geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES;
				geometry.triangles.vertexBuffer = streamoutBuffer_POS.IsValid() ? streamoutBuffer_POS : vertexBuffer_POS;
				geometry.triangles.indexBuffer = indexBuffer;
				geometry.triangles.indexFormat = GetIndexFormat();
				geometry.triangles.indexCount = subset.indexCount;
				geometry.triangles.indexOffset = subset.indexOffset;
				geometry.triangles.vertexCount = (uint32_t)vertex_positions.size();
				geometry.triangles.vertexFormat = FORMAT_R32G32B32_FLOAT;
				geometry.triangles.vertexStride = sizeof(MeshComponent::Vertex_POS);
			}
#endif

			bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLAS);
			assert(success);
			device->SetName(&BLAS, "BLAS");
		}
	}
	void MeshComponent::ComputeNormals(COMPUTE_NORMALS compute)
	{
		// Start recalculating normals:

		switch (compute)
		{
		case wiScene::MeshComponent::COMPUTE_NORMALS_HARD: 
		{
			// Compute hard surface normals:

			std::vector<uint32_t> newIndexBuffer;
			std::vector<XMFLOAT3> newPositionsBuffer;
			std::vector<XMFLOAT3> newNormalsBuffer;
			std::vector<XMFLOAT2> newUV0Buffer;
			std::vector<XMFLOAT2> newUV1Buffer;
			std::vector<XMFLOAT2> newAtlasBuffer;
			std::vector<XMUINT4> newBoneIndicesBuffer;
			std::vector<XMFLOAT4> newBoneWeightsBuffer;
			std::vector<uint32_t> newColorsBuffer;

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
		break;

		case wiScene::MeshComponent::COMPUTE_NORMALS_SMOOTH:
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

		case wiScene::MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST:
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
	SPHERE MeshComponent::GetBoundingSphere() const
	{
		XMFLOAT3 halfwidth = aabb.getHalfWidth();

		SPHERE sphere;
		sphere.center = aabb.getCenter();
		sphere.radius = std::max(halfwidth.x, std::max(halfwidth.y, halfwidth.z));

		return sphere;
	}

	void ObjectComponent::ClearLightmap()
	{
		lightmapWidth = 0;
		lightmapHeight = 0;
		globalLightMapMulAdd = XMFLOAT4(0, 0, 0, 0);
		lightmapIterationCount = 0; 
		lightmapTextureData.clear();
		SetLightmapRenderRequest(false);
	}
	void ObjectComponent::SaveLightmap()
	{
		if (lightmap.IsValid())
		{
			bool success = wiHelper::saveTextureToMemory(lightmap, lightmapTextureData);
			assert(success);
		}
	}
	FORMAT ObjectComponent::GetLightmapFormat()
	{
		uint32_t stride = (uint32_t)lightmapTextureData.size() / lightmapWidth / lightmapHeight;

		switch (stride)
		{
		case 4: return FORMAT_R8G8B8A8_UNORM;
		case 8: return FORMAT_R16G16B16A16_FLOAT;
		case 16: return FORMAT_R32G32B32A32_FLOAT;
		}

		return FORMAT_UNKNOWN;
	}

	void SoftBodyPhysicsComponent::CreateFromMesh(const MeshComponent& mesh)
	{
		vertex_positions_simulation.resize(mesh.vertex_positions.size());
		vertex_tangents_tmp.resize(mesh.vertex_tangents.size());
		vertex_tangents_simulation.resize(mesh.vertex_tangents.size());

		XMMATRIX W = XMLoadFloat4x4(&worldMatrix);
		XMFLOAT3 _min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
		XMFLOAT3 _max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (size_t i = 0; i < vertex_positions_simulation.size(); ++i)
		{
			XMFLOAT3 pos = mesh.vertex_positions[i];
			XMStoreFloat3(&pos, XMVector3Transform(XMLoadFloat3(&pos), W));
			XMFLOAT3 nor = mesh.vertex_normals.empty() ? XMFLOAT3(1, 1, 1) : mesh.vertex_normals[i];
			XMStoreFloat3(&nor, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&nor), W)));
			const uint8_t wind = mesh.vertex_windweights.empty() ? 0xFF : mesh.vertex_windweights[i];
			vertex_positions_simulation[i].FromFULL(pos, nor, wind);
			_min = wiMath::Min(_min, pos);
			_max = wiMath::Max(_max, pos);
		}
		aabb = AABB(_min, _max);

		if(physicsToGraphicsVertexMapping.empty())
		{
			// Create a mapping that maps unique vertex positions to all vertex indices that share that. Unique vertex positions will make up the physics mesh:
			std::unordered_map<size_t, uint32_t> uniquePositions;
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
		GraphicsDevice* device = wiRenderer::GetDevice();
		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_DESCRIPTOR_MANAGEMENT) && !descriptorTable.IsValid())
		{
			descriptorTable.resources.resize(DESCRIPTORTABLE_ENTRY_COUNT);
			descriptorTable.resources[DESCRIPTORTABLE_ENTRY_SUBSETS_MATERIAL] = { CONSTANTBUFFER, 0, MAX_DESCRIPTOR_INDEXING };
			descriptorTable.resources[DESCRIPTORTABLE_ENTRY_SUBSETS_TEXTURE_BASECOLOR] = { TEXTURE2D, 0, MAX_DESCRIPTOR_INDEXING };
			descriptorTable.resources[DESCRIPTORTABLE_ENTRY_SUBSETS_INDEXBUFFER] = { TYPEDBUFFER, MAX_DESCRIPTOR_INDEXING, MAX_DESCRIPTOR_INDEXING };
			descriptorTable.resources[DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_POSITION_NORMAL_WIND] = { RAWBUFFER, MAX_DESCRIPTOR_INDEXING * 2, MAX_DESCRIPTOR_INDEXING };
			descriptorTable.resources[DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_UV0] = { TYPEDBUFFER, MAX_DESCRIPTOR_INDEXING * 3, MAX_DESCRIPTOR_INDEXING };
			descriptorTable.resources[DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_UV1] = { TYPEDBUFFER, MAX_DESCRIPTOR_INDEXING * 4, MAX_DESCRIPTOR_INDEXING };

			bool success = device->CreateDescriptorTable(&descriptorTable);
			assert(success);
		}

		wiJobSystem::context ctx;

		RunPreviousFrameTransformUpdateSystem(ctx);

		RunAnimationUpdateSystem(ctx, dt);

		RunTransformUpdateSystem(ctx);

		wiJobSystem::Wait(ctx); // dependencies

		RunHierarchyUpdateSystem(ctx);

		RunSpringUpdateSystem(ctx, dt);

		RunInverseKinematicsUpdateSystem(ctx);

		RunArmatureUpdateSystem(ctx);

		RunMeshUpdateSystem(ctx);

		RunMaterialUpdateSystem(ctx, dt);

		RunImpostorUpdateSystem(ctx);

		RunWeatherUpdateSystem(ctx);

		wiPhysicsEngine::RunPhysicsUpdateSystem(ctx, *this, dt); // this syncs dependencies internally

		RunObjectUpdateSystem(ctx);

		RunCameraUpdateSystem(ctx);

		RunDecalUpdateSystem(ctx);

		RunProbeUpdateSystem(ctx);

		RunForceUpdateSystem(ctx);

		RunLightUpdateSystem(ctx);

		RunParticleUpdateSystem(ctx, dt);

		RunSoundUpdateSystem(ctx);

		wiJobSystem::Wait(ctx); // dependencies

		// Merge parallel bounds computation (depends on object update system):
		bounds = AABB();
		for (auto& group_bound : parallel_bounds)
		{
			bounds = AABB::Merge(bounds, group_bound);
		}

		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		{
			// Recreate top level acceleration structure if the object count changed:
			if (dt > 0 && objects.GetCount() > 0 && objects.GetCount() != TLAS.desc.toplevel.count)
			{
				RaytracingAccelerationStructureDesc desc;
				desc._flags = RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;
				desc.type = RaytracingAccelerationStructureDesc::TOPLEVEL;
				desc.toplevel.count = (uint32_t)objects.GetCount();
				GPUBufferDesc bufdesc;
				bufdesc.MiscFlags |= RESOURCE_MISC_RAY_TRACING;
				bufdesc.ByteWidth = desc.toplevel.count * (uint32_t)device->GetTopLevelAccelerationStructureInstanceSize();
				bool success = device->CreateBuffer(&bufdesc, nullptr, &desc.toplevel.instanceBuffer);
				assert(success);
				device->SetName(&desc.toplevel.instanceBuffer, "TLAS.instanceBuffer");
				success = device->CreateRaytracingAccelerationStructure(&desc, &TLAS);
				assert(success);
				device->SetName(&TLAS, "TLAS");
			}
		}

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
		wiArchive archive;

		// First write the entity to staging area:
		archive.SetReadModeAndResetPos(false);
		Entity_Serialize(archive, entity);

		// Then deserialize:
		archive.SetReadModeAndResetPos(true);
		return Entity_Serialize(archive, entity);
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
		float range)
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
		light.SetType(LightComponent::POINT);

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

		if (!textureName.empty())
		{
			material.baseColorMapName = textureName;
			material.baseColorMap = wiResourceManager::Load(material.baseColorMapName);
		}
		if (!normalMapName.empty())
		{
			material.normalMapName = normalMapName;
			material.normalMap = wiResourceManager::Load(material.normalMapName);
		}

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
		sound.soundResource = wiResourceManager::Load(filename);
		wiAudio::CreateSoundInstance(sound.soundResource->sound, &sound.soundinstance);

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

		// Add a new hierarchy node to the end of container:
		hierarchy.Create(entity).parentID = parent;

		// Detect breaks in the tree and fix them:
		//	when children are before parents, we move the parents before the children while keeping ordering of other components intact
		if (hierarchy.GetCount() > 1)
		{
			for (size_t i = hierarchy.GetCount() - 1; i > 0; --i)
			{
				Entity parent_candidate_entity = hierarchy.GetEntity(i);
				for (size_t j = 0; j < i; ++j)
				{
					const HierarchyComponent& child_candidate = hierarchy[j];

					if (child_candidate.parentID == parent_candidate_entity)
					{
						hierarchy.MoveItem(i, j);
						++i; // next outer iteration will check the same index again as parent candidate, however things were moved upwards, so it will be a different entity!
						break;
					}
				}
			}
		}

		// Re-query parent after potential MoveItem(), because it invalidates references:
		HierarchyComponent& parentcomponent = *hierarchy.GetComponent(entity);

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
		// Save the initial layermask of the child so that it can be restored if detached:
		parentcomponent.layerMask_bind = layer_child->GetLayerMask();
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
				layer->layerMask = parent->layerMask_bind;
			}

			hierarchy.Remove_KeepSorted(entity);
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

	void Scene::RunPreviousFrameTransformUpdateSystem(wiJobSystem::context& ctx)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)prev_transforms.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			PreviousFrameTransformComponent& prev_transform = prev_transforms[args.jobIndex];
			Entity entity = prev_transforms.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			prev_transform.world_prev = transform.world;
		});
	}
	void Scene::RunAnimationUpdateSystem(wiJobSystem::context& ctx, float dt)
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

				TransformComponent& target_transform = *transforms.GetComponent(channel.target);
				TransformComponent transform = target_transform;

				if (sampler.mode == AnimationComponent::AnimationSampler::Mode::STEP || keyLeft == keyRight)
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
					}
				}
				else
				{
					// Linear interpolation method:
					float right = animationdata->keyframe_times[keyRight];
					float t = (animation.timer - left) / (right - left);

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
					}
				}

				target_transform.SetDirty();

				const float t = animation.amount;

				const XMVECTOR aS = XMLoadFloat3(&target_transform.scale_local);
				const XMVECTOR aR = XMLoadFloat4(&target_transform.rotation_local);
				const XMVECTOR aT = XMLoadFloat3(&target_transform.translation_local);

				const XMVECTOR bS = XMLoadFloat3(&transform.scale_local);
				const XMVECTOR bR = XMLoadFloat4(&transform.rotation_local);
				const XMVECTOR bT = XMLoadFloat3(&transform.translation_local);

				const XMVECTOR S = XMVectorLerp(aS, bS, t);
				const XMVECTOR R = XMQuaternionSlerp(aR, bR, t);
				const XMVECTOR T = XMVectorLerp(aT, bT, t);

				XMStoreFloat3(&target_transform.scale_local, S);
				XMStoreFloat4(&target_transform.rotation_local, R);
				XMStoreFloat3(&target_transform.translation_local, T);

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
	void Scene::RunTransformUpdateSystem(wiJobSystem::context& ctx)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)transforms.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			TransformComponent& transform = transforms[args.jobIndex];
			transform.UpdateTransform();
		});
	}
	void Scene::RunHierarchyUpdateSystem(wiJobSystem::context& ctx)
	{
		// This needs serialized execution because there are dependencies enforced by component order!

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


			LayerComponent* layer_child = layers.GetComponent(entity);
			LayerComponent* layer_parent = layers.GetComponent(parentcomponent.parentID);
			if (layer_child != nullptr && layer_parent != nullptr)
			{
				layer_child->layerMask = parentcomponent.layerMask_bind & layer_parent->GetLayerMask();
			}

		}
	}
	void Scene::RunSpringUpdateSystem(wiJobSystem::context& ctx, float dt)
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
	void Scene::RunInverseKinematicsUpdateSystem(wiJobSystem::context& ctx)
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
	void Scene::RunArmatureUpdateSystem(wiJobSystem::context& ctx)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)armatures.GetCount(), 1, [&](wiJobArgs args) {

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

			XMFLOAT3 _min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
			XMFLOAT3 _max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			int boneIndex = 0;
			for (Entity boneEntity : armature.boneCollection)
			{
				const TransformComponent& bone = *transforms.GetComponent(boneEntity);

				XMMATRIX B = XMLoadFloat4x4(&armature.inverseBindMatrices[boneIndex]);
				XMMATRIX W = XMLoadFloat4x4(&bone.world);
				XMMATRIX M = B * W * R;

				armature.boneData[boneIndex++].Store(M);

				const float bone_radius = 1;
				XMFLOAT3 bonepos = bone.GetPosition();
				AABB boneAABB;
				boneAABB.createFromHalfWidth(bonepos, XMFLOAT3(bone_radius, bone_radius, bone_radius));
				_min = wiMath::Min(_min, boneAABB._min);
				_max = wiMath::Max(_max, boneAABB._max);
			}

			armature.aabb = AABB(_min, _max);

		});
	}
	void Scene::RunMeshUpdateSystem(wiJobSystem::context& ctx)
	{
		geometryOffset.store(0);

		wiJobSystem::Dispatch(ctx, (uint32_t)meshes.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			MeshComponent& mesh = meshes[args.jobIndex];
			mesh.TLAS_geometryOffset = geometryOffset.fetch_add((uint32_t)mesh.subsets.size());

			GraphicsDevice* device = wiRenderer::GetDevice();
			uint32_t subsetIndex = 0;
			for (auto& subset : mesh.subsets)
			{
				const MaterialComponent* material = materials.GetComponent(subset.materialID);
				if (material != nullptr)
				{
					if (descriptorTable.IsValid())
					{
						uint32_t global_geometryIndex = mesh.TLAS_geometryOffset + subsetIndex;
						device->WriteDescriptor(
							&descriptorTable,
							DESCRIPTORTABLE_ENTRY_SUBSETS_MATERIAL,
							global_geometryIndex,
							&material->constantBuffer
						);
						device->WriteDescriptor(
							&descriptorTable,
							DESCRIPTORTABLE_ENTRY_SUBSETS_TEXTURE_BASECOLOR,
							global_geometryIndex,
							material->baseColorMap ? material->baseColorMap->texture : nullptr
						);
						device->WriteDescriptor(
							&descriptorTable,
							DESCRIPTORTABLE_ENTRY_SUBSETS_INDEXBUFFER,
							global_geometryIndex,
							&mesh.indexBuffer,
							subset.indexBuffer_subresource
						);
						device->WriteDescriptor(
							&descriptorTable,
							DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_POSITION_NORMAL_WIND,
							global_geometryIndex,
							&mesh.vertexBuffer_POS
						);
						device->WriteDescriptor(
							&descriptorTable,
							DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_UV0,
							global_geometryIndex,
							&mesh.vertexBuffer_UV0
						);
						device->WriteDescriptor(
							&descriptorTable,
							DESCRIPTORTABLE_ENTRY_SUBSETS_VERTEXBUFFER_UV1,
							global_geometryIndex,
							&mesh.vertexBuffer_UV1
						);
					}

					if (mesh.BLAS.IsValid())
					{
						uint32_t flags = mesh.BLAS.desc.bottomlevel.geometries[subsetIndex]._flags;
						if (material->IsAlphaTestEnabled() || (material->GetRenderTypes() & RENDERTYPE_TRANSPARENT))
						{
							mesh.BLAS.desc.bottomlevel.geometries[subsetIndex]._flags &=
								~RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE;
						}
						else
						{
							mesh.BLAS.desc.bottomlevel.geometries[subsetIndex]._flags =
								RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE;
						}
						if (flags != mesh.BLAS.desc.bottomlevel.geometries[subsetIndex]._flags)
						{
							// New flags invalidate BLAS
							mesh.BLAS_build_pending = true;
						}
					}
				}
				subsetIndex++;
			}

		});
	}
	void Scene::RunMaterialUpdateSystem(wiJobSystem::context& ctx, float dt)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)materials.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			MaterialComponent& material = materials[args.jobIndex];

			if (!material.constantBuffer.IsValid())
			{
				material.CreateRenderData();
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

			if (material.subsurfaceProfile == MaterialComponent::SUBSURFACE_SKIN)
			{
				material.engineStencilRef = STENCILREF_SKIN;
			}
			else if (material.subsurfaceProfile == MaterialComponent::SUBSURFACE_SNOW)
			{
				material.engineStencilRef = STENCILREF_SNOW;
			}

		});
	}
	void Scene::RunImpostorUpdateSystem(wiJobSystem::context& ctx)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)impostors.GetCount(), 1, [&](wiJobArgs args) {

			ImpostorComponent& impostor = impostors[args.jobIndex];
			impostor.aabb = AABB();
			impostor.instanceMatrices.clear();
		});
	}
	void Scene::RunObjectUpdateSystem(wiJobSystem::context& ctx)
	{
		assert(objects.GetCount() == aabb_objects.GetCount());

		parallel_bounds.clear();
		parallel_bounds.resize((size_t)wiJobSystem::DispatchGroupCount((uint32_t)objects.GetCount(), small_subtask_groupsize));
		
		wiJobSystem::Dispatch(ctx, (uint32_t)objects.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			ObjectComponent& object = objects[args.jobIndex];
			AABB& aabb = aabb_objects[args.jobIndex];

			aabb = AABB();
			object.rendertypeMask = 0;
			object.SetDynamic(false);
			object.SetCastShadow(false);
			object.SetImpostorPlacement(false);
			object.SetRequestPlanarReflection(false);

			if (object.meshID != INVALID_ENTITY)
			{
				Entity entity = objects.GetEntity(args.jobIndex);
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

							object.SetCastShadow(material->IsCastingShadow());
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

						const SPHERE boundingsphere = mesh->GetBoundingSphere();

						locker.lock();
						impostor->instanceMatrices.emplace_back();
						XMStoreFloat4x4(&impostor->instanceMatrices.back(),
							XMMatrixScaling(boundingsphere.radius, boundingsphere.radius, boundingsphere.radius) *
							XMMatrixTranslation(boundingsphere.center.x, boundingsphere.center.y, boundingsphere.center.z) *
							W
						);
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
				}

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
	void Scene::RunCameraUpdateSystem(wiJobSystem::context& ctx)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)cameras.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

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
	void Scene::RunDecalUpdateSystem(wiJobSystem::context& ctx)
	{
		assert(decals.GetCount() == aabb_decals.GetCount());

		wiJobSystem::Dispatch(ctx, (uint32_t)decals.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			DecalComponent& decal = decals[args.jobIndex];
			Entity entity = decals.GetEntity(args.jobIndex);
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

			AABB& aabb = aabb_decals[args.jobIndex];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.transform(transform.world);

			const MaterialComponent& material = *materials.GetComponent(entity);
			decal.color = material.baseColor;
			decal.emissive = material.GetEmissiveStrength();
			decal.texture = material.baseColorMap;
			decal.normal = material.normalMap;
		});
	}
	void Scene::RunProbeUpdateSystem(wiJobSystem::context& ctx)
	{
		assert(probes.GetCount() == aabb_probes.GetCount());

		wiJobSystem::Dispatch(ctx, (uint32_t)probes.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			EnvironmentProbeComponent& probe = probes[args.jobIndex];
			Entity entity = probes.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			probe.position = transform.GetPosition();

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMStoreFloat4x4(&probe.inverseMatrix, XMMatrixInverse(nullptr, W));

			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);
			XMFLOAT3 scale;
			XMStoreFloat3(&scale, S);
			probe.range = std::max(scale.x, std::max(scale.y, scale.z)) * 2;

			AABB& aabb = aabb_probes[args.jobIndex];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.transform(transform.world);
		});
	}
	void Scene::RunForceUpdateSystem(wiJobSystem::context& ctx)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)forces.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

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
	void Scene::RunLightUpdateSystem(wiJobSystem::context& ctx)
	{
		assert(lights.GetCount() == aabb_lights.GetCount());

		wiJobSystem::Dispatch(ctx, (uint32_t)lights.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			LightComponent& light = lights[args.jobIndex];
			Entity entity = lights.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);
			AABB& aabb = aabb_lights[args.jobIndex];

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
				aabb.createFromHalfWidth(wiRenderer::GetCamera().Eye, XMFLOAT3(10000, 10000, 10000));
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
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
				XMStoreFloat3(&light.right, XMVector3TransformNormal(XMVectorSet(-1, 0, 0, 0), W));
				XMStoreFloat3(&light.front, XMVector3TransformNormal(XMVectorSet(0, 0, -1, 0), W));
				// area lights have no bounds, just like directional lights (todo: but they should have real bounds)
				aabb.createFromHalfWidth(wiRenderer::GetCamera().Eye, XMFLOAT3(10000, 10000, 10000));
				break;
			}

		});
	}
	void Scene::RunParticleUpdateSystem(wiJobSystem::context& ctx, float dt)
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)emitters.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			wiEmittedParticle& emitter = emitters[args.jobIndex];
			Entity entity = emitters.GetEntity(args.jobIndex);
			const TransformComponent& transform = *transforms.GetComponent(entity);
			emitter.UpdateCPU(transform, dt);
		});

		wiJobSystem::Dispatch(ctx, (uint32_t)hairs.GetCount(), small_subtask_groupsize, [&](wiJobArgs args) {

			wiHairParticle& hair = hairs[args.jobIndex];

			if (hair.meshID != INVALID_ENTITY)
			{
				Entity entity = hairs.GetEntity(args.jobIndex);
				const MeshComponent* mesh = meshes.GetComponent(hair.meshID);

				if (mesh != nullptr)
				{
					const TransformComponent& transform = *transforms.GetComponent(entity);

					hair.UpdateCPU(transform, *mesh, dt);
				}
			}

		});
	}
	void Scene::RunWeatherUpdateSystem(wiJobSystem::context& ctx)
	{
		if (weathers.GetCount() > 0)
		{
			weather = weathers[0];
			weather.most_important_light_index = ~0;
		}
	}
	void Scene::RunSoundUpdateSystem(wiJobSystem::context& ctx)
	{
		const CameraComponent& camera = wiRenderer::GetCamera();
		wiAudio::SoundInstance3D instance3D;
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
					wiAudio::Update3D(&sound.soundinstance, instance3D);
				}
			}
			if (sound.IsPlaying())
			{
				wiAudio::Play(&sound.soundinstance);
			}
			else
			{
				wiAudio::Stop(&sound.soundinstance);
			}
			if (!sound.IsLooped())
			{
				wiAudio::ExitLoop(&sound.soundinstance);
			}
			wiAudio::SetVolume(sound.volume, &sound.soundinstance);
		}
	}


	XMVECTOR SkinVertex(const MeshComponent& mesh, const ArmatureComponent& armature, uint32_t index, XMVECTOR* N)
	{
		XMVECTOR P = XMLoadFloat3(&mesh.vertex_positions[index]);
		const XMUINT4& ind = mesh.vertex_boneindices[index];
		const XMFLOAT4& wei = mesh.vertex_boneweights[index];

		const XMMATRIX M[] = {
			armature.boneData[ind.x].Load(),
			armature.boneData[ind.y].Load(),
			armature.boneData[ind.z].Load(),
			armature.boneData[ind.w].Load(),
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
		wiArchive archive(fileName, true);
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
				scene.transforms.GetComponent(root)->MatrixTransform(transformMatrix);
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

	PickResult Pick(const RAY& ray, uint32_t renderTypeMask, uint32_t layerMask, const Scene& scene)
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

						float distance;
						XMFLOAT2 bary;
						if (wiMath::RayTriangleIntersects(rayOrigin_local, rayDirection_local, p0, p1, p2, distance, bary))
						{
							const XMVECTOR pos = XMVector3Transform(XMVectorAdd(rayOrigin_local, rayDirection_local*distance), objectMat);
							distance = wiMath::Distance(pos, rayOrigin);

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

	SceneIntersectSphereResult SceneIntersectSphere(const SPHERE& sphere, uint32_t renderTypeMask, uint32_t layerMask, const Scene& scene)
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
	SceneIntersectSphereResult SceneIntersectCapsule(const CAPSULE& capsule, uint32_t renderTypeMask, uint32_t layerMask, const Scene& scene)
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
								XMVECTOR Point1 = wiMath::ClosestPointOnLineSegment(p0, p1, LinePlaneIntersection);

								// Edge 1,2
								XMVECTOR Point2 = wiMath::ClosestPointOnLineSegment(p1, p2, LinePlaneIntersection);

								// Edge 2,0
								XMVECTOR Point3 = wiMath::ClosestPointOnLineSegment(p2, p0, LinePlaneIntersection);

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
						XMVECTOR Center = wiMath::ClosestPointOnLineSegment(A, B, ReferencePoint);

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
						XMVECTOR Point1 = wiMath::ClosestPointOnLineSegment(p0, p1, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point1)), RadiusSq));

						// Edge 1,2
						XMVECTOR Point2 = wiMath::ClosestPointOnLineSegment(p1, p2, Center);

						// If the distance to the center of the sphere to the point is less than 
						// the radius of the sphere then it must intersect.
						Intersection = XMVectorOrInt(Intersection, XMVectorLessOrEqual(XMVector3LengthSq(XMVectorSubtract(Center, Point2)), RadiusSq));

						// Edge 2,0
						XMVECTOR Point3 = wiMath::ClosestPointOnLineSegment(p2, p0, Center);

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
