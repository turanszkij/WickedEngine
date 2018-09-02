#include "stdafx.h"
#include "ModelImporter.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <fstream>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiSceneSystem;
using namespace wiECS;

Entity ImportModel_OBJ(const std::string& fileName)
{
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	wiHelper::RemoveExtensionFromFileName(name);


	Scene& scene = wiRenderer::GetScene();


	tinyobj::attrib_t obj_attrib;
	vector<tinyobj::shape_t> obj_shapes;
	vector<tinyobj::material_t> obj_materials;
	string obj_errors;

	bool success = tinyobj::LoadObj(&obj_attrib, &obj_shapes, &obj_materials, &obj_errors, fileName.c_str(), directory.c_str(), true);

	if (success)
	{
		//Model* model = new Model;
		//model->name = name;

		Entity modelEntity = scene.Entity_CreateModel(name);
		ModelComponent& model = *scene.models.GetComponent(modelEntity);


		// Load material library:
		vector<Entity> materialLibrary = {};
		for (auto& obj_material : obj_materials)
		{
			//Material* material = new Material(obj_material.name);

			Entity materialEntity = scene.Entity_CreateMaterial(obj_material.name);
			MaterialComponent& material = *scene.materials.GetComponent(materialEntity);

			material.baseColor = XMFLOAT4(obj_material.diffuse[0], obj_material.diffuse[1], obj_material.diffuse[2], 1);
			material.baseColorMapName = obj_material.diffuse_texname;
			material.displacementMapName = obj_material.displacement_texname;
			if (material.displacementMapName.empty())
			{
				material.displacementMapName = obj_material.bump_texname;
			}
			material.emissive = max(obj_material.emission[0], max(obj_material.emission[1], obj_material.emission[2]));
			//obj_material.emissive_texname;
			material.refractionIndex = obj_material.ior;
			material.metalness = obj_material.metallic;
			//obj_material.metallic_texname;
			material.normalMapName = obj_material.normal_texname;
			material.surfaceMapName = obj_material.reflection_texname;
			material.roughness = obj_material.roughness;
			//obj_material.roughness_texname;
			//material.specular_power = (int)obj_material.shininess;
			//material.specular = XMFLOAT4(obj_material.specular[0], obj_material.specular[1], obj_material.specular[2], 1);
			//material.specularMapName = obj_material.specular_texname;

			if (!material.surfaceMapName.empty())
			{
				material.surfaceMapName = directory + material.surfaceMapName;
				material.surfaceMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material.surfaceMapName);
			}
			if (!material.baseColorMapName.empty())
			{
				material.baseColorMapName = directory + material.baseColorMapName;
				material.baseColorMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material.baseColorMapName);
			}
			if (!material.normalMapName.empty())
			{
				material.normalMapName = directory + material.normalMapName;
				material.normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material.normalMapName);
			}
			if (!material.displacementMapName.empty())
			{
				material.displacementMapName = directory + material.displacementMapName;
				material.displacementMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material.displacementMapName);
			}
			//if (!material.specularMapName.empty())
			//{
			//	material.specularMapName = directory + material.specularMapName;
			//	material.specularMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material.specularMapName);
			//}

			//material.ConvertToPhysicallyBasedMaterial();

			materialLibrary.push_back(materialEntity); // for subset-indexing...
			model.materials.insert(materialEntity);
			//model->materials.insert(make_pair(material.name, material));
		}

		if (materialLibrary.empty())
		{
			// Create default material if nothing was found:
			Entity materialEntity = scene.Entity_CreateMaterial("OBJImport_defaultMaterial");
			MaterialComponent& material = *scene.materials.GetComponent(materialEntity);
			materialLibrary.push_back(materialEntity); // for subset-indexing...
			model.materials.insert(materialEntity);
			//Material* material = new Material("OBJImport_defaultMaterial");
			//materialLibrary.push_back(material);
			//model->materials.insert(make_pair(material.name, material));
		}

		// Load objects, meshes:
		for (auto& shape : obj_shapes)
		{
			//Object* object = new Object(shape.name);
			//Mesh* mesh = new Mesh(shape.name + "_mesh");

			Entity objectEntity = scene.Entity_CreateObject(shape.name);
			Entity meshEntity = scene.Entity_CreateMesh(shape.name + "_mesh");
			ObjectComponent& object = *scene.objects.GetComponent(objectEntity);
			MeshComponent& mesh = *scene.meshes.GetComponent(meshEntity);

			//scene.Component_Attach(objectEntity, modelEntity);

			object.meshID = meshEntity;
			mesh.renderable = true;

			XMFLOAT3 min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
			XMFLOAT3 max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

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
					MeshComponent::Vertex_FULL vert;

					vert.pos = XMFLOAT4(
						obj_attrib.vertices[index.vertex_index * 3 + 0],
						obj_attrib.vertices[index.vertex_index * 3 + 1],
						obj_attrib.vertices[index.vertex_index * 3 + 2],
						0
					);

					if (!obj_attrib.normals.empty())
					{
						vert.nor = XMFLOAT4(
							obj_attrib.normals[index.normal_index * 3 + 0],
							obj_attrib.normals[index.normal_index * 3 + 1],
							obj_attrib.normals[index.normal_index * 3 + 2],
							0
						);
					}

					if (index.texcoord_index >= 0 && !obj_attrib.texcoords.empty())
					{
						vert.tex = XMFLOAT4(
							obj_attrib.texcoords[index.texcoord_index * 2 + 0],
							1 - obj_attrib.texcoords[index.texcoord_index * 2 + 1],
							0, 0
						);
					}

					int materialIndex = max(0, shape.mesh.material_ids[i / 3]); // this indexes the material library
					if (registered_materialIndices.count(materialIndex) == 0)
					{
						registered_materialIndices[materialIndex] = (int)mesh.subsets.size();
						mesh.subsets.push_back(MeshComponent::MeshSubset());
						//MaterialComponent* material = materialLibrary[materialIndex];
						mesh.subsets.back().materialID = materialLibrary[materialIndex];
						//mesh.materialNames.push_back(material.name);
					}
					vert.tex.z = (float)registered_materialIndices[materialIndex]; // this indexes a mesh subset

					// todo: option parameter would be better
					const bool flipZ = true;
					if (flipZ)
					{
						vert.pos.z *= -1;
						vert.nor.z *= -1;
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
						uniqueVertices[vertexHash] = (uint32_t)mesh.vertices_FULL.size();
						mesh.vertices_FULL.push_back(vert);
					}
					mesh.indices.push_back(uniqueVertices[vertexHash]);

					min = wiMath::Min(min, XMFLOAT3(vert.pos.x, vert.pos.y, vert.pos.z));
					max = wiMath::Max(max, XMFLOAT3(vert.pos.x, vert.pos.y, vert.pos.z));
				}
			}
			mesh.aabb.create(min, max);
			mesh.CreateRenderData();

			//// We need to eliminate colliding mesh names, because objects can reference them by names:
			////	Note: in engine, object is decoupled from mesh, for instancing support. OBJ file have only meshes and names can collide there.
			//string meshName = mesh->name;
			//uint32_t unique_counter = 0;
			//bool meshNameCollision = model->meshes.count(meshName) != 0;
			//while (meshNameCollision)
			//{
			//	meshName = mesh->name + to_string(unique_counter);
			//	meshNameCollision = model->meshes.count(meshName) != 0;
			//	unique_counter++;
			//}
			//mesh->name = meshName;

			//object->meshName = mesh->name;

			model.objects.insert(objectEntity);
			model.meshes.insert(meshEntity);

			//model->objects.insert(object);
			//model->meshes.insert(make_pair(mesh->name, mesh));
		}

		//model->FinishLoading();

		return modelEntity;
	}

	if (!obj_errors.empty())
	{
		wiBackLog::post(obj_errors.c_str());
		wiHelper::messageBox("OBJ import failed! Check backlog for errors!", "Error!");
	}

	return INVALID_ENTITY;
}
