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
#include "wiLua.h"

#include "shaders/ShaderInterop_SurfelGI.h"
#include "shaders/ShaderInterop_DDGI.h"

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
		ShaderMaterial material;
		material.baseColor = baseColor;
		material.emissive_r11g11b10 = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(emissiveColor.x * emissiveColor.w, emissiveColor.y * emissiveColor.w, emissiveColor.z * emissiveColor.w));
		material.specular_r11g11b10 = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(specularColor.x * specularColor.w, specularColor.y * specularColor.w, specularColor.z * specularColor.w));
		material.texMulAdd = texMulAdd;
		material.roughness = roughness;
		material.reflectance = reflectance;
		material.metalness = metalness;
		material.refraction = refraction;
		material.normalMapStrength = (textures[NORMALMAP].resource.IsValid() ? normalMapStrength : 0);
		material.parallaxOcclusionMapping = parallaxOcclusionMapping;
		material.displacementMapping = displacementMapping;
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
		material.subsurfaceScattering = sss;
		material.subsurfaceScattering_inv = sss_inv;
		material.uvset_baseColorMap = textures[BASECOLORMAP].GetUVSet();
		material.uvset_surfaceMap = textures[SURFACEMAP].GetUVSet();
		material.uvset_normalMap = textures[NORMALMAP].GetUVSet();
		material.uvset_displacementMap = textures[DISPLACEMENTMAP].GetUVSet();
		material.uvset_emissiveMap = textures[EMISSIVEMAP].GetUVSet();
		material.uvset_occlusionMap = textures[OCCLUSIONMAP].GetUVSet();
		material.uvset_transmissionMap = textures[TRANSMISSIONMAP].GetUVSet();
		material.uvset_sheenColorMap = textures[SHEENCOLORMAP].GetUVSet();
		material.uvset_sheenRoughnessMap = textures[SHEENROUGHNESSMAP].GetUVSet();
		material.uvset_clearcoatMap = textures[CLEARCOATMAP].GetUVSet();
		material.uvset_clearcoatRoughnessMap = textures[CLEARCOATROUGHNESSMAP].GetUVSet();
		material.uvset_clearcoatNormalMap = textures[CLEARCOATNORMALMAP].GetUVSet();
		material.uvset_specularMap = textures[SPECULARMAP].GetUVSet();
		material.sheenColor_r11g11b10 = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(sheenColor.x, sheenColor.y, sheenColor.z));
		material.sheenRoughness = sheenRoughness;
		material.clearcoat = clearcoat;
		material.clearcoatRoughness = clearcoatRoughness;
		material.alphaTest = 1 - alphaRef;
		material.layerMask = layerMask;
		material.transmission = transmission;
		material.shaderType = (uint)shaderType;
		material.userdata = userdata;

		material.options = 0;
		if (IsUsingVertexColors())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS;
		}
		if (IsUsingSpecularGlossinessWorkflow())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW;
		}
		if (IsOcclusionEnabled_Primary())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY;
		}
		if (IsOcclusionEnabled_Secondary())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY;
		}
		if (IsUsingWind())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_USE_WIND;
		}
		if (IsReceiveShadow())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW;
		}
		if (IsCastingShadow())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_CAST_SHADOW;
		}
		if (IsDoubleSided())
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_DOUBLE_SIDED;
		}
		if (GetRenderTypes() & RENDERTYPE_TRANSPARENT)
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_TRANSPARENT;
		}
		if (userBlendMode == BLENDMODE_ADDITIVE)
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_ADDITIVE;
		}
		if (shaderType == SHADERTYPE_UNLIT)
		{
			material.options |= SHADERMATERIAL_OPTION_BIT_UNLIT;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		material.texture_basecolormap_index = device->GetDescriptorIndex(textures[BASECOLORMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_surfacemap_index = device->GetDescriptorIndex(textures[SURFACEMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_emissivemap_index = device->GetDescriptorIndex(textures[EMISSIVEMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_normalmap_index = device->GetDescriptorIndex(textures[NORMALMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_displacementmap_index = device->GetDescriptorIndex(textures[DISPLACEMENTMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_occlusionmap_index = device->GetDescriptorIndex(textures[OCCLUSIONMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_transmissionmap_index = device->GetDescriptorIndex(textures[TRANSMISSIONMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_sheencolormap_index = device->GetDescriptorIndex(textures[SHEENCOLORMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_sheenroughnessmap_index = device->GetDescriptorIndex(textures[SHEENROUGHNESSMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_clearcoatmap_index = device->GetDescriptorIndex(textures[CLEARCOATMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_clearcoatroughnessmap_index = device->GetDescriptorIndex(textures[CLEARCOATROUGHNESSMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_clearcoatnormalmap_index = device->GetDescriptorIndex(textures[CLEARCOATNORMALMAP].GetGPUResource(), SubresourceType::SRV);
		material.texture_specularmap_index = device->GetDescriptorIndex(textures[SPECULARMAP].GetGPUResource(), SubresourceType::SRV);

		std::memcpy(dest, &material, sizeof(ShaderMaterial)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
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

		generalBuffer = {};
		streamoutBuffer = {};
		ib = {};
		vb_pos_nor_wind = {};
		vb_tan = {};
		vb_uvs = {};
		vb_atl = {};
		vb_col = {};
		vb_bon = {};
		so_pos_nor_wind = {};
		so_tan = {};
		so_pre = {};

		if (vertex_tangents.empty() && !vertex_uvset_0.empty() && !vertex_normals.empty())
		{
			// Generate tangents if not found:
			vertex_tangents.resize(vertex_positions.size());

			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = subsets[subsetIndex];
				for (size_t i = 0; i < subset.indexCount; i += 3)
				{
					const uint32_t i0 = indices[subset.indexOffset + i + 0];
					const uint32_t i1 = indices[subset.indexOffset + i + 1];
					const uint32_t i2 = indices[subset.indexOffset + i + 2];

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
		}

		{
			vertex_subsets.resize(vertex_positions.size());
			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = subsets[subsetIndex];
				for (uint32_t i = 0; i < subset.indexCount; ++i)
				{
					uint32_t index = indices[subset.indexOffset + i];
					vertex_subsets[index] = subsetIndex;
				}
			}
		}

		const size_t uv_count = std::max(vertex_uvset_0.size(), vertex_uvset_1.size());

		GPUBufferDesc bd;
		bd.usage = Usage::DEFAULT;
		bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE;
		bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
		}
		const uint64_t alignment = device->GetMinOffsetAlignment(&bd);
		bd.size =
			AlignTo(indices.size() * GetIndexStride(), alignment) +
			AlignTo(vertex_positions.size() * sizeof(Vertex_POS), alignment) +
			AlignTo(vertex_tangents.size() * sizeof(Vertex_TAN), alignment) +
			AlignTo(uv_count * sizeof(Vertex_UVS), alignment) +
			AlignTo(vertex_atlas.size() * sizeof(Vertex_TEX), alignment) +
			AlignTo(vertex_colors.size() * sizeof(Vertex_COL), alignment) +
			AlignTo(vertex_boneindices.size() * sizeof(Vertex_BON), alignment)
			;

		// single allocation storage for GPU buffer data:
		wi::vector<uint8_t> buffer_data(bd.size);
		uint64_t buffer_offset = 0ull;

		// Create index buffer GPU data:
		if (GetIndexFormat() == IndexBufferFormat::UINT32)
		{
			ib.offset = buffer_offset;
			ib.size = indices.size() * sizeof(uint32_t);
			uint32_t* indexdata = (uint32_t*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(ib.size, alignment);
			for (size_t i = 0; i < indices.size(); ++i)
			{
				indexdata[i] = indices[i];
			}
		}
		else
		{
			ib.offset = buffer_offset;
			ib.size = indices.size() * sizeof(uint16_t);
			uint16_t* indexdata = (uint16_t*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(ib.size, alignment);
			for (size_t i = 0; i < indices.size(); ++i)
			{
				indexdata[i] = (uint16_t)indices[i];
			}
		}

		XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

		// vertexBuffer - POSITION + NORMAL + WIND:
		{
			if (!morph_targets.empty())
			{
				vertex_positions_morphed.resize(vertex_positions.size());
				dirty_morph = true;
			}

			vb_pos_nor_wind.offset = buffer_offset;
			vb_pos_nor_wind.size = vertex_positions.size() * sizeof(Vertex_POS);
			Vertex_POS* vertices = (Vertex_POS*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(vb_pos_nor_wind.size, alignment);
			for (size_t i = 0; i < vertex_positions.size(); ++i)
			{
				const XMFLOAT3& pos = vertex_positions[i];
				XMFLOAT3 nor = vertex_normals.empty() ? XMFLOAT3(1, 1, 1) : vertex_normals[i];
				XMStoreFloat3(&nor, XMVector3Normalize(XMLoadFloat3(&nor)));
				const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
				vertices[i].FromFULL(pos, nor, wind);

				_min = wi::math::Min(_min, pos);
				_max = wi::math::Max(_max, pos);
			}
		}

		aabb = AABB(_min, _max);

		// vertexBuffer - TANGENTS
		if (!vertex_tangents.empty())
		{
			vb_tan.offset = buffer_offset;
			vb_tan.size = vertex_tangents.size() * sizeof(Vertex_TAN);
			Vertex_TAN* vertices = (Vertex_TAN*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(vb_tan.size, alignment);
			for (size_t i = 0; i < vertex_tangents.size(); ++i)
			{
				vertices[i].FromFULL(vertex_tangents[i]);
			}
		}

		// vertexBuffer - UV SETS
		if (!vertex_uvset_0.empty() || !vertex_uvset_1.empty())
		{
			const XMFLOAT2* uv0_stream = vertex_uvset_0.empty() ? vertex_uvset_1.data() : vertex_uvset_0.data();
			const XMFLOAT2* uv1_stream = vertex_uvset_1.empty() ? vertex_uvset_0.data() : vertex_uvset_1.data();

			vb_uvs.offset = buffer_offset;
			vb_uvs.size = uv_count * sizeof(Vertex_UVS);
			Vertex_UVS* vertices = (Vertex_UVS*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(vb_uvs.size, alignment);
			for (size_t i = 0; i < uv_count; ++i)
			{
				vertices[i].uv0.FromFULL(uv0_stream[i]);
				vertices[i].uv1.FromFULL(uv1_stream[i]);
			}
		}

		// vertexBuffer - ATLAS
		if (!vertex_atlas.empty())
		{
			vb_atl.offset = buffer_offset;
			vb_atl.size = vertex_atlas.size() * sizeof(Vertex_TEX);
			Vertex_TEX* vertices = (Vertex_TEX*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(vb_atl.size, alignment);
			for (size_t i = 0; i < vertex_atlas.size(); ++i)
			{
				vertices[i].FromFULL(vertex_atlas[i]);
			}
		}

		// vertexBuffer - COLORS
		if (!vertex_colors.empty())
		{
			vb_col.offset = buffer_offset;
			vb_col.size = vertex_colors.size() * sizeof(Vertex_COL);
			Vertex_COL* vertices = (Vertex_COL*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(vb_col.size, alignment);
			for (size_t i = 0; i < vertex_colors.size(); ++i)
			{
				vertices[i].color = vertex_colors[i];
			}
		}

		// skinning buffers:
		if (!vertex_boneindices.empty())
		{
			vb_bon.offset = buffer_offset;
			vb_bon.size = vertex_boneindices.size() * sizeof(Vertex_BON);
			Vertex_BON* vertices = (Vertex_BON*)(buffer_data.data() + buffer_offset);
			buffer_offset += AlignTo(vb_bon.size, alignment);
			assert(vertex_boneindices.size() == vertex_boneweights.size());
			for (size_t i = 0; i < vertex_boneindices.size(); ++i)
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
			CreateStreamoutRenderData();
		}

		bool success = device->CreateBuffer(&bd, buffer_data.data(), &generalBuffer);
		assert(success);
		device->SetName(&generalBuffer, "MeshComponent::generalBuffer");

		assert(ib.IsValid());
		const Format ib_format = GetIndexFormat() == IndexBufferFormat::UINT32 ? Format::R32_UINT : Format::R16_UINT;
		ib.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, ib.offset, ib.size, &ib_format);
		ib.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, ib.subresource_srv);

		assert(vb_pos_nor_wind.IsValid());
		vb_pos_nor_wind.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_pos_nor_wind.offset, vb_pos_nor_wind.size);
		vb_pos_nor_wind.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_pos_nor_wind.subresource_srv);

		if (vb_tan.IsValid())
		{
			vb_tan.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_tan.offset, vb_tan.size);
			vb_tan.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_tan.subresource_srv);
		}
		if (vb_uvs.IsValid())
		{
			vb_uvs.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_uvs.offset, vb_uvs.size);
			vb_uvs.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_uvs.subresource_srv);
		}
		if (vb_atl.IsValid())
		{
			vb_atl.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_atl.offset, vb_atl.size);
			vb_atl.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_atl.subresource_srv);
		}
		if (vb_col.IsValid())
		{
			vb_col.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_col.offset, vb_col.size);
			vb_col.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_col.subresource_srv);
		}
		if (vb_bon.IsValid())
		{
			vb_bon.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_bon.offset, vb_bon.size);
			vb_bon.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_bon.subresource_srv);
		}

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;

			RaytracingAccelerationStructureDesc desc;
			desc.type = RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL;

			if (streamoutBuffer.IsValid())
			{
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE;
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;
			}
			else
			{
				desc.flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE;
			}

			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = subsets[subsetIndex];
				desc.bottom_level.geometries.emplace_back();
				auto& geometry = desc.bottom_level.geometries.back();
				geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES;
				geometry.triangles.vertex_buffer = generalBuffer;
				geometry.triangles.vertex_byte_offset = vb_pos_nor_wind.offset;
				geometry.triangles.index_buffer = generalBuffer;
				geometry.triangles.index_format = GetIndexFormat();
				geometry.triangles.index_count = subset.indexCount;
				geometry.triangles.index_offset = ib.offset / GetIndexStride() + subset.indexOffset;
				geometry.triangles.vertex_count = (uint32_t)vertex_positions.size();
				geometry.triangles.vertex_format = Format::R32G32B32_FLOAT;
				geometry.triangles.vertex_stride = sizeof(MeshComponent::Vertex_POS);
			}

			bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLAS);
			assert(success);
			device->SetName(&BLAS, "MeshComponent::BLAS");
		}
	}
	void MeshComponent::CreateStreamoutRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		GPUBufferDesc desc;
		desc.usage = Usage::DEFAULT;
		desc.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			desc.misc_flags |= ResourceMiscFlag::RAY_TRACING;
		}
		const uint64_t alignment = device->GetMinOffsetAlignment(&desc);
		desc.size =
			AlignTo(vertex_positions.size() * sizeof(Vertex_POS) * 2, alignment) + // *2 because prevpos also goes into this!
			AlignTo(vertex_tangents.size() * sizeof(Vertex_TAN), alignment)
			;

		bool success = device->CreateBuffer(&desc, nullptr, &streamoutBuffer);
		assert(success);
		device->SetName(&streamoutBuffer, "MeshComponent::streamoutBuffer");

		uint64_t buffer_offset = 0ull;

		so_pos_nor_wind.offset = buffer_offset;
		so_pos_nor_wind.size = vb_pos_nor_wind.size;
		buffer_offset += AlignTo(so_pos_nor_wind.size, alignment);
		so_pos_nor_wind.subresource_srv = device->CreateSubresource(&streamoutBuffer, SubresourceType::SRV, so_pos_nor_wind.offset, so_pos_nor_wind.size);
		so_pos_nor_wind.subresource_uav = device->CreateSubresource(&streamoutBuffer, SubresourceType::UAV, so_pos_nor_wind.offset, so_pos_nor_wind.size);
		so_pos_nor_wind.descriptor_srv = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::SRV, so_pos_nor_wind.subresource_srv);
		so_pos_nor_wind.descriptor_uav = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::UAV, so_pos_nor_wind.subresource_uav);

		if (vb_tan.IsValid())
		{
			so_tan.offset = buffer_offset;
			so_tan.size = vb_tan.size;
			buffer_offset += AlignTo(so_tan.size, alignment);
			so_tan.subresource_srv = device->CreateSubresource(&streamoutBuffer, SubresourceType::SRV, so_tan.offset, so_tan.size);
			so_tan.subresource_uav = device->CreateSubresource(&streamoutBuffer, SubresourceType::UAV, so_tan.offset, so_tan.size);
			so_tan.descriptor_srv = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::SRV, so_tan.subresource_srv);
			so_tan.descriptor_uav = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::UAV, so_tan.subresource_uav);
		}

		so_pre.offset = buffer_offset;
		so_pre.size = vb_pos_nor_wind.size;
		buffer_offset += AlignTo(so_pre.size, alignment);
		so_pre.subresource_srv = device->CreateSubresource(&streamoutBuffer, SubresourceType::SRV, so_pre.offset, so_pre.size);
		so_pre.subresource_uav = device->CreateSubresource(&streamoutBuffer, SubresourceType::UAV, so_pre.offset, so_pre.size);
		so_pre.descriptor_srv = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::SRV, so_pre.subresource_srv);
		so_pre.descriptor_uav = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::UAV, so_pre.subresource_uav);
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
		Sphere sphere;
		sphere.center = aabb.getCenter();
		sphere.radius = aabb.getRadius();
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

		bool success = device->CreateBuffer(&bd, nullptr, &boneBuffer);
		assert(success);
		device->SetName(&boneBuffer, "ArmatureComponent::boneBuffer");
		descriptor_srv = device->GetDescriptorIndex(&boneBuffer, SubresourceType::SRV);
	}

	AnimationComponent::AnimationChannel::PathDataType AnimationComponent::AnimationChannel::GetPathDataType() const
	{
		switch (path)
		{
		case wi::scene::AnimationComponent::AnimationChannel::Path::TRANSLATION:
			return PathDataType::Float3;
		case wi::scene::AnimationComponent::AnimationChannel::Path::ROTATION:
			return PathDataType::Float4;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SCALE:
			return PathDataType::Float3;
		case wi::scene::AnimationComponent::AnimationChannel::Path::WEIGHTS:
			return PathDataType::Weights;

		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
			return PathDataType::Float3;
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
			return PathDataType::Float;

		case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_PLAY:
		case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_STOP:
			return PathDataType::Event;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_VOLUME:
			return PathDataType::Float;

		case wi::scene::AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT:
			return PathDataType::Float;

		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_FOV:
		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH:
		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE:
			return PathDataType::Float;
		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE:
			return PathDataType::Float2;

		case wi::scene::AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY:
		case wi::scene::AnimationComponent::AnimationChannel::Path::SCRIPT_STOP:
			return PathDataType::Event;

		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR:
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE:
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD:
			return PathDataType::Float4;
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS:
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE:
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS:
			return PathDataType::Float;

		default:
			assert(0);
			break;
		}
		return PathDataType::Event;
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
		XMMATRIX W = XMLoadFloat4x4(&transform.world);

		XMVECTOR _Eye = XMVector3Transform(XMVectorSet(0, 0, 0, 1), W);
		XMVECTOR _At = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), W));
		XMVECTOR _Up = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), W));

		XMMATRIX _V = XMMatrixLookToLH(_Eye, _At, _Up);
		XMStoreFloat4x4(&View, _V);

		XMStoreFloat3x3(&rotationMatrix, XMMatrixInverse(nullptr, _V));

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
	void CameraComponent::Lerp(const CameraComponent& a, const CameraComponent& b, float t)
	{
		SetDirty();

		width = wi::math::Lerp(a.width, b.width, t);
		height = wi::math::Lerp(a.height, b.height, t);
		zNearP = wi::math::Lerp(a.zNearP, b.zNearP, t);
		zFarP = wi::math::Lerp(a.zFarP, b.zFarP, t);
		fov = wi::math::Lerp(a.fov, b.fov, t);
		focal_length = wi::math::Lerp(a.focal_length, b.focal_length, t);
		aperture_size = wi::math::Lerp(a.aperture_size, b.aperture_size, t);
		aperture_shape = wi::math::Lerp(a.aperture_shape, b.aperture_shape, t);
	}

	void ScriptComponent::CreateFromFile(const std::string& filename)
	{
		this->filename = filename;
		resource = wi::resourcemanager::Load(filename, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
		script.clear(); // will be created on first Update()
	}



	const uint32_t small_subtask_groupsize = 64u;

	void Scene::Update(float dt)
	{
		this->dt = dt;

		wi::jobsystem::context ctx;

		// Script system runs first, because it could create new entities and components
		//	So GPU persistent resources need to be created accordingly for them too:
		RunScriptUpdateSystem(ctx);

		GraphicsDevice* device = wi::graphics::GetDevice();

		instanceArraySize = objects.GetCount() + hairs.GetCount() + emitters.GetCount();
		if (impostors.GetCount() > 0)
		{
			impostorInstanceOffset = uint32_t(instanceArraySize);
			instanceArraySize += 1;
		}
		if (instanceBuffer.desc.size < (instanceArraySize * sizeof(ShaderMeshInstance)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMeshInstance);
			desc.size = desc.stride * instanceArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&desc, nullptr, &instanceBuffer);
			device->SetName(&instanceBuffer, "Scene::instanceBuffer");

			desc.usage = Usage::UPLOAD;
			desc.bind_flags = BindFlag::NONE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			for (int i = 0; i < arraysize(instanceUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &instanceUploadBuffer[i]);
				device->SetName(&instanceUploadBuffer[i], "Scene::instanceUploadBuffer");
			}
		}
		instanceArrayMapped = (ShaderMeshInstance*)instanceUploadBuffer[device->GetBufferIndex()].mapped_data;

		materialArraySize = materials.GetCount();
		if (impostors.GetCount() > 0)
		{
			impostorMaterialOffset = uint32_t(materialArraySize);
			materialArraySize += 1;
		}
		if (materialBuffer.desc.size < (materialArraySize * sizeof(ShaderMaterial)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMaterial);
			desc.size = desc.stride * materialArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&desc, nullptr, &materialBuffer);
			device->SetName(&materialBuffer, "Scene::materialBuffer");

			desc.usage = Usage::UPLOAD;
			desc.bind_flags = BindFlag::NONE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			for (int i = 0; i < arraysize(materialUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &materialUploadBuffer[i]);
				device->SetName(&materialUploadBuffer[i], "Scene::materialUploadBuffer");
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
					device->SetName(&TLAS_instancesUpload[i], "Scene::TLAS_instancesUpload");
				}
			}
			TLAS_instancesMapped = TLAS_instancesUpload[device->GetBufferIndex()].mapped_data;
		}

		// Occlusion culling read:
		if(wi::renderer::GetOcclusionCullingEnabled() && !wi::renderer::GetFreezeCullingCameraEnabled())
		{
			uint32_t minQueryCount = uint32_t(objects.GetCount() + lights.GetCount() + 1); // +1 : ocean
			if (queryHeap.desc.query_count < minQueryCount)
			{
				GPUQueryHeapDesc desc;
				desc.type = GpuQueryType::OCCLUSION_BINARY;
				desc.query_count = minQueryCount * 2; // *2 to grow fast
				bool success = device->CreateQueryHeap(&desc, &queryHeap);
				assert(success);

				GPUBufferDesc bd;
				bd.usage = Usage::READBACK;
				bd.size = desc.query_count * sizeof(uint64_t);

				for (int i = 0; i < arraysize(queryResultBuffer); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &queryResultBuffer[i]);
					assert(success);
					device->SetName(&queryResultBuffer[i], "Scene::queryResultBuffer");
				}

				if (device->CheckCapability(GraphicsDeviceCapability::PREDICATION))
				{
					bd.usage = Usage::DEFAULT;
					bd.misc_flags |= ResourceMiscFlag::PREDICATION;
					success = device->CreateBuffer(&bd, nullptr, &queryPredicationBuffer);
					assert(success);
					device->SetName(&queryPredicationBuffer, "Scene::queryPredicationBuffer");
				}
			}

			// Advance to next query result buffer to use (this will be the oldest one that was written)
			queryheap_idx = (queryheap_idx + 1) % arraysize(queryResultBuffer);

			// Clear query allocation state:
			queryAllocator.store(0);
		}

		// Scan mesh subset counts to allocate GPU geometry data:
		geometryAllocator.store(0u);
		wi::jobsystem::Dispatch(ctx, (uint32_t)meshes.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {
			MeshComponent& mesh = meshes[args.jobIndex];
			mesh.geometryOffset = geometryAllocator.fetch_add((uint32_t)mesh.subsets.size());
		});

		wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
			// Must not keep inactive TLAS instances, so zero them out for safety:
			std::memset(TLAS_instancesMapped, 0, TLAS_instancesUpload->desc.size);
		});

		wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
			// Must not keep inactive instances, so init them for safety:
			ShaderMeshInstance inst;
			inst.init();
			for (uint32_t i = 0; i < instanceArraySize; ++i)
			{
				std::memcpy(instanceArrayMapped + i, &inst, sizeof(inst));
			}
		});

		wi::physics::RunPhysicsUpdateSystem(ctx, *this, dt);

		RunAnimationUpdateSystem(ctx);

		RunTransformUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunHierarchyUpdateSystem(ctx);

		// GPU subset count allocation is ready at this point:
		geometryArraySize = geometryAllocator.load();
		geometryArraySize += hairs.GetCount();
		geometryArraySize += emitters.GetCount();
		if (impostors.GetCount() > 0)
		{
			impostorGeometryOffset = uint32_t(geometryArraySize);
			geometryArraySize += 1;
		}
		if (geometryBuffer.desc.size < (geometryArraySize * sizeof(ShaderGeometry)))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderGeometry);
			desc.size = desc.stride * geometryArraySize * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&desc, nullptr, &geometryBuffer);
			device->SetName(&geometryBuffer, "Scene::geometryBuffer");

			desc.usage = Usage::UPLOAD;
			desc.bind_flags = BindFlag::NONE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			for (int i = 0; i < arraysize(geometryUploadBuffer); ++i)
			{
				device->CreateBuffer(&desc, nullptr, &geometryUploadBuffer[i]);
				device->SetName(&geometryUploadBuffer[i], "Scene::geometryUploadBuffer");
			}
		}
		geometryArrayMapped = (ShaderGeometry*)geometryUploadBuffer[device->GetBufferIndex()].mapped_data;

		RunExpressionUpdateSystem(ctx);

		RunMeshUpdateSystem(ctx);

		RunMaterialUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunInverseKinematicsUpdateSystem(ctx);

		RunColliderUpdateSystem(ctx);

		RunSpringUpdateSystem(ctx);

		RunArmatureUpdateSystem(ctx);

		RunWeatherUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		RunObjectUpdateSystem(ctx);

		RunCameraUpdateSystem(ctx);

		RunDecalUpdateSystem(ctx);

		RunProbeUpdateSystem(ctx);

		RunForceUpdateSystem(ctx);

		RunLightUpdateSystem(ctx);

		RunParticleUpdateSystem(ctx);

		RunSoundUpdateSystem(ctx);

		RunImpostorUpdateSystem(ctx);

		wi::jobsystem::Wait(ctx); // dependencies

		// Merge parallel bounds computation (depends on object update system):
		bounds = AABB();
		for (auto& group_bound : parallel_bounds)
		{
			bounds = AABB::Merge(bounds, group_bound);
		}

		// Meshlet buffer:
		uint32_t meshletCount = meshletAllocator.load();
		if(meshletBuffer.desc.size < meshletCount * sizeof(ShaderMeshlet))
		{
			GPUBufferDesc desc;
			desc.stride = sizeof(ShaderMeshlet);
			desc.size = desc.stride * meshletCount * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			bool success = device->CreateBuffer(&desc, nullptr, &meshletBuffer);
			assert(success);
			device->SetName(&meshletBuffer, "meshletBuffer");
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
				device->SetName(&desc.top_level.instance_buffer, "Scene::TLAS.instanceBuffer");
				success = device->CreateRaytracingAccelerationStructure(&desc, &TLAS);
				assert(success);
				device->SetName(&TLAS, "Scene::TLAS");
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
				desc.size = SURFEL_STATS_SIZE;
				desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				uint stats_data[] = { 0,0,SURFEL_CAPACITY,0,0,0 };
				device->CreateBuffer(&desc, &stats_data, &surfelStatsBuffer);
				device->SetName(&surfelStatsBuffer, "surfelStatsBuffer");

				desc.stride = sizeof(uint);
				desc.size = SURFEL_INDIRECT_SIZE;
				desc.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::INDIRECT_ARGS;
				uint indirect_data[] = { 0,0,0, 0,0,0, 0,0,0 };
				device->CreateBuffer(&desc, &indirect_data, &surfelIndirectBuffer);
				device->SetName(&surfelIndirectBuffer, "surfelIndirectBuffer");

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

				desc.stride = sizeof(SurfelRayDataPacked);
				desc.size = desc.stride * SURFEL_RAY_BUDGET;
				desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&desc, nullptr, &surfelRayBuffer);
				device->SetName(&surfelRayBuffer, "surfelRayBuffer");

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

		if (wi::renderer::GetDDGIEnabled())
		{
			ddgi.frame_index++;
			if (!ddgi.color_texture[1].IsValid()) // if just color_texture[0] is valid, it could be that ddgi was serialized, that's why we check color_texture[1] here
			{
				ddgi.frame_index = 0;

				const uint32_t probe_count = ddgi.grid_dimensions.x * ddgi.grid_dimensions.y * ddgi.grid_dimensions.z;

				GPUBufferDesc buf;
				buf.stride = sizeof(DDGIRayDataPacked);
				buf.size = buf.stride * probe_count * DDGI_MAX_RAYCOUNT;
				buf.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				buf.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
				device->CreateBuffer(&buf, nullptr, &ddgi.ray_buffer);
				device->SetName(&ddgi.ray_buffer, "ddgi.ray_buffer");

				buf.stride = sizeof(DDGIProbeOffset);
				buf.size = buf.stride * probe_count;
				buf.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				buf.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				device->CreateBuffer(&buf, nullptr, &ddgi.offset_buffer);
				device->SetName(&ddgi.offset_buffer, "ddgi.offset_buffer");

				TextureDesc tex;
				tex.width = DDGI_COLOR_TEXELS * ddgi.grid_dimensions.x * ddgi.grid_dimensions.y;
				tex.height = DDGI_COLOR_TEXELS * ddgi.grid_dimensions.z;
				//tex.format = Format::R11G11B10_FLOAT; // not enough precision with this format, causes green hue in GI
				tex.format = Format::R16G16B16A16_FLOAT;
				tex.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				device->CreateTexture(&tex, nullptr, &ddgi.color_texture[0]);
				device->SetName(&ddgi.color_texture[0], "ddgi.color_texture[0]");
				device->CreateTexture(&tex, nullptr, &ddgi.color_texture[1]);
				device->SetName(&ddgi.color_texture[1], "ddgi.color_texture[1]");

				tex.width = DDGI_DEPTH_TEXELS * ddgi.grid_dimensions.x * ddgi.grid_dimensions.y;
				tex.height = DDGI_DEPTH_TEXELS * ddgi.grid_dimensions.z;
				tex.format = Format::R16G16_FLOAT;
				tex.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
				device->CreateTexture(&tex, nullptr, &ddgi.depth_texture[0]);
				device->SetName(&ddgi.depth_texture[0], "ddgi.depth_texture[0]");
				device->CreateTexture(&tex, nullptr, &ddgi.depth_texture[1]);
				device->SetName(&ddgi.depth_texture[1], "ddgi.depth_texture[1]");
			}
			std::swap(ddgi.color_texture[0], ddgi.color_texture[1]);
			std::swap(ddgi.depth_texture[0], ddgi.depth_texture[1]);
		}
		else if (ddgi.color_texture[1].IsValid()) // if just color_texture[0] is valid, it could be that ddgi was serialized, that's why we check color_texture[1] here
		{
			ddgi.ray_buffer = {};
			ddgi.offset_buffer = {};
			ddgi.color_texture[0] = {};
			ddgi.color_texture[1] = {};
			ddgi.depth_texture[0] = {};
			ddgi.depth_texture[1] = {};
		}

		impostor_ib_format = (((objects.GetCount() * 4) < 655536) ? Format::R16_UINT : Format::R32_UINT);
		const size_t impostor_index_stride = impostor_ib_format == Format::R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t);
		const uint64_t required_impostor_buffer_size = objects.GetCount() * (sizeof(impostor_index_stride) * 6 + sizeof(uint4) * 4 + sizeof(uint2));
		if (impostorBuffer.desc.size < required_impostor_buffer_size)
		{
			GPUBufferDesc desc;
			desc.usage = Usage::DEFAULT;
			desc.size = required_impostor_buffer_size * 2; // *2 to grow fast
			desc.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
			device->CreateBuffer(&desc, nullptr, &impostorBuffer);
			device->SetName(&impostorBuffer, "impostorBuffer");

			const uint64_t alignment = device->GetMinOffsetAlignment(&desc);
			uint64_t buffer_offset = 0ull;

			impostor_ib.offset = buffer_offset;
			impostor_ib.size = objects.GetCount() * sizeof(impostor_index_stride) * 6;
			buffer_offset += AlignTo(impostor_ib.size, alignment);
			impostor_ib.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_ib.offset, impostor_ib.size, &impostor_ib_format);
			impostor_ib.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_ib.offset, impostor_ib.size, &impostor_ib_format);
			impostor_ib.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_ib.subresource_srv);
			impostor_ib.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_ib.subresource_uav);

			impostor_vb.offset = buffer_offset;
			impostor_vb.size = objects.GetCount() * sizeof(uint4) * 4;
			buffer_offset += AlignTo(impostor_vb.size, alignment);
			impostor_vb.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_vb.offset, impostor_vb.size);
			impostor_vb.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_vb.offset, impostor_vb.size);
			impostor_vb.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_vb.subresource_srv);
			impostor_vb.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_vb.subresource_uav);

			impostor_data.offset = buffer_offset;
			impostor_data.size = objects.GetCount() * sizeof(uint2);
			buffer_offset += AlignTo(impostor_data.size, alignment);
			impostor_data.subresource_srv = device->CreateSubresource(&impostorBuffer, SubresourceType::SRV, impostor_data.offset, impostor_data.size);
			impostor_data.subresource_uav = device->CreateSubresource(&impostorBuffer, SubresourceType::UAV, impostor_data.offset, impostor_data.size);
			impostor_data.descriptor_srv = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::SRV, impostor_data.subresource_srv);
			impostor_data.descriptor_uav = device->GetDescriptorIndex(&impostorBuffer, SubresourceType::UAV, impostor_data.subresource_uav);

			desc.stride = sizeof(IndirectDrawArgsIndexedInstanced);
			desc.size = desc.stride;
			desc.bind_flags = BindFlag::UNORDERED_ACCESS;
			desc.misc_flags = ResourceMiscFlag::INDIRECT_ARGS | ResourceMiscFlag::BUFFER_STRUCTURED;
			device->CreateBuffer(&desc, nullptr, &impostorIndirectBuffer);
			device->SetName(&impostorIndirectBuffer, "impostorIndirectBuffer");
		}

		// Shader scene resources:
		shaderscene.instancebuffer = device->GetDescriptorIndex(&instanceBuffer, SubresourceType::SRV);
		shaderscene.geometrybuffer = device->GetDescriptorIndex(&geometryBuffer, SubresourceType::SRV);
		shaderscene.materialbuffer = device->GetDescriptorIndex(&materialBuffer, SubresourceType::SRV);
		shaderscene.meshletbuffer = device->GetDescriptorIndex(&meshletBuffer, SubresourceType::SRV);
		shaderscene.envmaparray = device->GetDescriptorIndex(&envmapArray, SubresourceType::SRV);
		if (weather.skyMap.IsValid())
		{
			shaderscene.globalenvmap = device->GetDescriptorIndex(&weather.skyMap.GetTexture(), SubresourceType::SRV);
		}
		else
		{
			shaderscene.globalenvmap = -1;
		}
		shaderscene.impostorInstanceOffset = impostorInstanceOffset;
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
		shaderscene.weather.most_important_light_index = weather.most_important_light_index;
		shaderscene.weather.ambient = weather.ambient;
		shaderscene.weather.fog.start = weather.fogStart;
		shaderscene.weather.fog.end = weather.fogEnd;
		shaderscene.weather.fog.height_start = weather.fogHeightStart;
		shaderscene.weather.fog.height_end = weather.fogHeightEnd;
		shaderscene.weather.horizon = weather.horizon;
		shaderscene.weather.zenith = weather.zenith;
		shaderscene.weather.sky_exposure = weather.skyExposure;
		shaderscene.weather.wind.speed = weather.windSpeed;
		shaderscene.weather.wind.randomness = weather.windRandomness;
		shaderscene.weather.wind.wavesize = weather.windWaveSize;
		shaderscene.weather.wind.direction = weather.windDirection;
		shaderscene.weather.atmosphere = weather.atmosphereParameters;
		shaderscene.weather.volumetric_clouds = weather.volumetricCloudParameters;
		shaderscene.weather.ocean.water_color = weather.oceanParameters.waterColor;
		shaderscene.weather.ocean.water_height = weather.oceanParameters.waterHeight;
		shaderscene.weather.ocean.patch_size_rcp = 1.0f / weather.oceanParameters.patch_length;
		shaderscene.weather.ocean.texture_displacementmap = device->GetDescriptorIndex(ocean.getDisplacementMap(), SubresourceType::SRV);
		shaderscene.weather.ocean.texture_gradientmap = device->GetDescriptorIndex(ocean.getGradientMap(), SubresourceType::SRV);
		shaderscene.weather.stars = weather.stars;
		XMStoreFloat4x4(&shaderscene.weather.stars_rotation, XMMatrixRotationQuaternion(XMLoadFloat4(&weather.stars_rotation_quaternion)));

		shaderscene.ddgi.grid_dimensions = ddgi.grid_dimensions;
		shaderscene.ddgi.probe_count = ddgi.grid_dimensions.x * ddgi.grid_dimensions.y * ddgi.grid_dimensions.z;
		shaderscene.ddgi.color_texture_resolution = uint2(ddgi.color_texture[0].desc.width, ddgi.color_texture[0].desc.height);
		shaderscene.ddgi.color_texture_resolution_rcp = float2(1.0f / shaderscene.ddgi.color_texture_resolution.x, 1.0f / shaderscene.ddgi.color_texture_resolution.y);
		shaderscene.ddgi.depth_texture_resolution = uint2(ddgi.depth_texture[0].desc.width, ddgi.depth_texture[0].desc.height);
		shaderscene.ddgi.depth_texture_resolution_rcp = float2(1.0f / shaderscene.ddgi.depth_texture_resolution.x, 1.0f / shaderscene.ddgi.depth_texture_resolution.y);
		shaderscene.ddgi.color_texture = device->GetDescriptorIndex(&ddgi.color_texture[0], SubresourceType::SRV);
		shaderscene.ddgi.depth_texture = device->GetDescriptorIndex(&ddgi.depth_texture[0], SubresourceType::SRV);
		shaderscene.ddgi.offset_buffer = device->GetDescriptorIndex(&ddgi.offset_buffer, SubresourceType::SRV);
		shaderscene.ddgi.grid_min.x = shaderscene.aabb_min.x - 1;
		shaderscene.ddgi.grid_min.y = shaderscene.aabb_min.y - 1;
		shaderscene.ddgi.grid_min.z = shaderscene.aabb_min.z - 1;
		float3 grid_max = shaderscene.aabb_max;
		grid_max.x += 1;
		grid_max.y += 1;
		grid_max.z += 1;
		shaderscene.ddgi.grid_extents.x = abs(grid_max.x - shaderscene.ddgi.grid_min.x);
		shaderscene.ddgi.grid_extents.y = abs(grid_max.y - shaderscene.ddgi.grid_min.y);
		shaderscene.ddgi.grid_extents.z = abs(grid_max.z - shaderscene.ddgi.grid_min.z);
		shaderscene.ddgi.grid_extents_rcp.x = 1.0f / shaderscene.ddgi.grid_extents.x;
		shaderscene.ddgi.grid_extents_rcp.y = 1.0f / shaderscene.ddgi.grid_extents.y;
		shaderscene.ddgi.grid_extents_rcp.z = 1.0f / shaderscene.ddgi.grid_extents.z;
		shaderscene.ddgi.cell_size.x = shaderscene.ddgi.grid_extents.x / (ddgi.grid_dimensions.x - 1);
		shaderscene.ddgi.cell_size.y = shaderscene.ddgi.grid_extents.y / (ddgi.grid_dimensions.y - 1);
		shaderscene.ddgi.cell_size.z = shaderscene.ddgi.grid_extents.z / (ddgi.grid_dimensions.z - 1);
		shaderscene.ddgi.cell_size_rcp.x = 1.0f / shaderscene.ddgi.cell_size.x;
		shaderscene.ddgi.cell_size_rcp.y = 1.0f / shaderscene.ddgi.cell_size.y;
		shaderscene.ddgi.cell_size_rcp.z = 1.0f / shaderscene.ddgi.cell_size.z;
		shaderscene.ddgi.max_distance = std::max(shaderscene.ddgi.cell_size.x, std::max(shaderscene.ddgi.cell_size.y, shaderscene.ddgi.cell_size.z)) * 1.5f;
	}
	void Scene::Clear()
	{
		for(auto& entry : componentLibrary.entries)
		{
			entry.second.component_manager->Clear();
		}

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

		ddgi = {};
	}
	void Scene::Merge(Scene& other)
	{
		for (auto& entry : componentLibrary.entries)
		{
			entry.second.component_manager->Merge(*other.componentLibrary.entries[entry.first].component_manager);
		}

		bounds = AABB::Merge(bounds, other.bounds);

		if (!ddgi.color_texture[0].IsValid() && other.ddgi.color_texture[0].IsValid())
		{
			ddgi = std::move(other.ddgi);
		}
	}
	void Scene::FindAllEntities(wi::unordered_set<wi::ecs::Entity>& entities) const
	{
		for (auto& entry : componentLibrary.entries)
		{
			entities.insert(entry.second.component_manager->GetEntityArray().begin(), entry.second.component_manager->GetEntityArray().end());
		}
	}

	void Scene::Entity_Remove(Entity entity, bool recursive)
	{
		if (recursive)
		{
			wi::vector<Entity> entities_to_remove;
			for (size_t i = 0; i < hierarchy.GetCount(); ++i)
			{
				const HierarchyComponent& hier = hierarchy[i];
				if (hier.parentID == entity)
				{
					Entity child = hierarchy.GetEntity(i);
					entities_to_remove.push_back(child);
				}
			}
			for (auto& child : entities_to_remove)
			{
				Entity_Remove(child);
			}
		}

		for (auto& entry : componentLibrary.entries)
		{
			entry.second.component_manager->Remove(entity);
		}
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
		EntitySerializer seri;

		// First write the root entity to staging area:
		archive.SetReadModeAndResetPos(false);
		Entity_Serialize(archive, seri, entity, EntitySerializeFlags::RECURSIVE);

		// Then deserialize root:
		archive.SetReadModeAndResetPos(true);
		Entity root = Entity_Serialize(archive, seri, INVALID_ENTITY, EntitySerializeFlags::RECURSIVE | EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);

		return root;
	}
	Entity Scene::Entity_CreateTransform(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		transforms.Create(entity);

		return entity;
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
		float intensity,
		float range,
		LightComponent::LightType type,
		float outerConeAngle,
		float innerConeAngle)
	{
		Entity entity = CreateEntity();

		names.Create(entity) = name;

		layers.Create(entity);

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		aabb_lights.Create(entity).createFromHalfWidth(position, XMFLOAT3(range, range, range));

		LightComponent& light = lights.Create(entity);
		light.intensity = intensity;
		light.range = range;
		light.color = color;
		light.SetType(type);
		light.outerConeAngle = outerConeAngle;
		light.innerConeAngle = innerConeAngle;

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
		force.range = 0;
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

		if (!filename.empty())
		{
			SoundComponent& sound = sounds.Create(entity);
			sound.filename = filename;
			sound.soundResource = wi::resourcemanager::Load(filename, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
			wi::audio::CreateSoundInstance(&sound.soundResource.GetSound(), &sound.soundinstance);
		}

		TransformComponent& transform = transforms.Create(entity);
		transform.Translate(position);
		transform.UpdateTransform();

		return entity;
	}
	Entity Scene::Entity_CreateCube(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		if (!name.empty())
		{
			names.Create(entity) = name;
		}

		layers.Create(entity);

		transforms.Create(entity);

		aabb_objects.Create(entity);

		ObjectComponent& object = objects.Create(entity);

		MeshComponent& mesh = meshes.Create(entity);

		// object references the mesh entity (there can be multiple objects referencing one mesh):
		object.meshID = entity;

		mesh.vertex_positions = {
			// -Z
			XMFLOAT3(-1,1,	-1),
			XMFLOAT3(-1,-1,	-1),
			XMFLOAT3(1,-1,	-1),
			XMFLOAT3(1,1,	-1),

			// +Z
			XMFLOAT3(-1,1,	1),
			XMFLOAT3(-1,-1,	1),
			XMFLOAT3(1,-1,	1),
			XMFLOAT3(1,1,	1),

			// -X
			XMFLOAT3(-1, -1,1),
			XMFLOAT3(-1, -1,-1),
			XMFLOAT3(-1, 1,-1),
			XMFLOAT3(-1, 1,1),

			// +X
			XMFLOAT3(1, -1,1),
			XMFLOAT3(1, -1,-1),
			XMFLOAT3(1, 1,-1),
			XMFLOAT3(1, 1,1),

			// -Y
			XMFLOAT3(-1, -1,1),
			XMFLOAT3(-1, -1,-1),
			XMFLOAT3(1, -1,-1),
			XMFLOAT3(1, -1,1),

			// +Y
			XMFLOAT3(-1, 1,1),
			XMFLOAT3(-1, 1,-1),
			XMFLOAT3(1, 1,-1),
			XMFLOAT3(1, 1,1),
		};

		mesh.vertex_normals = {
			XMFLOAT3(0,0,-1),
			XMFLOAT3(0,0,-1),
			XMFLOAT3(0,0,-1),
			XMFLOAT3(0,0,-1),

			XMFLOAT3(0,0,1),
			XMFLOAT3(0,0,1),
			XMFLOAT3(0,0,1),
			XMFLOAT3(0,0,1),

			XMFLOAT3(-1,0,0),
			XMFLOAT3(-1,0,0),
			XMFLOAT3(-1,0,0),
			XMFLOAT3(-1,0,0),

			XMFLOAT3(1,0,0),
			XMFLOAT3(1,0,0),
			XMFLOAT3(1,0,0),
			XMFLOAT3(1,0,0),

			XMFLOAT3(0,-1,0),
			XMFLOAT3(0,-1,0),
			XMFLOAT3(0,-1,0),
			XMFLOAT3(0,-1,0),

			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
		};

		mesh.vertex_uvset_0 = {
			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),

			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),
		};

		mesh.indices = {
			0,			1,			2,			0,			2,			3,
			0 + 4,		2 + 4,		1 + 4,		0 + 4,		3 + 4,		2 + 4,		// swapped winding
			0 + 4 * 2,	1 + 4 * 2,	2 + 4 * 2,	0 + 4 * 2,	2 + 4 * 2,	3 + 4 * 2,
			0 + 4 * 3,	2 + 4 * 3,	1 + 4 * 3,	0 + 4 * 3,	3 + 4 * 3,	2 + 4 * 3,	// swapped winding
			0 + 4 * 4,	2 + 4 * 4,	1 + 4 * 4,	0 + 4 * 4,	3 + 4 * 4,	2 + 4 * 4,	// swapped winding
			0 + 4 * 5,	1 + 4 * 5,	2 + 4 * 5,	0 + 4 * 5,	2 + 4 * 5,	3 + 4 * 5,
		};

		// Subset maps a part of the mesh to a material:
		MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
		subset.indexCount = uint32_t(mesh.indices.size());
		materials.Create(entity);
		subset.materialID = entity; // the material component is created on the same entity as the mesh component, though it is not required as it could also use a different material entity

		// vertex buffer GPU data will be packed and uploaded here:
		mesh.CreateRenderData();

		return entity;
	}
	Entity Scene::Entity_CreatePlane(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		if (!name.empty())
		{
			names.Create(entity) = name;
		}

		layers.Create(entity);

		transforms.Create(entity);

		aabb_objects.Create(entity);

		ObjectComponent& object = objects.Create(entity);

		MeshComponent& mesh = meshes.Create(entity);

		// object references the mesh entity (there can be multiple objects referencing one mesh):
		object.meshID = entity;

		mesh.vertex_positions = {
			// +Y
			XMFLOAT3(-1, 0,1),
			XMFLOAT3(-1, 0,-1),
			XMFLOAT3(1, 0,-1),
			XMFLOAT3(1, 0,1),
		};

		mesh.vertex_normals = {
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,1,0),
		};

		mesh.vertex_uvset_0 = {
			XMFLOAT2(0,0),
			XMFLOAT2(0,1),
			XMFLOAT2(1,1),
			XMFLOAT2(1,0),
		};

		mesh.indices = {
			0, 1, 2, 0, 2, 3,
		};

		// Subset maps a part of the mesh to a material:
		MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
		subset.indexCount = uint32_t(mesh.indices.size());
		materials.Create(entity);
		subset.materialID = entity; // the material component is created on the same entity as the mesh component, though it is not required as it could also use a different material entity

		// vertex buffer GPU data will be packed and uploaded here:
		mesh.CreateRenderData();

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
		TransformComponent* transform_child = transforms.GetComponent(entity);
		if (transform_parent != nullptr && transform_child != nullptr)
		{
			if (!child_already_in_local_space)
			{
				XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world));
				transform_child->MatrixTransform(B);
				transform_child->UpdateTransform();
			}
			transform_child->UpdateTransform_Parented(*transform_parent);
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


	void Scene::RunAnimationUpdateSystem(wi::jobsystem::context& ctx)
	{
		for (size_t i = 0; i < animations.GetCount(); ++i)
		{
			AnimationComponent& animation = animations[i];
			if (!animation.IsPlaying() && animation.last_update_time == animation.timer)
			{
				continue;
			}
			animation.last_update_time = animation.timer;

			for (const AnimationComponent::AnimationChannel& channel : animation.channels)
			{
				assert(channel.samplerIndex < (int)animation.samplers.size());
				const AnimationComponent::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
				const AnimationDataComponent* animationdata = animation_datas.GetComponent(sampler.data);
				if (animationdata == nullptr)
				{
					continue;
				}

				const AnimationComponent::AnimationChannel::PathDataType path_data_type = channel.GetPathDataType();

				float timeFirst = std::numeric_limits<float>::max();
				float timeLast = std::numeric_limits<float>::min();
				int keyLeft = 0;	float timeLeft = std::numeric_limits<float>::min();
				int keyRight = 0;	float timeRight = std::numeric_limits<float>::max();

				// search for usable keyframes:
				for (int k = 0; k < (int)animationdata->keyframe_times.size(); ++k)
				{
					const float time = animationdata->keyframe_times[k];
					if (time < timeFirst)
					{
						timeFirst = time;
					}
					if (time > timeLast)
					{
						timeLast = time;
					}
					if (time <= animation.timer && time > timeLeft)
					{
						timeLeft = time;
						keyLeft = k;
					}
					if (time >= animation.timer && time < timeRight)
					{
						timeRight = time;
						keyRight = k;
					}
				}
				if (path_data_type != AnimationComponent::AnimationChannel::PathDataType::Event)
				{
					if (animation.timer < timeFirst || animation.timer > timeLast)
					{
						// timer is outside range of keyframes, don't update animation:
						continue;
					}
				}
				else
				{
					timeLeft = std::max(timeLeft, timeFirst);
					timeRight = std::max(timeRight, timeLast);
				}

				const float left = animationdata->keyframe_times[keyLeft];
				const float right = animationdata->keyframe_times[keyRight];

				union Interpolator
				{
					XMFLOAT4 f4 = {};
					XMFLOAT3 f3;
					XMFLOAT2 f2;
					float f;
				} interpolator;

				TransformComponent* target_transform = nullptr;
				MeshComponent* target_mesh = nullptr;
				LightComponent* target_light = nullptr;
				SoundComponent* target_sound = nullptr;
				EmittedParticleSystem* target_emitter = nullptr;
				CameraComponent* target_camera = nullptr;
				ScriptComponent* target_script = nullptr;
				MaterialComponent* target_material = nullptr;

				if (
					channel.path == AnimationComponent::AnimationChannel::Path::TRANSLATION ||
					channel.path == AnimationComponent::AnimationChannel::Path::ROTATION ||
					channel.path == AnimationComponent::AnimationChannel::Path::SCALE
					)
				{
					target_transform = transforms.GetComponent(channel.target);
					if (target_transform == nullptr)
						continue;
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::TRANSLATION:
						interpolator.f3 = target_transform->translation_local;
						break;
					case AnimationComponent::AnimationChannel::Path::ROTATION:
						interpolator.f4 = target_transform->rotation_local;
						break;
					case AnimationComponent::AnimationChannel::Path::SCALE:
						interpolator.f3 = target_transform->scale_local;
						break;
					default:
						break;
					}
				}
				else if (channel.path == AnimationComponent::AnimationChannel::Path::WEIGHTS)
				{
					ObjectComponent* object = objects.GetComponent(channel.target);
					if (object == nullptr)
						continue;
					target_mesh = meshes.GetComponent(object->meshID);
					if (target_mesh == nullptr)
						continue;
					animation.morph_weights_temp.resize(target_mesh->morph_targets.size());
				}
				else if (
					channel.path >= AnimationComponent::AnimationChannel::Path::LIGHT_COLOR &&
					channel.path < AnimationComponent::AnimationChannel::Path::_LIGHT_RANGE_END
					)
				{
					target_light = lights.GetComponent(channel.target);
					if (target_light == nullptr)
						continue;
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
						interpolator.f3 = target_light->color;
						break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
						interpolator.f = target_light->intensity;
						break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
						interpolator.f = target_light->range;
						break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
						interpolator.f = target_light->innerConeAngle;
						break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
						interpolator.f = target_light->outerConeAngle;
						break;
					default:
						break;
					}
				}
				else if (
					channel.path >= AnimationComponent::AnimationChannel::Path::SOUND_PLAY &&
					channel.path < AnimationComponent::AnimationChannel::Path::_SOUND_RANGE_END
					)
				{
					target_sound = sounds.GetComponent(channel.target);
					if (target_sound == nullptr)
						continue;
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::SOUND_VOLUME:
						interpolator.f = target_sound->volume;
						break;
					default:
						break;
					}
				}
				else if (
					channel.path >= AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT &&
					channel.path < AnimationComponent::AnimationChannel::Path::_EMITTER_RANGE_END
					)
				{
					target_emitter = emitters.GetComponent(channel.target);
					if (target_emitter == nullptr)
						continue;
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT:
						interpolator.f = target_emitter->count;
						break;
					default:
						break;
					}
				}
				else if (
					channel.path >= AnimationComponent::AnimationChannel::Path::CAMERA_FOV &&
					channel.path < AnimationComponent::AnimationChannel::Path::_CAMERA_RANGE_END
					)
				{
					target_camera = cameras.GetComponent(channel.target);
					if (target_camera == nullptr)
						continue;
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::CAMERA_FOV:
						interpolator.f = target_camera->fov;
						break;
					case AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH:
						interpolator.f = target_camera->focal_length;
						break;
					case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE:
						interpolator.f = target_camera->aperture_size;
						break;
					case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE:
						interpolator.f2 = target_camera->aperture_shape;
						break;
					default:
						break;
					}
				}
				else if (
					channel.path >= AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY &&
					channel.path < AnimationComponent::AnimationChannel::Path::_SCRIPT_RANGE_END
					)
				{
					target_script = scripts.GetComponent(channel.target);
					if (target_script == nullptr)
						continue;
				}
				else if (
					channel.path >= AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR &&
					channel.path < AnimationComponent::AnimationChannel::Path::_MATERIAL_RANGE_END
					)
				{
					target_material = materials.GetComponent(channel.target);
					if (target_material == nullptr)
						continue;
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR:
						interpolator.f4 = target_material->baseColor;
						break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE:
						interpolator.f4 = target_material->emissiveColor;
						break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS:
						interpolator.f = target_material->roughness;
						break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS:
						interpolator.f = target_material->metalness;
						break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE:
						interpolator.f = target_material->reflectance;
						break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD:
						interpolator.f4 = target_material->texMulAdd;
						break;
					default:
						break;
					}
				}
				else
				{
					assert(0);
					continue;
				}

				if (path_data_type == AnimationComponent::AnimationChannel::PathDataType::Event)
				{
					// No path data, only event trigger:
					if (keyLeft == channel.next_event && animation.timer >= timeLeft)
					{
						channel.next_event++;
						switch (channel.path)
						{
						case AnimationComponent::AnimationChannel::Path::SOUND_PLAY:
							target_sound->Play();
							break;
						case AnimationComponent::AnimationChannel::Path::SOUND_STOP:
							target_sound->Stop();
							break;
						case AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY:
							target_script->Play();
							break;
						case AnimationComponent::AnimationChannel::Path::SCRIPT_STOP:
							target_script->Stop();
							break;
						default:
							break;
						}
					}
				}
				else
				{
					// Path data interpolation:
					switch (sampler.mode)
					{
					default:
					case AnimationComponent::AnimationSampler::Mode::STEP:
					{
						// Nearest neighbor method:
						const int key = wi::math::InverseLerp(timeLeft, timeRight, animation.timer) > 0.5f ? keyRight : keyLeft;
						switch (path_data_type)
						{
						default:
						case AnimationComponent::AnimationChannel::PathDataType::Float:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size());
							interpolator.f = animationdata->keyframe_data[key];
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float2:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 2);
							interpolator.f2 = ((const XMFLOAT2*)animationdata->keyframe_data.data())[key];
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float3:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
							interpolator.f3 = ((const XMFLOAT3*)animationdata->keyframe_data.data())[key];
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float4:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4);
							interpolator.f4 = ((const XMFLOAT4*)animationdata->keyframe_data.data())[key];
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Weights:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size());
							for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
							{
								animation.morph_weights_temp[j] = animationdata->keyframe_data[key * animation.morph_weights_temp.size() + j];
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
							t = (animation.timer - left) / (right - left);
						}

						switch (path_data_type)
						{
						default:
						case AnimationComponent::AnimationChannel::PathDataType::Float:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size());
							float vLeft = animationdata->keyframe_data[keyLeft];
							float vRight = animationdata->keyframe_data[keyRight];
							float vAnim = wi::math::Lerp(vLeft, vRight, t);
							interpolator.f = vAnim;
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float2:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 2);
							const XMFLOAT2* data = (const XMFLOAT2*)animationdata->keyframe_data.data();
							XMVECTOR vLeft = XMLoadFloat2(&data[keyLeft]);
							XMVECTOR vRight = XMLoadFloat2(&data[keyRight]);
							XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
							XMStoreFloat2(&interpolator.f2, vAnim);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float3:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3);
							const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
							XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft]);
							XMVECTOR vRight = XMLoadFloat3(&data[keyRight]);
							XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
							XMStoreFloat3(&interpolator.f3, vAnim);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float4:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4);
							const XMFLOAT4* data = (const XMFLOAT4*)animationdata->keyframe_data.data();
							XMVECTOR vLeft = XMLoadFloat4(&data[keyLeft]);
							XMVECTOR vRight = XMLoadFloat4(&data[keyRight]);
							XMVECTOR vAnim = XMQuaternionSlerp(vLeft, vRight, t);
							vAnim = XMQuaternionNormalize(vAnim);
							XMStoreFloat4(&interpolator.f4, vAnim);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Weights:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size());
							for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
							{
								float vLeft = animationdata->keyframe_data[keyLeft * animation.morph_weights_temp.size() + j];
								float vRight = animationdata->keyframe_data[keyRight * animation.morph_weights_temp.size() + j];
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
							t = (animation.timer - left) / (right - left);
						}

						const float t2 = t * t;
						const float t3 = t2 * t;

						switch (path_data_type)
						{
						default:
						case AnimationComponent::AnimationChannel::PathDataType::Float:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size());
							float vLeft = animationdata->keyframe_data[keyLeft * 3 + 1];
							float vLeftTanOut = animationdata->keyframe_data[keyLeft * 3 + 2];
							float vRightTanIn = animationdata->keyframe_data[keyRight * 3 + 0];
							float vRight = animationdata->keyframe_data[keyRight * 3 + 1];
							float vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
							interpolator.f = vAnim;
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float2:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 2 * 3);
							const XMFLOAT2* data = (const XMFLOAT2*)animationdata->keyframe_data.data();
							XMVECTOR vLeft = XMLoadFloat2(&data[keyLeft * 3 + 1]);
							XMVECTOR vLeftTanOut = dt * XMLoadFloat2(&data[keyLeft * 3 + 2]);
							XMVECTOR vRightTanIn = dt * XMLoadFloat2(&data[keyRight * 3 + 0]);
							XMVECTOR vRight = XMLoadFloat2(&data[keyRight * 3 + 1]);
							XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
							XMStoreFloat2(&interpolator.f2, vAnim);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float3:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 3 * 3);
							const XMFLOAT3* data = (const XMFLOAT3*)animationdata->keyframe_data.data();
							XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft * 3 + 1]);
							XMVECTOR vLeftTanOut = dt * XMLoadFloat3(&data[keyLeft * 3 + 2]);
							XMVECTOR vRightTanIn = dt * XMLoadFloat3(&data[keyRight * 3 + 0]);
							XMVECTOR vRight = XMLoadFloat3(&data[keyRight * 3 + 1]);
							XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
							XMStoreFloat3(&interpolator.f3, vAnim);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float4:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * 4 * 3);
							const XMFLOAT4* data = (const XMFLOAT4*)animationdata->keyframe_data.data();
							XMVECTOR vLeft = XMLoadFloat4(&data[keyLeft * 3 + 1]);
							XMVECTOR vLeftTanOut = dt * XMLoadFloat4(&data[keyLeft * 3 + 2]);
							XMVECTOR vRightTanIn = dt * XMLoadFloat4(&data[keyRight * 3 + 0]);
							XMVECTOR vRight = XMLoadFloat4(&data[keyRight * 3 + 1]);
							XMVECTOR vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
							vAnim = XMQuaternionNormalize(vAnim);
							XMStoreFloat4(&interpolator.f4, vAnim);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Weights:
						{
							assert(animationdata->keyframe_data.size() == animationdata->keyframe_times.size() * animation.morph_weights_temp.size() * 3);
							for (size_t j = 0; j < animation.morph_weights_temp.size(); ++j)
							{
								float vLeft = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 1];
								float vLeftTanOut = animationdata->keyframe_data[(keyLeft * animation.morph_weights_temp.size() + j) * 3 + 2];
								float vRightTanIn = animationdata->keyframe_data[(keyRight * animation.morph_weights_temp.size() + j) * 3 + 0];
								float vRight = animationdata->keyframe_data[(keyRight * animation.morph_weights_temp.size() + j) * 3 + 1];
								float vAnim = (2 * t3 - 3 * t2 + 1) * vLeft + (t3 - 2 * t2 + t) * vLeftTanOut + (-2 * t3 + 3 * t2) * vRight + (t3 - t2) * vRightTanIn;
								animation.morph_weights_temp[j] = vAnim;
							}
						}
						break;
						}
					}
					break;
					}
				}

				// The interpolated raw values will be blended on top of component values:
				const float t = animation.amount;

				if (target_transform != nullptr)
				{
					target_transform->SetDirty();

					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::TRANSLATION:
					{
						const XMVECTOR aT = XMLoadFloat3(&target_transform->translation_local);
						const XMVECTOR bT = XMLoadFloat3(&interpolator.f3);
						const XMVECTOR T = XMVectorLerp(aT, bT, t);
						XMStoreFloat3(&target_transform->translation_local, T);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::ROTATION:
					{
						const XMVECTOR aR = XMLoadFloat4(&target_transform->rotation_local);
						const XMVECTOR bR = XMLoadFloat4(&interpolator.f4);
						const XMVECTOR R = XMQuaternionSlerp(aR, bR, t);
						XMStoreFloat4(&target_transform->rotation_local, R);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::SCALE:
					{
						const XMVECTOR aS = XMLoadFloat3(&target_transform->scale_local);
						const XMVECTOR bS = XMLoadFloat3(&interpolator.f3);
						const XMVECTOR S = XMVectorLerp(aS, bS, t);
						XMStoreFloat3(&target_transform->scale_local, S);
					}
					break;
					default:
						break;
					}
				}

				if (target_mesh != nullptr)
				{
					for (size_t j = 0; j < target_mesh->morph_targets.size(); ++j)
					{
						target_mesh->morph_targets[j].weight = wi::math::Lerp(target_mesh->morph_targets[j].weight, animation.morph_weights_temp[j], t);
					}

					target_mesh->dirty_morph = true;
				}

				if (target_light != nullptr)
				{
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
					{
						target_light->color = wi::math::Lerp(target_light->color, interpolator.f3, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
					{
						target_light->intensity = wi::math::Lerp(target_light->intensity, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
					{
						target_light->range = wi::math::Lerp(target_light->range, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
					{
						target_light->innerConeAngle = wi::math::Lerp(target_light->innerConeAngle, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
					{
						target_light->outerConeAngle = wi::math::Lerp(target_light->outerConeAngle, interpolator.f, t);
					}
					break;
					default:
						break;
					}
				}

				if (target_sound != nullptr)
				{
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::SOUND_VOLUME:
					{
						target_sound->volume = wi::math::Lerp(target_sound->volume, interpolator.f, t);
					}
					break;
					default:
						break;
					}
				}

				if (target_emitter != nullptr)
				{
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT:
					{
						target_emitter->count = wi::math::Lerp(target_emitter->count, interpolator.f, t);
					}
					break;
					default:
						break;
					}
				}

				if (target_camera != nullptr)
				{
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::CAMERA_FOV:
					{
						target_camera->fov = wi::math::Lerp(target_camera->fov, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH:
					{
						target_camera->focal_length = wi::math::Lerp(target_camera->focal_length, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE:
					{
						target_camera->aperture_size = wi::math::Lerp(target_camera->aperture_size, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE:
					{
						target_camera->aperture_shape = wi::math::Lerp(target_camera->aperture_shape, interpolator.f2, t);
					}
					break;
					default:
						break;
					}
				}

				if (target_material != nullptr)
				{
					target_material->SetDirty();

					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR:
					{
						target_material->baseColor = wi::math::Lerp(target_material->baseColor, interpolator.f4, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE:
					{
						target_material->baseColor = wi::math::Lerp(target_material->emissiveColor, interpolator.f4, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS:
					{
						target_material->roughness = wi::math::Lerp(target_material->roughness, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS:
					{
						target_material->metalness = wi::math::Lerp(target_material->metalness, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE:
					{
						target_material->reflectance = wi::math::Lerp(target_material->reflectance, interpolator.f, t);
					}
					break;
					case AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD:
					{
						target_material->texMulAdd = wi::math::Lerp(target_material->texMulAdd, interpolator.f4, t);
					}
					break;
					default:
						break;
					}
				}

			}

			if (animation.IsPlaying())
			{
				animation.timer += dt * animation.speed;
			}

			if (animation.IsLooped() && animation.timer > animation.end)
			{
				animation.timer = animation.start;
				for (auto& channel : animation.channels)
				{
					channel.next_event = 0;
				}
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
	void Scene::RunExpressionUpdateSystem(wi::jobsystem::context& ctx)
	{
		for (size_t i = 0; i < expressions.GetCount(); ++i)
		{
			ExpressionComponent& expression_mastering = expressions[i];

			// Procedural blink:
			expression_mastering.blink_timer += expression_mastering.blink_frequency * dt;
			if (expression_mastering.blink_timer >= 1)
			{
				int blink = expression_mastering.presets[(int)ExpressionComponent::Preset::Blink];
				if (blink >= 0 && blink < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[blink];
					expression_mastering.blink_count = std::max(1, expression_mastering.blink_count);
					float one_blink_length = expression_mastering.blink_length * expression_mastering.blink_frequency;
					float all_blink_length = one_blink_length * (float)expression_mastering.blink_count;
					float blink_index = std::floor(wi::math::Lerp(0, (float)expression_mastering.blink_count, (expression_mastering.blink_timer - 1) / all_blink_length));
					float blink_trim = 1 + one_blink_length * blink_index;
					float blink_state = wi::math::InverseLerp(0, one_blink_length, expression_mastering.blink_timer - blink_trim);
					if (blink_state < 0.5f)
					{
						// closing
						expression.weight = wi::math::Lerp(0, 1, wi::math::saturate(blink_state * 2));
					}
					else
					{
						// opening
						expression.weight = wi::math::Lerp(1, 0, wi::math::saturate((blink_state - 0.5f) * 2));
					}
					if (expression_mastering.blink_timer >= 1 + all_blink_length)
					{
						expression.weight = 0;
						expression_mastering.blink_timer = 0;
					}
					expression.SetDirty();
				}
			}

			// Procedural look:
			if (expression_mastering.look_timer == 0)
			{
				// Roll new random look direction for next look away event:
				float vertical = wi::random::GetRandom(-1.0f, 1.0f);
				float horizontal = wi::random::GetRandom(-1.0f, 1.0f);
				expression_mastering.look_weights[0] = wi::math::saturate(vertical);
				expression_mastering.look_weights[1] = wi::math::saturate(-vertical);
				expression_mastering.look_weights[2] = wi::math::saturate(horizontal);
				expression_mastering.look_weights[3] = wi::math::saturate(-horizontal);
			}
			expression_mastering.look_timer += expression_mastering.look_frequency * dt;
			if (expression_mastering.look_timer >= 1)
			{
				int looks[] = {
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookDown],
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookUp],
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookLeft],
					expression_mastering.presets[(int)ExpressionComponent::Preset::LookRight],
				};
				for (int idx = 0; idx<arraysize(looks); ++idx)
				{
					int look = looks[idx];
					const float weight = expression_mastering.look_weights[idx];
					if (look >= 0 && look < expression_mastering.expressions.size())
					{
						ExpressionComponent::Expression& expression = expression_mastering.expressions[look];
						float look_state = wi::math::InverseLerp(0, expression_mastering.look_length * expression_mastering.look_frequency, expression_mastering.look_timer - 1);
						if (look_state < 0.25f)
						{
							expression.weight = wi::math::Lerp(0, weight, wi::math::saturate(look_state * 4));
						}
						else
						{
							expression.weight = wi::math::Lerp(weight, 0, wi::math::saturate((look_state - 0.75f) * 4));
						}
						expression.SetDirty();
					}
				}
				if (expression_mastering.look_timer >= 1 + expression_mastering.look_length * expression_mastering.look_frequency)
				{
					expression_mastering.look_timer = 0;
				}
			}

			float overrideMouthBlend = 0;
			float overrideBlinkBlend = 0;
			float overrideLookBlend = 0;

			// Pass 1: reset targets that will be modified by expressions:
			//	Also accumulate override weights
			for(ExpressionComponent::Expression& expression : expression_mastering.expressions)
			{
				const float blend = expression.IsBinary() ? (expression.weight > 0 ? 1 : 0) : expression.weight;
				if (expression.override_mouth == ExpressionComponent::Override::Block)
				{
					overrideMouthBlend += 1;
				}
				if (expression.override_mouth == ExpressionComponent::Override::Blend)
				{
					overrideMouthBlend += blend;
				}
				if (expression.override_blink == ExpressionComponent::Override::Block)
				{
					overrideBlinkBlend += 1;
				}
				if (expression.override_blink == ExpressionComponent::Override::Blend)
				{
					overrideBlinkBlend += blend;
				}
				if (expression.override_look == ExpressionComponent::Override::Block)
				{
					overrideLookBlend += 1;
				}
				if (expression.override_look == ExpressionComponent::Override::Blend)
				{
					overrideLookBlend += blend;
				}

				if (!expression.IsDirty())
					continue;

				for (const ExpressionComponent::Expression::MorphTargetBinding& morph_target_binding : expression.morph_target_bindings)
				{
					MeshComponent* mesh = meshes.GetComponent(morph_target_binding.meshID);
					if (mesh != nullptr && (int)mesh->morph_targets.size() > morph_target_binding.index)
					{
						MeshComponent::MorphTarget& morph_target = mesh->morph_targets[morph_target_binding.index];
						if (morph_target.weight > 0)
						{
							morph_target.weight = 0;
						}
					}
				}
			}

			// Override weights are factored in:
			const int mouths[] = {
				expression_mastering.presets[(int)ExpressionComponent::Preset::Aa],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Ih],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Ou],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Ee],
				expression_mastering.presets[(int)ExpressionComponent::Preset::Oh],
			};
			for (int mouth : mouths)
			{
				if (mouth >= 0 && mouth < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[mouth];
					expression.weight *= 1 - wi::math::saturate(overrideMouthBlend);
				}
			}
			const int blinks[] = {
				expression_mastering.presets[(int)ExpressionComponent::Preset::Blink],
				expression_mastering.presets[(int)ExpressionComponent::Preset::BlinkLeft],
				expression_mastering.presets[(int)ExpressionComponent::Preset::BlinkRight],
			};
			for (int blink : blinks)
			{
				if (blink >= 0 && blink < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[blink];
					expression.weight *= 1 - wi::math::saturate(overrideBlinkBlend);
				}
			}
			const int looks[] = {
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookUp],
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookDown],
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookLeft],
				expression_mastering.presets[(int)ExpressionComponent::Preset::LookRight],
			};
			for (int look : looks)
			{
				if (look >= 0 && look < expression_mastering.expressions.size())
				{
					ExpressionComponent::Expression& expression = expression_mastering.expressions[look];
					expression.weight *= 1 - wi::math::saturate(overrideLookBlend);
				}
			}

			// Pass 2: apply expressions:
			for (ExpressionComponent::Expression& expression : expression_mastering.expressions)
			{
				if (!expression.IsDirty())
					continue;

				expression.SetDirty(false);
				const float blend = expression.IsBinary() ? (expression.weight > 0 ? 1 : 0) : expression.weight;

				for (const ExpressionComponent::Expression::MorphTargetBinding& morph_target_binding : expression.morph_target_bindings)
				{
					MeshComponent* mesh = meshes.GetComponent(morph_target_binding.meshID);
					if (mesh != nullptr && (int)mesh->morph_targets.size() > morph_target_binding.index)
					{
						MeshComponent::MorphTarget& morph_target = mesh->morph_targets[morph_target_binding.index];
						morph_target.weight = wi::math::Lerp(morph_target.weight, morph_target_binding.weight, blend);
						mesh->dirty_morph = true;
					}
				}
			}
		}
	}
	void Scene::RunColliderUpdateSystem(wi::jobsystem::context& ctx)
	{
		for (size_t i = 0; i < colliders.GetCount(); ++i)
		{
			ColliderComponent& collider = colliders[i];
			Entity entity = colliders.GetEntity(i);
			const TransformComponent* transform = transforms.GetComponent(entity);
			if (transform == nullptr)
				continue;

			XMFLOAT3 scale = transform->GetScale();
			collider.sphere.radius = collider.radius * std::max(scale.x, std::max(scale.y, scale.z));
			collider.capsule.radius = collider.sphere.radius;

			XMMATRIX W = XMLoadFloat4x4(&transform->world);
			XMVECTOR offset = XMLoadFloat3(&collider.offset);
			XMVECTOR tail = XMLoadFloat3(&collider.tail);
			offset = XMVector3Transform(offset, W);
			tail = XMVector3Transform(tail, W);

			XMStoreFloat3(&collider.sphere.center, offset);
			XMVECTOR N = XMVector3Normalize(offset - tail);
			offset += N * collider.capsule.radius;
			tail -= N * collider.capsule.radius;
			XMStoreFloat3(&collider.capsule.base, offset);
			XMStoreFloat3(&collider.capsule.tip, tail);

			if (collider.shape == ColliderComponent::Shape::Plane)
			{
				collider.planeOrigin = collider.sphere.center;
				XMVECTOR N = XMVectorSet(0, 1, 0, 0);
				N = XMVector3Normalize(XMVector3TransformNormal(N, W));
				XMStoreFloat3(&collider.planeNormal, N);

				XMMATRIX PLANE = XMMatrixScaling(collider.radius, 1, collider.radius);
				PLANE = PLANE * XMMatrixTranslationFromVector(XMLoadFloat3(&collider.offset));
				PLANE = PLANE * W;
				PLANE = XMMatrixInverse(nullptr, PLANE);
				XMStoreFloat4x4(&collider.planeProjection, PLANE);
			}
		}
	}
	void Scene::RunSpringUpdateSystem(wi::jobsystem::context& ctx)
	{
		static float time = 0;
		time += dt;
		const XMVECTOR windDir = XMLoadFloat3(&weather.windDirection);

		for (size_t i = 0; i < springs.GetCount(); ++i)
		{
			SpringComponent& spring = springs[i];
			if (spring.IsDisabled())
			{
				continue;
			}
			Entity entity = springs.GetEntity(i);
			size_t transform_index = transforms.GetIndex(entity);
			if (transform_index == ~0ull)
			{
				continue;
			}
			TransformComponent& transform = transforms[transform_index];

			//XMVECTOR rotation_local = XMLoadFloat4(&transform.rotation_local);
			XMVECTOR rotation_parent_world = XMQuaternionIdentity();
			XMMATRIX parentWorldMatrix = XMMatrixIdentity();

			const HierarchyComponent* hier = hierarchy.GetComponent(entity);
			size_t parent_index = hier == nullptr ? ~0ull : transforms.GetIndex(hier->parentID);
			if (parent_index != ~0ull)
			{
				// Spring hierarchy resolve depends on spring component order!
				//	It works best when parent spring is located before child spring!
				//	It will work the other way, but results will be less convincing
				const TransformComponent& parent_transform = transforms[parent_index];
				transform.UpdateTransform_Parented(parent_transform);
				rotation_parent_world = parent_transform.GetRotationV();
				parentWorldMatrix = XMLoadFloat4x4(&parent_transform.world);
			}

			XMVECTOR position_root = transform.GetPositionV();
			//XMVECTOR rotation_combined = XMQuaternionNormalize(XMQuaternionMultiply(rotation_parent_world, rotation_local));

			if (spring.IsResetting() && dt > 0)
			{
				spring.Reset(false);

				XMVECTOR tail = position_root + XMVectorSet(0, 1, 0, 0);
				// Search for child to find the rest pose tail position:
				bool child_found = false;
				for (size_t j = 0; j < hierarchy.GetCount(); ++j)
				{
					const HierarchyComponent& hier = hierarchy[j];
					Entity child = hierarchy.GetEntity(j);
					if (hier.parentID == entity && transforms.Contains(child))
					{
						const TransformComponent& child_transform = *transforms.GetComponent(child);
						tail = child_transform.GetPositionV();
						child_found = true;
						break;
					}
				}
				if (!child_found)
				{
					// No child, try to guess tail position compared to parent (if it has parent):
					const HierarchyComponent* hier = hierarchy.GetComponent(entity);
					if (hier != nullptr && transforms.Contains(hier->parentID))
					{
						const TransformComponent& parent_transform = *transforms.GetComponent(hier->parentID);
						XMVECTOR ab = position_root - parent_transform.GetPositionV();
						tail = position_root + ab;
					}
				}
				XMVECTOR axis = tail - position_root;
				XMVECTOR length = XMVector3Length(axis);
				//axis = XMVector3Rotate(axis, XMQuaternionNormalize(XMQuaternionInverse(rotation_combined)));
				axis /= length;
				XMStoreFloat3(&spring.boneAxis, axis);
				XMStoreFloat3(&spring.currentTail, tail);
				spring.prevTail = spring.currentTail;
				spring.boneLength = XMVectorGetX(length);
			}

			XMVECTOR boneAxis = XMLoadFloat3(&spring.boneAxis);
			//boneAxis = XMVector3Normalize(XMVector3Rotate(boneAxis, rotation_combined));

			const float boneLength = spring.boneLength;
			const float dragForce = spring.dragForce;
			const float stiffnessForce = spring.stiffnessForce;
			const XMVECTOR gravityDir = XMLoadFloat3(&spring.gravityDir);
			const float gravityPower = spring.gravityPower;

#if 0
			// Debug axis:
			wi::renderer::RenderableLine line;
			line.color_start = line.color_end = XMFLOAT4(1, 1, 0, 1);
			XMStoreFloat3(&line.start, position_root);
			XMStoreFloat3(&line.end, position_root + boneAxis * boneLength);
			wi::renderer::DrawLine(line);
#endif

			const XMVECTOR tail_current = XMLoadFloat3(&spring.currentTail);
			const XMVECTOR tail_prev = XMLoadFloat3(&spring.prevTail);

			XMVECTOR inertia = (tail_current - tail_prev) * (1 - dragForce);
			XMVECTOR stiffness = boneAxis * stiffnessForce;
			XMVECTOR external = XMVectorZero();

			if (spring.windForce > 0)
			{
				external += std::sin(time * weather.windSpeed + XMVectorGetX(XMVector3Dot(tail_current, windDir))) * windDir * spring.windForce;
			}
			if (spring.IsGravityEnabled())
			{
				external += gravityDir * gravityPower;
			}

			XMVECTOR tail_next = tail_current + inertia + dt * (stiffness + external);
			XMVECTOR to_tail = XMVector3Normalize(tail_next - position_root);

			if (!spring.IsStretchEnabled())
			{
				// Limit offset to keep distance from parent:
				tail_next = position_root + to_tail * boneLength;
			}

#if 1
			// Collider checks:
			//	apply scaling to radius:
			XMFLOAT3 scale = transform.GetScale();
			const float hitRadius = spring.hitRadius * std::max(scale.x, std::max(scale.y, scale.z));

			for (size_t collider_index = 0; collider_index < colliders.GetCount(); ++collider_index)
			{
				const ColliderComponent& collider = colliders[collider_index];

				wi::primitive::Sphere tail_sphere;
				XMStoreFloat3(&tail_sphere.center, tail_next); // tail_sphere center can change within loop!
				tail_sphere.radius = hitRadius;
				float dist = 0;
				XMFLOAT3 direction = {};
				switch (collider.shape)
				{
				default:
				case ColliderComponent::Shape::Sphere:
					tail_sphere.intersects(collider.sphere, dist, direction);
					break;
				case ColliderComponent::Shape::Capsule:
					tail_sphere.intersects(collider.capsule, dist, direction);
					break;
				case ColliderComponent::Shape::Plane:
					dist = wi::math::GetPlanePointDistance(XMLoadFloat3(&collider.planeOrigin), XMLoadFloat3(&collider.planeNormal), tail_next);
					direction = collider.planeNormal;
					if (dist < 0)
					{
						direction.x *= -1;
						direction.y *= -1;
						direction.z *= -1;
						dist = std::abs(dist);
					}
					dist = dist - tail_sphere.radius;
					if (dist < 0)
					{
						XMMATRIX planeProjection = XMLoadFloat4x4(&collider.planeProjection);
						XMVECTOR clipSpacePos = XMVector3Transform(tail_next, planeProjection);
						XMVECTOR uvw = clipSpacePos * XMVectorSet(0.5f, -0.5f, 0.5f, 1) + XMVectorSet(0.5f, 0.5f, 0.5f, 0);
						XMVECTOR uvw_sat = XMVectorSaturate(uvw);
						if (std::abs(XMVectorGetX(uvw) - XMVectorGetX(uvw_sat)) > std::numeric_limits<float>::epsilon())
							dist = 1; // force no collision
						else if (std::abs(XMVectorGetY(uvw) - XMVectorGetY(uvw_sat)) > std::numeric_limits<float>::epsilon())
							dist = 1; // force no collision
						else if (std::abs(XMVectorGetZ(uvw) - XMVectorGetZ(uvw_sat)) > std::numeric_limits<float>::epsilon())
							dist = 1; // force no collision
					}
					break;
				}

				if (dist < 0)
				{
					tail_next = tail_next - XMLoadFloat3(&direction) * dist;
					to_tail = XMVector3Normalize(tail_next - position_root);

					if (!spring.IsStretchEnabled())
					{
						// Limit offset to keep distance from parent:
						tail_next = position_root + to_tail * boneLength;
					}
				}
			}
#endif

			XMStoreFloat3(&spring.prevTail, tail_current);
			XMStoreFloat3(&spring.currentTail, tail_next);

			// Rotate to face tail position:
			const XMVECTOR axis = XMVector3Normalize(XMVector3Cross(boneAxis, to_tail));
			const float angle = XMScalarACos(XMVectorGetX(XMVector3Dot(boneAxis, to_tail)));
			const XMVECTOR Q = XMQuaternionNormalize(XMQuaternionRotationNormal(axis, angle));
			TransformComponent tmp = transform;
			tmp.ApplyTransform();
			tmp.Rotate(Q);
			tmp.UpdateTransform();
			transform.world = tmp.world; // only store world space result, not modifying actual local space!

		}
	}
	void Scene::RunInverseKinematicsUpdateSystem(wi::jobsystem::context& ctx)
	{
		if (inverse_kinematics.GetCount() > 0)
		{
			transforms_temp.resize(transforms.GetCount());
			transforms_temp = transforms.GetComponentArray(); // make copy
		}

		bool recompute_hierarchy = false;
		for (size_t i = 0; i < inverse_kinematics.GetCount(); ++i)
		{
			const InverseKinematicsComponent& ik = inverse_kinematics[i];
			if (ik.IsDisabled())
			{
				continue;
			}
			Entity entity = inverse_kinematics.GetEntity(i);
			size_t transform_index = transforms.GetIndex(entity);
			size_t target_index = transforms.GetIndex(ik.target);
			const HierarchyComponent* hier = hierarchy.GetComponent(entity);
			if (transform_index == ~0ull || target_index == ~0ull || hier == nullptr)
			{
				continue;
			}
			TransformComponent& transform = transforms_temp[transform_index];
			TransformComponent& target = transforms_temp[target_index];

			const XMVECTOR target_pos = target.GetPositionV();
			for (uint32_t iteration = 0; iteration < ik.iteration_count; ++iteration)
			{
				TransformComponent* stack[32] = {};
				Entity parent_entity = hier->parentID;
				TransformComponent* child_transform = &transform;
				for (uint32_t chain = 0; chain < std::min(ik.chain_length, (uint32_t)arraysize(stack)); ++chain)
				{
					recompute_hierarchy = true; // any IK will trigger a full transform hierarchy recompute step at the end(**)

					// stack stores all traversed chain links so far:
					stack[chain] = child_transform;

					// Compute required parent rotation that moves ik transform closer to target transform:
					size_t parent_index = transforms.GetIndex(parent_entity);
					if (parent_index == ~0ull)
						continue;
					TransformComponent& parent_transform = transforms_temp[parent_index];
					const XMVECTOR parent_pos = parent_transform.GetPositionV();
					const XMVECTOR dir_parent_to_ik = XMVector3Normalize(transform.GetPositionV() - parent_pos);
					const XMVECTOR dir_parent_to_target = XMVector3Normalize(target_pos - parent_pos);
					const XMVECTOR axis = XMVector3Normalize(XMVector3Cross(dir_parent_to_ik, dir_parent_to_target));
					const float angle = XMScalarACos(XMVectorGetX(XMVector3Dot(dir_parent_to_ik, dir_parent_to_target)));
					const XMVECTOR Q = XMQuaternionNormalize(XMQuaternionRotationNormal(axis, angle));

					// parent to world space:
					parent_transform.ApplyTransform();
					// rotate parent:
					parent_transform.Rotate(Q);
					parent_transform.UpdateTransform();
					// parent back to local space (if parent has parent):
					const HierarchyComponent* hier_parent = hierarchy.GetComponent(parent_entity);
					if (hier_parent != nullptr)
					{
						Entity parent_of_parent_entity = hier_parent->parentID;
						size_t parent_of_parent_index = transforms.GetIndex(parent_of_parent_entity);
						if (parent_of_parent_index != ~0ull)
						{
							const TransformComponent* transform_parent_of_parent = &transforms_temp[parent_of_parent_index];
							XMMATRIX parent_of_parent_inverse = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent_of_parent->world));
							parent_transform.MatrixTransform(parent_of_parent_inverse);
							// Do not call UpdateTransform() here, to keep parent world matrix in world space!
						}
					}

					// update chain from parent to children:
					const TransformComponent* recurse_parent = &parent_transform;
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
					child_transform = &parent_transform;
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

				size_t transform_index = transforms.GetIndex(entity);
				size_t parent_index = transforms.GetIndex(parentcomponent.parentID);
				if (transform_index != ~0ull && parent_index != ~0ull)
				{
					TransformComponent* transform_child = &transforms_temp[transform_index];
					TransformComponent* transform_parent = &transforms_temp[parent_index];
					transform_child->UpdateTransform_Parented(*transform_parent);
				}
			}
		}

		if (inverse_kinematics.GetCount() > 0)
		{
			for (size_t i = 0; i < transforms.GetCount(); ++i)
			{
				// IK shouldn't modify local space, so only update the world matrices!
				transforms[i].world = transforms_temp[i].world;
			}
		}
	}
	void Scene::RunArmatureUpdateSystem(wi::jobsystem::context& ctx)
	{
		wi::jobsystem::Dispatch(ctx, (uint32_t)armatures.GetCount(), 1, [&](wi::jobsystem::JobArgs args) {

			ArmatureComponent& armature = armatures[args.jobIndex];
			Entity entity = armatures.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;
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
				const TransformComponent* bone = transforms.GetComponent(boneEntity);
				if (bone == nullptr)
					continue;

				XMMATRIX B = XMLoadFloat4x4(&armature.inverseBindMatrices[boneIndex]);
				XMMATRIX W = XMLoadFloat4x4(&bone->world);
				XMMATRIX M = B * W * R;

				XMFLOAT4X4 mat;
				XMStoreFloat4x4(&mat, M);
				armature.boneData[boneIndex++].Create(mat);

				const float bone_radius = 1;
				XMFLOAT3 bonepos = bone->GetPosition();
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

			if (!mesh.streamoutBuffer.IsValid())
			{
				const SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(entity);
				if (softbody != nullptr && wi::physics::IsSimulationEnabled())
				{
					mesh.CreateStreamoutRenderData();
				}
			}

			if (mesh.so_pos_nor_wind.IsValid() && mesh.so_pre.IsValid())
			{
				std::swap(mesh.so_pos_nor_wind, mesh.so_pre);
			}

			mesh._flags &= ~MeshComponent::TLAS_FORCE_DOUBLE_SIDED;

			// Update morph targets if needed:
			if (mesh.dirty_morph && !mesh.morph_targets.empty())
			{
			    XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
			    XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

				mesh.morph_temp_pos = mesh.vertex_positions;
				mesh.morph_temp_nor = mesh.vertex_normals;

				for (const MeshComponent::MorphTarget& morph : mesh.morph_targets)
				{
					if (morph.weight <= 0)
						continue;
					if (morph.sparse_indices.empty())
					{
						for (size_t i = 0; i < morph.vertex_positions.size(); ++i)
						{
							mesh.morph_temp_pos[i].x += morph.weight * morph.vertex_positions[i].x;
							mesh.morph_temp_pos[i].y += morph.weight * morph.vertex_positions[i].y;
							mesh.morph_temp_pos[i].z += morph.weight * morph.vertex_positions[i].z;

							if (!morph.vertex_normals.empty())
							{
								mesh.morph_temp_nor[i].x += morph.weight * morph.vertex_normals[i].x;
								mesh.morph_temp_nor[i].y += morph.weight * morph.vertex_normals[i].y;
								mesh.morph_temp_nor[i].z += morph.weight * morph.vertex_normals[i].z;
							}
						}
					}
					else
					{
						for (size_t i = 0; i < morph.sparse_indices.size(); ++i)
						{
							const uint32_t ind = morph.sparse_indices[i];
							mesh.morph_temp_pos[ind].x += morph.weight * morph.vertex_positions[i].x;
							mesh.morph_temp_pos[ind].y += morph.weight * morph.vertex_positions[i].y;
							mesh.morph_temp_pos[ind].z += morph.weight * morph.vertex_positions[i].z;

							if (!morph.vertex_normals.empty())
							{
								mesh.morph_temp_nor[ind].x += morph.weight * morph.vertex_normals[i].x;
								mesh.morph_temp_nor[ind].y += morph.weight * morph.vertex_normals[i].y;
								mesh.morph_temp_nor[ind].z += morph.weight * morph.vertex_normals[i].z;
							}
						}
					}
				}

			    for (size_t i = 0; i < mesh.morph_temp_pos.size(); ++i)
			    {
					XMFLOAT3 pos = mesh.morph_temp_pos[i];
					XMFLOAT3 nor = mesh.morph_temp_nor.empty() ? XMFLOAT3(1, 1, 1) : mesh.morph_temp_nor[i];
					const uint8_t wind = mesh.vertex_windweights.empty() ? 0xFF : mesh.vertex_windweights[i];

					XMStoreFloat3(&nor, XMVector3Normalize(XMLoadFloat3(&nor)));
					mesh.vertex_positions_morphed[i].FromFULL(pos, nor, wind);

					_min = wi::math::Min(_min, pos);
					_max = wi::math::Max(_max, pos);
			    }

			    mesh.aabb = AABB(_min, _max);
			}

			ShaderGeometry geometry;
			geometry.init();
			geometry.ib = mesh.ib.descriptor_srv;
			if (mesh.so_pos_nor_wind.IsValid())
			{
				geometry.vb_pos_nor_wind = mesh.so_pos_nor_wind.descriptor_srv;
			}
			else
			{
				geometry.vb_pos_nor_wind = mesh.vb_pos_nor_wind.descriptor_srv;
			}
			if (mesh.so_tan.IsValid())
			{
				geometry.vb_tan = mesh.so_tan.descriptor_srv;
			}
			else
			{
				geometry.vb_tan = mesh.vb_tan.descriptor_srv;
			}
			geometry.vb_col = mesh.vb_col.descriptor_srv;
			geometry.vb_uvs = mesh.vb_uvs.descriptor_srv;
			geometry.vb_atl = mesh.vb_atl.descriptor_srv;
			geometry.vb_pre = mesh.so_pre.descriptor_srv;
			geometry.aabb_min = mesh.aabb._min;
			geometry.aabb_max = mesh.aabb._max;
			geometry.tessellation_factor = mesh.tessellationFactor;

			const ImpostorComponent* impostor = impostors.GetComponent(entity);
			if (impostor != nullptr && impostor->textureIndex >= 0)
			{
				geometry.impostorSliceOffset = impostor->textureIndex * impostorCaptureAngles * 3;
			}

			if (mesh.IsDoubleSided())
			{
				geometry.flags |= SHADERMESH_FLAG_DOUBLE_SIDED;
			}

			mesh.meshletCount = 0;

			uint32_t subsetIndex = 0;
			for (auto& subset : mesh.subsets)
			{
				const MaterialComponent* material = materials.GetComponent(subset.materialID);
				if (material != nullptr)
				{
					subset.materialIndex = (uint32_t)materials.GetIndex(subset.materialID);
					const uint32_t lod_index = mesh.subsets_per_lod > 0 ? subsetIndex / mesh.subsets_per_lod : 0;
					if (lod_index == 0 && mesh.BLAS.IsValid())
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
						if (flags != geometry.flags || mesh.dirty_morph)
						{
							mesh.BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;
						}
						if (mesh.streamoutBuffer.IsValid())
						{
							mesh.BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;
							geometry.triangles.vertex_buffer = mesh.streamoutBuffer;
							geometry.triangles.vertex_byte_offset = mesh.so_pos_nor_wind.offset;
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

				geometry.indexOffset = subset.indexOffset;
				geometry.materialIndex = subset.materialIndex;
				geometry.meshletOffset = mesh.meshletCount;
				geometry.meshletCount = triangle_count_to_meshlet_count(subset.indexCount / 3u);
				mesh.meshletCount += geometry.meshletCount;
				std::memcpy(geometryArrayMapped + mesh.geometryOffset + subsetIndex, &geometry, sizeof(geometry));
				subsetIndex++;
			}

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
				if (material.IsOutlineEnabled())
				{
					material.engineStencilRef = STENCILREF_CUSTOMSHADER_OUTLINE;
				}
				else
				{
					material.engineStencilRef = STENCILREF_CUSTOMSHADER;
				}
			}
			else if (material.IsOutlineEnabled())
			{
				material.engineStencilRef = STENCILREF_OUTLINE;
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
			desc.misc_flags = ResourceMiscFlag::TRANSIENT_ATTACHMENT;
			device->CreateTexture(&desc, nullptr, &impostorDepthStencil);
			device->SetName(&impostorDepthStencil, "impostorDepthStencil");

			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			desc.array_size = maxImpostorCount * impostorCaptureAngles * 3;
			desc.format = Format::R8G8B8A8_UNORM;
			desc.layout = ResourceState::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::NONE;
			device->CreateTexture(&desc, nullptr, &impostorArray);
			device->SetName(&impostorArray, "impostorArray");

			for (uint32_t i = 0; i < desc.array_size; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&impostorArray, SubresourceType::RTV, i, 1, 0, 1);
				assert(subresource_index == i);
			}

			renderpasses_impostor.resize(desc.array_size / 3);
			for (uint32_t i = 0; i < desc.array_size / 3; ++i)
			{
				RenderPassDesc renderpassdesc;
				renderpassdesc.attachments.push_back(
					RenderPassAttachment::RenderTarget(
						&impostorArray,
						RenderPassAttachment::LoadOp::CLEAR,
						RenderPassAttachment::StoreOp::STORE,
						ResourceState::RENDERTARGET,
						ResourceState::RENDERTARGET,
						ResourceState::RENDERTARGET
					)
				);
				renderpassdesc.attachments.back().subresource = i * 3;

				renderpassdesc.attachments.push_back(
					RenderPassAttachment::RenderTarget(
						&impostorArray,
						RenderPassAttachment::LoadOp::CLEAR,
						RenderPassAttachment::StoreOp::STORE,
						ResourceState::RENDERTARGET,
						ResourceState::RENDERTARGET,
						ResourceState::RENDERTARGET
					)
				);
				renderpassdesc.attachments.back().subresource = i * 3 + 1;

				renderpassdesc.attachments.push_back(
					RenderPassAttachment::RenderTarget(
						&impostorArray,
						RenderPassAttachment::LoadOp::CLEAR,
						RenderPassAttachment::StoreOp::STORE,
						ResourceState::RENDERTARGET,
						ResourceState::RENDERTARGET,
						ResourceState::RENDERTARGET
					)
				);
				renderpassdesc.attachments.back().subresource = i * 3 + 2;

				renderpassdesc.attachments.push_back(
					RenderPassAttachment::DepthStencil(
						&impostorDepthStencil,
						RenderPassAttachment::LoadOp::CLEAR,
						RenderPassAttachment::StoreOp::DONTCARE
					)
				);

				device->CreateRenderPass(&renderpassdesc, &renderpasses_impostor[i]);
			}
		}

		// reconstruct impostor array status:
		bool impostorTaken[maxImpostorCount] = {};
		for (size_t i = 0; i < impostors.GetCount(); ++i)
		{
			ImpostorComponent& impostor = impostors[i];
			if (impostor.textureIndex >= 0 && impostor.textureIndex < maxImpostorCount)
			{
				impostorTaken[impostor.textureIndex] = true;
			}
			else
			{
				impostor.textureIndex = -1;
			}
		}

		for (size_t i = 0; i < impostors.GetCount(); ++i)
		{
			ImpostorComponent& impostor = impostors[i];

			if (impostor.IsDirty())
			{
				impostor.SetDirty(false);
				impostor.render_dirty = true;
			}

			if (impostor.render_dirty && impostor.textureIndex < 0)
			{
				// need to take a free impostor texture slot:
				for (int i = 0; i < arraysize(impostorTaken); ++i)
				{
					if (impostorTaken[i] == false)
					{
						impostorTaken[i] = true;
						impostor.textureIndex = i;
						break;
					}
				}
			}
		}

		if (impostors.GetCount() > 0)
		{
			ShaderMaterial material;
			material.init();
			material.shaderType = ~0u;
			std::memcpy(materialArrayMapped + impostorMaterialOffset, &material, sizeof(material));

			ShaderGeometry geometry;
			geometry.init();
			geometry.meshletCount = triangle_count_to_meshlet_count(uint32_t(objects.GetCount()) * 2);
			geometry.meshletOffset = 0; // local meshlet offset
			geometry.ib = impostor_ib.descriptor_srv;
			geometry.vb_pos_nor_wind = impostor_vb.descriptor_srv;
			geometry.materialIndex = impostorMaterialOffset;
			std::memcpy(geometryArrayMapped + impostorGeometryOffset, &geometry, sizeof(geometry));

			ShaderMeshInstance instance;
			instance.init();
			instance.geometryOffset = impostorGeometryOffset;
			instance.geometryCount = 1;
			instance.meshletOffset = meshletAllocator.fetch_add(geometry.meshletCount); // global meshlet offset
			std::memcpy(instanceArrayMapped + impostorInstanceOffset, &instance, sizeof(instance));
		}
	}
	void Scene::RunObjectUpdateSystem(wi::jobsystem::context& ctx)
	{
		assert(objects.GetCount() == aabb_objects.GetCount());

		meshletAllocator.store(0u);

		parallel_bounds.clear();
		parallel_bounds.resize((size_t)wi::jobsystem::DispatchGroupCount((uint32_t)objects.GetCount(), small_subtask_groupsize));
		
		wi::jobsystem::Dispatch(ctx, (uint32_t)objects.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			Entity entity = objects.GetEntity(args.jobIndex);
			ObjectComponent& object = objects[args.jobIndex];
			AABB& aabb = aabb_objects[args.jobIndex];

			// Update occlusion culling status:
			if (!wi::renderer::GetFreezeCullingCameraEnabled())
			{
				object.occlusionHistory <<= 1u; // advance history by 1 frame
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
			object.SetRequestPlanarReflection(false);
			object.fadeDistance = object.draw_distance;

			if (object.meshID != INVALID_ENTITY && meshes.Contains(object.meshID) && transforms.Contains(entity))
			{
				// These will only be valid for a single frame:
				object.mesh_index = (uint32_t)meshes.GetIndex(object.meshID);

				uint32_t transform_index = (uint32_t)transforms.GetIndex(entity);
				const TransformComponent& transform = transforms[transform_index];

				if (object.mesh_index != ~0u)
				{
					const MeshComponent& mesh = meshes[object.mesh_index];

					XMMATRIX W = XMLoadFloat4x4(&transform.world);
					aabb = mesh.aabb.transform(W);

					// This is instance bounding box matrix:
					XMFLOAT4X4 meshMatrix;
					XMStoreFloat4x4(&meshMatrix, mesh.aabb.getAsBoxMatrix() * W);

					// We need sometimes the center of the instance bounding box, not the transform position (which can be outside the bounding box)
					object.center = *((XMFLOAT3*)&meshMatrix._41);

					if (mesh.IsSkinned() || mesh.IsDynamic())
					{
						object.SetDynamic(true);
						const ArmatureComponent* armature = armatures.GetComponent(mesh.armatureID);
						if (armature != nullptr)
						{
							aabb = AABB::Merge(aabb, armature->aabb);
						}
					}

					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh.GetLODSubsetRange(0, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
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
						object.fadeDistance = std::min(object.fadeDistance, impostor->swapInDistance);
					}

					SoftBodyPhysicsComponent* softbody = softbodies.GetComponent(object.meshID);
					if (softbody != nullptr && mesh.streamoutBuffer.IsValid())
					{
						if (wi::physics::IsSimulationEnabled())
						{
							// this will be registered as soft body in the next physics update
							softbody->_flags |= SoftBodyPhysicsComponent::SAFE_TO_REGISTER;

							// soft body manipulated with the object matrix
							softbody->worldMatrix = transform.world;

							if (softbody->graphicsToPhysicsVertexMapping.empty())
							{
								softbody->CreateFromMesh(mesh);
							}
						}

						// simulation aabb will be used for soft bodies
						aabb = softbody->aabb;

						// soft bodies have no transform, their vertices are simulated in world space
						transform_index = ~0u;
					}

					object.center = aabb.getCenter();
					object.radius = aabb.getRadius();

					// Create GPU instance data:
					GraphicsDevice* device = wi::graphics::GetDevice();
					ShaderMeshInstance inst;
					inst.init();
					inst.transformPrev.Create(object.worldMatrix);
					object.worldMatrix = transform_index == ~0u ? wi::math::IDENTITY_MATRIX : transforms[transform_index].world;
					inst.transform.Create(object.worldMatrix);

					XMMATRIX worldMatrixInverseTranspose = XMLoadFloat4x4(&object.worldMatrix);
					worldMatrixInverseTranspose = XMMatrixInverse(nullptr, worldMatrixInverseTranspose);
					worldMatrixInverseTranspose = XMMatrixTranspose(worldMatrixInverseTranspose);
					XMFLOAT4X4 transformIT;
					XMStoreFloat4x4(&transformIT, worldMatrixInverseTranspose);

					inst.transformInverseTranspose.Create(transformIT);
					if (object.lightmap.IsValid())
					{
						inst.lightmap = device->GetDescriptorIndex(&object.lightmap, SubresourceType::SRV);
					}
					inst.uid = entity;
					inst.layerMask = layerMask;
					inst.color = wi::math::CompressColor(object.color);
					inst.emissive = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(object.emissiveColor.x * object.emissiveColor.w, object.emissiveColor.y * object.emissiveColor.w, object.emissiveColor.z * object.emissiveColor.w));
					inst.geometryOffset = mesh.geometryOffset;
					inst.geometryCount = (uint)mesh.subsets.size();
					inst.meshletOffset = meshletAllocator.fetch_add(mesh.meshletCount);
					inst.fadeDistance = object.fadeDistance;
					inst.center = object.center;
					inst.radius = object.radius;

					std::memcpy(instanceArrayMapped + args.jobIndex, &inst, sizeof(inst)); // memcpy whole structure into mapped pointer to avoid read from uncached memory

					if (TLAS_instancesMapped != nullptr)
					{
						// TLAS instance data:
						RaytracingAccelerationStructureDesc::TopLevel::Instance instance;
						for (int i = 0; i < arraysize(instance.transform); ++i)
						{
							for (int j = 0; j < arraysize(instance.transform[i]); ++j)
							{
								instance.transform[i][j] = object.worldMatrix.m[j][i];
							}
						}
						instance.instance_id = args.jobIndex;
						instance.instance_mask = layerMask & 0xFF;
						instance.bottom_level = &mesh.BLAS;
						instance.instance_contribution_to_hit_group_index = 0;
						instance.flags = 0;
						
						if (mesh.IsDoubleSided() || mesh._flags & MeshComponent::TLAS_FORCE_DOUBLE_SIDED)
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

					// LOD select:
					{
						const float distsq = wi::math::DistanceSquared(camera.Eye, object.center);
						const float radius = object.radius;
						const float radiussq = radius * radius;
						if (distsq < radiussq)
						{
							object.lod = 0;
						}
						else
						{
							const MeshComponent* mesh = meshes.GetComponent(object.meshID);
							if (mesh != nullptr && mesh->subsets_per_lod > 0)
							{
								const float dist = std::sqrt(distsq);
								const float dist_to_sphere = dist - radius;
								object.lod = uint32_t(dist_to_sphere * object.lod_distance_multiplier);
								object.lod = std::min(object.lod, mesh->GetLODCount() - 1);
							}
						}
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
			if (!transforms.Contains(entity))
				continue;
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
			desc.height = envmapRes;
			desc.width = envmapRes;
			desc.mip_levels = 1;
			desc.usage = Usage::DEFAULT;

			desc.bind_flags = BindFlag::DEPTH_STENCIL | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R16_TYPELESS;
			desc.layout = ResourceState::SHADER_RESOURCE;
			desc.sample_count = envmapMSAASampleCount;
			device->CreateTexture(&desc, nullptr, &envrenderingDepthBuffer_MSAA);
			device->SetName(&envrenderingDepthBuffer_MSAA, "envrenderingDepthBuffer_MSAA");

			desc.bind_flags = BindFlag::RENDER_TARGET;
			desc.format = Format::R11G11B10_FLOAT;
			desc.layout = ResourceState::RENDERTARGET;
			desc.misc_flags = ResourceMiscFlag::TRANSIENT_ATTACHMENT;
			device->CreateTexture(&desc, nullptr, &envrenderingColorBuffer_MSAA);
			device->SetName(&envrenderingColorBuffer_MSAA, "envrenderingColorBuffer_MSAA");

			desc.sample_count = 1;
			desc.array_size = envmapCount * 6;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS | BindFlag::RENDER_TARGET;
			desc.format = Format::R11G11B10_FLOAT;
			desc.height = envmapRes;
			desc.width = envmapRes;
			desc.mip_levels = envmapMIPs;
			desc.misc_flags = ResourceMiscFlag::TEXTURECUBE;
			desc.usage = Usage::DEFAULT;
			desc.layout = ResourceState::SHADER_RESOURCE;
			device->CreateTexture(&desc, nullptr, &envmapArray);
			device->SetName(&envmapArray, "envmapArray");

			desc.array_size = 6;
			desc.mip_levels = 1;
			desc.format = Format::R16_TYPELESS;
			desc.bind_flags = BindFlag::DEPTH_STENCIL | BindFlag::SHADER_RESOURCE;
			desc.layout = ResourceState::SHADER_RESOURCE;
			device->CreateTexture(&desc, nullptr, &envrenderingDepthBuffer);
			device->SetName(&envrenderingDepthBuffer, "envrenderingDepthBuffer");


			// Cube arrays per mip level:
			for (uint32_t i = 0; i < envmapArray.desc.mip_levels; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::SRV, 0, envmapArray.desc.array_size, i, 1);
				assert(subresource_index == i);
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::UAV, 0, envmapArray.desc.array_size, i, 1);
				assert(subresource_index == i);
			}

			// individual cubes with mips:
			for (uint32_t i = 0; i < envmapCount; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::SRV, i * 6, 6, 0, -1);
				assert(subresource_index == envmapArray.desc.mip_levels + i);
			}

			// individual cubes only mip0:
			for (uint32_t i = 0; i < envmapCount; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::SRV, i * 6, 6, 0, 1);
				assert(subresource_index == envmapArray.desc.mip_levels + envmapCount + i);
			}

			renderpasses_envmap.resize(envmapCount);
			renderpasses_envmap_MSAA.resize(envmapCount);
			for (uint32_t i = 0; i < envmapCount; ++i)
			{
				// Non MSAA:
				{
					int subresource_index;
					subresource_index = device->CreateSubresource(&envmapArray, SubresourceType::RTV, i * 6, 6, 0, 1);
					assert(subresource_index == i);

					RenderPassDesc renderpassdesc;
					renderpassdesc.attachments.push_back(
						RenderPassAttachment::DepthStencil(
							&envrenderingDepthBuffer,
							RenderPassAttachment::LoadOp::CLEAR,
							RenderPassAttachment::StoreOp::STORE,
							ResourceState::SHADER_RESOURCE,
							ResourceState::DEPTHSTENCIL,
							ResourceState::SHADER_RESOURCE
						)
					);
					renderpassdesc.attachments.push_back(
						RenderPassAttachment::RenderTarget(&envmapArray,
							RenderPassAttachment::LoadOp::DONTCARE,
							RenderPassAttachment::StoreOp::STORE,
							ResourceState::SHADER_RESOURCE,
							ResourceState::RENDERTARGET,
							ResourceState::SHADER_RESOURCE,
							subresource_index
						)
					);
					device->CreateRenderPass(&renderpassdesc, &renderpasses_envmap[i]);
				}

				// MSAA:
				{
					RenderPassDesc renderpassdesc;
					renderpassdesc.attachments.clear();
					renderpassdesc.attachments.push_back(
						RenderPassAttachment::DepthStencil(
							&envrenderingDepthBuffer_MSAA,
							RenderPassAttachment::LoadOp::CLEAR,
							RenderPassAttachment::StoreOp::STORE,
							ResourceState::SHADER_RESOURCE,
							ResourceState::DEPTHSTENCIL,
							ResourceState::SHADER_RESOURCE
						)
					);
					renderpassdesc.attachments.push_back(
						RenderPassAttachment::RenderTarget(&envrenderingColorBuffer_MSAA,
							RenderPassAttachment::LoadOp::DONTCARE,
							RenderPassAttachment::StoreOp::DONTCARE,
							ResourceState::RENDERTARGET,
							ResourceState::RENDERTARGET,
							ResourceState::RENDERTARGET
						)
					);
					renderpassdesc.attachments.push_back(
						RenderPassAttachment::Resolve(&envmapArray,
							ResourceState::SHADER_RESOURCE,
							ResourceState::SHADER_RESOURCE,
							envmapArray.desc.mip_levels + envmapCount + i // subresource: individual cubes only mip0
						)
					);
					device->CreateRenderPass(&renderpassdesc, &renderpasses_envmap_MSAA[i]);
				}
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
			if (!transforms.Contains(entity))
				continue;
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
				for (int i = 0; i < arraysize(envmapTaken); ++i)
				{
					if (envmapTaken[i] == false)
					{
						envmapTaken[i] = true;
						probe.textureIndex = i;
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
			if (!transforms.Contains(entity))
				return;
			const TransformComponent& transform = *transforms.GetComponent(entity);

			XMMATRIX W = XMLoadFloat4x4(&transform.world);
			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, W);

			XMStoreFloat3(&force.position, T);
			XMStoreFloat3(&force.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, -1, 0, 0), W)));

		});
	}
	void Scene::RunLightUpdateSystem(wi::jobsystem::context& ctx)
	{
		assert(lights.GetCount() == aabb_lights.GetCount());

		wi::jobsystem::Dispatch(ctx, (uint32_t)lights.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			LightComponent& light = lights[args.jobIndex];
			Entity entity = lights.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;
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
			XMStoreFloat3(&light.direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), W)));

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
					weather.sunColor.x *= light.intensity;
					weather.sunColor.y *= light.intensity;
					weather.sunColor.z *= light.intensity;
					weather.sunDirection = light.direction;
					weather.stars_rotation_quaternion = light.rotation;
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
			if (!transforms.Contains(entity))
				return;

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
				}
			}

			GraphicsDevice* device = wi::graphics::GetDevice();

			uint32_t indexCount = (uint32_t)hair.primitiveBuffer.desc.size / std::max(1u, (uint32_t)hair.primitiveBuffer.desc.stride);
			uint32_t triangleCount = indexCount / 3u;
			uint32_t meshletCount = triangle_count_to_meshlet_count(triangleCount);
			uint32_t meshletOffset = meshletAllocator.fetch_add(meshletCount);

			ShaderGeometry geometry;
			geometry.init();
			geometry.indexOffset = 0;
			geometry.materialIndex = (uint)materials.GetIndex(entity);
			geometry.ib = device->GetDescriptorIndex(&hair.primitiveBuffer, SubresourceType::SRV);
			geometry.vb_pos_nor_wind = device->GetDescriptorIndex(&hair.vertexBuffer_POS[0], SubresourceType::SRV);
			geometry.vb_pre = device->GetDescriptorIndex(&hair.vertexBuffer_POS[1], SubresourceType::SRV);
			geometry.vb_uvs = device->GetDescriptorIndex(&hair.vertexBuffer_UVS, SubresourceType::SRV);
			geometry.flags = SHADERMESH_FLAG_DOUBLE_SIDED | SHADERMESH_FLAG_HAIRPARTICLE;
			geometry.meshletOffset = 0;
			geometry.meshletCount = meshletCount;

			size_t geometryAllocation = geometryAllocator.fetch_add(1);
			std::memcpy(geometryArrayMapped + geometryAllocation, &geometry, sizeof(geometry));

			ShaderMeshInstance inst;
			inst.init();
			inst.uid = entity;
			inst.layerMask = hair.layerMask;
			inst.geometryOffset = (uint)geometryAllocation;
			inst.emissive = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(1, 1, 1));
			inst.color = wi::math::CompressColor(XMFLOAT4(1, 1, 1, 1));
			inst.geometryCount = 1;
			inst.meshletOffset = meshletOffset;
			inst.center = hair.aabb.getCenter();
			inst.radius = hair.aabb.getRadius();

			const size_t instanceIndex = objects.GetCount() + args.jobIndex;
			std::memcpy(instanceArrayMapped + instanceIndex, &inst, sizeof(inst));

			if (TLAS_instancesMapped != nullptr && hair.BLAS.IsValid())
			{
				// TLAS instance data:
				RaytracingAccelerationStructureDesc::TopLevel::Instance instance;
				for (int i = 0; i < arraysize(instance.transform); ++i)
				{
					for (int j = 0; j < arraysize(instance.transform[i]); ++j)
					{
						instance.transform[i][j] = wi::math::IDENTITY_MATRIX.m[j][i];
					}
				}
				instance.instance_id = (uint32_t)instanceIndex;
				instance.instance_mask = hair.layerMask & 0xFF;
				instance.bottom_level = &hair.BLAS;
				instance.instance_contribution_to_hit_group_index = 0;
				instance.flags = RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;

				void* dest = (void*)((size_t)TLAS_instancesMapped + instanceIndex * device->GetTopLevelAccelerationStructureInstanceSize());
				device->WriteTopLevelAccelerationStructureInstance(&instance, dest);
			}

		});

		wi::jobsystem::Dispatch(ctx, (uint32_t)emitters.GetCount(), small_subtask_groupsize, [&](wi::jobsystem::JobArgs args) {

			EmittedParticleSystem& emitter = emitters[args.jobIndex];
			Entity entity = emitters.GetEntity(args.jobIndex);
			if (!transforms.Contains(entity))
				return;

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

			uint32_t indexCount = (uint32_t)emitter.primitiveBuffer.desc.size / std::max(1u, (uint32_t)emitter.primitiveBuffer.desc.stride);
			uint32_t triangleCount = indexCount / 3u;
			uint32_t meshletCount = triangle_count_to_meshlet_count(triangleCount);
			uint32_t meshletOffset = meshletAllocator.fetch_add(meshletCount);

			ShaderGeometry geometry;
			geometry.init();
			geometry.indexOffset = 0;
			geometry.materialIndex = (uint)materials.GetIndex(entity);
			geometry.ib = device->GetDescriptorIndex(&emitter.primitiveBuffer, SubresourceType::SRV);
			geometry.vb_pos_nor_wind = device->GetDescriptorIndex(&emitter.vertexBuffer_POS, SubresourceType::SRV);
			geometry.vb_uvs = device->GetDescriptorIndex(&emitter.vertexBuffer_UVS, SubresourceType::SRV);
			geometry.vb_col = device->GetDescriptorIndex(&emitter.vertexBuffer_COL, SubresourceType::SRV);
			geometry.flags = SHADERMESH_FLAG_DOUBLE_SIDED | SHADERMESH_FLAG_EMITTEDPARTICLE;
			geometry.meshletOffset = 0;
			geometry.meshletCount = meshletCount;

			size_t geometryAllocation = geometryAllocator.fetch_add(1);
			std::memcpy(geometryArrayMapped + geometryAllocation, &geometry, sizeof(geometry));

			ShaderMeshInstance inst;
			inst.init();
			inst.uid = entity;
			inst.layerMask = emitter.layerMask;
			inst.geometryOffset = (uint)geometryAllocation;
			inst.emissive = wi::math::Pack_R11G11B10_FLOAT(XMFLOAT3(1, 1, 1));
			inst.color = wi::math::CompressColor(XMFLOAT4(1, 1, 1, 1));
			inst.geometryCount = 1;
			inst.meshletOffset = meshletOffset;

			const size_t instanceIndex = objects.GetCount() + hairs.GetCount() + args.jobIndex;
			std::memcpy(instanceArrayMapped + instanceIndex, &inst, sizeof(inst));

			if (TLAS_instancesMapped != nullptr && emitter.BLAS.IsValid())
			{
				// TLAS instance data:
				RaytracingAccelerationStructureDesc::TopLevel::Instance instance;
				for (int i = 0; i < arraysize(instance.transform); ++i)
				{
					for (int j = 0; j < arraysize(instance.transform[i]); ++j)
					{
						instance.transform[i][j] = wi::math::IDENTITY_MATRIX.m[j][i];
					}
				}
				instance.instance_id = (uint32_t)instanceIndex;
				instance.instance_mask = emitter.layerMask & 0xFF;
				instance.bottom_level = &emitter.BLAS;
				instance.instance_contribution_to_hit_group_index = 0;
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

			// Ocean occlusion status:
			if (!wi::renderer::GetFreezeCullingCameraEnabled() && weather.IsOceanEnabled())
			{
				ocean.occlusionHistory <<= 1u; // advance history by 1 frame
				int query_id = ocean.occlusionQueries[queryheap_idx];
				if (queryResultBuffer[queryheap_idx].mapped_data != nullptr && query_id >= 0)
				{
					uint64_t visible = ((uint64_t*)queryResultBuffer[queryheap_idx].mapped_data)[query_id];
					if (visible)
					{
						ocean.occlusionHistory |= 1; // visible
					}
				}
				else
				{
					ocean.occlusionHistory |= 1; // visible
				}
			}
			ocean.occlusionQueries[queryheap_idx] = -1; // invalidate query
		}
	}
	void Scene::RunSoundUpdateSystem(wi::jobsystem::context& ctx)
	{
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
	void Scene::RunScriptUpdateSystem(wi::jobsystem::context& ctx)
	{
		for (size_t i = 0; i < scripts.GetCount(); ++i)
		{
			ScriptComponent& script = scripts[i];
			Entity entity = scripts.GetEntity(i);

			if (script.IsPlaying())
			{
				if (script.script.empty() && script.resource.IsValid())
				{
					script.script += "local function GetEntity() return " + std::to_string(entity) + "; end\n";
					script.script += script.resource.GetScript();
					wi::lua::AttachScriptParameters(script.script, script.filename);
				}
				wi::lua::RunText(script.script);

				if (script.IsPlayingOnlyOnce())
				{
					script.Stop();
				}
			}
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

				const XMMATRIX objectMat = XMLoadFloat4x4(&object.worldMatrix);
				const XMMATRIX objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);

				const XMVECTOR rayOrigin_local = XMVector3Transform(rayOrigin, objectMat_Inverse);
				const XMVECTOR rayDirection_local = XMVector3Normalize(XMVector3TransformNormal(rayDirection, objectMat_Inverse));

				const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;

				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh.GetLODSubsetRange(0, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
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
						if (wi::math::RayTriangleIntersects(rayOrigin_local, rayDirection_local, p0, p1, p2, distance, bary, ray.TMin, ray.TMax))
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
								result.subsetIndex = (int)subsetIndex;
								result.vertexID0 = (int)i0;
								result.vertexID1 = (int)i1;
								result.vertexID2 = (int)i2;
								result.bary = bary;
							}
						}
					}
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

				const XMMATRIX objectMat = XMLoadFloat4x4(&object.worldMatrix);

				const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;

				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh.GetLODSubsetRange(0, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
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

				const XMMATRIX objectMat = XMLoadFloat4x4(&object.worldMatrix);

				const ArmatureComponent* armature = mesh.IsSkinned() ? scene.armatures.GetComponent(mesh.armatureID) : nullptr;

				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh.GetLODSubsetRange(0, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
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
				}

			}
		}

		return result;
	}

}
