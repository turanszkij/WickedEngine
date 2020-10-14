#include "stdafx.h"
#include "wiScene.h"
#include "ModelImporter.h"
#include "wiRandom.h"

#include "Utility/stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_FS
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace std;
using namespace wiGraphics;
using namespace wiScene;
using namespace wiECS;


// Transform the data from glTF space to engine-space:
static const bool transform_to_LH = true;


namespace tinygltf
{

	bool FileExists(const std::string& abs_filename, void*) {
		return wiHelper::FileExists(abs_filename);
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
		return wiHelper::FileRead(filepath, *out);
	}

	bool WriteWholeFile(std::string* err, const std::string& filepath,
		const std::vector<unsigned char>& contents, void*) {
		return wiHelper::FileWrite(filepath, contents.data(), contents.size());
	}

	bool LoadImageData(Image *image, const int image_idx, std::string *err,
		std::string *warn, int req_width, int req_height,
		const unsigned char *bytes, int size, void *)
	{
		(void)warn;

		const int requiredComponents = 4;

		int w, h, comp;
		unsigned char *data = stbi_load_from_memory(bytes, size, &w, &h, &comp, requiredComponents);
		if (!data) {
			// NOTE: you can use `warn` instead of `err`
			if (err) {
				(*err) += "Unknown image format.\n";
			}
			return false;
		}

		if (w < 1 || h < 1) {
			free(data);
			if (err) {
				(*err) += "Invalid image data.\n";
			}
			return false;
		}

		if (req_width > 0) {
			if (req_width != w) {
				free(data);
				if (err) {
					(*err) += "Image width mismatch.\n";
				}
				return false;
			}
		}

		if (req_height > 0) {
			if (req_height != h) {
				free(data);
				if (err) {
					(*err) += "Image height mismatch.\n";
				}
				return false;
			}
		}

		image->width = w;
		image->height = h;
		image->component = requiredComponents;
		image->image.resize(static_cast<size_t>(w * h * image->component));
		std::copy(data, data + w * h * image->component, image->image.begin());

		free(data);

		return true;
	}

	bool WriteImageData(const std::string *basepath, const std::string *filename,
		Image *image, bool embedImages, void *)
	{
		assert(0); // TODO
		return false;
	}
}

std::shared_ptr<wiResource> RegisterTexture(tinygltf::Image *image, const string& type_name)
{
	// We will load the texture2d by hand here and register to the resource manager (if it was not already registered)
	if (!wiResourceManager::Contains(image->uri))
	{
		int width = image->width;
		int height = image->height;
		int channelCount = image->component;

		if (!image->image.empty())
		{
			GraphicsDevice* device = wiRenderer::GetDevice();

			TextureDesc desc;
			desc.ArraySize = 1;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.Format = FORMAT_R8G8B8A8_UNORM;
			desc.Height = uint32_t(height);
			desc.Width = uint32_t(width);
			desc.MipLevels = (uint32_t)log2(max(width, height));
			desc.MiscFlags = 0;
			desc.Usage = USAGE_DEFAULT;

			uint32_t mipwidth = width;
			vector<SubresourceData> InitData(desc.MipLevels);
			for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
			{
				InitData[mip].pSysMem = image->image.data();
				InitData[mip].SysMemPitch = uint32_t(mipwidth * channelCount);
				mipwidth = std::max(1u, mipwidth / 2);
			}

			Texture* tex = new Texture;
			if (device->CreateTexture(&desc, InitData.data(), tex))
			{
				for (uint32_t i = 0; i < tex->GetDesc().MipLevels; ++i)
				{
					int subresource_index;
					subresource_index = device->CreateSubresource(tex, SRV, 0, 1, i, 1);
					assert(subresource_index == i);
					subresource_index = device->CreateSubresource(tex, UAV, 0, 1, i, 1);
					assert(subresource_index == i);
				}

				if (tex != nullptr)
				{
					if (image->uri.empty())
					{
						// If the texture was embedded, export it as a file:
						stringstream ss;
						do {
							ss.str("");
							ss << "gltfimport_" << type_name << "_" << wiRandom::getRandom(INT_MAX) << ".png";
						} while (wiHelper::FileExists(ss.str())); // this is to avoid overwriting an existing exported image
						image->uri = ss.str();
						bool success = wiHelper::saveTextureToFile(image->image, desc, ss.str());
						assert(success);
					}

					// We loaded the texture2d, so register to the resource manager to be retrieved later:
					auto resource = wiResourceManager::Register(image->uri, tex, wiResource::IMAGE);
					wiRenderer::AddDeferredMIPGen(resource, true);
					return resource;
				}
			}
			else
			{
				assert(0);
			}

		}
	}

	return nullptr;
}



struct LoaderState
{
	tinygltf::Model gltfModel;
	Scene* scene;
	unordered_map<int, Entity> entityMap;  // node -> entity
};

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
			stringstream ss("");
			ss << "cam" << camID++;
			node.name = ss.str();
		}

		entity = scene.Entity_CreateCamera(node.name, (float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 800);
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
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(name));


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

	loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
	loader.SetImageWriter(tinygltf::WriteImageData, nullptr);
	
	LoaderState state;
	state.scene = &scene;

	std::vector<uint8_t> filedata;
	ret = wiHelper::FileRead(fileName, filedata);

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
		wiHelper::messageBox(err, "GLTF error!");
	}

	Entity rootEntity = CreateEntity();
	scene.transforms.Create(rootEntity);
	scene.names.Create(rootEntity) = name;

	// Create materials:
	for (auto& x : state.gltfModel.materials)
	{
		Entity materialEntity = scene.Entity_CreateMaterial(x.name);

		MaterialComponent& material = *scene.materials.GetComponent(materialEntity);

		material.baseColor = XMFLOAT4(1, 1, 1, 1);
		material.roughness = 1.0f;
		material.metalness = 1.0f;
		material.reflectance = 0.02f;

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
			auto& img = state.gltfModel.images[tex.source];
			material.baseColorMap = RegisterTexture(&img, "basecolor");
			material.baseColorMapName = img.uri;
			material.uvset_baseColorMap = baseColorTexture->second.TextureTexCoord();
		}
		if (normalTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[normalTexture->second.TextureIndex()];
			auto& img = state.gltfModel.images[tex.source];
			material.normalMap = RegisterTexture(&img, "normal");
			material.normalMapName = img.uri;
			material.uvset_normalMap = normalTexture->second.TextureTexCoord();
		}
		if (metallicRoughnessTexture != x.values.end())
		{
			auto& tex = state.gltfModel.textures[metallicRoughnessTexture->second.TextureIndex()];
			auto& img = state.gltfModel.images[tex.source];
			material.surfaceMap = RegisterTexture(&img, "roughness_metallic");
			material.surfaceMapName = img.uri;
			material.uvset_surfaceMap = metallicRoughnessTexture->second.TextureTexCoord();
		}
		if (emissiveTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[emissiveTexture->second.TextureIndex()];
			auto& img = state.gltfModel.images[tex.source];
			material.emissiveMap = RegisterTexture(&img, "emissive");
			material.emissiveMapName = img.uri;
			material.uvset_emissiveMap = emissiveTexture->second.TextureTexCoord();
		}
		if (occlusionTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[occlusionTexture->second.TextureIndex()];
			auto& img = state.gltfModel.images[tex.source];
			material.occlusionMap = RegisterTexture(&img, "occlusion");
			material.occlusionMapName = img.uri;
			material.uvset_occlusionMap = occlusionTexture->second.TextureTexCoord();
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
		if (alphaCutoff != x.additionalValues.end())
		{
			material.alphaRef = 1 - float(alphaCutoff->second.Factor());
		}
		if (alphaMode != x.additionalValues.end())
		{
			if (alphaMode->second.string_value.compare("BLEND") == 0)
			{
				material.userBlendMode = BLENDMODE_ALPHA;
			}
		}

		auto unlit = x.extensions.find("KHR_materials_unlit");
		if (unlit != x.extensions.end())
		{
			material.shaderType = MaterialComponent::SHADERTYPE_UNLIT;
		}

		// specular-glossiness workflow (todo):
		auto specularGlossinessWorkflow = x.extensions.find("KHR_materials_pbrSpecularGlossiness");
		if (specularGlossinessWorkflow != x.extensions.end())
		{
			material.SetUseSpecularGlossinessWorkflow(true);

			if (specularGlossinessWorkflow->second.Has("diffuseTexture"))
			{
				int index = specularGlossinessWorkflow->second.Get("diffuseTexture").Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				auto& img = state.gltfModel.images[tex.source];
				material.baseColorMap = RegisterTexture(&img, "diffuse");
				material.baseColorMapName = img.uri;
				material.uvset_baseColorMap = (uint32_t)specularGlossinessWorkflow->second.Get("diffuseTexture").Get("texCoord").Get<int>();
			}
			if (specularGlossinessWorkflow->second.Has("specularGlossinessTexture"))
			{
				int index = specularGlossinessWorkflow->second.Get("specularGlossinessTexture").Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				auto& img = state.gltfModel.images[tex.source];
				material.surfaceMap = RegisterTexture(&img, "specular_glossiness");
				material.surfaceMapName = img.uri;
				material.uvset_surfaceMap = (uint32_t)specularGlossinessWorkflow->second.Get("specularGlossinessTexture").Get("texCoord").Get<int>();
			}

			if (specularGlossinessWorkflow->second.Has("diffuseFactor"))
			{
				auto& factor = specularGlossinessWorkflow->second.Get("diffuseFactor");
				material.baseColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
				material.baseColor.y = factor.ArrayLen() > 1 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
				material.baseColor.z = factor.ArrayLen() > 2 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
				material.baseColor.w = factor.ArrayLen() > 3 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
			}
			//if (specularGlossinessWorkflow->second.Has("specularFactor"))
			//{
			//	auto& factor = specularGlossinessWorkflow->second.Get("specularFactor");
			//	material.baseColor.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
			//	material.baseColor.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
			//	material.baseColor.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
			//	material.baseColor.w = factor.ArrayLen() > 0 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
			//}
			if (specularGlossinessWorkflow->second.Has("glossinessFactor"))
			{
				auto& factor = specularGlossinessWorkflow->second.Get("glossinessFactor");
				material.roughness = 1 - float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
		}

		// Avoid zero roughness factors:
		material.roughness = max(0.001f, material.roughness);

	}

	if (scene.materials.GetCount() == 0)
	{
		scene.Entity_CreateMaterial("gltfimport_defaultMaterial");
	}

	// Create meshes:
	for (auto& x : state.gltfModel.meshes)
	{
		Entity meshEntity = scene.Entity_CreateMesh(x.name);
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

			mesh.subsets.back().materialID = scene.materials.GetEntity(max(0, prim.material));

			uint32_t vertexOffset = (uint32_t)mesh.vertex_positions.size();

			const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

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
				const string& attr_name = attr.first;
				int attr_data = attr.second;

				const tinygltf::Accessor& accessor = state.gltfModel.accessors[attr_data];
				const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

				int stride = accessor.ByteStride(bufferView);
				size_t vertexCount = accessor.count;

				const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				if (!attr_name.compare("POSITION"))
				{
					mesh.vertex_positions.resize(vertexOffset + vertexCount);
					assert(stride == 12);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_positions[vertexOffset + i] = ((XMFLOAT3*)data)[i];
					}
				}
				else if (!attr_name.compare("NORMAL"))
				{
					mesh.vertex_normals.resize(vertexOffset + vertexCount);
					assert(stride == 12);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_normals[vertexOffset + i] = ((XMFLOAT3*)data)[i];
					}
				}
				else if (!attr_name.compare("TANGENT"))
				{
					mesh.vertex_tangents.resize(vertexOffset + vertexCount);
					assert(stride == 16);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_tangents[vertexOffset + i] = ((XMFLOAT4*)data)[i];
					}
				}
				else if (!attr_name.compare("TEXCOORD_0"))
				{
					mesh.vertex_uvset_0.resize(vertexOffset + vertexCount);
					assert(stride == 8);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						const XMFLOAT2& tex = ((XMFLOAT2*)data)[i];

						mesh.vertex_uvset_0[vertexOffset + i].x = tex.x;
						mesh.vertex_uvset_0[vertexOffset + i].y = tex.y;
					}
				}
				else if (!attr_name.compare("TEXCOORD_1"))
				{
					mesh.vertex_uvset_1.resize(vertexOffset + vertexCount);
					assert(stride == 8);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						const XMFLOAT2& tex = ((XMFLOAT2*)data)[i];

						mesh.vertex_uvset_1[vertexOffset + i].x = tex.x;
						mesh.vertex_uvset_1[vertexOffset + i].y = tex.y;
					}
				}
				else if (!attr_name.compare("JOINTS_0"))
				{
					mesh.vertex_boneindices.resize(vertexOffset + vertexCount);
					if (stride == 4)
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
					else if (stride == 8)
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
					else
					{
						assert(0);
					}
				}
				else if (!attr_name.compare("WEIGHTS_0"))
				{
					mesh.vertex_boneweights.resize(vertexOffset + vertexCount);
					assert(stride == 16);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_boneweights[vertexOffset + i] = ((XMFLOAT4*)data)[i];
					}
				}
				else if (!attr_name.compare("COLOR_0"))
				{
					mesh.vertex_colors.resize(vertexOffset + vertexCount);
					assert(stride == 16);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						const XMFLOAT4& color = ((XMFLOAT4*)data)[i];
						uint32_t rgba = wiMath::CompressColor(color);

						mesh.vertex_colors[vertexOffset + i] = rgba;
					}
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
	const tinygltf::Scene &gltfScene = state.gltfModel.scenes[max(0, state.gltfModel.defaultScene)];
	for (size_t i = 0; i < gltfScene.nodes.size(); i++)
	{
		LoadNode(gltfScene.nodes[i], rootEntity, state);
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

			animationcomponent.samplers[i].data = CreateEntity();
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
					animationcomponent.start = min(animationcomponent.start, time);
					animationcomponent.end = max(animationcomponent.end, time);
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
			else
			{
				animationcomponent.channels[i].path = AnimationComponent::AnimationChannel::Path::UNKNOWN;
			}
		}

	}

	if (transform_to_LH)
	{
		TransformComponent& transform = *scene.transforms.GetComponent(rootEntity);
		transform.scale_local.z = -transform.scale_local.z;
		transform.SetDirty();
	}

	scene.Update(0);

}
