#include "stdafx.h"
#include "wiScene.h"
#include "ModelImporter.h"

#include "miniply.h"
#include "miniply.cpp"

using namespace miniply;

using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::ecs;

void ImportModel_PLY(const std::string& fileName, wi::scene::Scene& scene)
{
	PLYReader reader(fileName.c_str());
	if (!reader.valid())
	{
		wilog_error("PLY file invalid, aborting  import!");
		return;
	}

	uint32_t propIndices[64] = {};
	bool gotVerts = false;
	bool gotFaces = false;

	wi::vector<uint32_t> indices;
	wi::vector<XMFLOAT3> positions;
	wi::vector<XMFLOAT3> normals;
	wi::vector<XMFLOAT2> texcoords;

	// splats:
	wi::vector<XMFLOAT4> rotations;
	wi::vector<XMFLOAT3> scales;
	wi::vector<float> opacities;
	wi::vector<XMFLOAT3> f_dc;
	wi::vector<float> f_rest;

	while (reader.has_element() && (!gotVerts || !gotFaces))
	{
		if (reader.element_is(miniply::kPLYVertexElement) && reader.load_element() && reader.find_pos(propIndices))
		{
			const uint32_t vertexCount = reader.num_rows();
			positions.resize(vertexCount);
			reader.extract_properties(propIndices, 3, miniply::PLYPropertyType::Float, positions.data());
			if (reader.find_normal(propIndices))
			{
				normals.resize(vertexCount);
				reader.extract_properties(propIndices, 3, miniply::PLYPropertyType::Float, normals.data());
			}
			if (reader.find_texcoord(propIndices))
			{
				texcoords.resize(vertexCount);
				reader.extract_properties(propIndices, 2, miniply::PLYPropertyType::Float, texcoords.data());
			}
			if (reader.find_properties(propIndices, 4, "rot_0", "rot_1", "rot_2", "rot_3"))
			{
				rotations.resize(vertexCount);
				reader.extract_properties(propIndices, 4, miniply::PLYPropertyType::Float, rotations.data());
			}
			if (reader.find_properties(propIndices, 3, "scale_0", "scale_1", "scale_2"))
			{
				scales.resize(vertexCount);
				reader.extract_properties(propIndices, 3, miniply::PLYPropertyType::Float, scales.data());
			}
			if (reader.find_properties(propIndices, 1, "opacity"))
			{
				opacities.resize(vertexCount);
				reader.extract_properties(propIndices, 1, miniply::PLYPropertyType::Float, opacities.data());
			}
			if (reader.find_properties(propIndices, 3, "f_dc_0", "f_dc_1", "f_dc_2"))
			{
				f_dc.resize(vertexCount);
				reader.extract_properties(propIndices, 1, miniply::PLYPropertyType::Float, f_dc.data());
			}
			if (reader.find_properties(propIndices, 45,
				"f_rest_0", "f_rest_1", "f_rest_2", "f_rest_3", "f_rest_4", "f_rest_5",
				"f_rest_6", "f_rest_7", "f_rest_8", "f_rest_9", "f_rest_10", "f_rest_11", "f_rest_12",
				"f_rest_13", "f_rest_14", "f_rest_15", "f_rest_16", "f_rest_17", "f_rest_18",
				"f_rest_19", "f_rest_20", "f_rest_21", "f_rest_22", "f_rest_23", "f_rest_24",
				"f_rest_25", "f_rest_26", "f_rest_27", "f_rest_28", "f_rest_29", "f_rest_30", "f_rest_31",
				"f_rest_32", "f_rest_33", "f_rest_34", "f_rest_35", "f_rest_36", "f_rest_37", "f_rest_38",
				"f_rest_39", "f_rest_40", "f_rest_41", "f_rest_42", "f_rest_43", "f_rest_44"))
			{
				f_rest.resize(vertexCount * 45);
				reader.extract_properties(propIndices, 45, miniply::PLYPropertyType::Float, f_rest.data());
			}
			gotVerts = true;
		}
		else if (reader.element_is(miniply::kPLYFaceElement) && reader.load_element() && reader.find_indices(propIndices))
		{
			bool polys = reader.requires_triangulation(propIndices[0]);
			if (polys && !gotVerts)
			{
				wilog_error("PLY Error: need vertex positions to triangulate faces.");
				break;
			}
			if (polys)
			{
				indices.resize(reader.num_triangles(propIndices[0]) * 3);
				reader.extract_triangles(propIndices[0], (float*)positions.data(), (uint32_t)positions.size(), miniply::PLYPropertyType::Int, indices.data());
			}
			else
			{
				indices.resize(reader.num_rows() * 3);
				reader.extract_list_property(propIndices[0], miniply::PLYPropertyType::Int, indices.data());
			}
			gotFaces = true;
		}
		if (gotVerts && gotFaces)
			break;
		reader.next_element();
	}

	if (!positions.empty())
	{
		Entity entity = CreateEntity();
		scene.names.Create(entity) = wi::helper::GetFileNameFromPath(fileName);
		scene.transforms.Create(entity);
		if (!f_rest.empty())
		{
			// It's a gaussian splat model:
			wi::GaussianSplatModel& splat = scene.gaussian_splats.Create(entity);
			for (auto& x : positions)
			{
				//x.y *= -1;
			}
			for (auto& x : rotations)
			{
				x = XMFLOAT4(x.y, x.z, x.w, x.x);
				//x.x = -x.x;
				//x.y = -x.y;
				//x.z = -x.z;
				XMVECTOR Q = XMLoadFloat4(&x);
				Q = XMQuaternionNormalize(Q);
				XMStoreFloat4(&x, Q);
			}
			splat.positions = std::move(positions);
			splat.normals = std::move(normals);
			splat.rotations = std::move(rotations);
			splat.scales = std::move(scales);
			splat.opacities = std::move(opacities);
			splat.f_dc = std::move(f_dc);
			splat.f_rest = std::move(f_rest);
			splat.CreateRenderData();
		}
		else
		{
			// It's a regular mesh:
			MeshComponent& mesh = scene.meshes.Create(entity);
			mesh.indices = std::move(indices);
			mesh.vertex_positions = std::move(positions);
			mesh.vertex_normals = std::move(normals);
			mesh.vertex_uvset_0 = std::move(texcoords);
			MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
			subset.materialID = entity;
			subset.indexCount = (uint32_t)mesh.indices.size();
			if (mesh.subsets.back().indexCount == 0)
			{
				// Autogen indices:
				size_t vertexOffset = 0;
				size_t vertexCount = mesh.vertex_positions.size() / 3 * 3;
				size_t indexOffset = mesh.indices.size();
				mesh.indices.resize(indexOffset + vertexCount);
				for (size_t vi = 0; vi < vertexCount; vi += 3)
				{
					mesh.indices[indexOffset + vi + 0] = uint32_t(vertexOffset + vi);
					mesh.indices[indexOffset + vi + 1] = uint32_t(vertexOffset + vi);
					mesh.indices[indexOffset + vi + 2] = uint32_t(vertexOffset + vi);
				}
				mesh.subsets.back().indexOffset = (uint32_t)indexOffset;
				mesh.subsets.back().indexCount = (uint32_t)vertexCount;
			}
			mesh.FlipCulling();
			if (mesh.vertex_normals.empty())
			{
				mesh.ComputeNormals(MeshComponent::COMPUTE_NORMALS_HARD);
			}
			scene.materials.Create(entity);
			ObjectComponent& object = scene.objects.Create(entity);
			object.meshID = entity;
			mesh.CreateRenderData();
		}
	}
}
