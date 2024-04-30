#include "stdafx.h"
#include "wiScene.h"
#include "ModelImporter.h"

// ufbx Documentation: https://ufbx.github.io/
#define UFBX_REAL_TYPE float
#include "ufbx.h"
#include "ufbx.c"

using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::ecs;

void ImportModel_FBX(const std::string& filename, wi::scene::Scene& scene)
{
	ufbx_load_opts opts = {};
	opts.target_axes = ufbx_axes_left_handed_y_up;
	opts.target_unit_meters = 1.0f;
	opts.target_camera_axes = ufbx_axes_left_handed_y_up;
	opts.target_light_axes = ufbx_axes_left_handed_y_up;
	ufbx_error error = {};
	ufbx_scene* fbxscene = ufbx_load_file(filename.c_str(), &opts, &error);
	if (fbxscene == nullptr)
	{
		std::string str = "FBX import error: ";
		str += error.description.data;
		wi::backlog::post(str, wi::backlog::LogLevel::Error);
		wi::helper::messageBox(str, "Error!");
		return;
	}

	wi::unordered_map<const ufbx_material*, Entity> material_lookup;
	wi::unordered_map<const ufbx_mesh*, Entity> mesh_lookup;
	wi::unordered_map<const ufbx_node*, Entity> node_lookup;

	Entity rootEntity = INVALID_ENTITY;

	for (const ufbx_material* material : fbxscene->materials)
	{
		Entity entity = CreateEntity();
		material_lookup[material] = entity;
		if (material->name.length > 0)
		{
			scene.names.Create(entity) = material->name.data;
		}
		MaterialComponent& materialcomponent = scene.materials.Create(entity);
		materialcomponent.baseColor = XMFLOAT4(
			material->pbr.base_color.value_vec4.x,
			material->pbr.base_color.value_vec4.y,
			material->pbr.base_color.value_vec4.z,
			material->pbr.base_color.value_vec4.w
		);
		materialcomponent.roughness = material->pbr.roughness.value_real;
		materialcomponent.CreateRenderData();
	}

	for (const ufbx_mesh* mesh : fbxscene->meshes)
	{
		Entity entity = CreateEntity();
		mesh_lookup[mesh] = entity;
		if (mesh->name.length > 0)
		{
			scene.names.Create(entity) = mesh->name.data;
		}
		MeshComponent& meshcomponent = scene.meshes.Create(entity);
		wi::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);
		for (const ufbx_mesh_part& part : mesh->material_parts)
		{
			wi::vector<XMFLOAT3> positions;
			wi::vector<XMFLOAT3> normals;
			for (uint32_t face_index : part.face_indices)
			{
				ufbx_face face = mesh->faces[face_index];
				uint32_t num_tris = ufbx_triangulate_face(tri_indices.data(), tri_indices.size(), mesh, face);
				for (size_t i = 0; i < num_tris * 3; i++)
				{
					uint32_t index = tri_indices[i];
					if (mesh->vertex_position.exists)
					{
						positions.push_back(*(const XMFLOAT3*)mesh->vertex_position[index].v);
					}
					if (mesh->vertex_normal.exists)
					{
						normals.push_back(*(const XMFLOAT3*)mesh->vertex_normal[index].v);
					}
				}
			}
			assert(positions.size() == part.num_triangles * 3);

			wi::vector<uint32_t> indices;
			indices.resize(part.num_triangles * 3);

			ufbx_vertex_stream streams[] = {
				{ positions.data(), positions.size(), sizeof(positions[0])},
				{ normals.data(), normals.size(), sizeof(normals[0])},
			};

			size_t num_vertices = ufbx_generate_indices(streams, arraysize(streams), indices.data(), indices.size(), nullptr, nullptr);

			positions.resize(num_vertices);
			normals.resize(num_vertices);

			MeshComponent::MeshSubset& subset = meshcomponent.subsets.emplace_back();
			subset.indexOffset = (uint32_t)meshcomponent.indices.size();
			subset.indexCount = uint32_t(indices.size());
			subset.materialID = scene.materials.GetEntity(0);

			meshcomponent.indices.insert(meshcomponent.indices.begin(), indices.begin(), indices.end());
			meshcomponent.vertex_positions.insert(meshcomponent.vertex_positions.end(), positions.begin(), positions.end());
			meshcomponent.vertex_normals.insert(meshcomponent.vertex_normals.end(), normals.begin(), normals.end());

		}
		meshcomponent.CreateRenderData();
	}

	for (const ufbx_node* node : fbxscene->nodes)
	{
		Entity entity = CreateEntity();
		node_lookup[node] = entity;
		if (node->name.length > 0)
		{
			scene.names.Create(entity) = node->name.data;
		}

		if (node->is_root)
		{
			rootEntity = entity;
		}

		TransformComponent& transform = scene.transforms.Create(entity);
		transform.scale_local.x = node->local_transform.scale.x;
		transform.scale_local.y = node->local_transform.scale.y;
		transform.scale_local.z = node->local_transform.scale.z;
		transform.rotation_local.x = node->local_transform.rotation.x;
		transform.rotation_local.y = node->local_transform.rotation.y;
		transform.rotation_local.z = node->local_transform.rotation.z;
		transform.rotation_local.w = node->local_transform.rotation.w;
		transform.translation_local.x = node->local_transform.translation.x;
		transform.translation_local.y = node->local_transform.translation.y;
		transform.translation_local.z = node->local_transform.translation.z;

		if (node->mesh)
		{
			ObjectComponent& object = scene.objects.Create(entity);
			object.meshID = mesh_lookup[node->mesh];
		}
	}

	// After all nodes were iterated, determine hierarchy:
	for (size_t i = 0; i < fbxscene->nodes.count; i++)
	{
		ufbx_node* node = fbxscene->nodes.data[i];
		if (node->parent)
		{
			Entity entity = node_lookup[node];
			Entity parent = node_lookup[node->parent];
			scene.Component_Attach(entity, parent, true);
		}
	}

	ufbx_free_scene(fbxscene);
}
