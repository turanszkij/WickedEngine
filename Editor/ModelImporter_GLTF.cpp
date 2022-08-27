#include "stdafx.h"
#include "wiScene.h"
#include "ModelImporter.h"
#include "wiRandom.h"

#include "Utility/stb_image.h"

#include <string>
#include <limits>
#include <fstream>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_FS
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::ecs;


// Transform the data from glTF space to engine-space:
static const bool transform_to_LH = true;


namespace tinygltf
{

	bool FileExists(const std::string& abs_filename, void*) {
		return wi::helper::FileExists(abs_filename);
	}

	std::string ExpandFilePath(const std::string& filepath, void*) {
#ifdef _WIN32
		DWORD len = ExpandEnvironmentStringsA(filepath.c_str(), NULL, 0);
		char* str = new char[len];
		ExpandEnvironmentStringsA(filepath.c_str(), str, len);

		std::string s(str);

		delete[] str;

		return s;
#else

#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR) || \
	defined(__ANDROID__) || defined(__EMSCRIPTEN__)
		// no expansion
		std::string s = filepath;
#else
		std::string s;
		wordexp_t p;

		if (filepath.empty()) {
			return "";
		}

		// char** w;
		int ret = wordexp(filepath.c_str(), &p, 0);
		if (ret) {
			// err
			s = filepath;
			return s;
		}

		// Use first element only.
		if (p.we_wordv) {
			s = std::string(p.we_wordv[0]);
			wordfree(&p);
		}
		else {
			s = filepath;
		}

#endif

		return s;
#endif
	}

	bool ReadWholeFile(std::vector<unsigned char>* out, std::string* err,
		const std::string& filepath, void*) {
		return wi::helper::FileRead(filepath, *out);
	}

	bool WriteWholeFile(std::string* err, const std::string& filepath,
		const std::vector<unsigned char>& contents, void*) {
		return wi::helper::FileWrite(filepath, contents.data(), contents.size());
	}

	bool LoadImageData(Image *image, const int image_idx, std::string *err,
		std::string *warn, int req_width, int req_height,
		const unsigned char *bytes, int size, void *userdata)
	{
		(void)warn;

		if (image->uri.empty())
		{
			// Force some image resource name:
			std::string ss;
			do {
				ss.clear();
				ss += "gltfimport_" + std::to_string(wi::random::GetRandom(std::numeric_limits<int>::max())) + ".png";
			} while (wi::resourcemanager::Contains(ss)); // this is to avoid overwriting an existing imported image
			image->uri = ss;
		}

		auto resource = wi::resourcemanager::Load(
			image->uri,
			wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA,
			(const uint8_t*)bytes,
			(size_t)size
		);

		if (!resource.IsValid())
		{
			return false;
		}

		image->width = resource.GetTexture().desc.width;
		image->height = resource.GetTexture().desc.height;
		image->component = 4;

		wi::resourcemanager::ResourceSerializer* seri = (wi::resourcemanager::ResourceSerializer*)userdata;
		seri->resources.push_back(resource);

		return true;
	}

	bool WriteImageData(const std::string *basepath, const std::string *filename,
		Image *image, bool embedImages, void *)
	{
		assert(0); // TODO
		return false;
	}
}



struct LoaderState
{
	tinygltf::Model gltfModel;
	Scene* scene;
	wi::unordered_map<int, Entity> entityMap;  // node -> entity
	Entity rootEntity = INVALID_ENTITY;
};

void Import_Extension_VRM(LoaderState& state);
void Import_Extension_VRMC(LoaderState& state);

// Recursively loads nodes and resolves hierarchy:
void LoadNode(int nodeIndex, Entity parent, LoaderState& state)
{
	if (nodeIndex < 0)
	{
		return;
	}
	auto& node = state.gltfModel.nodes[nodeIndex];
	Scene& scene = *state.scene;
	Entity entity = INVALID_ENTITY;

	if(node.mesh >= 0)
	{
		assert(node.mesh < (int)scene.meshes.GetCount());

		if (node.skin >= 0)
		{
			// This node is an armature:
			MeshComponent& mesh = scene.meshes[node.mesh];
			assert(!mesh.vertex_boneindices.empty());
			entity = scene.armatures.GetEntity(node.skin);
			mesh.armatureID = entity;

			// The object component will use an identity transform but will be parented to the armature:
			Entity objectEntity = scene.Entity_CreateObject(node.name);
			ObjectComponent& object = *scene.objects.GetComponent(objectEntity);
			object.meshID = scene.meshes.GetEntity(node.mesh);
			scene.Component_Attach(objectEntity, entity, true);
		}
		else
		{
			// This node is a mesh instance:
			entity = scene.Entity_CreateObject(node.name);
			ObjectComponent& object = *scene.objects.GetComponent(entity);
			object.meshID = scene.meshes.GetEntity(node.mesh);
		}
	}
	else if (node.camera >= 0)
	{
		if (node.name.empty())
		{
			static int camID = 0;
			node.name = "cam" + std::to_string(camID++);
		}

		entity = scene.Entity_CreateCamera(node.name, 16, 9);
	}

	auto ext_lights_punctual = node.extensions.find("KHR_lights_punctual");
	if (ext_lights_punctual != node.extensions.end())
	{
		// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
		entity = scene.Entity_CreateLight(""); // light component will be filled later
	}

	if (entity == INVALID_ENTITY)
	{
		entity = CreateEntity();
		scene.transforms.Create(entity);
		scene.names.Create(entity) = node.name;
	}

	state.entityMap[nodeIndex] = entity;

	TransformComponent& transform = *scene.transforms.GetComponent(entity);
	if (!node.scale.empty())
	{
		transform.scale_local = XMFLOAT3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);
	}
	if (!node.rotation.empty())
	{
		transform.rotation_local = XMFLOAT4((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
	}
	if (!node.translation.empty())
	{
		transform.translation_local = XMFLOAT3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
	}
	if (!node.matrix.empty())
	{
		transform.world._11 = (float)node.matrix[0];
		transform.world._12 = (float)node.matrix[1];
		transform.world._13 = (float)node.matrix[2];
		transform.world._14 = (float)node.matrix[3];
		transform.world._21 = (float)node.matrix[4];
		transform.world._22 = (float)node.matrix[5];
		transform.world._23 = (float)node.matrix[6];
		transform.world._24 = (float)node.matrix[7];
		transform.world._31 = (float)node.matrix[8];
		transform.world._32 = (float)node.matrix[9];
		transform.world._33 = (float)node.matrix[10];
		transform.world._34 = (float)node.matrix[11];
		transform.world._41 = (float)node.matrix[12];
		transform.world._42 = (float)node.matrix[13];
		transform.world._43 = (float)node.matrix[14];
		transform.world._44 = (float)node.matrix[15];
		transform.ApplyTransform(); // this creates S, R, T vectors from world matrix
	}

	transform.UpdateTransform();

	if (parent != INVALID_ENTITY)
	{
		scene.Component_Attach(entity, parent, true);
	}

	if (!node.children.empty())
	{
		for (int child : node.children)
		{
			LoadNode(child, entity, state);
		}
	}
}

void ImportModel_GLTF(const std::string& fileName, Scene& scene)
{
	std::string directory = wi::helper::GetDirectoryFromPath(fileName);
	std::string name = wi::helper::GetFileNameFromPath(fileName);
	std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(name));


	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool ret;

	tinygltf::FsCallbacks callbacks;
	callbacks.ReadWholeFile = tinygltf::ReadWholeFile;
	callbacks.WriteWholeFile = tinygltf::WriteWholeFile;
	callbacks.FileExists = tinygltf::FileExists;
	callbacks.ExpandFilePath = tinygltf::ExpandFilePath;
	loader.SetFsCallbacks(callbacks);

	wi::resourcemanager::ResourceSerializer seri; // keep this alive to not delete loaded images while importing gltf
	loader.SetImageLoader(tinygltf::LoadImageData, &seri);
	loader.SetImageWriter(tinygltf::WriteImageData, nullptr);
	
	LoaderState state;
	state.scene = &scene;

	wi::vector<uint8_t> filedata;
	ret = wi::helper::FileRead(fileName, filedata);

	if (ret)
	{
		std::string basedir = tinygltf::GetBaseDir(fileName);

		if (!extension.compare("GLTF"))
		{
			ret = loader.LoadASCIIFromString(&state.gltfModel, &err, &warn, 
				reinterpret_cast<const char*>(&filedata.at(0)),
				static_cast<unsigned int>(filedata.size()), basedir);
		}
		else
		{
			ret = loader.LoadBinaryFromMemory(&state.gltfModel, &err, &warn,
				filedata.data(),
				static_cast<unsigned int>(filedata.size()), basedir);
		}
	}
	else
	{
		err = "Failed to read file: " + fileName;
	}

	if (!ret) {
		wi::helper::messageBox(err, "GLTF error!");
	}

	state.rootEntity = CreateEntity();
	scene.transforms.Create(state.rootEntity);
	scene.names.Create(state.rootEntity) = name;

	// Create materials:
	for (auto& x : state.gltfModel.materials)
	{
		Entity materialEntity = scene.Entity_CreateMaterial(x.name);
		scene.Component_Attach(materialEntity, state.rootEntity);

		MaterialComponent& material = *scene.materials.GetComponent(materialEntity);

		material.baseColor = XMFLOAT4(1, 1, 1, 1);
		material.roughness = 1.0f;
		material.metalness = 1.0f;
		material.reflectance = 0.04f;

		material.SetDoubleSided(x.doubleSided);

		// metallic-roughness workflow:
		auto baseColorTexture = x.values.find("baseColorTexture");
		auto metallicRoughnessTexture = x.values.find("metallicRoughnessTexture");
		auto baseColorFactor = x.values.find("baseColorFactor");
		auto roughnessFactor = x.values.find("roughnessFactor");
		auto metallicFactor = x.values.find("metallicFactor");

		// common workflow:
		auto normalTexture = x.additionalValues.find("normalTexture");
		auto emissiveTexture = x.additionalValues.find("emissiveTexture");
		auto occlusionTexture = x.additionalValues.find("occlusionTexture");
		auto emissiveFactor = x.additionalValues.find("emissiveFactor");
		auto alphaCutoff = x.additionalValues.find("alphaCutoff");
		auto alphaMode = x.additionalValues.find("alphaMode");

		if (baseColorTexture != x.values.end())
		{
			auto& tex = state.gltfModel.textures[baseColorTexture->second.TextureIndex()];
			int img_source = tex.source;
			if (tex.extensions.count("KHR_texture_basisu"))
			{
				img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
			}
			auto& img = state.gltfModel.images[img_source];
			material.textures[MaterialComponent::BASECOLORMAP].resource = wi::resourcemanager::Load(img.uri);
			material.textures[MaterialComponent::BASECOLORMAP].name = img.uri;
			material.textures[MaterialComponent::BASECOLORMAP].uvset = baseColorTexture->second.TextureTexCoord();
		}
		if (normalTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[normalTexture->second.TextureIndex()];
			int img_source = tex.source;
			if (tex.extensions.count("KHR_texture_basisu"))
			{
				img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
			}
			auto& img = state.gltfModel.images[img_source];
			material.textures[MaterialComponent::NORMALMAP].resource = wi::resourcemanager::Load(img.uri);
			material.textures[MaterialComponent::NORMALMAP].name = img.uri;
			material.textures[MaterialComponent::NORMALMAP].uvset = normalTexture->second.TextureTexCoord();
		}
		if (metallicRoughnessTexture != x.values.end())
		{
			auto& tex = state.gltfModel.textures[metallicRoughnessTexture->second.TextureIndex()];
			int img_source = tex.source;
			if (tex.extensions.count("KHR_texture_basisu"))
			{
				img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
			}
			auto& img = state.gltfModel.images[img_source];
			material.textures[MaterialComponent::SURFACEMAP].resource = wi::resourcemanager::Load(img.uri);
			material.textures[MaterialComponent::SURFACEMAP].name = img.uri;
			material.textures[MaterialComponent::SURFACEMAP].uvset = metallicRoughnessTexture->second.TextureTexCoord();
		}
		if (emissiveTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[emissiveTexture->second.TextureIndex()];
			int img_source = tex.source;
			if (tex.extensions.count("KHR_texture_basisu"))
			{
				img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
			}
			auto& img = state.gltfModel.images[img_source];
			material.textures[MaterialComponent::EMISSIVEMAP].resource = wi::resourcemanager::Load(img.uri);
			material.textures[MaterialComponent::EMISSIVEMAP].name = img.uri;
			material.textures[MaterialComponent::EMISSIVEMAP].uvset = emissiveTexture->second.TextureTexCoord();
		}
		if (occlusionTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[occlusionTexture->second.TextureIndex()];
			int img_source = tex.source;
			if (tex.extensions.count("KHR_texture_basisu"))
			{
				img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
			}
			auto& img = state.gltfModel.images[img_source];
			material.textures[MaterialComponent::OCCLUSIONMAP].resource = wi::resourcemanager::Load(img.uri);
			material.textures[MaterialComponent::OCCLUSIONMAP].name = img.uri;
			material.textures[MaterialComponent::OCCLUSIONMAP].uvset = occlusionTexture->second.TextureTexCoord();
			material.SetOcclusionEnabled_Secondary(true);
		}

		if (baseColorFactor != x.values.end())
		{
			material.baseColor.x = float(baseColorFactor->second.ColorFactor()[0]);
			material.baseColor.y = float(baseColorFactor->second.ColorFactor()[1]);
			material.baseColor.z = float(baseColorFactor->second.ColorFactor()[2]);
			material.baseColor.w = float(baseColorFactor->second.ColorFactor()[3]);
		}
		if (roughnessFactor != x.values.end())
		{
			material.roughness = float(roughnessFactor->second.Factor());
		}
		if (metallicFactor != x.values.end())
		{
			material.metalness = float(metallicFactor->second.Factor());
		}
		if (emissiveFactor != x.additionalValues.end())
		{
			material.emissiveColor.x = float(emissiveFactor->second.ColorFactor()[0]);
			material.emissiveColor.y = float(emissiveFactor->second.ColorFactor()[1]);
			material.emissiveColor.z = float(emissiveFactor->second.ColorFactor()[2]);
			material.emissiveColor.w = float(emissiveFactor->second.ColorFactor()[3]);
		}
		if (alphaMode != x.additionalValues.end())
		{
			if (alphaMode->second.string_value.compare("BLEND") == 0)
			{
				material.userBlendMode = wi::enums::BLENDMODE_ALPHA;
			}
			if (alphaMode->second.string_value.compare("MASK") == 0)
			{
				material.alphaRef = 0.5f;
			}
		}
		if (alphaCutoff != x.additionalValues.end())
		{
			material.alphaRef = 1 - float(alphaCutoff->second.Factor());
		}

		auto ext_unlit = x.extensions.find("KHR_materials_unlit");
		if (ext_unlit != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_unlit

			material.shaderType = MaterialComponent::SHADERTYPE_UNLIT;
		}

		auto ext_transmission = x.extensions.find("KHR_materials_transmission");
		if (ext_transmission != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_transmission

			if (ext_transmission->second.Has("transmissionFactor"))
			{
				auto& factor = ext_transmission->second.Get("transmissionFactor");
				material.transmission = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_transmission->second.Has("transmissionTexture"))
			{
				int index = ext_transmission->second.Get("transmissionTexture").Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::TRANSMISSIONMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::TRANSMISSIONMAP].name = img.uri;
				material.textures[MaterialComponent::TRANSMISSIONMAP].uvset = (uint32_t)ext_transmission->second.Get("transmissionTexture").Get("texCoord").Get<int>();
			}
		}

		// specular-glossiness workflow:
		auto specularGlossinessWorkflow = x.extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (specularGlossinessWorkflow != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness

			material.SetUseSpecularGlossinessWorkflow(true);

			if (specularGlossinessWorkflow->second.Has("diffuseTexture"))
			{
				int index = specularGlossinessWorkflow->second.Get("diffuseTexture").Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::BASECOLORMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::BASECOLORMAP].name = img.uri;
				material.textures[MaterialComponent::BASECOLORMAP].uvset = (uint32_t)specularGlossinessWorkflow->second.Get("diffuseTexture").Get("texCoord").Get<int>();
			}
			if (specularGlossinessWorkflow->second.Has("specularGlossinessTexture"))
			{
				int index = specularGlossinessWorkflow->second.Get("specularGlossinessTexture").Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::SURFACEMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::SURFACEMAP].name = img.uri;
				material.textures[MaterialComponent::SURFACEMAP].uvset = (uint32_t)specularGlossinessWorkflow->second.Get("specularGlossinessTexture").Get("texCoord").Get<int>();
			}

			if (specularGlossinessWorkflow->second.Has("diffuseFactor"))
			{
				auto& factor = specularGlossinessWorkflow->second.Get("diffuseFactor");
				material.baseColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
				material.baseColor.y = factor.ArrayLen() > 1 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
				material.baseColor.z = factor.ArrayLen() > 2 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
				material.baseColor.w = factor.ArrayLen() > 3 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
			}
			if (specularGlossinessWorkflow->second.Has("specularFactor"))
			{
				auto& factor = specularGlossinessWorkflow->second.Get("specularFactor");
				material.specularColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
				material.specularColor.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
				material.specularColor.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
				material.specularColor.w = factor.ArrayLen() > 0 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
			}
			if (specularGlossinessWorkflow->second.Has("glossinessFactor"))
			{
				auto& factor = specularGlossinessWorkflow->second.Get("glossinessFactor");
				material.roughness = 1 - float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
		}

		auto ext_sheen = x.extensions.find("KHR_materials_sheen");
		if (ext_sheen != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_sheen

			material.shaderType = MaterialComponent::SHADERTYPE_PBR_CLOTH;

			if (ext_sheen->second.Has("sheenColorFactor"))
			{
				auto& factor = ext_sheen->second.Get("sheenColorFactor");
				material.sheenColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
				material.sheenColor.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
				material.sheenColor.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
				material.sheenColor.w = factor.ArrayLen() > 0 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
			}
			if (ext_sheen->second.Has("sheenColorTexture"))
			{
				auto& param = ext_sheen->second.Get("sheenColorTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::SHEENCOLORMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::SHEENCOLORMAP].name = img.uri;
				material.textures[MaterialComponent::SHEENCOLORMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}
			if (ext_sheen->second.Has("sheenRoughnessFactor"))
			{
				auto& factor = ext_sheen->second.Get("sheenRoughnessFactor");
				material.sheenRoughness = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_sheen->second.Has("sheenRoughnessTexture"))
			{
				auto& param = ext_sheen->second.Get("sheenRoughnessTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::SHEENROUGHNESSMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::SHEENROUGHNESSMAP].name = img.uri;
				material.textures[MaterialComponent::SHEENROUGHNESSMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}
		}

		auto ext_clearcoat = x.extensions.find("KHR_materials_clearcoat");
		if (ext_clearcoat != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_clearcoat

			if (material.shaderType == MaterialComponent::SHADERTYPE_PBR_CLOTH)
			{
				material.shaderType = MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT;
			}
			else
			{
				material.shaderType = MaterialComponent::SHADERTYPE_PBR_CLEARCOAT;
			}

			if (ext_clearcoat->second.Has("clearcoatFactor"))
			{
				auto& factor = ext_clearcoat->second.Get("clearcoatFactor");
				material.clearcoat = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_clearcoat->second.Has("clearcoatTexture"))
			{
				auto& param = ext_clearcoat->second.Get("clearcoatTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::CLEARCOATMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::CLEARCOATMAP].name = img.uri;
				material.textures[MaterialComponent::CLEARCOATMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}
			if (ext_clearcoat->second.Has("clearcoatRoughnessFactor"))
			{
				auto& factor = ext_clearcoat->second.Get("clearcoatRoughnessFactor");
				material.clearcoatRoughness = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_clearcoat->second.Has("clearcoatRoughnessTexture"))
			{
				auto& param = ext_clearcoat->second.Get("clearcoatRoughnessTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::CLEARCOATROUGHNESSMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::CLEARCOATROUGHNESSMAP].name = img.uri;
				material.textures[MaterialComponent::CLEARCOATROUGHNESSMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}
			if (ext_clearcoat->second.Has("clearcoatNormalTexture"))
			{
				auto& param = ext_clearcoat->second.Get("clearcoatNormalTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::CLEARCOATNORMALMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::CLEARCOATNORMALMAP].name = img.uri;
				material.textures[MaterialComponent::CLEARCOATNORMALMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}
		}

		auto ext_ior = x.extensions.find("KHR_materials_ior");
		if (ext_ior != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_ior

			if (ext_ior->second.Has("ior"))
			{
				auto& factor = ext_ior->second.Get("ior");
				float ior = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());

				material.reflectance = std::pow((ior - 1.0f) / (ior + 1.0f), 2.0f);
			}
		}

		auto ext_specular = x.extensions.find("KHR_materials_specular");
		if (ext_specular != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_specular

			material.specularColor = XMFLOAT4(1, 1, 1, 1);

			if (ext_specular->second.Has("specularFactor"))
			{
				auto& factor = ext_specular->second.Get("specularFactor");
				material.specularColor.w = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_specular->second.Has("specularTexture"))
			{
				if (!material.textures[MaterialComponent::SURFACEMAP].resource.IsValid())
				{
					auto& param = ext_specular->second.Get("specularTexture");
					int index = param.Get("index").Get<int>();
					auto& tex = state.gltfModel.textures[index];
					int img_source = tex.source;
					if (tex.extensions.count("KHR_texture_basisu"))
					{
						img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
					}
					auto& img = state.gltfModel.images[img_source];
					material.textures[MaterialComponent::SURFACEMAP].resource = wi::resourcemanager::Load(img.uri);
					material.textures[MaterialComponent::SURFACEMAP].name = img.uri;
					material.textures[MaterialComponent::SURFACEMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
				}
				else if (!material.textures[MaterialComponent::SPECULARMAP].resource.IsValid())
				{
					auto& param = ext_specular->second.Get("specularTexture");
					int index = param.Get("index").Get<int>();
					auto& tex = state.gltfModel.textures[index];
					int img_source = tex.source;
					if (tex.extensions.count("KHR_texture_basisu"))
					{
						img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
					}
					auto& img = state.gltfModel.images[img_source];
					material.textures[MaterialComponent::SPECULARMAP].resource = wi::resourcemanager::Load(img.uri);
					material.textures[MaterialComponent::SPECULARMAP].name = img.uri;
					material.textures[MaterialComponent::SPECULARMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
				}
				else
				{
					wi::backlog::post("[KHR_materials_specular warning] specularTexture must be either in surfaceMap.a or specularColorTexture.a! specularTexture discarded!", wi::backlog::LogLevel::Warning);
				}
			}
			if (ext_specular->second.Has("specularColorTexture"))
			{
				auto& param = ext_specular->second.Get("specularColorTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::SPECULARMAP].resource = wi::resourcemanager::Load(img.uri);
				material.textures[MaterialComponent::SPECULARMAP].name = img.uri;
				material.textures[MaterialComponent::SPECULARMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}
			if (ext_specular->second.Has("specularColorFactor"))
			{
				auto& factor = ext_specular->second.Get("specularColorFactor");
				material.specularColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
				material.specularColor.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
				material.specularColor.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
			}
		}

	}

	if (scene.materials.GetCount() == 0)
	{
		scene.Entity_CreateMaterial("gltfimport_defaultMaterial");
	}

	// Create meshes:
	for (auto& x : state.gltfModel.meshes)
	{
		Entity meshEntity = scene.Entity_CreateMesh(x.name);
		scene.Component_Attach(meshEntity, state.rootEntity);
		MeshComponent& mesh = *scene.meshes.GetComponent(meshEntity);

		for (auto& prim : x.primitives)
		{
			assert(prim.indices >= 0);

			// Fill indices:
			const tinygltf::Accessor& accessor = state.gltfModel.accessors[prim.indices];
			const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

			int stride = accessor.ByteStride(bufferView);
			size_t indexCount = accessor.count;
			size_t indexOffset = mesh.indices.size();
			mesh.indices.resize(indexOffset + indexCount);

			mesh.subsets.push_back(MeshComponent::MeshSubset());
			mesh.subsets.back().indexOffset = (uint32_t)indexOffset;
			mesh.subsets.back().indexCount = (uint32_t)indexCount;

			mesh.subsets.back().materialID = scene.materials.GetEntity(std::max(0, prim.material));
			MaterialComponent* material = scene.materials.GetComponent(mesh.subsets.back().materialID);

			uint32_t vertexOffset = (uint32_t)mesh.vertex_positions.size();

			const uint8_t* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

			int index_remap[3];
			if (transform_to_LH)
			{
				index_remap[0] = 0;
				index_remap[1] = 1;
				index_remap[2] = 2;
			}
			else
			{
				index_remap[0] = 0;
				index_remap[1] = 2;
				index_remap[2] = 1;
			}

			if (stride == 1)
			{
				for (size_t i = 0; i < indexCount; i += 3)
				{
					mesh.indices[indexOffset + i + 0] = vertexOffset + data[i + index_remap[0]];
					mesh.indices[indexOffset + i + 1] = vertexOffset + data[i + index_remap[1]];
					mesh.indices[indexOffset + i + 2] = vertexOffset + data[i + index_remap[2]];
				}
			}
			else if (stride == 2)
			{
				for (size_t i = 0; i < indexCount; i += 3)
				{
					mesh.indices[indexOffset + i + 0] = vertexOffset + ((uint16_t*)data)[i + index_remap[0]];
					mesh.indices[indexOffset + i + 1] = vertexOffset + ((uint16_t*)data)[i + index_remap[1]];
					mesh.indices[indexOffset + i + 2] = vertexOffset + ((uint16_t*)data)[i + index_remap[2]];
				}
			}
			else if (stride == 4)
			{
				for (size_t i = 0; i < indexCount; i += 3)
				{
					mesh.indices[indexOffset + i + 0] = vertexOffset + ((uint32_t*)data)[i + index_remap[0]];
					mesh.indices[indexOffset + i + 1] = vertexOffset + ((uint32_t*)data)[i + index_remap[1]];
					mesh.indices[indexOffset + i + 2] = vertexOffset + ((uint32_t*)data)[i + index_remap[2]];
				}
			}
			else
			{
				assert(0 && "unsupported index stride!");
			}

			for (auto& attr : prim.attributes)
			{
				const std::string& attr_name = attr.first;
				int attr_data = attr.second;

				const tinygltf::Accessor& accessor = state.gltfModel.accessors[attr_data];
				const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

				int stride = accessor.ByteStride(bufferView);
				size_t vertexCount = accessor.count;

				const uint8_t* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				if (!attr_name.compare("POSITION"))
				{
					mesh.vertex_positions.resize(vertexOffset + vertexCount);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_positions[vertexOffset + i] = ((XMFLOAT3*)data)[i];
					}

					if (accessor.sparse.isSparse)
					{
						auto& sparse = accessor.sparse;
						const tinygltf::BufferView& sparse_indices_view = state.gltfModel.bufferViews[sparse.indices.bufferView];
						const tinygltf::BufferView& sparse_values_view = state.gltfModel.bufferViews[sparse.values.bufferView];
						const tinygltf::Buffer& sparse_indices_buffer = state.gltfModel.buffers[sparse_indices_view.buffer];
						const tinygltf::Buffer& sparse_values_buffer = state.gltfModel.buffers[sparse_values_view.buffer];
						const uint8_t* sparse_indices_data = sparse_indices_buffer.data.data() + sparse.indices.byteOffset + sparse_indices_view.byteOffset;
						const uint8_t* sparse_values_data = sparse_values_buffer.data.data() + sparse.values.byteOffset + sparse_values_view.byteOffset;
						switch (sparse.indices.componentType)
						{
						default:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							for (int s = 0; s < sparse.count; ++s)
							{
								mesh.vertex_positions[sparse_indices_data[s]] = ((const XMFLOAT3*)sparse_values_data)[s];
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							for (int s = 0; s < sparse.count; ++s)
							{
								mesh.vertex_positions[((const uint16_t*)sparse_indices_data)[s]] = ((const XMFLOAT3*)sparse_values_data)[s];
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							for (int s = 0; s < sparse.count; ++s)
							{
								mesh.vertex_positions[((const uint32_t*)sparse_indices_data)[s]] = ((const XMFLOAT3*)sparse_values_data)[s];
							}
							break;
						}
					}
				}
				else if (!attr_name.compare("NORMAL"))
				{
					mesh.vertex_normals.resize(vertexOffset + vertexCount);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_normals[vertexOffset + i] = ((XMFLOAT3*)data)[i];
					}

					if (accessor.sparse.isSparse)
					{
						auto& sparse = accessor.sparse;
						const tinygltf::BufferView& sparse_indices_view = state.gltfModel.bufferViews[sparse.indices.bufferView];
						const tinygltf::BufferView& sparse_values_view = state.gltfModel.bufferViews[sparse.values.bufferView];
						const tinygltf::Buffer& sparse_indices_buffer = state.gltfModel.buffers[sparse_indices_view.buffer];
						const tinygltf::Buffer& sparse_values_buffer = state.gltfModel.buffers[sparse_values_view.buffer];
						const uint8_t* sparse_indices_data = sparse_indices_buffer.data.data() + sparse.indices.byteOffset + sparse_indices_view.byteOffset;
						const uint8_t* sparse_values_data = sparse_values_buffer.data.data() + sparse.values.byteOffset + sparse_values_view.byteOffset;
						switch (sparse.indices.componentType)
						{
						default:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							for (int s = 0; s < sparse.count; ++s)
							{
								mesh.vertex_normals[sparse_indices_data[s]] = ((const XMFLOAT3*)sparse_values_data)[s];
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							for (int s = 0; s < sparse.count; ++s)
							{
								mesh.vertex_normals[((const uint16_t*)sparse_indices_data)[s]] = ((const XMFLOAT3*)sparse_values_data)[s];
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							for (int s = 0; s < sparse.count; ++s)
							{
								mesh.vertex_normals[((const uint32_t*)sparse_indices_data)[s]] = ((const XMFLOAT3*)sparse_values_data)[s];
							}
							break;
						}
					}
				}
				else if (!attr_name.compare("TANGENT"))
				{
					mesh.vertex_tangents.resize(vertexOffset + vertexCount);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_tangents[vertexOffset + i] = ((XMFLOAT4*)data)[i];
					}
				}
				else if (!attr_name.compare("TEXCOORD_0"))
				{
					mesh.vertex_uvset_0.resize(vertexOffset + vertexCount);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const XMFLOAT2& tex = ((XMFLOAT2*)data)[i];

							mesh.vertex_uvset_0[vertexOffset + i].x = tex.x;
							mesh.vertex_uvset_0[vertexOffset + i].y = tex.y;
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const uint8_t& s = *(uint8_t*)((size_t)data + i * stride + 0);
							const uint8_t& t = *(uint8_t*)((size_t)data + i * stride + 1);

							mesh.vertex_uvset_0[vertexOffset + i].x = s / 255.0f;
							mesh.vertex_uvset_0[vertexOffset + i].y = t / 255.0f;
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const uint16_t& s = *(uint16_t*)((size_t)data + i * stride + 0 * sizeof(uint16_t));
							const uint16_t& t = *(uint16_t*)((size_t)data + i * stride + 1 * sizeof(uint16_t));

							mesh.vertex_uvset_0[vertexOffset + i].x = s / 65535.0f;
							mesh.vertex_uvset_0[vertexOffset + i].y = t / 65535.0f;
						}
					}
				}
				else if (!attr_name.compare("TEXCOORD_1"))
				{
					mesh.vertex_uvset_1.resize(vertexOffset + vertexCount);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const XMFLOAT2& tex = *(XMFLOAT2*)((size_t)data + i * stride);

							mesh.vertex_uvset_1[vertexOffset + i].x = tex.x;
							mesh.vertex_uvset_1[vertexOffset + i].y = tex.y;
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const uint8_t& s = *(uint8_t*)((size_t)data + i * stride + 0);
							const uint8_t& t = *(uint8_t*)((size_t)data + i * stride + 1);

							mesh.vertex_uvset_1[vertexOffset + i].x = s / 255.0f;
							mesh.vertex_uvset_1[vertexOffset + i].y = t / 255.0f;
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const uint16_t& s = *(uint16_t*)((size_t)data + i * stride + 0 * sizeof(uint16_t));
							const uint16_t& t = *(uint16_t*)((size_t)data + i * stride + 1 * sizeof(uint16_t));

							mesh.vertex_uvset_1[vertexOffset + i].x = s / 65535.0f;
							mesh.vertex_uvset_1[vertexOffset + i].y = t / 65535.0f;
						}
					}
				}
				else if (!attr_name.compare("JOINTS_0"))
				{
					mesh.vertex_boneindices.resize(vertexOffset + vertexCount);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						struct JointTmp
						{
							uint8_t ind[4];
						};

						for (size_t i = 0; i < vertexCount; ++i)
						{
							const JointTmp& joint = ((JointTmp*)data)[i];

							mesh.vertex_boneindices[vertexOffset + i].x = joint.ind[0];
							mesh.vertex_boneindices[vertexOffset + i].y = joint.ind[1];
							mesh.vertex_boneindices[vertexOffset + i].z = joint.ind[2];
							mesh.vertex_boneindices[vertexOffset + i].w = joint.ind[3];
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						struct JointTmp
						{
							uint16_t ind[4];
						};

						for (size_t i = 0; i < vertexCount; ++i)
						{
							const JointTmp& joint = ((JointTmp*)data)[i];

							mesh.vertex_boneindices[vertexOffset + i].x = joint.ind[0];
							mesh.vertex_boneindices[vertexOffset + i].y = joint.ind[1];
							mesh.vertex_boneindices[vertexOffset + i].z = joint.ind[2];
							mesh.vertex_boneindices[vertexOffset + i].w = joint.ind[3];
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						struct JointTmp
						{
							uint32_t ind[4];
						};

						for (size_t i = 0; i < vertexCount; ++i)
						{
							const JointTmp& joint = ((JointTmp*)data)[i];

							mesh.vertex_boneindices[vertexOffset + i].x = joint.ind[0];
							mesh.vertex_boneindices[vertexOffset + i].y = joint.ind[1];
							mesh.vertex_boneindices[vertexOffset + i].z = joint.ind[2];
							mesh.vertex_boneindices[vertexOffset + i].w = joint.ind[3];
						}
					}
					else
					{
						assert(0);
					}
				}
				else if (!attr_name.compare("WEIGHTS_0"))
				{
					mesh.vertex_boneweights.resize(vertexOffset + vertexCount);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							mesh.vertex_boneweights[vertexOffset + i] = *(XMFLOAT4*)((size_t)data + i * stride);
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const uint8_t& x = *(uint8_t*)((size_t)data + i * stride + 0);
							const uint8_t& y = *(uint8_t*)((size_t)data + i * stride + 1);
							const uint8_t& z = *(uint8_t*)((size_t)data + i * stride + 2);
							const uint8_t& w = *(uint8_t*)((size_t)data + i * stride + 3);

							mesh.vertex_boneweights[vertexOffset + i].x = x / 255.0f;
							mesh.vertex_boneweights[vertexOffset + i].x = y / 255.0f;
							mesh.vertex_boneweights[vertexOffset + i].x = z / 255.0f;
							mesh.vertex_boneweights[vertexOffset + i].x = w / 255.0f;
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const uint16_t& x = *(uint8_t*)((size_t)data + i * stride + 0 * sizeof(uint16_t));
							const uint16_t& y = *(uint8_t*)((size_t)data + i * stride + 1 * sizeof(uint16_t));
							const uint16_t& z = *(uint8_t*)((size_t)data + i * stride + 2 * sizeof(uint16_t));
							const uint16_t& w = *(uint8_t*)((size_t)data + i * stride + 3 * sizeof(uint16_t));

							mesh.vertex_boneweights[vertexOffset + i].x = x / 65535.0f;
							mesh.vertex_boneweights[vertexOffset + i].x = y / 65535.0f;
							mesh.vertex_boneweights[vertexOffset + i].x = z / 65535.0f;
							mesh.vertex_boneweights[vertexOffset + i].x = w / 65535.0f;
						}
					}
				}
				else if (!attr_name.compare("COLOR_0"))
				{
					if(material != nullptr)
					{
						material->SetUseVertexColors(true);
					}
					mesh.vertex_colors.resize(vertexOffset + vertexCount);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						if (accessor.type == TINYGLTF_TYPE_VEC3)
						{
							for (size_t i = 0; i < vertexCount; ++i)
							{
								const XMFLOAT3& color = *(XMFLOAT3*)((size_t)data + i * stride);
								uint32_t rgba = wi::math::CompressColor(color);

								mesh.vertex_colors[vertexOffset + i] = rgba;
							}
						}
						else if (accessor.type == TINYGLTF_TYPE_VEC4)
						{
							for (size_t i = 0; i < vertexCount; ++i)
							{
								const XMFLOAT4& color = *(XMFLOAT4*)((size_t)data + i * stride);
								uint32_t rgba = wi::math::CompressColor(color);

								mesh.vertex_colors[vertexOffset + i] = rgba;
							}
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						if (accessor.type == TINYGLTF_TYPE_VEC3)
						{
							for (size_t i = 0; i < vertexCount; ++i)
							{
								const uint8_t& r = *(uint8_t*)((size_t)data + i * stride + 0);
								const uint8_t& g = *(uint8_t*)((size_t)data + i * stride + 1);
								const uint8_t& b = *(uint8_t*)((size_t)data + i * stride + 2);
								const uint8_t a = 0xFF;
								wi::Color color = wi::Color(r, g, b, a);

								mesh.vertex_colors[vertexOffset + i] = color;
							}
						}
						else if (accessor.type == TINYGLTF_TYPE_VEC4)
						{
							for (size_t i = 0; i < vertexCount; ++i)
							{
								const uint8_t& r = *(uint8_t*)((size_t)data + i * stride + 0);
								const uint8_t& g = *(uint8_t*)((size_t)data + i * stride + 1);
								const uint8_t& b = *(uint8_t*)((size_t)data + i * stride + 2);
								const uint8_t& a = *(uint8_t*)((size_t)data + i * stride + 3);
								wi::Color color = wi::Color(r, g, b, a);

								mesh.vertex_colors[vertexOffset + i] = color;
							}
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						if (accessor.type == TINYGLTF_TYPE_VEC3)
						{
							for (size_t i = 0; i < vertexCount; ++i)
							{
								const uint16_t& r = *(uint16_t*)((size_t)data + i * stride + 0 * sizeof(uint16_t));
								const uint16_t& g = *(uint16_t*)((size_t)data + i * stride + 1 * sizeof(uint16_t));
								const uint16_t& b = *(uint16_t*)((size_t)data + i * stride + 2 * sizeof(uint16_t));
								uint32_t rgba = wi::math::CompressColor(XMFLOAT3(r / 65535.0f, g / 65535.0f, b / 65535.0f));

								mesh.vertex_colors[vertexOffset + i] = rgba;
							}
						}
						else if (accessor.type == TINYGLTF_TYPE_VEC4)
						{
							for (size_t i = 0; i < vertexCount; ++i)
							{
								const uint16_t& r = *(uint16_t*)((size_t)data + i * stride + 0 * sizeof(uint16_t));
								const uint16_t& g = *(uint16_t*)((size_t)data + i * stride + 1 * sizeof(uint16_t));
								const uint16_t& b = *(uint16_t*)((size_t)data + i * stride + 2 * sizeof(uint16_t));
								const uint16_t& a = *(uint16_t*)((size_t)data + i * stride + 3 * sizeof(uint16_t));
								uint32_t rgba = wi::math::CompressColor(XMFLOAT4(r / 65535.0f, g / 65535.0f, b / 65535.0f, a / 65535.0f));

								mesh.vertex_colors[vertexOffset + i] = rgba;
							}
						}
					}
				}

				mesh.morph_targets.resize(prim.targets.size());
				for (size_t i = 0; i < mesh.morph_targets.size(); i++)
				{
					for (auto& attr : prim.targets[i])
					{
						const std::string& attr_name = attr.first;
						int attr_data = attr.second;

						const tinygltf::Accessor& accessor = state.gltfModel.accessors[attr_data];

						if (!attr_name.compare("POSITION"))
						{
							if (accessor.sparse.isSparse)
							{
								auto& sparse = accessor.sparse;
								const tinygltf::BufferView& sparse_indices_view = state.gltfModel.bufferViews[sparse.indices.bufferView];
								const tinygltf::BufferView& sparse_values_view = state.gltfModel.bufferViews[sparse.values.bufferView];
								const tinygltf::Buffer& sparse_indices_buffer = state.gltfModel.buffers[sparse_indices_view.buffer];
								const tinygltf::Buffer& sparse_values_buffer = state.gltfModel.buffers[sparse_values_view.buffer];
								const uint8_t* sparse_indices_data = sparse_indices_buffer.data.data() + sparse.indices.byteOffset + sparse_indices_view.byteOffset;
								const uint8_t* sparse_values_data = sparse_values_buffer.data.data() + sparse.values.byteOffset + sparse_values_view.byteOffset;
								mesh.morph_targets[i].vertex_positions.resize(sparse.count);
								mesh.morph_targets[i].sparse_indices.resize(sparse.count);

								switch (sparse.indices.componentType)
								{
								default:
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
									for (int s = 0; s < sparse.count; ++s)
									{
										mesh.morph_targets[i].sparse_indices[s] = vertexOffset + sparse_indices_data[s];
										mesh.morph_targets[i].vertex_positions[s] = ((const XMFLOAT3*)sparse_values_data)[s];
									}
									break;
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
									for (int s = 0; s < sparse.count; ++s)
									{
										mesh.morph_targets[i].sparse_indices[s] = vertexOffset + ((const uint16_t*)sparse_indices_data)[s];
										mesh.morph_targets[i].vertex_positions[s] = ((const XMFLOAT3*)sparse_values_data)[s];
									}
									break;
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
									for (int s = 0; s < sparse.count; ++s)
									{
										mesh.morph_targets[i].sparse_indices[s] = vertexOffset + ((const uint32_t*)sparse_indices_data)[s];
										mesh.morph_targets[i].vertex_positions[s] = ((const XMFLOAT3*)sparse_values_data)[s];
									}
									break;
								}
							}
							else
							{
								const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
								const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

								int stride = accessor.ByteStride(bufferView);
								size_t vertexCount = accessor.count;

								const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

								mesh.morph_targets[i].vertex_positions.resize(vertexOffset + vertexCount);
								for (size_t j = 0; j < vertexCount; ++j)
								{
									mesh.morph_targets[i].vertex_positions[vertexOffset + j] = ((XMFLOAT3*)data)[j];
								}
							}
						}
						else if (!attr_name.compare("NORMAL"))
						{
							if (accessor.sparse.isSparse)
							{
								auto& sparse = accessor.sparse;
								const tinygltf::BufferView& sparse_indices_view = state.gltfModel.bufferViews[sparse.indices.bufferView];
								const tinygltf::BufferView& sparse_values_view = state.gltfModel.bufferViews[sparse.values.bufferView];
								const tinygltf::Buffer& sparse_indices_buffer = state.gltfModel.buffers[sparse_indices_view.buffer];
								const tinygltf::Buffer& sparse_values_buffer = state.gltfModel.buffers[sparse_values_view.buffer];
								const uint8_t* sparse_indices_data = sparse_indices_buffer.data.data() + sparse.indices.byteOffset + sparse_indices_view.byteOffset;
								const uint8_t* sparse_values_data = sparse_values_buffer.data.data() + sparse.values.byteOffset + sparse_values_view.byteOffset;
								mesh.morph_targets[i].vertex_normals.resize(sparse.count);
								mesh.morph_targets[i].sparse_indices.resize(sparse.count);

								switch (sparse.indices.componentType)
								{
								default:
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
									for (int s = 0; s < sparse.count; ++s)
									{
										mesh.morph_targets[i].sparse_indices[s] = vertexOffset + sparse_indices_data[s];
										mesh.morph_targets[i].vertex_normals[s] = ((const XMFLOAT3*)sparse_values_data)[s];
									}
									break;
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
									for (int s = 0; s < sparse.count; ++s)
									{
										mesh.morph_targets[i].sparse_indices[s] = vertexOffset + ((const uint16_t*)sparse_indices_data)[s];
										mesh.morph_targets[i].vertex_normals[s] = ((const XMFLOAT3*)sparse_values_data)[s];
									}
									break;
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
									for (int s = 0; s < sparse.count; ++s)
									{
										mesh.morph_targets[i].sparse_indices[s] = vertexOffset + ((const uint32_t*)sparse_indices_data)[s];
										mesh.morph_targets[i].vertex_normals[s] = ((const XMFLOAT3*)sparse_values_data)[s];
									}
									break;
								}
							}
							else
							{
								const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
								const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

								int stride = accessor.ByteStride(bufferView);
								size_t vertexCount = accessor.count;

								const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

								mesh.morph_targets[i].vertex_normals.resize(vertexOffset + vertexCount);
								for (size_t j = 0; j < vertexCount; ++j)
								{
									mesh.morph_targets[i].vertex_normals[vertexOffset + j] = ((XMFLOAT3*)data)[j];
								}
							}
						}
					}
				}
				for (size_t i = 0; i < x.weights.size(); i++)
				{
					mesh.morph_targets[i].weight = static_cast<float_t>(x.weights[i]);
				}
			}

		}

		mesh.CreateRenderData();
	}

	// Create armatures:
	for (auto& skin : state.gltfModel.skins)
	{
		Entity armatureEntity = CreateEntity();
		scene.names.Create(armatureEntity) = skin.name;
		scene.layers.Create(armatureEntity);
		scene.transforms.Create(armatureEntity);
		scene.Component_Attach(armatureEntity, state.rootEntity);
		ArmatureComponent& armature = scene.armatures.Create(armatureEntity);

		if (skin.inverseBindMatrices >= 0)
		{
			const tinygltf::Accessor &accessor = state.gltfModel.accessors[skin.inverseBindMatrices];
			const tinygltf::BufferView &bufferView = state.gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = state.gltfModel.buffers[bufferView.buffer];
			armature.inverseBindMatrices.resize(accessor.count);
			memcpy(armature.inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(XMFLOAT4X4));
		}
		else
		{
			assert(0);
		}
	}

	// Create transform hierarchy, assign objects, meshes, armatures, cameras:
	const tinygltf::Scene &gltfScene = state.gltfModel.scenes[std::max(0, state.gltfModel.defaultScene)];
	for (size_t i = 0; i < gltfScene.nodes.size(); i++)
	{
		LoadNode(gltfScene.nodes[i], state.rootEntity, state);
	}

	// Create armature-bone mappings:
	int armatureIndex = 0;
	for (auto& skin : state.gltfModel.skins)
	{
		ArmatureComponent& armature = scene.armatures[armatureIndex++];

		const size_t jointCount = skin.joints.size();

		armature.boneCollection.resize(jointCount);

		// Create bone collection:
		for (size_t i = 0; i < jointCount; ++i)
		{
			int jointIndex = skin.joints[i];
			Entity boneEntity = state.entityMap[jointIndex];

			armature.boneCollection[i] = boneEntity;
		}
	}

	// Create animations:
	for (auto& anim : state.gltfModel.animations)
	{
		Entity entity = CreateEntity();
		scene.names.Create(entity) = anim.name;
		scene.Component_Attach(entity, state.rootEntity);
		AnimationComponent& animationcomponent = scene.animations.Create(entity);
		animationcomponent.samplers.resize(anim.samplers.size());
		animationcomponent.channels.resize(anim.channels.size());

		for (size_t i = 0; i < anim.samplers.size(); ++i)
		{
			auto& sam = anim.samplers[i];

			if (!sam.interpolation.compare("LINEAR"))
			{
				animationcomponent.samplers[i].mode = AnimationComponent::AnimationSampler::Mode::LINEAR;
			}
			else if (!sam.interpolation.compare("STEP"))
			{
				animationcomponent.samplers[i].mode = AnimationComponent::AnimationSampler::Mode::STEP;
			}
			else if (!sam.interpolation.compare("CUBICSPLINE"))
			{
				animationcomponent.samplers[i].mode = AnimationComponent::AnimationSampler::Mode::CUBICSPLINE;
			}

			animationcomponent.samplers[i].data = CreateEntity();
			scene.Component_Attach(animationcomponent.samplers[i].data, state.rootEntity);
			AnimationDataComponent& animationdata = scene.animation_datas.Create(animationcomponent.samplers[i].data);

			// AnimationSampler input = keyframe times
			{
				const tinygltf::Accessor& accessor = state.gltfModel.accessors[sam.input];
				const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				int stride = accessor.ByteStride(bufferView);
				size_t count = accessor.count;

				animationdata.keyframe_times.resize(count);

				const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				assert(stride == 4);

				for (size_t j = 0; j < count; ++j)
				{
					float time = ((float*)data)[j];
					animationdata.keyframe_times[j] = time;
					animationcomponent.start = std::min(animationcomponent.start, time);
					animationcomponent.end = std::max(animationcomponent.end, time);
				}

			}

			// AnimationSampler output = keyframe data
			{
				const tinygltf::Accessor& accessor = state.gltfModel.accessors[sam.output];
				const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

				int stride = accessor.ByteStride(bufferView);
				size_t count = accessor.count;

				const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				switch (accessor.type)
				{
				case TINYGLTF_TYPE_SCALAR:
				{
					assert(stride == sizeof(float));
					animationdata.keyframe_data.resize(count);
					for (size_t j = 0; j < count; ++j)
					{
						animationdata.keyframe_data[j] = ((float*)data)[j];
					}
				}
				break;
				case TINYGLTF_TYPE_VEC3:
				{
					assert(stride == sizeof(XMFLOAT3));
					animationdata.keyframe_data.resize(count * 3);
					for (size_t j = 0; j < count; ++j)
					{
						((XMFLOAT3*)animationdata.keyframe_data.data())[j] = ((XMFLOAT3*)data)[j];
					}
				}
				break;
				case TINYGLTF_TYPE_VEC4:
				{
					assert(stride == sizeof(XMFLOAT4));
					animationdata.keyframe_data.resize(count * 4);
					for (size_t j = 0; j < count; ++j)
					{
						((XMFLOAT4*)animationdata.keyframe_data.data())[j] = ((XMFLOAT4*)data)[j];
					}
				}
				break;
				default: assert(0); break;

				}

			}

		}

		for (size_t i = 0; i < anim.channels.size(); ++i)
		{
			auto& channel = anim.channels[i];

			animationcomponent.channels[i].target = state.entityMap[channel.target_node];
			assert(channel.sampler >= 0);
			animationcomponent.channels[i].samplerIndex = (uint32_t)channel.sampler;

			if (!channel.target_path.compare("scale"))
			{
				animationcomponent.channels[i].path = AnimationComponent::AnimationChannel::Path::SCALE;
			}
			else if (!channel.target_path.compare("rotation"))
			{
				animationcomponent.channels[i].path = AnimationComponent::AnimationChannel::Path::ROTATION;
			}
			else if (!channel.target_path.compare("translation"))
			{
				animationcomponent.channels[i].path = AnimationComponent::AnimationChannel::Path::TRANSLATION;
			}
			else if (!channel.target_path.compare("weights"))
			{
				animationcomponent.channels[i].path = AnimationComponent::AnimationChannel::Path::WEIGHTS;
			}
			else
			{
				animationcomponent.channels[i].path = AnimationComponent::AnimationChannel::Path::UNKNOWN;
			}
		}

	}

	// Create lights:
	int lightIndex = 0;
	for (auto& x : state.gltfModel.lights)
	{
		Entity entity = scene.lights.GetEntity(lightIndex);
		LightComponent& light = scene.lights[lightIndex++];
		NameComponent& name = *scene.names.GetComponent(entity);
		name = x.name;

		if (!x.type.compare("spot"))
		{
			light.type = LightComponent::LightType::SPOT;
		}
		if (!x.type.compare("point"))
		{
			light.type = LightComponent::LightType::POINT;
		}
		if (!x.type.compare("directional"))
		{
			light.type = LightComponent::LightType::DIRECTIONAL;
		}

		if (!x.color.empty())
		{
			light.color = XMFLOAT3(float(x.color[0]), float(x.color[1]), float(x.color[2]));
		}

		light.intensity = float(x.intensity);
		light.range = x.range > 0 ? float(x.range) : std::numeric_limits<float>::max();
		light.outerConeAngle = float(x.spot.outerConeAngle);
		light.innerConeAngle = float(x.spot.innerConeAngle);
		light.SetCastShadow(true);

		// In gltf, default light direction is forward, in engine, it's downwards, so apply a rotation:
		TransformComponent& transform = *scene.transforms.GetComponent(entity);
		transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
	}

	int cameraIndex = 0;
	for (auto& x : state.gltfModel.cameras)
	{
		Entity entity = scene.cameras.GetEntity(cameraIndex);
		CameraComponent& camera = scene.cameras[cameraIndex++];
		TransformComponent& transform = *scene.transforms.GetComponent(entity);
		transform.RotateRollPitchYaw(XMFLOAT3(XM_PI, 0, XM_PI));
	}

	if (transform_to_LH)
	{
		TransformComponent& transform = *scene.transforms.GetComponent(state.rootEntity);
		transform.scale_local.z = -transform.scale_local.z;
		transform.SetDirty();
	}

	Import_Extension_VRM(state);
	Import_Extension_VRMC(state);

	// Update the scene, to have up to date values immediately after loading:
	//	For example, snap to camera functionality relies on this
	scene.Update(0);
}

void Import_Extension_VRM(LoaderState& state)
{
	auto ext_vrm = state.gltfModel.extensions.find("VRM");
	if (ext_vrm != state.gltfModel.extensions.end())
	{
		// Rotate VRM humanoid character to face -Z:
		TransformComponent& transform = *state.scene->transforms.GetComponent(state.rootEntity);
		transform.RotateRollPitchYaw(XMFLOAT3(0, XM_PI, 0));

		if (ext_vrm->second.Has("blendShapeMaster"))
		{
			// https://github.com/vrm-c/vrm-specification/tree/master/specification/0.0#vrm-extension-morph-setting-jsonextensionsvrmblendshapemaster

			const auto& blendShapeMaster = ext_vrm->second.Get("blendShapeMaster");
			if (blendShapeMaster.Has("blendShapeGroups"))
			{
				const auto& blendShapeGroups = blendShapeMaster.Get("blendShapeGroups");
				for (size_t blendShapeGroup_index = 0; blendShapeGroup_index < blendShapeGroups.ArrayLen(); ++blendShapeGroup_index)
				{
					const auto& blendShapeGroup = blendShapeGroups.Get(int(blendShapeGroup_index));
					Entity entity = CreateEntity();
					ExpressionComponent& component = state.scene->expressions.Create(entity);
					state.scene->Component_Attach(entity, state.rootEntity);

					if (blendShapeGroup.Has("name"))
					{
						const auto& value = blendShapeGroup.Get("name");
						state.scene->names.Create(entity) = value.Get<std::string>();
					}
					if (blendShapeGroup.Has("presetName") && !state.scene->names.Contains(entity))
					{
						const auto& value = blendShapeGroup.Get("presetName");
						state.scene->names.Create(entity) = value.Get<std::string>();
					}
					if (blendShapeGroup.Has("isBinary"))
					{
						const auto& value = blendShapeGroup.Get("isBinary");
						if (value.Get<bool>())
						{
							component._flags |= ExpressionComponent::BINARY;
						}
					}
					if (blendShapeGroup.Has("binds"))
					{
						const auto& binds = blendShapeGroup.Get("binds");
						for (size_t bind_index = 0; bind_index < binds.ArrayLen(); ++bind_index)
						{
							const auto& bind = binds.Get(int(bind_index));
							ExpressionComponent::MorphTargetBinding& morph_target_binding = component.morph_target_bindings.emplace_back();

							if (bind.Has("mesh"))
							{
								const auto& value = bind.Get("mesh");
								morph_target_binding.meshID = state.scene->meshes.GetEntity(value.GetNumberAsInt());
							}
							if (bind.Has("index"))
							{
								const auto& value = bind.Get("index");
								morph_target_binding.index = value.GetNumberAsInt();
							}
							if (bind.Has("weight"))
							{
								const auto& value = bind.Get("weight");
								morph_target_binding.weight = float(value.GetNumberAsInt()) / 100.0f;
							}
						}
					}
				}
			}
		}

		if (ext_vrm->second.Has("secondaryAnimation"))
		{
			// https://github.com/vrm-c/vrm-specification/tree/master/specification/0.0#vrm-extension-spring-bone-settings-jsonextensionsvrmsecondaryanimation

			const auto& secondaryAnimation = ext_vrm->second.Get("secondaryAnimation");
			if (secondaryAnimation.Has("boneGroups"))
			{
				const auto& boneGroups = secondaryAnimation.Get("boneGroups");
				for (size_t boneGroup_index = 0; boneGroup_index < boneGroups.ArrayLen(); ++boneGroup_index)
				{
					const auto& boneGroup = boneGroups.Get(int(boneGroup_index));
					SpringComponent component;

					if (boneGroup.Has("dragForce"))
					{
						auto& value = boneGroup.Get("dragForce");
						component.dragForce = float(value.GetNumberAsDouble());
					}
					if (boneGroup.Has("gravityDir"))
					{
						auto& value = boneGroup.Get("gravityDir");
						if (value.Has("x"))
						{
							component.gravityDir.x = float(value.Get("x").GetNumberAsDouble());
						}
						if (value.Has("y"))
						{
							component.gravityDir.y = float(value.Get("y").GetNumberAsDouble());
						}
						if (value.Has("z"))
						{
							component.gravityDir.z = float(value.Get("z").GetNumberAsDouble());
						}
					}
					//if (boneGroup.Has("center"))
					//{
					//	auto& value = boneGroup.Get("center");
					//	center = float(value.GetNumberAsDouble());
					//}
					if (boneGroup.Has("gravityPower"))
					{
						auto& value = boneGroup.Get("gravityPower");
						component.gravityPower = float(value.GetNumberAsDouble());
					}
					if (boneGroup.Has("hitRadius"))
					{
						auto& value = boneGroup.Get("hitRadius");
						component.hitRadius = float(value.GetNumberAsDouble());
					}
					if (boneGroup.Has("stiffiness")) // yes, not stiffness, but stiffiness
					{
						auto& value = boneGroup.Get("stiffiness");
						component.stiffnessForce = float(value.GetNumberAsDouble());
					}
					if (boneGroup.Has("colliderGroups"))
					{
						const auto& colliderGroups = boneGroup.Get("colliderGroups");
						for (size_t collider_group_index = 0; collider_group_index < colliderGroups.ArrayLen(); ++collider_group_index)
						{
							int colliderGroupIndex = colliderGroups.Get(int(collider_group_index)).GetNumberAsInt();
							const auto& colliderGroup = secondaryAnimation.Get("colliderGroups").Get(colliderGroupIndex);

							Entity transformID = INVALID_ENTITY;
							if (colliderGroup.Has("node"))
							{
								auto& value = colliderGroup.Get("node");
								int node = value.GetNumberAsInt();
								transformID = state.entityMap[node];
							}
							if (colliderGroup.Has("colliders"))
							{
								const auto& colliders = colliderGroup.Get("colliders");
								for (size_t collider_index = 0; collider_index < colliders.ArrayLen(); ++collider_index)
								{
									Entity colliderID = CreateEntity();
									//component.colliders.push_back(colliderID); // for now, we will just use all colliders in the scene for every spring
									ColliderComponent& collider_component = state.scene->colliders.Create(colliderID);
									state.scene->transforms.Create(colliderID);
									state.scene->Component_Attach(colliderID, transformID, true);

									const auto& collider = colliders.Get(int(collider_index));
									if (collider.Has("offset"))
									{
										auto& value = collider.Get("offset");
										if (value.Has("x"))
										{
											collider_component.offset.x = float(value.Get("x").GetNumberAsDouble());
										}
										if (value.Has("y"))
										{
											collider_component.offset.y = float(value.Get("y").GetNumberAsDouble());
										}
										if (value.Has("z"))
										{
											collider_component.offset.z = float(value.Get("z").GetNumberAsDouble());
										}
									}
									if (collider.Has("radius"))
									{
										auto& value = collider.Get("radius");
										collider_component.radius = float(value.GetNumberAsDouble());
									}
								}
							}
						}
					}
					if (boneGroup.Has("bones"))
					{
						auto& bones = boneGroup.Get("bones");
						for (size_t bone_index = 0; bone_index < bones.ArrayLen(); ++bone_index)
						{
							const auto& bone = bones.Get(int(bone_index));
							int node = bone.GetNumberAsInt();
							Entity entity = state.entityMap[node];
							state.scene->springs.Create(entity) = component;

							wi::vector<int> stack = state.gltfModel.nodes[node].children;
							while (!stack.empty())
							{
								int child_node = stack.back();
								stack.pop_back();
								Entity child_entity = state.entityMap[child_node];
								state.scene->springs.Create(child_entity) = component;
								stack.insert(stack.end(), state.gltfModel.nodes[child_node].children.begin(), state.gltfModel.nodes[child_node].children.end());
							}
						}
					}
				}
			}
		}
	}

}

void Import_Extension_VRMC(LoaderState& state)
{
	auto ext_vrm = state.gltfModel.extensions.find("VRMC_vrm");
	if (ext_vrm != state.gltfModel.extensions.end())
	{
		if (ext_vrm->second.Has("expressions"))
		{
			// https://github.com/vrm-c/vrm-specification/blob/master/specification/VRMC_vrm-1.0-beta/expressions.md#vrmc_vrmexpressions

			const auto& expressions = ext_vrm->second.Get("expressions");
			static const char* expression_types[] = {
				"preset",
				"custom",
			};

			for (auto& expression_type : expression_types)
			{
				if (expressions.Has(expression_type))
				{
					const auto& names = expressions.Get(expression_type);
					for (auto& name : names.Keys())
					{
						const auto& expression = names.Get(name);
						Entity entity = CreateEntity();
						ExpressionComponent& component = state.scene->expressions.Create(entity);
						state.scene->Component_Attach(entity, state.rootEntity);
						state.scene->names.Create(entity) = name;

						if (expression.Has("isBinary"))
						{
							const auto& value = expression.Get("isBinary");
							component.SetBinary(value.Get<bool>());
						}
						if (expression.Has("morphTargetBinds"))
						{
							const auto& morpTargetBinds = expression.Get("morphTargetBinds");
							for (size_t morphTargetBind_index = 0; morphTargetBind_index < morpTargetBinds.ArrayLen(); ++morphTargetBind_index)
							{
								const auto& morphTargetBind = morpTargetBinds.Get(int(morphTargetBind_index));
								ExpressionComponent::MorphTargetBinding& morph_target_binding = component.morph_target_bindings.emplace_back();

								if (morphTargetBind.Has("node"))
								{
									const auto& value = morphTargetBind.Get("node");
									morph_target_binding.meshID = state.scene->meshes.GetEntity(state.gltfModel.nodes[value.GetNumberAsInt()].mesh);
								}
								if (morphTargetBind.Has("index"))
								{
									const auto& value = morphTargetBind.Get("index");
									morph_target_binding.index = value.GetNumberAsInt();
								}
								if (morphTargetBind.Has("weight"))
								{
									const auto& value = morphTargetBind.Get("weight");
									morph_target_binding.weight = float(value.GetNumberAsDouble());
								}
							}
						}
						//if (expression.Has("materialColorBinds"))
						//{
						//	const auto& materialColorBinds = expression.Get("materialColorBinds");
						//	// TODO: find example model and implement
						//}
						//if (expression.Has("textureTransformBinds "))
						//{
						//	const auto& textureTransformBinds = expression.Get("textureTransformBinds");
						//	// TODO: find example model and implement
						//}

					}
				}
			}
		}
	}

	auto ext_vrmc_springbone = state.gltfModel.extensions.find("VRMC_springBone");
	if (ext_vrmc_springbone != state.gltfModel.extensions.end())
	{
		// https://github.com/vrm-c/vrm-specification/tree/master/specification/VRMC_springBone-1.0-beta

		// Colliders:
		if (ext_vrmc_springbone->second.Has("colliders"))
		{
			const auto& colliders = ext_vrmc_springbone->second.Get("colliders");
			for (size_t collider_index = 0; collider_index < colliders.ArrayLen(); ++collider_index)
			{
				const auto& collider = colliders.Get(int(collider_index));
				ColliderComponent component;

				if (collider.Has("shape"))
				{
					const auto& shape = collider.Get("shape");
					if (shape.Has("capsule"))
					{
						auto& capsule = shape.Get("capsule");
						component.shape = ColliderComponent::Shape::Capsule;

						if (capsule.Has("offset"))
						{
							auto& value = capsule.Get("offset");
							component.offset.x = float(value.Get(0).GetNumberAsDouble());
							component.offset.y = float(value.Get(1).GetNumberAsDouble());
							component.offset.z = float(value.Get(2).GetNumberAsDouble());
						}
						if (capsule.Has("radius"))
						{
							auto& value = capsule.Get("radius");
							component.radius = float(value.GetNumberAsDouble());
						}
						if (capsule.Has("tail"))
						{
							auto& value = capsule.Get("tail");
							component.tail.x = float(value.Get(0).GetNumberAsDouble());
							component.tail.y = float(value.Get(1).GetNumberAsDouble());
							component.tail.z = float(value.Get(2).GetNumberAsDouble());
						}
					}
					if (shape.Has("sphere"))
					{
						auto& sphere = shape.Get("sphere");
						component.shape = ColliderComponent::Shape::Sphere;

						if (sphere.Has("offset"))
						{
							auto& value = sphere.Get("offset");
							component.offset.x = float(value.Get(0).GetNumberAsDouble());
							component.offset.y = float(value.Get(1).GetNumberAsDouble());
							component.offset.z = float(value.Get(2).GetNumberAsDouble());
						}
						if (sphere.Has("radius"))
						{
							auto& value = sphere.Get("radius");
							component.radius = float(value.GetNumberAsDouble());
						}
					}
				}
				if (collider.Has("node"))
				{
					const auto& node = collider.Get("node");
					int node_index = node.GetNumberAsInt();
					Entity entity = state.entityMap[node_index];
					Entity colliderID = CreateEntity();
					state.scene->colliders.Create(colliderID) = component;
					state.scene->transforms.Create(colliderID);
					state.scene->Component_Attach(colliderID, entity, true);
				}
			}
		}

		// Springs:
		if (ext_vrmc_springbone->second.Has("springs"))
		{
			const auto& springs = ext_vrmc_springbone->second.Get("springs");
			for (size_t spring_index = 0; spring_index < springs.ArrayLen(); ++spring_index)
			{
				const auto& spring = springs.Get(int(spring_index));
				//if (spring.Has("center"))
				//{
				//	const auto& center = spring.Get("center");
				//}
				//wi::vector<Entity> colliderIDs;
				//if (spring.Has("colliderGroups"))
				//{
				//	// collider group references:
				//	const auto& colliderGroups = spring.Get("colliderGroups");
				//	for (size_t collider_group_index = 0; collider_group_index < colliderGroups.ArrayLen(); ++collider_group_index)
				//	{
				//		const auto& colliderGroup = ext_vrmc_springbone->second.Get("colliderGroups").Get(int(collider_group_index));
				//		if (colliderGroup.Has("colliders"))
				//		{
				//			const auto& colliders = colliderGroup.Get("colliders");
				//			for (size_t collider_index = 0; collider_index < colliders.ArrayLen(); ++collider_index)
				//			{
				//				int collider = colliders.Get(int(collider_index)).GetNumberAsInt();
				//				colliderIDs.push_back(state.scene->colliders.GetEntity(collider));
				//			}
				//		}
				//	}
				//}
				if (spring.Has("joints"))
				{
					const auto& joints = spring.Get("joints");
					for (size_t joint_index = 0; joint_index < joints.ArrayLen(); ++joint_index)
					{
						const auto& joint = joints.Get(int(joint_index));
						SpringComponent component;
						//component.colliders = colliderIDs; // for now, we will just use all colliders in the scene for every spring

						if (joint.Has("dragForce"))
						{
							auto& value = joint.Get("dragForce");
							component.dragForce = float(value.GetNumberAsDouble());
						}
						if (joint.Has("gravityDir"))
						{
							auto& value = joint.Get("gravityDir");
							component.gravityDir.x = float(value.Get(0).GetNumberAsDouble());
							component.gravityDir.y = float(value.Get(1).GetNumberAsDouble());
							component.gravityDir.z = float(value.Get(2).GetNumberAsDouble());
						}
						if (joint.Has("gravityPower"))
						{
							auto& value = joint.Get("gravityPower");
							component.gravityPower = float(value.GetNumberAsDouble());
						}
						if (joint.Has("hitRadius"))
						{
							auto& value = joint.Get("hitRadius");
							component.hitRadius = float(value.GetNumberAsDouble());
						}
						if (joint.Has("stiffness"))
						{
							auto& value = joint.Get("stiffness");
							component.stiffnessForce = float(value.GetNumberAsDouble());
						}
						if (joint.Has("node"))
						{
							auto& value = joint.Get("node");
							int node = value.GetNumberAsInt();
							Entity entity = state.entityMap[node];
							state.scene->springs.Create(entity) = component;
						}
					}
				}
				if (spring.Has("name"))
				{
					const auto& name = spring.Get("name");
				}
			}

			if (ext_vrmc_springbone->second.Has("colliders"))
			{
				const auto& colliders = ext_vrmc_springbone->second.Get("colliders");
			}

		}
	}
}
