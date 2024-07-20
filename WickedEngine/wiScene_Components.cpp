#include "wiScene_Components.h"
#include "wiTextureHelper.h"
#include "wiResourceManager.h"
#include "wiPhysics.h"
#include "wiRenderer.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBacklog.h"
#include "wiTimer.h"
#include "wiUnorderedMap.h"
#include "wiLua.h"

#include "Utility/mikktspace.h"
#include "Utility/meshoptimizer/meshoptimizer.h"

#if __has_include("OpenImageDenoise/oidn.hpp")
#include "OpenImageDenoise/oidn.hpp"
#if OIDN_VERSION_MAJOR >= 2
#define OPEN_IMAGE_DENOISE
#pragma comment(lib,"OpenImageDenoise.lib")
// Also provide the required DLL files from OpenImageDenoise release near the exe!
#endif // OIDN_VERSION_MAJOR >= 2
#endif // __has_include("OpenImageDenoise/oidn.hpp")

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
	XMFLOAT3 TransformComponent::GetForward() const
	{
		return wi::math::GetForward(world);
	}
	XMFLOAT3 TransformComponent::GetUp() const
	{
		return wi::math::GetUp(world);
	}
	XMFLOAT3 TransformComponent::GetRight() const
	{
		return wi::math::GetRight(world);
	}
	XMVECTOR TransformComponent::GetForwardV() const
	{
		XMFLOAT3 v = wi::math::GetForward(world);
		return XMLoadFloat3(&v);
	}
	XMVECTOR TransformComponent::GetUpV() const
	{
		XMFLOAT3 v = wi::math::GetUp(world);
		return XMLoadFloat3(&v);
	}
	XMVECTOR TransformComponent::GetRightV() const
	{
		XMFLOAT3 v = wi::math::GetRight(world);
		return XMLoadFloat3(&v);
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
		using namespace wi::math;

		ShaderMaterial material;
		material.init();
		material.baseColor = pack_half4(baseColor);
		material.emissive = pack_half3(XMFLOAT3(emissiveColor.x * emissiveColor.w, emissiveColor.y * emissiveColor.w, emissiveColor.z * emissiveColor.w));
		material.specular = pack_half3(XMFLOAT3(specularColor.x * specularColor.w, specularColor.y * specularColor.w, specularColor.z * specularColor.w));
		material.texMulAdd = texMulAdd;
		material.roughness_reflectance_metalness_refraction = pack_half4(roughness, reflectance, metalness, refraction);
		material.normalmap_pom_alphatest_displacement = pack_half4(normalMapStrength, parallaxOcclusionMapping, 1 - alphaRef, displacementMapping);
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
		material.subsurfaceScattering = pack_half4(sss);
		material.subsurfaceScattering_inv = pack_half4(sss_inv);

		if (shaderType == SHADERTYPE_WATER)
		{
			material.sheenColor = pack_half3(XMFLOAT3(1 - extinctionColor.x, 1 - extinctionColor.y, 1 - extinctionColor.z));
		}
		else
		{
			material.sheenColor = pack_half3(XMFLOAT3(sheenColor.x, sheenColor.y, sheenColor.z));
		}
		material.transmission_sheenroughness_clearcoat_clearcoatroughness = pack_half4(transmission, sheenRoughness, clearcoat, clearcoatRoughness);
		material.layerMask = layerMask;
		float _anisotropy_strength = 0;
		float _anisotropy_rotation_sin = 0;
		float _anisotropy_rotation_cos = 0;
		float _blend_with_terrain_height_rcp = 0;
		if (shaderType == SHADERTYPE_PBR_ANISOTROPIC)
		{
			_anisotropy_strength = clamp(anisotropy_strength, 0.0f, 0.99f);
			_anisotropy_rotation_sin = std::sin(anisotropy_rotation);
			_anisotropy_rotation_cos = std::cos(anisotropy_rotation);
		}
		if (blend_with_terrain_height > 0)
		{
			_blend_with_terrain_height_rcp = 1.0f / blend_with_terrain_height;
		}
		material.aniso_anisosin_anisocos_terrainblend = pack_half4(_anisotropy_strength, _anisotropy_rotation_sin, _anisotropy_rotation_cos, _blend_with_terrain_height_rcp);
		material.shaderType = (uint)shaderType;
		material.userdata = userdata;

		material.options_stencilref = 0;
		if (IsUsingVertexColors())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_USE_VERTEXCOLORS;
		}
		if (IsUsingSpecularGlossinessWorkflow())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_SPECULARGLOSSINESS_WORKFLOW;
		}
		if (IsOcclusionEnabled_Primary())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_PRIMARY;
		}
		if (IsOcclusionEnabled_Secondary())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_OCCLUSION_SECONDARY;
		}
		if (IsUsingWind())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_USE_WIND;
		}
		if (IsReceiveShadow())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_RECEIVE_SHADOW;
		}
		if (IsCastingShadow())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_CAST_SHADOW;
		}
		if (IsDoubleSided())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_DOUBLE_SIDED;
		}
		if (GetFilterMask() & FILTER_TRANSPARENT)
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_TRANSPARENT;
		}
		if (userBlendMode == BLENDMODE_ADDITIVE)
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_ADDITIVE;
		}
		if (shaderType == SHADERTYPE_UNLIT)
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_UNLIT;
		}
		if (!IsVertexAODisabled())
		{
			material.options_stencilref |= SHADERMATERIAL_OPTION_BIT_USE_VERTEXAO;
		}

		material.options_stencilref |= wi::renderer::CombineStencilrefs(engineStencilRef, userStencilRef) << 24u;

		GraphicsDevice* device = wi::graphics::GetDevice();
		for (int i = 0; i < TEXTURESLOT_COUNT; ++i)
		{
			material.textures[i].uvset_lodclamp = (textures[i].uvset & 1) | (XMConvertFloatToHalf(textures[i].lod_clamp) << 1u);
			if (textures[i].resource.IsValid())
			{
				int subresource = -1;
				switch (i)
				{
				case BASECOLORMAP:
				case EMISSIVEMAP:
				case SPECULARMAP:
				case SHEENCOLORMAP:
					subresource = textures[i].resource.GetTextureSRGBSubresource();
					break;
				case SURFACEMAP:
					if (IsUsingSpecularGlossinessWorkflow())
					{
						subresource = textures[i].resource.GetTextureSRGBSubresource();
					}
					break;
				default:
					break;
				}
				material.textures[i].texture_descriptor = device->GetDescriptorIndex(textures[i].GetGPUResource(), SubresourceType::SRV, subresource);
			}
			else
			{
				material.textures[i].texture_descriptor = -1;
			}
			material.textures[i].sparse_residencymap_descriptor = textures[i].sparse_residencymap_descriptor;
			material.textures[i].sparse_feedbackmap_descriptor = textures[i].sparse_feedbackmap_descriptor;
		}

		if (sampler_descriptor < 0)
		{
			material.sampler_descriptor = device->GetDescriptorIndex(wi::renderer::GetSampler(wi::enums::SAMPLER_OBJECTSHADER));
		}
		else
		{
			material.sampler_descriptor = sampler_descriptor;
		}

		std::memcpy(dest, &material, sizeof(ShaderMaterial)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
	}
	void MaterialComponent::WriteShaderTextureSlot(ShaderMaterial* dest, int slot, int descriptor)
	{
		std::memcpy(&dest->textures[slot].texture_descriptor, &descriptor, sizeof(descriptor)); // memcpy into mapped pointer to avoid read from uncached memory
	}
	void MaterialComponent::WriteTextures(const wi::graphics::GPUResource** dest, int count) const
	{
		count = std::min(count, (int)TEXTURESLOT_COUNT);
		for (int i = 0; i < count; ++i)
		{
			dest[i] = textures[i].GetGPUResource();
		}
	}
	uint32_t MaterialComponent::GetFilterMask() const
	{
		if (IsCustomShader() && customShaderID < (int)wi::renderer::GetCustomShaders().size())
		{
			auto& customShader = wi::renderer::GetCustomShaders()[customShaderID];
			return customShader.filterMask;
		}
		if (shaderType == SHADERTYPE_WATER)
		{
			return FILTER_TRANSPARENT | FILTER_WATER;
		}
		if (transmission > 0)
		{
			return FILTER_TRANSPARENT;
		}
		if (userBlendMode == BLENDMODE_OPAQUE)
		{
			return FILTER_OPAQUE;
		}
		return FILTER_TRANSPARENT;
	}
	wi::resourcemanager::Flags MaterialComponent::GetTextureSlotResourceFlags(TEXTURESLOT slot)
	{
		wi::resourcemanager::Flags flags = wi::resourcemanager::Flags::NONE;
		if (!IsPreferUncompressedTexturesEnabled())
		{
			flags |= wi::resourcemanager::Flags::IMPORT_BLOCK_COMPRESSED;
		}
		if (!IsTextureStreamingDisabled())
		{
			flags |= wi::resourcemanager::Flags::STREAMING;
		}
		switch (slot)
		{
		case NORMALMAP:
		case CLEARCOATNORMALMAP:
			flags |= wi::resourcemanager::Flags::IMPORT_NORMALMAP;
			break;
		default:
			break;
		}
		return flags;
	}
	void MaterialComponent::CreateRenderData(bool force_recreate)
	{
		if (force_recreate)
		{
			for (uint32_t slot = 0; slot < TEXTURESLOT_COUNT; ++slot)
			{
				auto& textureslot = textures[slot];
				if (textureslot.resource.IsValid())
				{
					textureslot.resource.SetOutdated();
				}
			}
		}
		for (uint32_t slot = 0; slot < TEXTURESLOT_COUNT; ++slot)
		{
			auto& textureslot = textures[slot];
			if (!textureslot.name.empty())
			{
				wi::resourcemanager::Flags flags = GetTextureSlotResourceFlags(TEXTURESLOT(slot));
				textureslot.resource = wi::resourcemanager::Load(textureslot.name, flags);
			}
		}
	}
	uint32_t MaterialComponent::GetStencilRef() const
	{
		return wi::renderer::CombineStencilrefs(engineStencilRef, userStencilRef);
	}

	struct MikkTSpaceUserdata
	{
		MeshComponent* mesh = nullptr;
		const uint32_t* indicesLOD0 = nullptr;
		int faceCountLOD0 = 0;
	};
	int get_num_faces(const SMikkTSpaceContext* context)
	{
		const MikkTSpaceUserdata* userdata = static_cast<const MikkTSpaceUserdata*>(context->m_pUserData);
		return userdata->faceCountLOD0;
	}
	int get_num_vertices_of_face(const SMikkTSpaceContext* context, const int iFace)
	{
		return 3;
	}
	int get_vertex_index(const SMikkTSpaceContext* context, int iFace, int iVert)
	{
		const MikkTSpaceUserdata* userdata = static_cast<const MikkTSpaceUserdata*>(context->m_pUserData);
		int face_size = get_num_vertices_of_face(context, iFace);
		int indices_index = iFace * face_size + iVert;
		int index = int(userdata->indicesLOD0[indices_index]);
		return index;
	}
	void get_position(const SMikkTSpaceContext* context, float* outpos, const int iFace, const int iVert)
	{
		const MikkTSpaceUserdata* userdata = static_cast<const MikkTSpaceUserdata*>(context->m_pUserData);
		int index = get_vertex_index(context, iFace, iVert);
		const XMFLOAT3& vert = userdata->mesh->vertex_positions[index];
		outpos[0] = vert.x;
		outpos[1] = vert.y;
		outpos[2] = vert.z;
	}
	void get_normal(const SMikkTSpaceContext* context, float* outnormal, const int iFace, const int iVert)
	{
		const MikkTSpaceUserdata* userdata = static_cast<const MikkTSpaceUserdata*>(context->m_pUserData);
		int index = get_vertex_index(context, iFace, iVert);
		const XMFLOAT3& vert = userdata->mesh->vertex_normals[index];
		outnormal[0] = vert.x;
		outnormal[1] = vert.y;
		outnormal[2] = vert.z;
	}
	void get_tex_coords(const SMikkTSpaceContext* context, float* outuv, const int iFace, const int iVert)
	{
		const MikkTSpaceUserdata* userdata = static_cast<const MikkTSpaceUserdata*>(context->m_pUserData);
		int index = get_vertex_index(context, iFace, iVert);
		const XMFLOAT2& vert = userdata->mesh->vertex_uvset_0[index];
		outuv[0] = vert.x;
		outuv[1] = vert.y;
	}
	void set_tspace_basic(const SMikkTSpaceContext* context, const float* tangentu, const float fSign, const int iFace, const int iVert)
	{
		const MikkTSpaceUserdata* userdata = static_cast<const MikkTSpaceUserdata*>(context->m_pUserData);
		auto index = get_vertex_index(context, iFace, iVert);
		XMFLOAT4& vert = userdata->mesh->vertex_tangents[index];
		vert.x = tangentu[0];
		vert.y = tangentu[1];
		vert.z = tangentu[2];
		vert.w = fSign;
	}

	void MeshComponent::DeleteRenderData()
	{
		generalBuffer = {};
		streamoutBuffer = {};
		ib = {};
		vb_pos_wind = {};
		vb_nor = {};
		vb_tan = {};
		vb_uvs = {};
		vb_atl = {};
		vb_col = {};
		vb_bon = {};
		so_pos = {};
		so_nor = {};
		so_tan = {};
		so_pre = {};
		BLASes.clear();
		for (MorphTarget& morph : morph_targets)
		{
			morph.offset_pos = ~0ull;
			morph.offset_nor = ~0ull;
		}
	}
	void MeshComponent::CreateRenderData()
	{
		DeleteRenderData();

		GraphicsDevice* device = wi::graphics::GetDevice();

		if (vertex_tangents.empty() && !vertex_uvset_0.empty() && !vertex_normals.empty())
		{
			// Generate tangents if not found:
			vertex_tangents.resize(vertex_positions.size());

#if 1
			// MikkTSpace tangent generation:
			MikkTSpaceUserdata userdata;
			userdata.mesh = this;
			uint32_t indexOffsetLOD0 = ~0u;
			uint32_t indexCountLOD0 = 0;
			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = subsets[subsetIndex];
				indexOffsetLOD0 = std::min(indexOffsetLOD0, subset.indexOffset);
				indexCountLOD0 = std::max(indexCountLOD0, subset.indexCount);
			}
			userdata.indicesLOD0 = indices.data() + indexOffsetLOD0;
			userdata.faceCountLOD0 = int(indexCountLOD0) / 3;

			SMikkTSpaceInterface iface = {};
			iface.m_getNumFaces = get_num_faces;
			iface.m_getNumVerticesOfFace = get_num_vertices_of_face;
			iface.m_getNormal = get_normal;
			iface.m_getPosition = get_position;
			iface.m_getTexCoord = get_tex_coords;
			iface.m_setTSpaceBasic = set_tspace_basic;
			SMikkTSpaceContext context = {};
			context.m_pInterface = &iface;
			context.m_pUserData = &userdata;
			tbool mikktspace_result = genTangSpaceDefault(&context);
			assert(mikktspace_result == 1);

#else
			// Old tangent generation logic:
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
#endif
		}

		const size_t uv_count = std::max(vertex_uvset_0.size(), vertex_uvset_1.size());

		// Bounds computation:
		XMFLOAT3 _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		XMFLOAT3 _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
		for (size_t i = 0; i < vertex_positions.size(); ++i)
		{
			const XMFLOAT3& pos = vertex_positions[i];
			_min = wi::math::Min(_min, pos);
			_max = wi::math::Max(_max, pos);
		}
		aabb = AABB(_min, _max);

		if (IsQuantizedPositionsDisabled())
		{
			position_format = vertex_windweights.empty() ? Vertex_POS32::FORMAT : Vertex_POS32W::FORMAT;
		}
		else
		{
			// Determine minimum precision for positions:
			const float target_precision = 1.0f / 1000.0f; // millimeter
			position_format = Vertex_POS10::FORMAT;
			for (size_t i = 0; i < vertex_positions.size(); ++i)
			{
				const XMFLOAT3& pos = vertex_positions[i];
				const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
				if (position_format == Vertex_POS10::FORMAT)
				{
					Vertex_POS10 v;
					v.FromFULL(aabb, pos, wind);
					XMFLOAT3 p = v.GetPOS(aabb);
					if (
						std::abs(p.x - pos.x) <= target_precision &&
						std::abs(p.y - pos.y) <= target_precision &&
						std::abs(p.z - pos.z) <= target_precision &&
						wind == v.GetWind()
						)
					{
						// success, continue to next vertex with 8 bits
						continue;
					}
					position_format = Vertex_POS16::FORMAT; // failed, increase to 16 bits
				}
				if (position_format == Vertex_POS16::FORMAT)
				{
					Vertex_POS16 v;
					v.FromFULL(aabb, pos, wind);
					XMFLOAT3 p = v.GetPOS(aabb);
					if (
						std::abs(p.x - pos.x) <= target_precision &&
						std::abs(p.y - pos.y) <= target_precision &&
						std::abs(p.z - pos.z) <= target_precision &&
						wind == v.GetWind()
						)
					{
						// success, continue to next vertex with 16 bits
						continue;
					}
					position_format = vertex_windweights.empty() ? Vertex_POS32::FORMAT : Vertex_POS32W::FORMAT; // failed, increase to 32 bits
					break; // since 32 bit is the max, we can bail out
				}
			}

			if (IsFormatUnorm(position_format))
			{
				// This is done to avoid 0 scaling on any axis of the UNORM remap matrix of the AABB
				//	It specifically solves a problem with hardware raytracing which treats AABB with zero axis as invisible
				//	Also there was problem with using float epsilon value, it did not enough precision for raytracing
				constexpr float min_dim = 0.01f;
				if (aabb._max.x - aabb._min.x < min_dim)
				{
					aabb._max.x += min_dim;
					aabb._min.x -= min_dim;
				}
				if (aabb._max.y - aabb._min.y < min_dim)
				{
					aabb._max.y += min_dim;
					aabb._min.y -= min_dim;
				}
				if (aabb._max.z - aabb._min.z < min_dim)
				{
					aabb._max.z += min_dim;
					aabb._min.z -= min_dim;
				}
			}
		}

		// Determine UV range for normalization:
		if (!vertex_uvset_0.empty() || !vertex_uvset_1.empty())
		{
			const XMFLOAT2* uv0_stream = vertex_uvset_0.empty() ? vertex_uvset_1.data() : vertex_uvset_0.data();
			const XMFLOAT2* uv1_stream = vertex_uvset_1.empty() ? vertex_uvset_0.data() : vertex_uvset_1.data();

			uv_range_min = XMFLOAT2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
			uv_range_max = XMFLOAT2(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
			for (size_t i = 0; i < uv_count; ++i)
			{
				uv_range_max = wi::math::Max(uv_range_max, uv0_stream[i]);
				uv_range_max = wi::math::Max(uv_range_max, uv1_stream[i]);
				uv_range_min = wi::math::Min(uv_range_min, uv0_stream[i]);
				uv_range_min = wi::math::Min(uv_range_min, uv1_stream[i]);
			}
		}

		const size_t position_stride = GetFormatStride(position_format);

		GPUBufferDesc bd;
		if (device->CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
		{
			// In UMA mode, it is better to create UPLOAD buffer, this avoids one copy from UPLOAD to DEFAULT
			bd.usage = Usage::UPLOAD;
		}
		else
		{
			bd.usage = Usage::DEFAULT;
		}
		bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE;
		bd.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::TYPED_FORMAT_CASTING | ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS;
		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			bd.misc_flags |= ResourceMiscFlag::RAY_TRACING;
		}
		const uint64_t alignment = device->GetMinOffsetAlignment(&bd);
		bd.size =
			AlignTo(vertex_positions.size() * position_stride, alignment) + // position will be first to have 0 offset for flexible alignment!
			AlignTo(indices.size() * GetIndexStride(), alignment) +
			AlignTo(vertex_normals.size() * sizeof(Vertex_NOR), alignment) +
			AlignTo(vertex_tangents.size() * sizeof(Vertex_TAN), alignment) +
			AlignTo(uv_count * sizeof(Vertex_UVS), alignment) +
			AlignTo(vertex_atlas.size() * sizeof(Vertex_TEX), alignment) +
			AlignTo(vertex_colors.size() * sizeof(Vertex_COL), alignment) +
			AlignTo(vertex_boneindices.size() * sizeof(Vertex_BON), alignment) +
			AlignTo(vertex_boneindices2.size() * sizeof(Vertex_BON), alignment)
			;

		constexpr Format morph_format = Format::R16G16B16A16_FLOAT;
		constexpr size_t morph_stride = GetFormatStride(morph_format);
		for (MorphTarget& morph : morph_targets)
		{
			if (!morph.vertex_positions.empty())
			{
				bd.size += AlignTo(vertex_positions.size() * morph_stride, alignment);
			}
			if (!morph.vertex_normals.empty())
			{
				bd.size += AlignTo(vertex_normals.size() * morph_stride, alignment);
			}
		}

		auto init_callback = [&](void* dest) {
			uint8_t* buffer_data = (uint8_t*)dest;
			uint64_t buffer_offset = 0ull;

			// vertexBuffer - POSITION + WIND:
			switch (position_format)
			{
			case Vertex_POS10::FORMAT:
			{
				vb_pos_wind.offset = buffer_offset;
				vb_pos_wind.size = vertex_positions.size() * sizeof(Vertex_POS10);
				Vertex_POS10* vertices = (Vertex_POS10*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_pos_wind.size, alignment);
				for (size_t i = 0; i < vertex_positions.size(); ++i)
				{
					XMFLOAT3 pos = vertex_positions[i];
					const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
					Vertex_POS10 vert;
					vert.FromFULL(aabb, pos, wind);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}
			break;
			case Vertex_POS16::FORMAT:
			{
				vb_pos_wind.offset = buffer_offset;
				vb_pos_wind.size = vertex_positions.size() * sizeof(Vertex_POS16);
				Vertex_POS16* vertices = (Vertex_POS16*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_pos_wind.size, alignment);
				for (size_t i = 0; i < vertex_positions.size(); ++i)
				{
					XMFLOAT3 pos = vertex_positions[i];
					const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
					Vertex_POS16 vert;
					vert.FromFULL(aabb, pos, wind);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}
			break;
			case Vertex_POS32::FORMAT:
			{
				vb_pos_wind.offset = buffer_offset;
				vb_pos_wind.size = vertex_positions.size() * sizeof(Vertex_POS32);
				Vertex_POS32* vertices = (Vertex_POS32*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_pos_wind.size, alignment);
				for (size_t i = 0; i < vertex_positions.size(); ++i)
				{
					const XMFLOAT3& pos = vertex_positions[i];
					const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
					Vertex_POS32 vert;
					vert.FromFULL(pos);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}
			break;
			case Vertex_POS32W::FORMAT:
			{
				vb_pos_wind.offset = buffer_offset;
				vb_pos_wind.size = vertex_positions.size() * sizeof(Vertex_POS32W);
				Vertex_POS32W* vertices = (Vertex_POS32W*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_pos_wind.size, alignment);
				for (size_t i = 0; i < vertex_positions.size(); ++i)
				{
					const XMFLOAT3& pos = vertex_positions[i];
					const uint8_t wind = vertex_windweights.empty() ? 0xFF : vertex_windweights[i];
					Vertex_POS32W vert;
					vert.FromFULL(pos, wind);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}
			break;
			default:
				assert(0);
				break;
			}

			// Create index buffer GPU data:
			if (GetIndexFormat() == IndexBufferFormat::UINT32)
			{
				ib.offset = buffer_offset;
				ib.size = indices.size() * sizeof(uint32_t);
				uint32_t* indexdata = (uint32_t*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(ib.size, alignment);
				std::memcpy(indexdata, indices.data(), ib.size);
			}
			else
			{
				ib.offset = buffer_offset;
				ib.size = indices.size() * sizeof(uint16_t);
				uint16_t* indexdata = (uint16_t*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(ib.size, alignment);
				for (size_t i = 0; i < indices.size(); ++i)
				{
					std::memcpy(indexdata + i, &indices[i], sizeof(uint16_t));
				}
			}

			// vertexBuffer - NORMALS:
			if (!vertex_normals.empty())
			{
				vb_nor.offset = buffer_offset;
				vb_nor.size = vertex_normals.size() * sizeof(Vertex_NOR);
				Vertex_NOR* vertices = (Vertex_NOR*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_nor.size, alignment);
				for (size_t i = 0; i < vertex_normals.size(); ++i)
				{
					Vertex_NOR vert;
					vert.FromFULL(vertex_normals[i]);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}

			// vertexBuffer - TANGENTS
			if (!vertex_tangents.empty())
			{
				vb_tan.offset = buffer_offset;
				vb_tan.size = vertex_tangents.size() * sizeof(Vertex_TAN);
				Vertex_TAN* vertices = (Vertex_TAN*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_tan.size, alignment);
				for (size_t i = 0; i < vertex_tangents.size(); ++i)
				{
					Vertex_TAN vert;
					vert.FromFULL(vertex_tangents[i]);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}

			// vertexBuffer - UV SETS
			if (!vertex_uvset_0.empty() || !vertex_uvset_1.empty())
			{
				const XMFLOAT2* uv0_stream = vertex_uvset_0.empty() ? vertex_uvset_1.data() : vertex_uvset_0.data();
				const XMFLOAT2* uv1_stream = vertex_uvset_1.empty() ? vertex_uvset_0.data() : vertex_uvset_1.data();

				vb_uvs.offset = buffer_offset;
				vb_uvs.size = uv_count * sizeof(Vertex_UVS);
				Vertex_UVS* vertices = (Vertex_UVS*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_uvs.size, alignment);
				for (size_t i = 0; i < uv_count; ++i)
				{
					Vertex_UVS vert;
					vert.uv0.FromFULL(uv0_stream[i], uv_range_min, uv_range_max);
					vert.uv1.FromFULL(uv1_stream[i], uv_range_min, uv_range_max);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}

			// vertexBuffer - ATLAS
			if (!vertex_atlas.empty())
			{
				vb_atl.offset = buffer_offset;
				vb_atl.size = vertex_atlas.size() * sizeof(Vertex_TEX);
				Vertex_TEX* vertices = (Vertex_TEX*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_atl.size, alignment);
				for (size_t i = 0; i < vertex_atlas.size(); ++i)
				{
					Vertex_TEX vert;
					vert.FromFULL(vertex_atlas[i]);
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}

			// vertexBuffer - COLORS
			if (!vertex_colors.empty())
			{
				vb_col.offset = buffer_offset;
				vb_col.size = vertex_colors.size() * sizeof(Vertex_COL);
				Vertex_COL* vertices = (Vertex_COL*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_col.size, alignment);
				for (size_t i = 0; i < vertex_colors.size(); ++i)
				{
					Vertex_COL vert;
					vert.color = vertex_colors[i];
					std::memcpy(vertices + i, &vert, sizeof(vert));
				}
			}

			// bone reference buffers (skinning, soft body):
			if (!vertex_boneindices.empty())
			{
				vb_bon.offset = buffer_offset;
				const size_t influence_div4 = GetBoneInfluenceDiv4();
				vb_bon.size = (vertex_boneindices.size() + vertex_boneindices2.size()) * sizeof(Vertex_BON);
				Vertex_BON* vertices = (Vertex_BON*)(buffer_data + buffer_offset);
				buffer_offset += AlignTo(vb_bon.size, alignment);
				assert(vertex_boneindices.size() == vertex_boneweights.size()); // must have same number of indices as weights
				assert(vertex_boneindices2.empty() || vertex_boneindices2.size() == vertex_boneindices.size()); // if second influence stream exists, it must be as large as the first
				assert(vertex_boneindices2.size() == vertex_boneweights2.size()); // must have same number of indices as weights
				for (size_t i = 0; i < vertex_boneindices.size(); ++i)
				{
					// Normalize weights:
					//	Note: if multiple influence streams are present,
					//	we have to normalize them together, not separately
					float weights[8] = {};
					weights[0] = vertex_boneweights[i].x;
					weights[1] = vertex_boneweights[i].y;
					weights[2] = vertex_boneweights[i].z;
					weights[3] = vertex_boneweights[i].w;
					if (influence_div4 > 1)
					{
						weights[4] = vertex_boneweights2[i].x;
						weights[5] = vertex_boneweights2[i].y;
						weights[6] = vertex_boneweights2[i].z;
						weights[7] = vertex_boneweights2[i].w;
					}
					float sum = 0;
					for (auto& weight : weights)
					{
						sum += weight;
					}
					if (sum > 0)
					{
						const float norm = 1.0f / sum;
						for (auto& weight : weights)
						{
							weight *= norm;
						}
					}
					// Store back normalized weights:
					vertex_boneweights[i].x = weights[0];
					vertex_boneweights[i].y = weights[1];
					vertex_boneweights[i].z = weights[2];
					vertex_boneweights[i].w = weights[3];
					if (influence_div4 > 1)
					{
						vertex_boneweights2[i].x = weights[4];
						vertex_boneweights2[i].y = weights[5];
						vertex_boneweights2[i].z = weights[6];
						vertex_boneweights2[i].w = weights[7];
					}

					Vertex_BON vert;
					vert.FromFULL(vertex_boneindices[i], vertex_boneweights[i]);
					std::memcpy(vertices + (i * influence_div4 + 0), &vert, sizeof(vert));

					if (influence_div4 > 1)
					{
						vert.FromFULL(vertex_boneindices2[i], vertex_boneweights2[i]);
						std::memcpy(vertices + (i * influence_div4 + 1), &vert, sizeof(vert));
					}
				}
			}

			// morph buffers:
			if (!morph_targets.empty())
			{
				vb_mor.offset = buffer_offset;
				for (MorphTarget& morph : morph_targets)
				{
					if (!morph.vertex_positions.empty())
					{
						morph.offset_pos = (buffer_offset - vb_mor.offset) / morph_stride;
						XMHALF4* vertices = (XMHALF4*)(buffer_data + buffer_offset);
						std::fill(vertices, vertices + vertex_positions.size(), 0);
						if (morph.sparse_indices_positions.empty())
						{
							// flat morphs:
							for (size_t i = 0; i < morph.vertex_positions.size(); ++i)
							{
								XMStoreHalf4(vertices + i, XMLoadFloat3(&morph.vertex_positions[i]));
							}
						}
						else
						{
							// sparse morphs will be flattened for GPU because they will be evaluated in skinning for every vertex:
							for (size_t i = 0; i < morph.sparse_indices_positions.size(); ++i)
							{
								const uint32_t ind = morph.sparse_indices_positions[i];
								XMStoreHalf4(vertices + ind, XMLoadFloat3(&morph.vertex_positions[i]));
							}
						}
						buffer_offset += AlignTo(morph.vertex_positions.size() * sizeof(XMHALF4), alignment);
					}
					if (!morph.vertex_normals.empty())
					{
						morph.offset_nor = (buffer_offset - vb_mor.offset) / morph_stride;
						XMHALF4* vertices = (XMHALF4*)(buffer_data + buffer_offset);
						std::fill(vertices, vertices + vertex_normals.size(), 0);
						if (morph.sparse_indices_normals.empty())
						{
							// flat morphs:
							for (size_t i = 0; i < morph.vertex_normals.size(); ++i)
							{
								XMStoreHalf4(vertices + i, XMLoadFloat3(&morph.vertex_normals[i]));
							}
						}
						else
						{
							// sparse morphs will be flattened for GPU because they will be evaluated in skinning for every vertex:
							for (size_t i = 0; i < morph.sparse_indices_normals.size(); ++i)
							{
								const uint32_t ind = morph.sparse_indices_normals[i];
								XMStoreHalf4(vertices + ind, XMLoadFloat3(&morph.vertex_normals[i]));
							}
						}
						buffer_offset += AlignTo(morph.vertex_normals.size() * sizeof(XMHALF4), alignment);
					}
				}
				vb_mor.size = buffer_offset - vb_mor.offset;
			}
		};

		bool success = device->CreateBuffer2(&bd, init_callback, &generalBuffer);
		assert(success);
		device->SetName(&generalBuffer, "MeshComponent::generalBuffer");

		assert(ib.IsValid());
		const Format ib_format = GetIndexFormat() == IndexBufferFormat::UINT32 ? Format::R32_UINT : Format::R16_UINT;
		ib.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, ib.offset, ib.size, &ib_format);
		ib.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, ib.subresource_srv);

		assert(vb_pos_wind.IsValid());
		vb_pos_wind.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_pos_wind.offset, vb_pos_wind.size, &position_format);
		vb_pos_wind.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_pos_wind.subresource_srv);

		if (vb_nor.IsValid())
		{
			vb_nor.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_nor.offset, vb_nor.size, &Vertex_NOR::FORMAT);
			vb_nor.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_nor.subresource_srv);
		}
		if (vb_tan.IsValid())
		{
			vb_tan.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_tan.offset, vb_tan.size, &Vertex_TAN::FORMAT);
			vb_tan.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_tan.subresource_srv);
		}
		if (vb_uvs.IsValid())
		{
			vb_uvs.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_uvs.offset, vb_uvs.size, &Vertex_UVS::FORMAT);
			vb_uvs.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_uvs.subresource_srv);
		}
		if (vb_atl.IsValid())
		{
			vb_atl.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_atl.offset, vb_atl.size, &Vertex_TEX::FORMAT);
			vb_atl.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_atl.subresource_srv);
		}
		if (vb_col.IsValid())
		{
			vb_col.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_col.offset, vb_col.size, &Vertex_COL::FORMAT);
			vb_col.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_col.subresource_srv);
		}
		if (vb_bon.IsValid())
		{
			vb_bon.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_bon.offset, vb_bon.size);
			vb_bon.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_bon.subresource_srv);
		}
		if (vb_mor.IsValid())
		{
			vb_mor.subresource_srv = device->CreateSubresource(&generalBuffer, SubresourceType::SRV, vb_mor.offset, vb_mor.size, &morph_format);
			vb_mor.descriptor_srv = device->GetDescriptorIndex(&generalBuffer, SubresourceType::SRV, vb_mor.subresource_srv);
		}

		if (!vertex_boneindices.empty() || !morph_targets.empty())
		{
			CreateStreamoutRenderData();
		}
	}
	void MeshComponent::CreateStreamoutRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		GPUBufferDesc desc;
		desc.usage = Usage::DEFAULT;
		desc.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
		desc.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::TYPED_FORMAT_CASTING | ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS;
		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			desc.misc_flags |= ResourceMiscFlag::RAY_TRACING;
		}

		const uint64_t alignment = device->GetMinOffsetAlignment(&desc) * sizeof(Vertex_POS32); // additional alignment for RGB32F
		desc.size =
			AlignTo(vertex_positions.size() * sizeof(Vertex_POS32), alignment) + // pos
			AlignTo(vertex_positions.size() * sizeof(Vertex_POS32), alignment) + // prevpos
			AlignTo(vertex_normals.size() * sizeof(Vertex_NOR), alignment) +
			AlignTo(vertex_tangents.size() * sizeof(Vertex_TAN), alignment)
			;

		bool success = device->CreateBuffer(&desc, nullptr, &streamoutBuffer);
		assert(success);
		device->SetName(&streamoutBuffer, "MeshComponent::streamoutBuffer");

		uint64_t buffer_offset = 0ull;

		so_pos.offset = buffer_offset;
		so_pos.size = vertex_positions.size() * sizeof(Vertex_POS32);
		buffer_offset += AlignTo(so_pos.size, alignment);
		so_pos.subresource_srv = device->CreateSubresource(&streamoutBuffer, SubresourceType::SRV, so_pos.offset, so_pos.size, &Vertex_POS32::FORMAT);
		so_pos.subresource_uav = device->CreateSubresource(&streamoutBuffer, SubresourceType::UAV, so_pos.offset, so_pos.size); // UAV can't have RGB32_F format!
		so_pos.descriptor_srv = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::SRV, so_pos.subresource_srv);
		so_pos.descriptor_uav = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::UAV, so_pos.subresource_uav);

		so_pre.offset = buffer_offset;
		so_pre.size = so_pos.size;
		buffer_offset += AlignTo(so_pre.size, alignment);
		so_pre.subresource_srv = device->CreateSubresource(&streamoutBuffer, SubresourceType::SRV, so_pre.offset, so_pre.size, &Vertex_POS32::FORMAT);
		so_pre.subresource_uav = device->CreateSubresource(&streamoutBuffer, SubresourceType::UAV, so_pre.offset, so_pre.size); // UAV can't have RGB32_F format!
		so_pre.descriptor_srv = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::SRV, so_pre.subresource_srv);
		so_pre.descriptor_uav = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::UAV, so_pre.subresource_uav);

		if (vb_nor.IsValid())
		{
			so_nor.offset = buffer_offset;
			so_nor.size = vb_nor.size;
			buffer_offset += AlignTo(so_nor.size, alignment);
			so_nor.subresource_srv = device->CreateSubresource(&streamoutBuffer, SubresourceType::SRV, so_nor.offset, so_nor.size, &Vertex_NOR::FORMAT);
			so_nor.subresource_uav = device->CreateSubresource(&streamoutBuffer, SubresourceType::UAV, so_nor.offset, so_nor.size, &Vertex_NOR::FORMAT);
			so_nor.descriptor_srv = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::SRV, so_nor.subresource_srv);
			so_nor.descriptor_uav = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::UAV, so_nor.subresource_uav);
		}

		if (vb_tan.IsValid())
		{
			so_tan.offset = buffer_offset;
			so_tan.size = vb_tan.size;
			buffer_offset += AlignTo(so_tan.size, alignment);
			so_tan.subresource_srv = device->CreateSubresource(&streamoutBuffer, SubresourceType::SRV, so_tan.offset, so_tan.size, &Vertex_TAN::FORMAT);
			so_tan.subresource_uav = device->CreateSubresource(&streamoutBuffer, SubresourceType::UAV, so_tan.offset, so_tan.size, &Vertex_TAN::FORMAT);
			so_tan.descriptor_srv = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::SRV, so_tan.subresource_srv);
			so_tan.descriptor_uav = device->GetDescriptorIndex(&streamoutBuffer, SubresourceType::UAV, so_tan.subresource_uav);
		}
	}
	void MeshComponent::CreateRaytracingRenderData()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		if (!device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			return;

		BLAS_state = MeshComponent::BLAS_STATE_NEEDS_REBUILD;

		const uint32_t lod_count = GetLODCount();
		BLASes.resize(lod_count);
		for (uint32_t lod = 0; lod < lod_count; ++lod)
		{
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
			GetLODSubsetRange(lod, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = subsets[subsetIndex];
				desc.bottom_level.geometries.emplace_back();
				auto& geometry = desc.bottom_level.geometries.back();
				geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES;
				geometry.triangles.vertex_buffer = generalBuffer;
				geometry.triangles.vertex_byte_offset = vb_pos_wind.offset;
				geometry.triangles.index_buffer = generalBuffer;
				geometry.triangles.index_format = GetIndexFormat();
				geometry.triangles.index_count = subset.indexCount;
				geometry.triangles.index_offset = ib.offset / GetIndexStride() + subset.indexOffset;
				geometry.triangles.vertex_count = (uint32_t)vertex_positions.size();
				if (so_pos.IsValid())
				{
					geometry.triangles.vertex_format = Vertex_POS32::FORMAT;
					geometry.triangles.vertex_stride = sizeof(Vertex_POS32);
				}
				else
				{
					geometry.triangles.vertex_format = position_format == Format::R32G32B32A32_FLOAT ? Format::R32G32B32_FLOAT : position_format;
					geometry.triangles.vertex_stride = GetFormatStride(position_format);
				}
			}

			bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLASes[lod]);
			assert(success);
			device->SetName(&BLASes[lod], std::string("MeshComponent::BLAS[LOD" + std::to_string(lod) + "]").c_str());
		}
	}
	void MeshComponent::BuildBVH()
	{
		bvh_leaf_aabbs.clear();
		uint32_t first_subset = 0;
		uint32_t last_subset = 0;
		GetLODSubsetRange(0, first_subset, last_subset);
		for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
		{
			const MeshComponent::MeshSubset& subset = subsets[subsetIndex];
			if (subset.indexCount == 0)
				continue;
			const uint32_t indexOffset = subset.indexOffset;
			const uint32_t triangleCount = subset.indexCount / 3;
			for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
			{
				const uint32_t i0 = indices[indexOffset + triangleIndex * 3 + 0];
				const uint32_t i1 = indices[indexOffset + triangleIndex * 3 + 1];
				const uint32_t i2 = indices[indexOffset + triangleIndex * 3 + 2];
				const XMFLOAT3& p0 = vertex_positions[i0];
				const XMFLOAT3& p1 = vertex_positions[i1];
				const XMFLOAT3& p2 = vertex_positions[i2];
				AABB aabb = wi::primitive::AABB(wi::math::Min(p0, wi::math::Min(p1, p2)), wi::math::Max(p0, wi::math::Max(p1, p2)));
				aabb.layerMask = triangleIndex;
				aabb.userdata = subsetIndex;
				bvh_leaf_aabbs.push_back(aabb);
			}
		}
		bvh.Build(bvh_leaf_aabbs.data(), (uint32_t)bvh_leaf_aabbs.size());
	}
	void MeshComponent::ComputeNormals(COMPUTE_NORMALS compute)
	{
		// Start recalculating normals:

		if (compute != COMPUTE_NORMALS_SMOOTH_FAST)
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
						wi::math::float_equal(v_search_pos.x, v0.x) &&
						wi::math::float_equal(v_search_pos.y, v0.y) &&
						wi::math::float_equal(v_search_pos.z, v0.z);

					bool match_pos1 =
						wi::math::float_equal(v_search_pos.x, v1.x) &&
						wi::math::float_equal(v_search_pos.y, v1.y) &&
						wi::math::float_equal(v_search_pos.z, v1.z);

					bool match_pos2 =
						wi::math::float_equal(v_search_pos.x, v2.x) &&
						wi::math::float_equal(v_search_pos.y, v2.y) &&
						wi::math::float_equal(v_search_pos.z, v2.z);

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
			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = subsets[subsetIndex];
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
							wi::math::float_equal(p0.x, p1.x) &&
							wi::math::float_equal(p0.y, p1.y) &&
							wi::math::float_equal(p0.z, p1.z);

						const bool duplicated_uv0 =
							wi::math::float_equal(u00.x, u01.x) &&
							wi::math::float_equal(u00.y, u01.y);

						const bool duplicated_uv1 =
							wi::math::float_equal(u10.x, u11.x) &&
							wi::math::float_equal(u10.y, u11.y);

						const bool duplicated_atl =
							wi::math::float_equal(at0.x, at1.x) &&
							wi::math::float_equal(at0.y, at1.y);

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
	size_t MeshComponent::GetMemoryUsageCPU() const
	{
		size_t size = 
			vertex_positions.size() * sizeof(XMFLOAT3) +
			vertex_normals.size() * sizeof(XMFLOAT3) +
			vertex_tangents.size() * sizeof(XMFLOAT4) +
			vertex_uvset_0.size() * sizeof(XMFLOAT2) +
			vertex_uvset_1.size() * sizeof(XMFLOAT2) +
			vertex_boneindices.size() * sizeof(XMUINT4) +
			vertex_boneweights.size() * sizeof(XMFLOAT4) +
			vertex_atlas.size() * sizeof(XMFLOAT2) +
			vertex_colors.size() * sizeof(uint32_t) +
			vertex_windweights.size() * sizeof(uint8_t) +
			indices.size() * sizeof(uint32_t);

		for (const MorphTarget& morph : morph_targets)
		{
			size +=
				morph.vertex_positions.size() * sizeof(XMFLOAT3) +
				morph.vertex_normals.size() * sizeof(XMFLOAT3) +
				morph.sparse_indices_positions.size() * sizeof(uint32_t) +
				morph.sparse_indices_normals.size() * sizeof(uint32_t);
		}

		size += GetMemoryUsageBVH();

		return size;
	}
	size_t MeshComponent::GetMemoryUsageGPU() const
	{
		return generalBuffer.desc.size + streamoutBuffer.desc.size;
	}
	size_t MeshComponent::GetMemoryUsageBVH() const
	{
		return
			bvh.allocation.capacity() +
			bvh_leaf_aabbs.size() * sizeof(wi::primitive::AABB);
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

					oidn::BufferRef lightmapTextureData_buffer = device.newBuffer(lightmapTextureData.size());
					oidn::BufferRef texturedata_dst_buffer = device.newBuffer(texturedata_dst.size());

					lightmapTextureData_buffer.write(0, lightmapTextureData.size(), lightmapTextureData.data());

					// Create a denoising filter
					oidn::FilterRef filter = device.newFilter("RTLightmap");
					filter.setImage("color", lightmapTextureData_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
					filter.setImage("output", texturedata_dst_buffer, oidn::Format::Float3, width, height, 0, sizeof(XMFLOAT4));
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
					else
					{
						texturedata_dst_buffer.read(0, texturedata_dst.size(), texturedata_dst.data());
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
		if (IsLightmapDisableBlockCompression())
		{
			// Simple packing to R11G11B10_FLOAT format on CPU:
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
		}
		else
		{
			// BC6 compress on GPU:
			wi::texturehelper::CreateTexture(lightmap, lightmapTextureData.data(), lightmapWidth, lightmapHeight, lightmap.desc.format);
			TextureDesc desc = lightmap.desc;
			desc.format = Format::BC6H_UF16;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			Texture bc6tex;
			GraphicsDevice* device = GetDevice();
			device->CreateTexture(&desc, nullptr, &bc6tex);
			CommandList cmd = device->BeginCommandList();
			wi::renderer::BlockCompress(lightmap, bc6tex, cmd);
			wi::helper::saveTextureToMemory(bc6tex, lightmapTextureData); // internally waits for GPU completion
			lightmap.desc = desc;
		}
	}
	void ObjectComponent::DeleteRenderData()
	{
		vb_ao = {};
		vb_ao_srv = -1;
	}
	void ObjectComponent::CreateRenderData()
	{
		DeleteRenderData();

		GraphicsDevice* device = wi::graphics::GetDevice();

		if (!vertex_ao.empty())
		{
			GPUBufferDesc desc;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.size = sizeof(Vertex_AO) * vertex_ao.size();
			desc.format = Vertex_AO::FORMAT;

			auto fill_ao = [&](void* data) {
				std::memcpy(data, vertex_ao.data(), vertex_ao.size());
			};

			bool success = device->CreateBuffer2(&desc, fill_ao, &vb_ao);
			assert(success);
			device->SetName(&vb_ao, "ObjectComponent::vb_ao");

			vb_ao_srv = device->GetDescriptorIndex(&vb_ao, SubresourceType::SRV);
		}
	}

	void EnvironmentProbeComponent::CreateRenderData()
	{
		if (!textureName.empty() && !resource.IsValid())
		{
			resource = wi::resourcemanager::Load(textureName);
		}
		if (resource.IsValid())
		{
			texture = resource.GetTexture();
			SetDirty(false);
			return;
		}
		resolution = wi::math::GetNextPowerOfTwo(resolution);
		if (texture.IsValid() && resolution == texture.desc.width)
			return;
		SetDirty();

		GraphicsDevice* device = wi::graphics::GetDevice();

		TextureDesc desc;
		desc.array_size = 6;
		desc.height = resolution;
		desc.width = resolution;
		desc.usage = Usage::DEFAULT;
		desc.format = Format::BC6H_UF16;
		desc.sample_count = 1; // Note that this texture is always non-MSAA, even if probe is rendered as MSAA, because this contains resolved result
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.mip_levels = GetMipCount(resolution, resolution, 1, 16);
		desc.misc_flags = ResourceMiscFlag::TEXTURECUBE;
		desc.layout = ResourceState::SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &texture);
		device->SetName(&texture, "EnvironmentProbeComponent::texture");
	}
	void EnvironmentProbeComponent::DeleteResource()
	{
		if (resource.IsValid())
		{
			// only delete these if resource is actually valid!
			resource = {};
			texture = {};
			textureName = {};
		}
	}
	size_t EnvironmentProbeComponent::GetMemorySizeInBytes() const
	{
		return ComputeTextureMemorySizeInBytes(texture.desc);
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

	void SoftBodyPhysicsComponent::CreateFromMesh(MeshComponent& mesh)
	{
		if (weights.size() != mesh.vertex_positions.size())
		{
			weights.resize(mesh.vertex_positions.size());
			std::fill(weights.begin(), weights.end(), 1.0f);
		}
		if (physicsIndices.empty())
		{
			bool pinning_required = false;
			wi::vector<uint32_t> source;
			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			mesh.GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
				const uint32_t* indices = mesh.indices.data() + subset.indexOffset;
				for (uint32_t i = 0; i < subset.indexCount; ++i)
				{
					source.push_back(indices[i]);
					pinning_required |= weights[indices[i]] == 0;
				}
			}
			physicsIndices.resize(source.size());

			if (pinning_required)
			{
				// If there is pinning, we need to use precise LOD to retain difference between pinned and soft vertices:
				wi::vector<XMFLOAT4> vertices(mesh.vertex_positions.size());
				for (size_t i = 0; i < mesh.vertex_positions.size(); ++i)
				{
					vertices[i].x = mesh.vertex_positions[i].x;
					vertices[i].y = mesh.vertex_positions[i].y;
					vertices[i].z = mesh.vertex_positions[i].z;
					vertices[i].w = weights[i] == 0 ? 1.0f : 0.0f;
				}

				// Generate shadow indices for position+weight-only stream:
				wi::vector<uint32_t> shadow_indices(source.size());
				meshopt_generateShadowIndexBuffer(
					shadow_indices.data(), source.data(), source.size(),
					vertices.data(), vertices.size(), sizeof(XMFLOAT4), sizeof(XMFLOAT4)
				);

				size_t result = 0;
				size_t target_index_count = size_t(shadow_indices.size() * saturate(detail)) / 3 * 3;
				float target_error = 1 - saturate(detail);
				int tries = 0;
				while (result == 0 && tries < 100)
				{
					result = meshopt_simplify(
						&physicsIndices[0],
						&shadow_indices[0],
						shadow_indices.size(),
						(const float*)&mesh.vertex_positions[0],
						mesh.vertex_positions.size(),
						sizeof(XMFLOAT3),
						target_index_count,
						target_error
					);
					target_error *= 0.5f;
				}
				assert(result > 0);
				physicsIndices.resize(result);
			}
			else
			{
				// Sloppy LOD can be used if no pinning is required:
				size_t result = 0;
				size_t target_index_count = 0;
				float target_error = sqr(1 - saturate(detail));
				int tries = 0;
				while (result == 0 && tries < 100)
				{
					result = meshopt_simplifySloppy(
						&physicsIndices[0],
						&source[0],
						source.size(),
						(const float*)&mesh.vertex_positions[0],
						mesh.vertex_positions.size(),
						sizeof(XMFLOAT3),
						target_index_count,
						target_error
					);
					target_error *= 0.5f;
				}
				assert(result > 0);
				physicsIndices.resize(result);
			}

			physicsIndices.shrink_to_fit();

			// Remap physics indices to point to physics indices:
			physicsToGraphicsVertexMapping.clear();
			wi::unordered_map<uint32_t, size_t> physicsVertices;
			for (size_t i = 0; i < physicsIndices.size(); ++i)
			{
				const uint32_t graphicsInd = physicsIndices[i];
				if (physicsVertices.count(graphicsInd) == 0)
				{
					physicsVertices[graphicsInd] = physicsToGraphicsVertexMapping.size();
					physicsToGraphicsVertexMapping.push_back(graphicsInd);
				}
				physicsIndices[i] = (uint32_t)physicsVertices[graphicsInd];
			}
			physicsToGraphicsVertexMapping.shrink_to_fit();

			// BoneQueue is used for assigning the highest weighted fixed number of bones (soft body nodes) to a graphics vertex
			static constexpr int influence = 8;
			struct BoneQueue
			{
				struct Bone
				{
					uint32_t index = 0;
					float weight = 0;
					constexpr bool operator<(const Bone& other) const { return weight < other.weight; }
					constexpr bool operator>(const Bone& other) const { return weight > other.weight; }
				};
				Bone bones[influence];
				constexpr void add(uint32_t index, float weight)
				{
					int mini = 0;
					for (int i = 1; i < arraysize(bones); ++i)
					{
						if (bones[i].weight < bones[mini].weight)
						{
							mini = i;
						}
					}
					if (weight > bones[mini].weight)
					{
						bones[mini].weight = weight;
						bones[mini].index = index;
					}
				}
				void finalize()
				{
					std::sort(bones, bones + arraysize(bones), std::greater<Bone>());
					// Note: normalization of bone weights will be done in MeshComponent::CreateRenderData()
				}
				constexpr XMUINT4 get_indices() const
				{
					return XMUINT4(
						influence < 1 ? 0 : bones[0].index,
						influence < 2 ? 0 : bones[1].index,
						influence < 3 ? 0 : bones[2].index,
						influence < 4 ? 0 : bones[3].index
					);
				}
				constexpr XMUINT4 get_indices2() const
				{
					return XMUINT4(
						influence < 5 ? 0 : bones[4].index,
						influence < 6 ? 0 : bones[5].index,
						influence < 7 ? 0 : bones[6].index,
						influence < 8 ? 0 : bones[7].index
					);
				}
				constexpr XMFLOAT4 get_weights() const
				{
					return XMFLOAT4(
						influence < 1 ? 0 : bones[0].weight,
						influence < 2 ? 0 : bones[1].weight,
						influence < 3 ? 0 : bones[2].weight,
						influence < 4 ? 0 : bones[3].weight
					);
				}
				constexpr XMFLOAT4 get_weights2() const
				{
					return XMFLOAT4(
						influence < 5 ? 0 : bones[4].weight,
						influence < 6 ? 0 : bones[5].weight,
						influence < 7 ? 0 : bones[6].weight,
						influence < 8 ? 0 : bones[7].weight
					);
				}
			};

			// Create skinning bone vertex data:
			mesh.vertex_boneindices.resize(mesh.vertex_positions.size());
			mesh.vertex_boneweights.resize(mesh.vertex_positions.size());
			if (influence > 4)
			{
				mesh.vertex_boneindices2.resize(mesh.vertex_positions.size());
				mesh.vertex_boneweights2.resize(mesh.vertex_positions.size());
			}
			wi::jobsystem::context ctx;
			wi::jobsystem::Dispatch(ctx, (uint32_t)mesh.vertex_positions.size(), 64, [&](wi::jobsystem::JobArgs args) {
				const XMFLOAT3 position = mesh.vertex_positions[args.jobIndex];

				BoneQueue bones;
				for (size_t physicsInd = 0; physicsInd < physicsToGraphicsVertexMapping.size(); ++physicsInd)
				{
					const uint32_t graphicsInd = physicsToGraphicsVertexMapping[physicsInd];
					const XMFLOAT3 position2 = mesh.vertex_positions[graphicsInd];
					const float dist = wi::math::DistanceSquared(position, position2);
					// Note: 0.01 correction is carefully tweaked so that cloth_test and sponza curtains look good
					// (larger values blow up the curtains, lower values make the shading of the cloth look bad)
					const float weight = 1.0f / (0.01f + dist);
					bones.add((uint32_t)physicsInd, weight);
				}

				bones.finalize();
				mesh.vertex_boneindices[args.jobIndex] = bones.get_indices();
				mesh.vertex_boneweights[args.jobIndex] = bones.get_weights();
				if (influence > 4)
				{
					mesh.vertex_boneindices2[args.jobIndex] = bones.get_indices2();
					mesh.vertex_boneweights2[args.jobIndex] = bones.get_weights2();
				}
			});
			wi::jobsystem::Wait(ctx);
			mesh.CreateRenderData();
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
		XMMATRIX _InvV = XMMatrixInverse(nullptr, _V);
		XMStoreFloat4x4(&InvView, _InvV);
		XMStoreFloat3x3(&rotationMatrix, _InvV);
		XMStoreFloat4x4(&InvVP, XMMatrixInverse(nullptr, _VP));

		frustum.Create(_VP);
	}
	void CameraComponent::TransformCamera(const XMMATRIX& W)
	{
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

		clipPlaneOriginal = plane;

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
		resource = wi::resourcemanager::Load(filename);
		script.clear(); // will be created on first Update()
	}

	void SoundComponent::Play()
	{
		_flags |= PLAYING;
		wi::audio::Play(&soundinstance);
	}
	void SoundComponent::Stop()
	{
		_flags &= ~PLAYING;
		wi::audio::Stop(&soundinstance);
	}
	void SoundComponent::SetLooped(bool value)
	{
		soundinstance.SetLooped(value);
		if (value)
		{
			_flags |= LOOPED;
			wi::audio::CreateSoundInstance(&soundResource.GetSound(), &soundinstance);
		}
		else
		{
			_flags &= ~LOOPED;
			wi::audio::ExitLoop(&soundinstance);
		}
	}
	void SoundComponent::SetDisable3D(bool value)
	{
		if (value)
		{
			_flags |= DISABLE_3D;
		}
		else
		{
			_flags &= ~DISABLE_3D;
		}
		wi::audio::CreateSoundInstance(&soundResource.GetSound(), &soundinstance);
	}

}
