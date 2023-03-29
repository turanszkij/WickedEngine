#include "stdafx.h"
#include "wiScene.h"
#include "ModelImporter.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::ecs;

struct membuf : std::streambuf
{
	membuf(char* begin, char* end) {
		this->setg(begin, begin, end);
	}
};

// Custom material file reader:
class MaterialFileReader : public tinyobj::MaterialReader {
public:
	explicit MaterialFileReader(const std::string& mtl_basedir)
		: m_mtlBaseDir(mtl_basedir) {}
	virtual ~MaterialFileReader() {}
	virtual bool operator()(const std::string& matId,
		std::vector<tinyobj::material_t>* materials,
		std::map<std::string, int>* matMap, std::string* err)
	{
		std::string filepath;

		if (!m_mtlBaseDir.empty()) {
			filepath = std::string(m_mtlBaseDir) + matId;
		}
		else {
			filepath = matId;
		}

		wi::vector<uint8_t> filedata;
		if (!wi::helper::FileRead(filepath, filedata))
		{
			std::string ss;
			ss += "WARN: Material file [ " + filepath + " ] not found.\n";
			if (err) {
				(*err) += ss;
			}
			return false;
		}

		membuf sbuf((char*)filedata.data(), (char*)filedata.data() + filedata.size());
		std::istream matIStream(&sbuf);

		std::string warning;
		LoadMtl(matMap, materials, &matIStream, &warning);

		if (!warning.empty()) {
			if (err) {
				(*err) += warning;
			}
		}

		return true;
	}

private:
	std::string m_mtlBaseDir;
};

// Transform the data from OBJ space to engine-space:
static const bool transform_to_LH = true;

void ImportModel_OBJ(const std::string& fileName, Scene& scene)
{
	std::string directory = wi::helper::GetDirectoryFromPath(fileName);
	std::string name = wi::helper::GetFileNameFromPath(fileName);

	tinyobj::attrib_t obj_attrib;
	std::vector<tinyobj::shape_t> obj_shapes;
	std::vector<tinyobj::material_t> obj_materials;
	std::string obj_errors;

	wi::vector<uint8_t> filedata;
	bool success = wi::helper::FileRead(fileName, filedata);

	if (success)
	{
		membuf sbuf((char*)filedata.data(), (char*)filedata.data() + filedata.size());
		std::istream in(&sbuf);
		MaterialFileReader matFileReader(directory);
		success = tinyobj::LoadObj(&obj_attrib, &obj_shapes, &obj_materials, &obj_errors, &in, &matFileReader, true);
	}
	else
	{
		obj_errors = "Failed to read file: " + fileName;
	}

	if (!obj_errors.empty())
	{
		wi::backlog::post(obj_errors, wi::backlog::LogLevel::Error);
	}

	if (success)
	{
		Entity rootEntity = CreateEntity();
		scene.transforms.Create(rootEntity);
		scene.names.Create(rootEntity) = name;

		// Load material library:
		wi::vector<Entity> materialLibrary = {};
		for (auto& obj_material : obj_materials)
		{
			Entity materialEntity = scene.Entity_CreateMaterial(obj_material.name);
			scene.Component_Attach(materialEntity, rootEntity);
			MaterialComponent& material = *scene.materials.GetComponent(materialEntity);

			material.baseColor = XMFLOAT4(obj_material.diffuse[0], obj_material.diffuse[1], obj_material.diffuse[2], 1);
			material.textures[MaterialComponent::BASECOLORMAP].name = obj_material.diffuse_texname;
			material.textures[MaterialComponent::DISPLACEMENTMAP].name = obj_material.displacement_texname;
			material.emissiveColor.x = obj_material.emission[0];
			material.emissiveColor.y = obj_material.emission[1];
			material.emissiveColor.z = obj_material.emission[2];
			material.emissiveColor.w = std::max(obj_material.emission[0], std::max(obj_material.emission[1], obj_material.emission[2]));
			//material.refractionIndex = obj_material.ior;
			material.metalness = obj_material.metallic;
			material.textures[MaterialComponent::NORMALMAP].name = obj_material.normal_texname;
			material.textures[MaterialComponent::SURFACEMAP].name = obj_material.specular_texname;
			material.roughness = obj_material.roughness;

			if (material.textures[MaterialComponent::NORMALMAP].name.empty())
			{
				material.textures[MaterialComponent::NORMALMAP].name = obj_material.bump_texname;
			}
			if (material.textures[MaterialComponent::SURFACEMAP].name.empty())
			{
				material.textures[MaterialComponent::SURFACEMAP].name = obj_material.specular_highlight_texname;
			}

			for (auto& x : material.textures)
			{
				if (!x.name.empty())
				{
					x.name = directory + x.name;
				}
			}

			material.CreateRenderData();

			materialLibrary.push_back(materialEntity); // for subset-indexing...
		}

		if (materialLibrary.empty())
		{
			// Create default material if nothing was found:
			Entity materialEntity = scene.Entity_CreateMaterial("OBJImport_defaultMaterial");
			scene.Component_Attach(materialEntity, rootEntity);
			MaterialComponent& material = *scene.materials.GetComponent(materialEntity);
			materialLibrary.push_back(materialEntity); // for subset-indexing...
		}

		// Load objects, meshes:
		for (auto& shape : obj_shapes)
		{
			Entity objectEntity = scene.Entity_CreateObject(shape.name);
			scene.Component_Attach(objectEntity, rootEntity);
			Entity meshEntity = scene.Entity_CreateMesh(shape.name + "_mesh");
			scene.Component_Attach(meshEntity, rootEntity);
			ObjectComponent& object = *scene.objects.GetComponent(objectEntity);
			MeshComponent& mesh = *scene.meshes.GetComponent(meshEntity);

			object.meshID = meshEntity;

			wi::unordered_map<int, int> registered_materialIndices = {};
			wi::unordered_map<size_t, uint32_t> uniqueVertices = {};

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

					int materialIndex = std::max(0, shape.mesh.material_ids[i / 3]); // this indexes the material library
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
					size_t vertexHash = 0;
					wi::helper::hash_combine(vertexHash, index.vertex_index);
					wi::helper::hash_combine(vertexHash, index.normal_index);
					wi::helper::hash_combine(vertexHash, index.texcoord_index);
					wi::helper::hash_combine(vertexHash, materialIndex);

					if (uniqueVertices.count(vertexHash) == 0)
					{
						uniqueVertices[vertexHash] = (uint32_t)mesh.vertex_positions.size();
						mesh.vertex_positions.push_back(pos);
						mesh.vertex_normals.push_back(nor);
						mesh.vertex_uvset_0.push_back(tex);
					}
					mesh.indices.push_back(uniqueVertices[vertexHash]);
					mesh.subsets.back().indexCount++;
				}
			}
			mesh.CreateRenderData();
		}

	}
	else
	{
		wi::helper::messageBox("OBJ import failed! Check backlog for errors!", "Error!");
	}
}
