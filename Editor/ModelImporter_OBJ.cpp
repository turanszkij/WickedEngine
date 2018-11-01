#include "stdafx.h"
#include "wiSceneSystem.h"
#include "ModelImporter.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <fstream>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiSceneSystem;
using namespace wiECS;

// Transform the data from OBJ space to engine-space:
static const bool transform_to_LH = true;

void ImportModel_OBJ(const std::string& fileName)
{
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	wiHelper::RemoveExtensionFromFileName(name);

	Scene scene;

	tinyobj::attrib_t obj_attrib;
	vector<tinyobj::shape_t> obj_shapes;
	vector<tinyobj::material_t> obj_materials;
	string obj_errors;

	bool success = tinyobj::LoadObj(&obj_attrib, &obj_shapes, &obj_materials, &obj_errors, fileName.c_str(), directory.c_str(), true);

	if (!obj_errors.empty())
	{
		wiBackLog::post(obj_errors.c_str());
	}

	if (success)
	{
		// Load material library:
		vector<Entity> materialLibrary = {};
		for (auto& obj_material : obj_materials)
		{
			Entity materialEntity = scene.Entity_CreateMaterial(obj_material.name);
			MaterialComponent& material = *scene.materials.GetComponent(materialEntity);

			material.baseColor = XMFLOAT4(obj_material.diffuse[0], obj_material.diffuse[1], obj_material.diffuse[2], 1);
			material.baseColorMapName = obj_material.diffuse_texname;
			material.displacementMapName = obj_material.displacement_texname;
			material.emissive = max(obj_material.emission[0], max(obj_material.emission[1], obj_material.emission[2]));
			material.refractionIndex = obj_material.ior;
			material.metalness = obj_material.metallic;
			material.normalMapName = obj_material.normal_texname;
			material.surfaceMapName = obj_material.specular_texname;
			material.roughness = obj_material.roughness;

			if (material.normalMapName.empty())
			{
				material.normalMapName = obj_material.bump_texname;
			}
			if (material.surfaceMapName.empty())
			{
				material.surfaceMapName = obj_material.specular_highlight_texname;
			}

			if (!material.surfaceMapName.empty())
			{
				material.surfaceMap = (Texture2D*)wiResourceManager::GetGlobal().add(directory + material.surfaceMapName);
			}
			if (!material.baseColorMapName.empty())
			{
				material.baseColorMap = (Texture2D*)wiResourceManager::GetGlobal().add(directory + material.baseColorMapName);
			}
			if (!material.normalMapName.empty())
			{
				material.normalMap = (Texture2D*)wiResourceManager::GetGlobal().add(directory + material.normalMapName);
			}
			if (!material.displacementMapName.empty())
			{
				material.displacementMap = (Texture2D*)wiResourceManager::GetGlobal().add(directory + material.displacementMapName);
			}

			materialLibrary.push_back(materialEntity); // for subset-indexing...
		}

		if (materialLibrary.empty())
		{
			// Create default material if nothing was found:
			Entity materialEntity = scene.Entity_CreateMaterial("OBJImport_defaultMaterial");
			MaterialComponent& material = *scene.materials.GetComponent(materialEntity);
			materialLibrary.push_back(materialEntity); // for subset-indexing...
		}

		// Load objects, meshes:
		for (auto& shape : obj_shapes)
		{
			Entity objectEntity = scene.Entity_CreateObject(shape.name);
			Entity meshEntity = scene.Entity_CreateMesh(shape.name + "_mesh");
			ObjectComponent& object = *scene.objects.GetComponent(objectEntity);
			MeshComponent& mesh = *scene.meshes.GetComponent(meshEntity);

			object.meshID = meshEntity;

			unordered_map<int, int> registered_materialIndices = {};
			unordered_map<size_t, uint32_t> uniqueVertices = {};

			for (size_t i = 0; i < shape.mesh.indices.size(); i += 3)
			{
				tinyobj::index_t reordered_indices[] = {
					shape.mesh.indices[i + 0],
					shape.mesh.indices[i + 1],
					shape.mesh.indices[i + 2],
				};

				// todo: option param would be better
				bool flipCulling = false;
				if (flipCulling)
				{
					reordered_indices[1] = shape.mesh.indices[i + 2];
					reordered_indices[2] = shape.mesh.indices[i + 1];
				}

				for (auto& index : reordered_indices)
				{
					XMFLOAT3 pos = XMFLOAT3(
						obj_attrib.vertices[index.vertex_index * 3 + 0],
						obj_attrib.vertices[index.vertex_index * 3 + 1],
						obj_attrib.vertices[index.vertex_index * 3 + 2]
					);

					XMFLOAT3 nor = XMFLOAT3(0, 0, 0);
					if (!obj_attrib.normals.empty())
					{
						nor = XMFLOAT3(
							obj_attrib.normals[index.normal_index * 3 + 0],
							obj_attrib.normals[index.normal_index * 3 + 1],
							obj_attrib.normals[index.normal_index * 3 + 2]
						);
					}

					XMFLOAT2 tex = XMFLOAT2(0, 0);
					if (index.texcoord_index >= 0 && !obj_attrib.texcoords.empty())
					{
						tex = XMFLOAT2(
							obj_attrib.texcoords[index.texcoord_index * 2 + 0],
							1 - obj_attrib.texcoords[index.texcoord_index * 2 + 1]
						);
					}

					int materialIndex = max(0, shape.mesh.material_ids[i / 3]); // this indexes the material library
					if (registered_materialIndices.count(materialIndex) == 0)
					{
						registered_materialIndices[materialIndex] = (int)mesh.subsets.size();
						mesh.subsets.push_back(MeshComponent::MeshSubset());
						mesh.subsets.back().materialID = materialLibrary[materialIndex];
						mesh.subsets.back().indexOffset = (uint32_t)mesh.indices.size();
					}

					if (transform_to_LH)
					{
						pos.z *= -1;
						nor.z *= -1;
					}

					// eliminate duplicate vertices by means of hashing:
					size_t hashes[] = {
						hash<int>{}(index.vertex_index),
						hash<int>{}(index.normal_index),
						hash<int>{}(index.texcoord_index),
						hash<int>{}(materialIndex),
					};
					size_t vertexHash = (((hashes[0] ^ (hashes[1] << 1) >> 1) ^ (hashes[2] << 1)) >> 1) ^ (hashes[3] << 1);

					if (uniqueVertices.count(vertexHash) == 0)
					{
						uniqueVertices[vertexHash] = (uint32_t)mesh.vertex_positions.size();
						mesh.vertex_positions.push_back(pos);
						mesh.vertex_normals.push_back(nor);
						mesh.vertex_texcoords.push_back(tex);
					}
					mesh.indices.push_back(uniqueVertices[vertexHash]);
					mesh.subsets.back().indexCount++;
				}
			}
			mesh.CreateRenderData();
		}

		wiRenderer::GetScene().Merge(scene);
	}
	else
	{
		wiHelper::messageBox("OBJ import failed! Check backlog for errors!", "Error!");
	}
}
