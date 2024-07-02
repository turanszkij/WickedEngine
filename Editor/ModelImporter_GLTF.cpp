#include "stdafx.h"
#include "wiScene.h"
#include "ModelImporter.h"
#include "wiRandom.h"

#include "Utility/stb_image.h"
#include "Utility/dds.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_FS
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::ecs;

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
				ss += "gltfimport_" + std::to_string(wi::random::GetRandom(std::numeric_limits<uint32_t>::max())) + ".png";
			} while (wi::resourcemanager::Contains(ss)); // this is to avoid overwriting an existing imported image
			image->uri = ss;
		}

		auto resource = wi::resourcemanager::Load(
			image->uri,
			wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA | wi::resourcemanager::Flags::IMPORT_DELAY,
			(const uint8_t*)bytes,
			(size_t)size
		);

		if (!resource.IsValid())
		{
			return false;
		}

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
	std::string name;
	tinygltf::Model gltfModel;
	Scene* scene;
	wi::unordered_map<int, Entity> entityMap;  // node -> entity
	Entity rootEntity = INVALID_ENTITY;

	//Export states
	wi::unordered_map<std::string, int> textureMap; // path -> textureid
	wi::unordered_map<Entity, int> nodeMap;// entity -> node
	wi::unordered_map<size_t, TransformComponent> transforms_original; // original transform states
};

void Import_Extension_VRM(LoaderState& state);
void Import_Extension_VRMC(LoaderState& state);
void VRM_ToonMaterialCustomize(const std::string& name, MaterialComponent& material);
void Import_Mixamo_Bone(LoaderState& state, Entity boneEntity, const tinygltf::Node& node);

// Recursively loads nodes and resolves hierarchy:
void LoadNode(int nodeIndex, Entity parent, LoaderState& state)
{
	if (nodeIndex < 0 || state.entityMap.count(nodeIndex) != 0)
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
			entity = scene.armatures.GetEntity(node.skin);
			MeshComponent* mesh = &scene.meshes[node.mesh];
			Entity meshEntity = scene.meshes.GetEntity(node.mesh);
			assert(!mesh->vertex_boneindices.empty());
			if (mesh->armatureID != INVALID_ENTITY)
			{
				// Reuse mesh with different skin is not possible currently, so we create a new one:
				meshEntity = entity;
				MeshComponent& newMesh = scene.meshes.Create(meshEntity);
				newMesh = scene.meshes[node.mesh];
				newMesh.CreateRenderData();
				mesh = &newMesh;
			}
			mesh->armatureID = entity;

			// The object component will use an identity transform but will be parented to the armature:
			Entity objectEntity = scene.Entity_CreateObject(node.name);
			ObjectComponent& object = *scene.objects.GetComponent(objectEntity);
			object.meshID = meshEntity;
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
		// Note: limiting min scale because scale <= 0.0001 will break matrix decompose and mess up the model (float precision issue?)
		for (int idx = 0; idx < 3; ++idx)
		{
			if (std::abs(node.scale[idx]) <= 0.0001)
			{
				const double sign = node.scale[idx] < 0 ? -1 : 1;
				node.scale[idx] = 0.0001001 * sign;
			}
		}
		transform.scale_local = XMFLOAT3(float(node.scale[0]), float(node.scale[1]), float(node.scale[2]));
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

void FlipZAxis(LoaderState& state)
{
	Scene& wiscene = *state.scene;

	// Flip mesh data first
	for(size_t i = 0; i < wiscene.meshes.GetCount(); ++i)
	{
		auto& mesh = wiscene.meshes[i];
		for(auto& v_pos : mesh.vertex_positions)
		{
			v_pos.z *= -1.f;
		}
		for(auto& v_norm : mesh.vertex_normals)
		{
			v_norm.z *= -1.f;
		}
		for(auto& v_tan : mesh.vertex_tangents)
		{
			v_tan.z *= -1.f;
		}
		for(auto& v_morph : mesh.morph_targets)
		{
			for(auto& v_morph_norm : v_morph.vertex_normals)
			{
				v_morph_norm.z *= -1.f;
			}
			for(auto& v_morph_pos : v_morph.vertex_positions)
			{
				v_morph_pos.z *= -1.f;
			}
		}
		mesh.FlipCulling(); // calls CreateRenderData
	}

	// Flip scene's transformComponents
	bool state_restore = (state.transforms_original.size() > 0);
	if(!state_restore)
	{
		wi::unordered_map<wi::ecs::Entity, wi::ecs::Entity> hierarchy_list;
		wi::unordered_map<size_t, wi::scene::TransformComponent> correction_queue;

		for(size_t i = 0; i < wiscene.transforms.GetCount(); ++i)
		{
			auto transformEntity = wiscene.transforms.GetEntity(i);
			if(transformEntity == state.rootEntity)
				continue;

			correction_queue[i] = wiscene.transforms[i];

			auto hierarchy = wiscene.hierarchy.GetComponent(transformEntity);
			if(transformEntity != state.rootEntity && hierarchy != nullptr)
			{
				hierarchy_list[transformEntity] = hierarchy->parentID;
				wiscene.Component_Detach(transformEntity);
			}
		}
		state.transforms_original.insert(correction_queue.begin(), correction_queue.end());
		for(auto& correction_pair : correction_queue)
		{
			auto& transform = wiscene.transforms[correction_pair.first];
			auto& transform_original = correction_pair.second;

			XMVECTOR V_S,V_R,V_T;
			XMMatrixDecompose(&V_S, &V_R, &V_T, XMLoadFloat4x4(&transform_original.world));
			XMFLOAT3 pos, scale;
			XMFLOAT4 rot;
			XMStoreFloat3(&scale, V_S);
			XMStoreFloat3(&pos, V_T);
			XMStoreFloat4(&rot, V_R);
			pos.z *= -1.f;
			rot.x *= -1.f;
			rot.y *= -1.f;

			auto build_m = 
				XMMatrixScalingFromVector(XMLoadFloat3(&scale)) *
				XMMatrixRotationQuaternion(XMLoadFloat4(&rot)) *
				XMMatrixTranslationFromVector(XMLoadFloat3(&pos));

			XMFLOAT4X4 build_m4;
			XMStoreFloat4x4(&build_m4, build_m);

			transform.world = build_m4;
			transform.ApplyTransform();
		}
		for(auto& hierarchy_pair : hierarchy_list)
		{
			wiscene.Component_Attach(hierarchy_pair.first, hierarchy_pair.second);
		}
	}
	else
	{
		for(size_t i = 0; i < wiscene.transforms.GetCount(); ++i)
		{
			auto transform_original_find = state.transforms_original.find(i);
			if(transform_original_find != state.transforms_original.end())
			{
				auto& transform = wiscene.transforms[i];
				transform = transform_original_find->second;
			}
		}
		state.transforms_original.clear();
	}

	// Flip armature's bind pose
	for(size_t i = 0; i < wiscene.armatures.GetCount(); ++i)
	{
		auto& armature = wiscene.armatures[i];
		for(int i = 0; i < armature.inverseBindMatrices.size(); ++i)
		{
			auto& bind = armature.inverseBindMatrices[i];
			
			XMVECTOR V_S,V_R,V_T;
			XMMatrixDecompose(&V_S, &V_R, &V_T, XMLoadFloat4x4(&bind));
			XMFLOAT3 pos, scale;
			XMFLOAT4 rot;
			XMStoreFloat3(&scale, V_S);
			XMStoreFloat3(&pos, V_T);
			XMStoreFloat4(&rot, V_R);
			pos.z *= -1.f;
			rot.x *= -1.f;
			rot.y *= -1.f;

			auto build_m = 
				XMMatrixScalingFromVector(XMLoadFloat3(&scale)) *
				XMMatrixRotationQuaternion(XMLoadFloat4(&rot)) *
				XMMatrixTranslationFromVector(XMLoadFloat3(&pos));
			XMFLOAT4X4 build_m4;
			XMStoreFloat4x4(&build_m4, build_m);

			bind = build_m4;
		}
	}

	// Flip animation data for translation and rotation
	for(size_t i = 0; i < wiscene.animations.GetCount(); ++i)
	{
		auto& animation = wiscene.animations[i];
		
		for(auto& channel : animation.channels)
		{
			auto data = wiscene.animation_datas.GetComponent(animation.samplers[channel.samplerIndex].data);
			
			if(channel.path == wi::scene::AnimationComponent::AnimationChannel::Path::TRANSLATION)
			{
				for(size_t k = 0; k < data->keyframe_data.size()/3; ++k)
				{
					data->keyframe_data[k*3+2] *= -1.f;
				}
			}
			if(channel.path == wi::scene::AnimationComponent::AnimationChannel::Path::ROTATION)
			{
				for(size_t k = 0; k < data->keyframe_data.size()/4; ++k)
				{
					data->keyframe_data[k*4] *= -1.f;
					data->keyframe_data[k*4+1] *= -1.f;
				}
			}
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
	bool ret = wi::helper::FileRead(fileName, filedata);

	if (ret)
	{
		std::string basedir = tinygltf::GetBaseDir(fileName);

		if (!extension.compare("GLTF"))
		{
			ret = loader.LoadASCIIFromString(
				&state.gltfModel,
				&err,
				&warn, 
				reinterpret_cast<const char*>(filedata.data()),
				static_cast<unsigned int>(filedata.size()),
				basedir
			);
		}
		else
		{
			ret = loader.LoadBinaryFromMemory(
				&state.gltfModel,
				&err,
				&warn,
				filedata.data(),
				static_cast<unsigned int>(filedata.size()),
				basedir
			);
		}
	}
	else
	{
		err = "Failed to read file: " + fileName;
	}

	if (!ret)
	{
		wi::helper::messageBox(err, "GLTF error!");
	}

	state.rootEntity = CreateEntity();
	scene.transforms.Create(state.rootEntity);
	scene.names.Create(state.rootEntity) = name;
	state.name = name;

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

		auto ext_mtoon = x.extensions.find("VRMC_materials_mtoon");
		if (ext_mtoon != x.extensions.end())
		{
			// https://github.com/vrm-c/vrm-specification/tree/master/specification/VRMC_materials_mtoon-1.0
			VRM_ToonMaterialCustomize(x.name, material);
		}

		auto ext_emissiveStrength = x.extensions.find("KHR_materials_emissive_strength");
		if (ext_emissiveStrength != x.extensions.end())
		{
			// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_emissive_strength/README.md
			if (ext_emissiveStrength->second.Has("emissiveStrength"))
			{
				auto& factor = ext_emissiveStrength->second.Get("emissiveStrength");
				material.SetEmissiveStrength(float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>()));
			}
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

		auto ext_aniso = x.extensions.find("KHR_materials_anisotropy");
		if (ext_aniso != x.extensions.end())
		{
			// https://github.com/ux3d/glTF/tree/extensions/KHR_materials_anisotropy/extensions/2.0/Khronos/KHR_materials_anisotropy

			material.shaderType = MaterialComponent::SHADERTYPE_PBR_ANISOTROPIC;

			if (ext_aniso->second.Has("anisotropyStrength"))
			{
				auto& factor = ext_aniso->second.Get("anisotropyStrength");
				material.anisotropy_strength = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_aniso->second.Has("anisotropyRotation"))
			{
				auto& factor = ext_aniso->second.Get("anisotropyRotation");
				material.anisotropy_rotation = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_aniso->second.Has("anisotropyTexture"))
			{
				auto& param = ext_aniso->second.Get("anisotropyTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::ANISOTROPYMAP].name = img.uri;
				material.textures[MaterialComponent::ANISOTROPYMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}

			// Differently from the proposed spec, in the proposed sample model, I see different namings: https://github.com/KhronosGroup/glTF-Sample-Models/tree/Anisotropy-Barn-Lamp/2.0/AnisotropyBarnLamp
			if (ext_aniso->second.Has("anisotropy"))
			{
				auto& factor = ext_aniso->second.Get("anisotropy");
				material.anisotropy_strength = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_aniso->second.Has("anisotropyDirection"))
			{
				auto& factor = ext_aniso->second.Get("anisotropyDirection");
				material.anisotropy_rotation = float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>());
			}
			if (ext_aniso->second.Has("anisotropyDirectionTexture"))
			{
				auto& param = ext_aniso->second.Get("anisotropyDirectionTexture");
				int index = param.Get("index").Get<int>();
				auto& tex = state.gltfModel.textures[index];
				int img_source = tex.source;
				if (tex.extensions.count("KHR_texture_basisu"))
				{
					img_source = tex.extensions["KHR_texture_basisu"].Get("source").Get<int>();
				}
				auto& img = state.gltfModel.images[img_source];
				material.textures[MaterialComponent::ANISOTROPYMAP].name = img.uri;
				material.textures[MaterialComponent::ANISOTROPYMAP].uvset = (uint32_t)param.Get("texCoord").Get<int>();
			}
		}

		material.CreateRenderData();
	}

	// Create meshes:
	for (auto& x : state.gltfModel.meshes)
	{
		Entity meshEntity = scene.Entity_CreateMesh(x.name);
		scene.Component_Attach(meshEntity, state.rootEntity);
		MeshComponent& mesh = *scene.meshes.GetComponent(meshEntity);

		for (auto& prim : x.primitives)
		{
			mesh.subsets.push_back(MeshComponent::MeshSubset());
			if (scene.materials.GetCount() == 0)
			{
				// Create a material last minute if there was none
				scene.materials.Create(CreateEntity());
			}
			mesh.subsets.back().materialID = scene.materials.GetEntity(std::max(0, prim.material));
			MaterialComponent* material = scene.materials.GetComponent(mesh.subsets.back().materialID);
			uint32_t vertexOffset = (uint32_t)mesh.vertex_positions.size();

			const size_t index_remap[] = {
				0,2,1
			};

			if (prim.indices >= 0)
			{
				// Fill indices:
				const tinygltf::Accessor& accessor = state.gltfModel.accessors[prim.indices];
				const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

				int stride = accessor.ByteStride(bufferView);
				size_t indexCount = accessor.count;
				size_t indexOffset = mesh.indices.size();
				mesh.indices.resize(indexOffset + indexCount);
				mesh.subsets.back().indexOffset = (uint32_t)indexOffset;
				mesh.subsets.back().indexCount = (uint32_t)indexCount;

				const uint8_t* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

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

				if (mesh.subsets.back().indexCount == 0)
				{
					// Autogen indices:
					//	Note: this is not common, so it is simpler to create a dummy index buffer here than rewrite engine to support this case
					size_t indexOffset = mesh.indices.size();
					mesh.indices.resize(indexOffset + vertexCount);
					for (size_t vi = 0; vi < vertexCount; vi += 3)
					{
						mesh.indices[indexOffset + vi + 0] = uint32_t(vertexOffset + vi + index_remap[0]);
						mesh.indices[indexOffset + vi + 1] = uint32_t(vertexOffset + vi + index_remap[1]);
						mesh.indices[indexOffset + vi + 2] = uint32_t(vertexOffset + vi + index_remap[2]);
					}
					mesh.subsets.back().indexOffset = (uint32_t)indexOffset;
					mesh.subsets.back().indexCount = (uint32_t)vertexCount;
				}

				const uint8_t* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				if (!attr_name.compare("POSITION"))
				{
					mesh.vertex_positions.resize(vertexOffset + vertexCount);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						mesh.vertex_positions[vertexOffset + i] = *(const XMFLOAT3*)(data + i * stride);
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
						mesh.vertex_normals[vertexOffset + i] = *(const XMFLOAT3*)(data + i * stride);
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
						mesh.vertex_tangents[vertexOffset + i] = *(const XMFLOAT4*)(data + i * stride);
					}
				}
				else if (!attr_name.compare("TEXCOORD_0"))
				{
					mesh.vertex_uvset_0.resize(vertexOffset + vertexCount);
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						for (size_t i = 0; i < vertexCount; ++i)
						{
							const XMFLOAT2& tex = *(const XMFLOAT2*)((size_t)data + i * stride);

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
							const XMFLOAT2& tex = *(const XMFLOAT2*)((size_t)data + i * stride);

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
							const JointTmp& joint = *(const JointTmp*)(data + i * stride);

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
							const JointTmp& joint = *(const JointTmp*)(data + i * stride);

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
							const JointTmp& joint = *(const JointTmp*)(data + i * stride);

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
			}


			mesh.morph_targets.resize(prim.targets.size());
			for (size_t i = 0; i < prim.targets.size(); i++)
			{
				MeshComponent::MorphTarget& morph_target = mesh.morph_targets[i];
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
							morph_target.vertex_positions.resize(sparse.count);
							morph_target.sparse_indices_positions.resize(sparse.count);

							switch (sparse.indices.componentType)
							{
							default:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
								for (int s = 0; s < sparse.count; ++s)
								{
									morph_target.sparse_indices_positions[s] = vertexOffset + sparse_indices_data[s];
									morph_target.vertex_positions[s] = ((const XMFLOAT3*)sparse_values_data)[s];
								}
								break;
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
								for (int s = 0; s < sparse.count; ++s)
								{
									morph_target.sparse_indices_positions[s] = vertexOffset + ((const uint16_t*)sparse_indices_data)[s];
									morph_target.vertex_positions[s] = ((const XMFLOAT3*)sparse_values_data)[s];
								}
								break;
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
								for (int s = 0; s < sparse.count; ++s)
								{
									morph_target.sparse_indices_positions[s] = vertexOffset + ((const uint32_t*)sparse_indices_data)[s];
									morph_target.vertex_positions[s] = ((const XMFLOAT3*)sparse_values_data)[s];
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

							morph_target.vertex_positions.resize(vertexOffset + vertexCount);
							for (size_t j = 0; j < vertexCount; ++j)
							{
								morph_target.vertex_positions[vertexOffset + j] = ((XMFLOAT3*)data)[j];
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
							morph_target.vertex_normals.resize(sparse.count);
							morph_target.sparse_indices_normals.resize(sparse.count);

							switch (sparse.indices.componentType)
							{
							default:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
								for (int s = 0; s < sparse.count; ++s)
								{
									morph_target.sparse_indices_normals[s] = vertexOffset + sparse_indices_data[s];
									morph_target.vertex_normals[s] = ((const XMFLOAT3*)sparse_values_data)[s];
								}
								break;
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
								for (int s = 0; s < sparse.count; ++s)
								{
									morph_target.sparse_indices_normals[s] = vertexOffset + ((const uint16_t*)sparse_indices_data)[s];
									morph_target.vertex_normals[s] = ((const XMFLOAT3*)sparse_values_data)[s];
								}
								break;
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
								for (int s = 0; s < sparse.count; ++s)
								{
									morph_target.sparse_indices_normals[s] = vertexOffset + ((const uint32_t*)sparse_indices_data)[s];
									morph_target.vertex_normals[s] = ((const XMFLOAT3*)sparse_values_data)[s];
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

							morph_target.vertex_normals.resize(vertexOffset + vertexCount);
							for (size_t j = 0; j < vertexCount; ++j)
							{
								morph_target.vertex_normals[vertexOffset + j] = ((XMFLOAT3*)data)[j];
							}
						}
					}
				}
			}
		}

		for (size_t i = 0; i < x.weights.size(); i++)
		{
			mesh.morph_targets[i].weight = static_cast<float_t>(x.weights[i]);
		}

		if (mesh.vertex_normals.empty())
		{
			mesh.vertex_normals.resize(mesh.vertex_positions.size());
			mesh.ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST);
		}

		mesh.CreateRenderData(); // tangents are generated inside if needed, which must be done before FlipZAxis!
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
		Entity armatureEntity = scene.armatures.GetEntity(armatureIndex);
		ArmatureComponent& armature = scene.armatures[armatureIndex++];

		const size_t jointCount = skin.joints.size();

		armature.boneCollection.resize(jointCount);

		// Create bone collection:
		for (size_t i = 0; i < jointCount; ++i)
		{
			int jointIndex = skin.joints[i];
			Entity boneEntity = state.entityMap[jointIndex];

			armature.boneCollection[i] = boneEntity;

			Import_Mixamo_Bone(state, boneEntity, state.gltfModel.nodes[jointIndex]);
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
			scene.Component_Attach(animationcomponent.samplers[i].data, entity);
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
		if (!x.type.compare("orthographic"))
			continue;
		Entity entity = scene.cameras.GetEntity(cameraIndex);
		CameraComponent& camera = scene.cameras[cameraIndex++];

		if (x.perspective.aspectRatio > 0)
		{
			camera.width = float(x.perspective.aspectRatio);
			camera.height = 1.f;
		}
		camera.fov = (float)x.perspective.yfov;
		camera.zFarP = (float)x.perspective.zfar;
		camera.zNearP = (float)x.perspective.znear;
	}

	// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_lights_image_based/README.md
	auto env = state.gltfModel.extensions.find("EXT_lights_image_based");
	if (env != state.gltfModel.extensions.end())
	{
		int counter = 0;
		auto lights = env->second.Get("lights");
		for (int i = 0; i < (int)lights.ArrayLen(); ++i)
		{
			if (scene.weathers.GetCount() == 0)
			{
				Entity entity = CreateEntity();
				scene.weathers.Create(entity);
				scene.names.Create(entity) = "weather";
			}
			WeatherComponent& weather = scene.weathers[0];

			auto light = lights.Get(i);
			if (light.Has("intensity"))
			{
				auto value = light.Get("intensity");
				weather.skyExposure = (float)value.GetNumberAsDouble();
			}
			if (light.Has("rotation"))
			{
				auto value = light.Get("rotation");
				XMFLOAT4 quaternion = {};
				quaternion.x = value.ArrayLen() > 0 ? float(value.Get(0).IsNumber() ? value.Get(0).Get<double>() : value.Get(0).Get<int>()) : 0.0f;
				quaternion.y = value.ArrayLen() > 1 ? float(value.Get(1).IsNumber() ? value.Get(1).Get<double>() : value.Get(1).Get<int>()) : 0.0f;
				quaternion.z = value.ArrayLen() > 2 ? float(value.Get(2).IsNumber() ? value.Get(2).Get<double>() : value.Get(2).Get<int>()) : 0.0f;
				quaternion.w = value.ArrayLen() > 3 ? float(value.Get(3).IsNumber() ? value.Get(3).Get<double>() : value.Get(3).Get<int>()) : 1.0f;
				XMVECTOR Q = XMLoadFloat4(&quaternion);
				float angle;
				XMVECTOR axis;
				XMQuaternionToAxisAngle(&axis, &angle, Q);
				weather.sky_rotation = XM_2PI - angle;
			}
			//if (light.Has("irradianceCoefficients"))
			//{
			//	auto value = light.Get("irradianceCoefficients");
			//	float spherical_harmonics[9][3] = {};
			//	for (int c = 0; c < std::min(9, (int)value.ArrayLen()); ++c)
			//	{
			//		for (int f = 0; f < std::min(3, (int)value.Get(c).ArrayLen()); ++f)
			//		{
			//			spherical_harmonics[c][f] = (float)value.Get(c).Get(f).GetNumberAsDouble();
			//		}
			//	}
			//}
			if (light.Has("specularImages"))
			{
				auto mips = light.Get("specularImages");
				int mip_count = (int)mips.ArrayLen();

				TextureDesc desc;
				desc.format = Format::R9G9B9E5_SHAREDEXP;
				desc.bind_flags = BindFlag::SHADER_RESOURCE;
				if (light.Has("specularImageSize"))
				{
					auto value = light.Get("specularImageSize");
					desc.width = desc.height = (uint32_t)value.GetNumberAsInt();
				}
				desc.array_size = 6;
				desc.mip_levels = (uint32_t)mip_count;
				desc.misc_flags = ResourceMiscFlag::TEXTURECUBE;

				wi::vector<wi::vector<XMFLOAT3SE>> hdr_datas(mip_count * 6);

				for (int m = 0; m < mip_count; ++m)
				{
					auto mip = mips.Get(m);
					int face_count = (int)mip.ArrayLen();
					for (int f = 0; f < face_count; ++f)
					{
						auto index = mip.Get(f).GetNumberAsInt();
						auto& image = state.gltfModel.images[index];
						int idx = f * mip_count + m;
						wi::Resource res = wi::resourcemanager::Load(image.uri, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
						auto& imagefiledata = res.GetFileData();
						const stbi_uc* filedata = imagefiledata.data();
						size_t filesize = imagefiledata.size();
						int width, height, bpp;
						wi::Color* rgba = (wi::Color*)stbi_load_from_memory(filedata, (int)filesize, &width, &height, &bpp, 4);
						if (rgba == nullptr)
							continue;
						wi::vector<XMFLOAT3SE>& hdr_data = hdr_datas[idx];
						hdr_data.resize(width * height);
						for (int y = 0; y < height; ++y)
						{
							for (int x = 0; x < width; ++x)
							{
								int y_flip = height - 1 - y;
								wi::Color color = rgba[x + y_flip * width];
								XMFLOAT4 unpk = color.toFloat4();
								// Remove SRGB curve:
								unpk.x = std::pow(unpk.x, 2.2f);
								unpk.y = std::pow(unpk.y, 2.2f);
								unpk.z = std::pow(unpk.z, 2.2f);
								if (bpp == 4) // if has alpha channel, then it is assumed to have RGBD encoding
								{
									// RGBD conversion: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_lights_image_based/README.md#rgbd
									unpk.x /= unpk.w;
									unpk.y /= unpk.w;
									unpk.z /= unpk.w;
								}
								hdr_data[x + y * width] = XMFLOAT3SE(unpk.x, unpk.y, unpk.z);
							}
						}
						stbi_image_free(rgba);
					}
				}

				size_t wholeDataSize = 0;
				for (auto& x : hdr_datas)
				{
					wholeDataSize += x.size() * sizeof(XMFLOAT3SE);
				}

				wi::vector<uint8_t> dds;
				dds.resize(sizeof(dds::Header) + wholeDataSize);
				dds::write_header(
					dds.data(),
					dds::DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
					desc.width,
					desc.height,
					desc.mip_levels,
					desc.array_size,
					true
				);

				size_t offset = sizeof(dds::Header);
				for (auto& x : hdr_datas)
				{
					std::memcpy(dds.data() + offset, x.data(), x.size() * sizeof(XMFLOAT3SE));
					offset += x.size() * sizeof(XMFLOAT3SE);
				}
				
				weather.skyMapName = wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(fileName)) + "/EXT_lights_image_based_" + std::to_string(counter++) + ".dds";
				weather.skyMap = wi::resourcemanager::Load(
					weather.skyMapName,
					wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA,
					dds.data(),
					dds.size()
				);
				weather.ambient = {}; // remove ambient if gltf has env lighting
			}
		}
	}

	Import_Extension_VRM(state);
	Import_Extension_VRMC(state);

	//Correct orientation after importing
	scene.Update(0);
	FlipZAxis(state);

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
			Entity entity = state.rootEntity;
			if (state.scene->expressions.Contains(entity))
			{
				entity = CreateEntity();
				state.scene->Component_Attach(entity, state.rootEntity);
				state.scene->names.Create(entity) = "blendShapeMaster";
			}
			ExpressionComponent& component = state.scene->expressions.Create(entity);

			const auto& blendShapeMaster = ext_vrm->second.Get("blendShapeMaster");
			if (blendShapeMaster.Has("blendShapeGroups"))
			{
				const auto& blendShapeGroups = blendShapeMaster.Get("blendShapeGroups");
				for (size_t blendShapeGroup_index = 0; blendShapeGroup_index < blendShapeGroups.ArrayLen(); ++blendShapeGroup_index)
				{
					const auto& blendShapeGroup = blendShapeGroups.Get(int(blendShapeGroup_index));
					ExpressionComponent::Expression& expression = component.expressions.emplace_back();

					if (blendShapeGroup.Has("name"))
					{
						const auto& value = blendShapeGroup.Get("name");
						expression.name = value.Get<std::string>();
					}
					if (blendShapeGroup.Has("presetName"))
					{
						const auto& value = blendShapeGroup.Get("presetName");
						std::string presetName = wi::helper::toUpper(value.Get<std::string>());

						if (!presetName.compare("JOY"))
						{
							expression.preset = ExpressionComponent::Preset::Happy;
						}
						else if (!presetName.compare("ANGRY"))
						{
							expression.preset = ExpressionComponent::Preset::Angry;
						}
						else if (!presetName.compare("SORROW"))
						{
							expression.preset = ExpressionComponent::Preset::Sad;
						}
						else if (!presetName.compare("FUN"))
						{
							expression.preset = ExpressionComponent::Preset::Relaxed;
						}
						else if (!presetName.compare("A"))
						{
							expression.preset = ExpressionComponent::Preset::Aa;
						}
						else if (!presetName.compare("I"))
						{
							expression.preset = ExpressionComponent::Preset::Ih;
						}
						else if (!presetName.compare("U"))
						{
							expression.preset = ExpressionComponent::Preset::Ou;
						}
						else if (!presetName.compare("E"))
						{
							expression.preset = ExpressionComponent::Preset::Ee;
						}
						else if (!presetName.compare("O"))
						{
							expression.preset = ExpressionComponent::Preset::Oh;
						}
						else if (!presetName.compare("BLINK"))
						{
							expression.preset = ExpressionComponent::Preset::Blink;
						}
						else if (!presetName.compare("BLINK_L"))
						{
							expression.preset = ExpressionComponent::Preset::BlinkLeft;
						}
						else if (!presetName.compare("BLINK_R"))
						{
							expression.preset = ExpressionComponent::Preset::BlinkRight;
						}
						else if (!presetName.compare("LOOKUP"))
						{
							expression.preset = ExpressionComponent::Preset::LookUp;
						}
						else if (!presetName.compare("LOOKDOWN"))
						{
							expression.preset = ExpressionComponent::Preset::LookDown;
						}
						else if (!presetName.compare("LOOKLEFT"))
						{
							expression.preset = ExpressionComponent::Preset::LookLeft;
						}
						else if (!presetName.compare("LOOKRIGHT"))
						{
							expression.preset = ExpressionComponent::Preset::LookRight;
						}
						else if (!presetName.compare("NEUTRAL"))
						{
							expression.preset = ExpressionComponent::Preset::Neutral;
						}
						else if (!presetName.compare("SURPRISED"))
						{
							expression.preset = ExpressionComponent::Preset::Surprised;
						}
						else if (!wi::helper::toUpper(expression.name).compare("SURPRISED")) // wtf vroid
						{
							expression.preset = ExpressionComponent::Preset::Surprised;
						}

						const size_t preset_index = (size_t)expression.preset;
						if (preset_index < arraysize(component.presets))
						{
							component.presets[preset_index] = (int)component.expressions.size() - 1;
						}
					}
					if (blendShapeGroup.Has("isBinary"))
					{
						const auto& value = blendShapeGroup.Get("isBinary");
						expression.SetBinary(value.Get<bool>());
					}
					if (blendShapeGroup.Has("binds"))
					{
						const auto& binds = blendShapeGroup.Get("binds");
						for (size_t bind_index = 0; bind_index < binds.ArrayLen(); ++bind_index)
						{
							const auto& bind = binds.Get(int(bind_index));
							ExpressionComponent::Expression::MorphTargetBinding& morph_target_binding = expression.morph_target_bindings.emplace_back();

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
									state.scene->layers.Create(colliderID);
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

		if (ext_vrm->second.Has("humanoid"))
		{
			// https://github.com/vrm-c/vrm-specification/tree/master/specification/0.0#vrm-extension-models-bone-mapping-jsonextensionsvrmhumanoid
			Entity entity = state.rootEntity;
			if (state.scene->humanoids.Contains(entity))
			{
				entity = CreateEntity();
				state.scene->Component_Attach(entity, state.rootEntity);
				state.scene->names.Create(entity) = "humanoid";
			}
			HumanoidComponent& component = state.scene->humanoids.Create(entity);

			const auto& humanoid = ext_vrm->second.Get("humanoid");
			if (humanoid.Has("humanBones"))
			{
				const auto& humanBones = humanoid.Get("humanBones");

				for (size_t bone_index = 0; bone_index < humanBones.ArrayLen(); ++bone_index)
				{
					const auto& humanBone = humanBones.Get(int(bone_index));
					Entity boneID = INVALID_ENTITY;

					// https://github.com/vrm-c/vrm-specification/tree/master/specification/0.0#defined-bones
					if (humanBone.Has("node"))
					{
						const auto& value = humanBone.Get("node");
						int node = value.GetNumberAsInt();
						boneID = state.entityMap[node];
					}
					if (humanBone.Has("bone"))
					{
						const auto& value = humanBone.Get("bone");
						const std::string& type = wi::helper::toUpper(value.Get<std::string>());

						if (!type.compare("NECK"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::Neck)] = boneID;
						}
						else if (!type.compare("HEAD"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::Head)] = boneID;
						}
						else if (!type.compare("LEFTEYE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftEye)] = boneID;
						}
						else if (!type.compare("RIGHTEYE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightEye)] = boneID;
						}
						else if (!type.compare("JAW"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::Jaw)] = boneID;
						}
						else if (!type.compare("HIPS"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::Hips)] = boneID;
						}
						else if (!type.compare("SPINE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::Spine)] = boneID;
						}
						else if (!type.compare("CHEST"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::Chest)] = boneID;
						}
						else if (!type.compare("UPPERCHEST"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::UpperChest)] = boneID;
						}
						else if (!type.compare("LEFTSHOULDER"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftShoulder)] = boneID;
						}
						else if (!type.compare("RIGHTSHOULDER"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightShoulder)] = boneID;
						}
						else if (!type.compare("LEFTUPPERARM"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperArm)] = boneID;
						}
						else if (!type.compare("RIGHTUPPERARM"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightUpperArm)] = boneID;
						}
						else if (!type.compare("LEFTLOWERARM"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerArm)] = boneID;
						}
						else if (!type.compare("RIGHTLOWERARM"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightLowerArm)] = boneID;
						}
						else if (!type.compare("LEFTHAND"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftHand)] = boneID;
						}
						else if (!type.compare("RIGHTHAND"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightHand)] = boneID;
						}
						else if (!type.compare("LEFTUPPERLEG"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperLeg)] = boneID;
						}
						else if (!type.compare("RIGHTUPPERLEG"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightUpperLeg)] = boneID;
						}
						else if (!type.compare("LEFTLOWERLEG"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerLeg)] = boneID;
						}
						else if (!type.compare("RIGHTLOWERLEG"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightLowerLeg)] = boneID;
						}
						else if (!type.compare("LEFTFOOT"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftFoot)] = boneID;
						}
						else if (!type.compare("RIGHTFOOT"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightFoot)] = boneID;
						}
						else if (!type.compare("LEFTTOES"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftToes)] = boneID;
						}
						else if (!type.compare("RIGHTTOES"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightToes)] = boneID;
						}
						else if (!type.compare("LEFTTHUMBPROXIMAL"))
						{
							// VRM 0.0 thumb proximal = VRM 1.0 thumb metacarpal
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbMetacarpal)] = boneID;
						}
						else if (!type.compare("RIGHTTHUMBPROXIMAL"))
						{
							// VRM 0.0 thumb proximal = VRM 1.0 thumb metacarpal
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightThumbMetacarpal)] = boneID;
						}
						else if (!type.compare("LEFTTHUMBINTERMEDIATE"))
						{
							// VRM 0.0 thumb intermediate = VRM 1.0 thumb proximal
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbProximal)] = boneID;
						}
						else if (!type.compare("RIGHTTHUMBINTERMEDIATE"))
						{
							// VRM 0.0 thumb intermediate = VRM 1.0 thumb proximal
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightThumbProximal)] = boneID;
						}
						else if (!type.compare("LEFTTHUMBDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbDistal)] = boneID;
						}
						else if (!type.compare("RIGHTTHUMBDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightThumbDistal)] = boneID;
						}
						else if (!type.compare("LEFTINDEXPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexProximal)] = boneID;
						}
						else if (!type.compare("RIGHTINDEXPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightIndexProximal)] = boneID;
						}
						else if (!type.compare("LEFTINDEXINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexIntermediate)] = boneID;
						}
						else if (!type.compare("RIGHTINDEXINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightIndexIntermediate)] = boneID;
						}
						else if (!type.compare("LEFTINDEXDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexDistal)] = boneID;
						}
						else if (!type.compare("RIGHTINDEXDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightIndexDistal)] = boneID;
						}
						else if (!type.compare("LEFTMIDDLEPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleProximal)] = boneID;
						}
						else if (!type.compare("RIGHTMIDDLEPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleProximal)] = boneID;
						}
						else if (!type.compare("LEFTMIDDLEINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleIntermediate)] = boneID;
						}
						else if (!type.compare("RIGHTMIDDLEINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleIntermediate)] = boneID;
						}
						else if (!type.compare("LEFTMIDDLEDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleDistal)] = boneID;
						}
						else if (!type.compare("RIGHTMIDDLEDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleDistal)] = boneID;
						}
						else if (!type.compare("LEFTRINGPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftRingProximal)] = boneID;
						}
						else if (!type.compare("RIGHTRINGPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightRingProximal)] = boneID;
						}
						else if (!type.compare("LEFTRINGINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftRingIntermediate)] = boneID;
						}
						else if (!type.compare("RIGHTRINGINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightRingIntermediate)] = boneID;
						}
						else if (!type.compare("LEFTRINGDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftRingDistal)] = boneID;
						}
						else if (!type.compare("RIGHTRINGDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightRingDistal)] = boneID;
						}
						else if (!type.compare("LEFTLITTLEPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleProximal)] = boneID;
						}
						else if (!type.compare("RIGHTLITTLEPROXIMAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightLittleProximal)] = boneID;
						}
						else if (!type.compare("LEFTLITTLEINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleIntermediate)] = boneID;
						}
						else if (!type.compare("RIGHTLITTLEINTERMEDIATE"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightLittleIntermediate)] = boneID;
						}
						else if (!type.compare("LEFTLITTLEDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleDistal)] = boneID;
						}
						else if (!type.compare("RIGHTLITTLEDISTAL"))
						{
							component.bones[size_t(HumanoidComponent::HumanoidBone::RightLittleDistal)] = boneID;
						}
					}
				}
			}
		}

		if (ext_vrm->second.Has("materialProperties"))
		{
			const auto& materialProperties = ext_vrm->second.Get("materialProperties");

			for (size_t material_index = 0; material_index < materialProperties.ArrayLen(); ++material_index)
			{
				const auto& material = materialProperties.Get(int(material_index));

				if (material.Has("name"))
				{
					const auto& value = material.Get("name");
					const std::string& materialName = value.Get<std::string>();
					Entity entity = state.scene->Entity_FindByName(materialName);
					MaterialComponent* component = state.scene->materials.GetComponent(entity);
					if (component != nullptr)
					{
						if (material.Has("shader"))
						{
							const auto& value = material.Get("shader");
							const std::string& shaderName = wi::helper::toUpper(value.Get<std::string>());
							if (!shaderName.compare("VRM/MTOON"))
							{
								VRM_ToonMaterialCustomize(materialName, *component);
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
			// https://github.com/vrm-c/vrm-specification/tree/master/specification/VRMC_vrm-1.0#vrmc_vrmexpressions-facial-expressions-optional
			Entity entity = state.rootEntity;
			if (state.scene->expressions.Contains(entity))
			{
				entity = CreateEntity();
				state.scene->Component_Attach(entity, state.rootEntity);
				state.scene->names.Create(entity) = "expressions";
			}
			ExpressionComponent& component = state.scene->expressions.Create(entity);

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
						const auto& vrm_expression = names.Get(name);
						ExpressionComponent::Expression& expression = component.expressions.emplace_back();

						if (!strcmp(expression_type, "preset"))
						{
							std::string presetName = wi::helper::toUpper(name);
							if (!presetName.compare("HAPPY"))
							{
								expression.preset = ExpressionComponent::Preset::Happy;
							}
							else if (!presetName.compare("ANGRY"))
							{
								expression.preset = ExpressionComponent::Preset::Angry;
							}
							else if (!presetName.compare("SAD"))
							{
								expression.preset = ExpressionComponent::Preset::Sad;
							}
							else if (!presetName.compare("RELAXED"))
							{
								expression.preset = ExpressionComponent::Preset::Relaxed;
							}
							else if (!presetName.compare("SURPRISED"))
							{
								expression.preset = ExpressionComponent::Preset::Surprised;
							}
							else if (!presetName.compare("AA"))
							{
								expression.preset = ExpressionComponent::Preset::Aa;
							}
							else if (!presetName.compare("IH"))
							{
								expression.preset = ExpressionComponent::Preset::Ih;
							}
							else if (!presetName.compare("OU"))
							{
								expression.preset = ExpressionComponent::Preset::Ou;
							}
							else if (!presetName.compare("EE"))
							{
								expression.preset = ExpressionComponent::Preset::Ee;
							}
							else if (!presetName.compare("OH"))
							{
								expression.preset = ExpressionComponent::Preset::Oh;
							}
							else if (!presetName.compare("BLINK"))
							{
								expression.preset = ExpressionComponent::Preset::Blink;
							}
							else if (!presetName.compare("BLINKLEFT"))
							{
								expression.preset = ExpressionComponent::Preset::BlinkLeft;
							}
							else if (!presetName.compare("BLINKRIGHT"))
							{
								expression.preset = ExpressionComponent::Preset::BlinkRight;
							}
							else if (!presetName.compare("LOOKUP"))
							{
								expression.preset = ExpressionComponent::Preset::LookUp;
							}
							else if (!presetName.compare("LOOKDOWN"))
							{
								expression.preset = ExpressionComponent::Preset::LookDown;
							}
							else if (!presetName.compare("LOOKLEFT"))
							{
								expression.preset = ExpressionComponent::Preset::LookLeft;
							}
							else if (!presetName.compare("LOOKRIGHT"))
							{
								expression.preset = ExpressionComponent::Preset::LookRight;
							}
							else if (!presetName.compare("NEUTRAL"))
							{
								expression.preset = ExpressionComponent::Preset::Neutral;
							}

							const size_t preset_index = (size_t)expression.preset;
							if (preset_index < arraysize(component.presets))
							{
								component.presets[preset_index] = (int)component.expressions.size() - 1;
							}
						}
						expression.name = name;

						if (vrm_expression.Has("isBinary"))
						{
							const auto& value = vrm_expression.Get("isBinary");
							expression.SetBinary(value.Get<bool>());
						}
						if (vrm_expression.Has("overrideMouth"))
						{
							const auto& value = vrm_expression.Get("overrideMouth");
							const std::string& override_enum = value.Get<std::string>();
							if (!override_enum.compare("block"))
							{
								expression.override_mouth = ExpressionComponent::Override::Block;
							}
							if (!override_enum.compare("blend"))
							{
								expression.override_mouth = ExpressionComponent::Override::Blend;
							}
						}
						if (vrm_expression.Has("morphTargetBinds"))
						{
							const auto& morpTargetBinds = vrm_expression.Get("morphTargetBinds");
							for (size_t morphTargetBind_index = 0; morphTargetBind_index < morpTargetBinds.ArrayLen(); ++morphTargetBind_index)
							{
								const auto& morphTargetBind = morpTargetBinds.Get(int(morphTargetBind_index));
								ExpressionComponent::Expression::MorphTargetBinding& morph_target_binding = expression.morph_target_bindings.emplace_back();

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
						//if (vrm_expression.Has("materialColorBinds"))
						//{
						//	const auto& materialColorBinds = vrm_expression.Get("materialColorBinds");
						//	// TODO: find example model and implement
						//}
						//if (vrm_expression.Has("textureTransformBinds "))
						//{
						//	const auto& textureTransformBinds = vrm_expression.Get("textureTransformBinds");
						//	// TODO: find example model and implement
						//}

					}
				}
			}
		}

		if (ext_vrm->second.Has("humanoid"))
		{
			// https://github.com/vrm-c/vrm-specification/blob/master/specification/VRMC_vrm-1.0/humanoid.md
			Entity entity = state.rootEntity;
			if (state.scene->humanoids.Contains(entity))
			{
				entity = CreateEntity();
				state.scene->Component_Attach(entity, state.rootEntity);
				state.scene->names.Create(entity) = "humanoid";
			}
			HumanoidComponent& component = state.scene->humanoids.Create(entity);

			const auto& humanoid = ext_vrm->second.Get("humanoid");
			if (humanoid.Has("humanBones"))
			{
				const auto& humanBones = humanoid.Get("humanBones");

				const auto& keys = humanBones.Keys();
				for (const auto& key : keys)
				{
					Entity boneID = INVALID_ENTITY;

					const auto& value = humanBones.Get(key);
					if (value.Has("node"))
					{
						const auto& node_value = value.Get("node");
						int node = node_value.GetNumberAsInt();
						boneID = state.entityMap[node];
					}

					const std::string& type = wi::helper::toUpper(key);
					if (!type.compare("NECK"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::Neck)] = boneID;
					}
					else if (!type.compare("HEAD"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::Head)] = boneID;
					}
					else if (!type.compare("LEFTEYE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftEye)] = boneID;
					}
					else if (!type.compare("RIGHTEYE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightEye)] = boneID;
					}
					else if (!type.compare("JAW"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::Jaw)] = boneID;
					}
					else if (!type.compare("HIPS"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::Hips)] = boneID;
					}
					else if (!type.compare("SPINE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::Spine)] = boneID;
					}
					else if (!type.compare("CHEST"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::Chest)] = boneID;
					}
					else if (!type.compare("UPPERCHEST"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::UpperChest)] = boneID;
					}
					else if (!type.compare("LEFTSHOULDER"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftShoulder)] = boneID;
					}
					else if (!type.compare("RIGHTSHOULDER"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightShoulder)] = boneID;
					}
					else if (!type.compare("LEFTUPPERARM"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperArm)] = boneID;
					}
					else if (!type.compare("RIGHTUPPERARM"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightUpperArm)] = boneID;
					}
					else if (!type.compare("LEFTLOWERARM"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerArm)] = boneID;
					}
					else if (!type.compare("RIGHTLOWERARM"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightLowerArm)] = boneID;
					}
					else if (!type.compare("LEFTHAND"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftHand)] = boneID;
					}
					else if (!type.compare("RIGHTHAND"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightHand)] = boneID;
					}
					else if (!type.compare("LEFTUPPERLEG"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperLeg)] = boneID;
					}
					else if (!type.compare("RIGHTUPPERLEG"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightUpperLeg)] = boneID;
					}
					else if (!type.compare("LEFTLOWERLEG"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerLeg)] = boneID;
					}
					else if (!type.compare("RIGHTLOWERLEG"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightLowerLeg)] = boneID;
					}
					else if (!type.compare("LEFTFOOT"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftFoot)] = boneID;
					}
					else if (!type.compare("RIGHTFOOT"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightFoot)] = boneID;
					}
					else if (!type.compare("LEFTTOES"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftToes)] = boneID;
					}
					else if (!type.compare("RIGHTTOES"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightToes)] = boneID;
					}
					else if (!type.compare("LEFTTHUMBPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbProximal)] = boneID;
					}
					else if (!type.compare("RIGHTTHUMBPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightThumbProximal)] = boneID;
					}
					else if (!type.compare("LEFTTHUMBINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbMetacarpal)] = boneID;
					}
					else if (!type.compare("RIGHTTHUMBINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightThumbMetacarpal)] = boneID;
					}
					else if (!type.compare("LEFTTHUMBDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbDistal)] = boneID;
					}
					else if (!type.compare("RIGHTTHUMBDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightThumbDistal)] = boneID;
					}
					else if (!type.compare("LEFTINDEXPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexProximal)] = boneID;
					}
					else if (!type.compare("RIGHTINDEXPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightIndexProximal)] = boneID;
					}
					else if (!type.compare("LEFTINDEXINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexIntermediate)] = boneID;
					}
					else if (!type.compare("RIGHTINDEXINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightIndexIntermediate)] = boneID;
					}
					else if (!type.compare("LEFTINDEXDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexDistal)] = boneID;
					}
					else if (!type.compare("RIGHTINDEXDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightIndexDistal)] = boneID;
					}
					else if (!type.compare("LEFTMIDDLEPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleProximal)] = boneID;
					}
					else if (!type.compare("RIGHTMIDDLEPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleProximal)] = boneID;
					}
					else if (!type.compare("LEFTMIDDLEINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleIntermediate)] = boneID;
					}
					else if (!type.compare("RIGHTMIDDLEINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleIntermediate)] = boneID;
					}
					else if (!type.compare("LEFTMIDDLEDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleDistal)] = boneID;
					}
					else if (!type.compare("RIGHTMIDDLEDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleDistal)] = boneID;
					}
					else if (!type.compare("LEFTRINGPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftRingProximal)] = boneID;
					}
					else if (!type.compare("RIGHTRINGPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightRingProximal)] = boneID;
					}
					else if (!type.compare("LEFTRINGINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftRingIntermediate)] = boneID;
					}
					else if (!type.compare("RIGHTRINGINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightRingIntermediate)] = boneID;
					}
					else if (!type.compare("LEFTRINGDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftRingDistal)] = boneID;
					}
					else if (!type.compare("RIGHTRINGDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightRingDistal)] = boneID;
					}
					else if (!type.compare("LEFTLITTLEPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleProximal)] = boneID;
					}
					else if (!type.compare("RIGHTLITTLEPROXIMAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightLittleProximal)] = boneID;
					}
					else if (!type.compare("LEFTLITTLEINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleIntermediate)] = boneID;
					}
					else if (!type.compare("RIGHTLITTLEINTERMEDIATE"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightLittleIntermediate)] = boneID;
					}
					else if (!type.compare("LEFTLITTLEDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleDistal)] = boneID;
					}
					else if (!type.compare("RIGHTLITTLEDISTAL"))
					{
						component.bones[size_t(HumanoidComponent::HumanoidBone::RightLittleDistal)] = boneID;
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
					state.scene->layers.Create(colliderID);
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

	auto ext_vrmc_springbone_extended_collider = state.gltfModel.extensions.find("VRMC_springBone_extended_collider");
	if (ext_vrmc_springbone_extended_collider != state.gltfModel.extensions.end())
	{
		wi::backlog::post("VRMC_springBone_extended_collider extension found in model, but it is not implemented yet in the importer.", wi::backlog::LogLevel::Warning);
	}
}

void VRM_ToonMaterialCustomize(const std::string& name, MaterialComponent& material)
{
	material.shaderType = MaterialComponent::SHADERTYPE_CARTOON;

	// These customizations are just made for Wicked Engine, but not standardized:
	if (name.find("SKIN") != std::string::npos)
	{
		// For skin, apply some subsurface scattering to look softer:
		material.SetSubsurfaceScatteringAmount(0.5f);
		material.SetSubsurfaceScatteringColor(XMFLOAT3(255.0f / 255.0f, 148.0f / 255.0f, 142.0f / 255.0f));
	}
	if (name.find("EYE") != std::string::npos ||
		name.find("FaceEyeline") != std::string::npos ||
		name.find("FaceEyelash") != std::string::npos ||
		name.find("FaceBrow") != std::string::npos
		)
	{
		// Disable shadow casting for some elements on the face like eyes, eyebrows, etc:
		material.SetCastShadow(false);
	}
}

void Import_Mixamo_Bone(LoaderState& state, Entity boneEntity, const tinygltf::Node& node)
{
	auto get_humanoid = [&]() -> HumanoidComponent& {
		HumanoidComponent* component = state.scene->humanoids.GetComponent(state.rootEntity);
		if (component == nullptr)
		{
			component = &state.scene->humanoids.Create(state.rootEntity);
		}
		return *component;
	};

	if (!node.name.compare("mixamorig:Hips"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Hips)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:Spine"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Spine)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:Spine1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Chest)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:Spine2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::UpperChest)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:Neck"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Neck)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:Head"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Head)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftShoulder"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftShoulder)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightShoulder"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightShoulder)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperArm)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightUpperArm)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftForeArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerArm)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightForeArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLowerArm)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHand"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftHand)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHand"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightHand)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandThumb1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbMetacarpal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandThumb1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightThumbMetacarpal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandThumb2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandThumb2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightThumbProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandThumb3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandThumb3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightThumbDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandIndex1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandIndex1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightIndexProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandIndex2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandIndex2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightIndexIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandIndex3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandIndex3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightIndexDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandMiddle1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandMiddle1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandMiddle2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandMiddle2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandMiddle3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandMiddle3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandRing1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftRingProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandRing1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightRingProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandRing2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftRingIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandRing2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightRingIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandRing3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftRingDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandRing3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightRingDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandPinky1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandPinky1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLittleProximal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandPinky2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandPinky2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLittleIntermediate)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftHandPinky3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightHandPinky3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLittleDistal)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftUpLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperLeg)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightUpLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightUpperLeg)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerLeg)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLowerLeg)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftFoot"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftFoot)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightFoot"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightFoot)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:LeftToeBase"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftToes)] = boneEntity;
	}
	else if (!node.name.compare("mixamorig:RightToeBase"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightToes)] = boneEntity;
	}
}

template<typename T>
inline void _ExportHelper_valuetobuf(const T& input, tinygltf::Buffer& buffer_builder, size_t& buf_i)
{
	const size_t _right = buf_i + sizeof(input);
	if (_right > buffer_builder.data.size())
	{
		buffer_builder.data.resize(_right);
	}
	*(T*)(buffer_builder.data.data() + buf_i) = input;
	buf_i = _right;
}

inline tinygltf::Value _ExportHelper_tovalue(float input)
{
	return tinygltf::Value(input);
}

inline tinygltf::Value _ExportHelper_tovalue(XMFLOAT3 input)
{
	auto value_builder = tinygltf::Value(tinygltf::Value::Array({
		tinygltf::Value(input.x), tinygltf::Value(input.y), tinygltf::Value(input.z)
	}));
	return value_builder;
}

inline tinygltf::Value _ExportHelper_tovalue(XMFLOAT4 input)
{
	auto value_builder = tinygltf::Value(tinygltf::Value::Array({
		tinygltf::Value(input.x), tinygltf::Value(input.y), tinygltf::Value(input.z), tinygltf::Value(input.w)
	}));
	return value_builder;
}

wi::vector<std::string> original_texture_extension_iterator = 
{
	"png",
	"jpg",
	"jpeg",
};

inline std::string _ExportHelper_GetOriginalTexture(std::string texture_file)
{
	for(auto& ext : original_texture_extension_iterator)
	{
		std::string target_file = wi::helper::ReplaceExtension(texture_file, ext);
		wi::backlog::post(target_file);
		if (wi::helper::FileExists(target_file))
		{
			return target_file;
		}
	}

	return texture_file;
}

inline tinygltf::TextureInfo _ExportHelper_StoreMaterialTexture(LoaderState& state, const std::string& gltf_dir, const MaterialComponent& material, MaterialComponent::TEXTURESLOT slot)
{
	const MaterialComponent::TextureMap& textureSlot = material.textures[slot];

	std::string texture_file = textureSlot.name;

	tinygltf::TextureInfo textureinfo_builder;
	int texture_index = -1;
	auto find_texture_id = state.textureMap.find(texture_file);

	if(find_texture_id == state.textureMap.end())
	{
		std::string mime_type;
		std::string src_extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(texture_file));
		if (src_extension == "PNG")
		{
			mime_type = "image/png";
		}
		else if (src_extension == "JPG" || src_extension == "JPEG")
		{
			mime_type = "image/jpeg";
		}
		else if (src_extension == "BMP")
		{
			mime_type = "image/bmp";
		}
		else if (src_extension == "GIF")
		{
			mime_type = "image/gif";
		}

		int image_bufferView_index = 0;

		tinygltf::Buffer buffer_builder;
		int buffer_index = (int)state.gltfModel.buffers.size();
		wi::vector<uint8_t> texturedata;
		size_t buffer_size = 0;
		if (!textureSlot.resource.GetFileData().empty() && !mime_type.empty())
		{
			// If texture file data is available and gltf compatible, simply save it to the gltf as-is:
			buffer_builder.data = textureSlot.resource.GetFileData();
			buffer_size = buffer_builder.data.size();
		}
		else
		{
			// If the texture file data is not available or it is block compressed, download it (and optionally decompress) from GPU:
			Texture tex = textureSlot.resource.GetTexture();
			if (IsFormatBlockCompressed(tex.desc.format))
			{
				// Decompress block compressed texture on GPU:
				const XMFLOAT4 texMulAdd = textureSlot.uvset == 0 ? material.texMulAdd : XMFLOAT4(1, 1, 0, 0);
				TextureDesc desc;
				desc.format = Format::R8G8B8A8_UNORM;
				desc.bind_flags = BindFlag::UNORDERED_ACCESS;
				desc.width = uint32_t(tex.desc.width * texMulAdd.x);
				desc.height = uint32_t(tex.desc.height * texMulAdd.y);
				desc.mip_levels = 1;
				Texture tex_decompressed;
				GraphicsDevice* device = GetDevice();
				device->CreateTexture(&desc, nullptr, &tex_decompressed);
				CommandList cmd = device->BeginCommandList();
				device->CreateSubresource(&tex_decompressed, SubresourceType::UAV, 0, 1, 0, 1);
				wi::renderer::CopyTexture2D(
					tex_decompressed, 0, 0, 0,
					tex, 0, int(texMulAdd.z * tex.desc.width), int(texMulAdd.w * tex.desc.height),
					cmd,
					wi::renderer::BORDEREXPAND_DISABLE,
					IsFormatSRGB(tex.desc.format)
				); // copy that supports format conversion / decompression
				tex = tex_decompressed;
			}
			if (wi::helper::saveTextureToMemory(tex, texturedata))
			{
				wi::vector<uint8_t> filedata;
				if (wi::helper::saveTextureToMemoryFile(texturedata, tex.desc, "PNG", filedata))
				{
					buffer_size = filedata.size();
					buffer_builder.data = std::move(filedata);
					mime_type = "image/png";
				}
			}
		}
		state.gltfModel.buffers.push_back(buffer_builder);
			
		tinygltf::BufferView bufferView_builder;
		image_bufferView_index = (int)state.gltfModel.bufferViews.size();
		bufferView_builder.buffer = buffer_index;
		bufferView_builder.byteLength = buffer_size;
		state.gltfModel.bufferViews.push_back(bufferView_builder);

		tinygltf::Image image_builder;
		//image_builder.uri = texture_file;
		image_builder.bufferView = image_bufferView_index;
		image_builder.mimeType = mime_type;

		int image_index = (int)state.gltfModel.images.size();
		wi::helper::MakePathRelative(gltf_dir, texture_file);
		state.gltfModel.images.push_back(image_builder);

		tinygltf::Texture texture_builder;
		texture_index = (int)state.gltfModel.textures.size();
		texture_builder.source = image_index;
		state.gltfModel.textures.push_back(texture_builder);

		state.textureMap[texture_file] = texture_index;
	}
	else
	{
		texture_index = find_texture_id->second;
	}

	textureinfo_builder.index = texture_index;
	textureinfo_builder.texCoord = (int)textureSlot.uvset;

	return textureinfo_builder;
}

void ExportModel_GLTF(const std::string& filename, Scene& scene)
{
	tinygltf::TinyGLTF writer;

	tinygltf::FsCallbacks callbacks;
	callbacks.ReadWholeFile = tinygltf::ReadWholeFile;
	callbacks.WriteWholeFile = tinygltf::WriteWholeFile;
	callbacks.FileExists = tinygltf::FileExists;
	callbacks.ExpandFilePath = tinygltf::ExpandFilePath;
	writer.SetFsCallbacks(callbacks);

	LoaderState state;
	state.scene = &scene;
	auto& wiscene = *state.scene;

	// Prerequisite: flip world Z coordinate
	FlipZAxis(state);
	wiscene.Update(0.f);

	// Add extension prerequisite
	state.gltfModel.extensionsUsed = {
		"KHR_materials_ior",
		"KHR_materials_specular",
	};

	if (scene.lights.GetCount() > 0)
	{
		state.gltfModel.extensionsUsed.push_back("KHR_lights_punctual");
	}
	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		const MaterialComponent& material = scene.materials[i];
		if (material.transmission > 0 || material.textures[wi::scene::MaterialComponent::TRANSMISSIONMAP].resource.IsValid())
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_transmission");
		}
		if (material.IsUsingSpecularGlossinessWorkflow())
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_pbrSpecularGlossiness");
		}
		if (material.GetEmissiveStrength() != 1.0f)
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_emissive_strength");
		}

		if (material.shaderType == MaterialComponent::SHADERTYPE::SHADERTYPE_PBR_CLOTH)
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_sheen");
		}
		else if (material.shaderType == MaterialComponent::SHADERTYPE::SHADERTYPE_PBR_CLEARCOAT)
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_clearcoat");
		}
		else if (material.shaderType == MaterialComponent::SHADERTYPE::SHADERTYPE_PBR_CLOTH_CLEARCOAT)
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_sheen");
			state.gltfModel.extensionsUsed.push_back("KHR_materials_clearcoat");
		}
		else if (material.shaderType == MaterialComponent::SHADERTYPE::SHADERTYPE_UNLIT)
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_unlit");
		}
		else if (material.shaderType == MaterialComponent::SHADERTYPE::SHADERTYPE_PBR_ANISOTROPIC)
		{
			state.gltfModel.extensionsUsed.push_back("KHR_materials_anisotropy");
		}
	}

	if (wiscene.materials.GetCount() == 0)
	{
		state.gltfModel.materials.emplace_back().name = "dummyMaterial";
	}

	// Terrain chunks need some work to remap virtual texture atlas to individual textures for GLTF:
	for (size_t i = 0; i < scene.terrains.GetCount(); ++i)
	{
		using namespace wi::terrain;
		Terrain& terrain = scene.terrains[i];
		for (auto& it : terrain.chunks)
		{
			const Chunk& chunk = it.first;
			ChunkData& chunk_data = it.second;

			MaterialComponent* material = scene.materials.GetComponent(chunk_data.entity);
			if (material == nullptr)
				continue;

			VirtualTexture& vt = *chunk_data.vt;
			for (uint32_t map_type = 0; map_type < arraysize(terrain.atlas.maps); ++map_type)
			{
				if (material->textures[map_type].name.empty())
				{
					const NameComponent* chunk_name = scene.names.GetComponent(chunk_data.entity);
					if (chunk_name != nullptr)
					{
						switch (map_type)
						{
						default:
						case MaterialComponent::BASECOLORMAP:
							material->textures[map_type].name = chunk_name->name + "_basecolormap.png";
							break;
						case MaterialComponent::NORMALMAP:
							material->textures[map_type].name = chunk_name->name + "_normalmap.png";
							break;
						case MaterialComponent::SURFACEMAP:
							material->textures[map_type].name = chunk_name->name + "_surfacemap.png";
							break;
						}
					}
				}

				if (map_type == 0)
				{
					auto tile = vt.residency ? vt.tiles[vt.tiles.size() - 2] : vt.tiles.back(); // last nonpacked mip
					const float2 resolution_rcp = float2(
						1.0f / (float)terrain.atlas.maps[map_type].texture.desc.width,
						1.0f / (float)terrain.atlas.maps[map_type].texture.desc.height
					);
					material->texMulAdd.x = (float)SVT_TILE_SIZE * resolution_rcp.x;
					material->texMulAdd.y = (float)SVT_TILE_SIZE * resolution_rcp.y;
					material->texMulAdd.z = ((float)tile.x * (float)SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER) * resolution_rcp.x;
					material->texMulAdd.w = ((float)tile.y * (float)SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER) * resolution_rcp.y;
				}
			}
		}
	}

	// Write Materials
	for(size_t mt_id = 0; mt_id < wiscene.materials.GetCount(); ++mt_id)
	{
		auto& material = wiscene.materials[mt_id];
		auto materialEntity = wiscene.materials.GetEntity(mt_id);
		auto nameComponent = wiscene.names.GetComponent(materialEntity);

		tinygltf::Material material_builder;

		if(nameComponent != nullptr)
		{
			material_builder.name = nameComponent->name;
		}

		// Dielectric-Metallic Workflow (Base PBR)
		// Textures
		if(material.textures[wi::scene::MaterialComponent::BASECOLORMAP].resource.IsValid())
		{
			material_builder.pbrMetallicRoughness.baseColorTexture = _ExportHelper_StoreMaterialTexture(
				state, 
				wi::helper::GetDirectoryFromPath(filename), 
				material,
				wi::scene::MaterialComponent::BASECOLORMAP
			);
		}
		if(material.textures[wi::scene::MaterialComponent::NORMALMAP].resource.IsValid())
		{
			auto normalTexInfo_pre = _ExportHelper_StoreMaterialTexture(
				state, 
				wi::helper::GetDirectoryFromPath(filename), 
				material,
				wi::scene::MaterialComponent::NORMALMAP
			);
			material_builder.normalTexture.index = normalTexInfo_pre.index;
			material_builder.normalTexture.texCoord = normalTexInfo_pre.texCoord;
		}
		if(material.textures[wi::scene::MaterialComponent::OCCLUSIONMAP].resource.IsValid())
		{
			auto occlTexInfo_pre = _ExportHelper_StoreMaterialTexture(
				state, 
				wi::helper::GetDirectoryFromPath(filename), 
				material,
				wi::scene::MaterialComponent::OCCLUSIONMAP
			);
			material_builder.occlusionTexture.index = occlTexInfo_pre.index;
			material_builder.occlusionTexture.texCoord = occlTexInfo_pre.texCoord;
		}
		if(material.textures[wi::scene::MaterialComponent::EMISSIVEMAP].resource.IsValid())
		{
			material_builder.emissiveTexture = _ExportHelper_StoreMaterialTexture(
				state, 
				wi::helper::GetDirectoryFromPath(filename), 
				material,
				wi::scene::MaterialComponent::EMISSIVEMAP
			);
		}
		if(material.textures[wi::scene::MaterialComponent::SURFACEMAP].resource.IsValid())
		{
			material_builder.pbrMetallicRoughness.metallicRoughnessTexture = _ExportHelper_StoreMaterialTexture(
				state, 
				wi::helper::GetDirectoryFromPath(filename), 
				material,
				wi::scene::MaterialComponent::SURFACEMAP
			);				
		}
		// Values
		material_builder.pbrMetallicRoughness.baseColorFactor = {
			material.baseColor.x,
			material.baseColor.y,
			material.baseColor.z,
			material.baseColor.w
		};
		material_builder.pbrMetallicRoughness.roughnessFactor = { material.roughness };
		material_builder.pbrMetallicRoughness.metallicFactor = { material.metalness };
		material_builder.emissiveFactor = { 
			material.emissiveColor.x,
			material.emissiveColor.y,
			material.emissiveColor.z,
		};
		if (material.alphaRef < 1.f)
		{
			material_builder.alphaMode = "MASK";
			material_builder.alphaCutoff = 1.f - material.alphaRef;
		}
		switch(material.userBlendMode)
		{
			case wi::enums::BLENDMODE_ALPHA:
				material_builder.alphaMode = "BLEND";
				break;
			default:
				break;
		}
		material_builder.doubleSided = material.IsDoubleSided();

		// Unlit extension (KHR_materials_unlit)
		// Values
		if (material.shaderType == wi::scene::MaterialComponent::SHADERTYPE_UNLIT)
		{
			material_builder.extensions["KHR_materials_unlit"] = tinygltf::Value();
		}

		if (material.GetEmissiveStrength() != 1.0f)
		{
			tinygltf::Value::Object KHR_materials_emissive_strength_builder = {
				{"emissiveStrength", tinygltf::Value(double(material.GetEmissiveStrength()))}
			};
			material_builder.extensions["KHR_materials_emissive_strength"] = tinygltf::Value(KHR_materials_emissive_strength_builder);
		}

		// Transmission extension (KHR_materials_transmission)
		// Values
		if (material.transmission > 0 || material.textures[wi::scene::MaterialComponent::TRANSMISSIONMAP].resource.IsValid())
		{
			tinygltf::Value::Object KHR_materials_transmission_builder = {
				{"transmissionFactor", tinygltf::Value(double(material.transmission))}
			};
			// Textures
			if (material.textures[wi::scene::MaterialComponent::TRANSMISSIONMAP].resource.IsValid())
			{
				auto transmissionTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state,
					wi::helper::GetDirectoryFromPath(filename),
					material,
					wi::scene::MaterialComponent::TRANSMISSIONMAP
				);
				KHR_materials_transmission_builder["transmissionTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(transmissionTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(transmissionTexInfo_pre.texCoord)}
					});
			}
			material_builder.extensions["KHR_materials_transmission"] = tinygltf::Value(KHR_materials_transmission_builder);
		}

		// Specular-glosiness extension (KHR_materials_pbrSpecularGlossiness)
		if(material.IsUsingSpecularGlossinessWorkflow())
		{
			// Values
			tinygltf::Value::Object KHR_materials_pbrSpecularGlossiness_builder = {
				{"diffuseFactor", tinygltf::Value({
					tinygltf::Value(double(material.baseColor.x)),
					tinygltf::Value(double(material.baseColor.y)),
					tinygltf::Value(double(material.baseColor.z)),
					tinygltf::Value(double(material.baseColor.w))
				})},
				{"specularFactor", tinygltf::Value({
					tinygltf::Value(double(material.specularColor.x)),
					tinygltf::Value(double(material.specularColor.y)),
					tinygltf::Value(double(material.specularColor.z))
				})},
				{"glossinessFactor", tinygltf::Value(double(material.roughness))}
			};
			// Textures
			if(material.textures[MaterialComponent::BASECOLORMAP].resource.IsValid())
			{
				auto diffuseTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state, 
					wi::helper::GetDirectoryFromPath(filename), 
					material,
					wi::scene::MaterialComponent::BASECOLORMAP
				);
				KHR_materials_pbrSpecularGlossiness_builder["diffuseTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(diffuseTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(diffuseTexInfo_pre.texCoord)}
					});
			}
			if(material.textures[MaterialComponent::SURFACEMAP].resource.IsValid())
			{
				auto specglossTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state, 
					wi::helper::GetDirectoryFromPath(filename), 
					material,
					wi::scene::MaterialComponent::SURFACEMAP
				);
				KHR_materials_pbrSpecularGlossiness_builder["specularGlossinessTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(specglossTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(specglossTexInfo_pre.texCoord)}
					});
			}
		}

		// Sheen extension (KHR_materials_sheen)
		if(material.shaderType == wi::scene::MaterialComponent::SHADERTYPE_PBR_CLOTH || material.shaderType == wi::scene::MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT)
		{
			// Values
			tinygltf::Value::Object KHR_materials_sheen_builder = {
				{"sheenColorFactor", tinygltf::Value({
					tinygltf::Value(double(material.sheenColor.x)),
					tinygltf::Value(double(material.sheenColor.y)),
					tinygltf::Value(double(material.sheenColor.z))
				})},
				{"sheenRoughnessFactor", tinygltf::Value(double(material.sheenRoughness))}
			};
			// Textures
			if(material.textures[wi::scene::MaterialComponent::SHEENCOLORMAP].resource.IsValid())
			{
				auto sheencolorTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state, 
					wi::helper::GetDirectoryFromPath(filename), 
					material,
					wi::scene::MaterialComponent::SHEENCOLORMAP
				);
				KHR_materials_sheen_builder["sheenColorTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(sheencolorTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(sheencolorTexInfo_pre.texCoord)}
					});
			}
			if(material.textures[wi::scene::MaterialComponent::SHEENROUGHNESSMAP].resource.IsValid())
			{
				auto sheenRoughTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state, 
					wi::helper::GetDirectoryFromPath(filename), 
					material,
					wi::scene::MaterialComponent::SHEENROUGHNESSMAP
				);
				KHR_materials_sheen_builder["sheenRoughnessTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(sheenRoughTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(sheenRoughTexInfo_pre.texCoord)}
					});
			}
			material_builder.extensions["KHR_materials_sheen"] = tinygltf::Value(KHR_materials_sheen_builder);
		}

		// Clearcoat extension (KHR_materials_clearcoat)
		// Values
		if (material.shaderType == MaterialComponent::SHADERTYPE_PBR_CLEARCOAT || material.shaderType == MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT)
		{
			tinygltf::Value::Object KHR_materials_clearcoat_builder = {
				{"clearcoatFactor", tinygltf::Value(double(material.clearcoat))},
				{"clearcoatRoughnessFactor", tinygltf::Value(double(material.clearcoatRoughness))}
			};
			// Textures
			if (material.textures[wi::scene::MaterialComponent::CLEARCOATMAP].resource.IsValid())
			{
				auto clearcoatTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state,
					wi::helper::GetDirectoryFromPath(filename),
					material,
					wi::scene::MaterialComponent::CLEARCOATMAP
				);
				KHR_materials_clearcoat_builder["clearcoatTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(clearcoatTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(clearcoatTexInfo_pre.texCoord)}
					});
			}
			if (material.textures[wi::scene::MaterialComponent::CLEARCOATNORMALMAP].resource.IsValid())
			{
				auto clearcoatNormTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state,
					wi::helper::GetDirectoryFromPath(filename),
					material,
					wi::scene::MaterialComponent::CLEARCOATNORMALMAP
				);
				KHR_materials_clearcoat_builder["clearcoatNormalTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(clearcoatNormTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(clearcoatNormTexInfo_pre.texCoord)}
					});
			}
			if (material.textures[wi::scene::MaterialComponent::CLEARCOATROUGHNESSMAP].resource.IsValid())
			{
				auto clearcoatRoughTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state,
					wi::helper::GetDirectoryFromPath(filename),
					material,
					wi::scene::MaterialComponent::CLEARCOATROUGHNESSMAP
				);
				KHR_materials_clearcoat_builder["clearcoatRoughnessTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(clearcoatRoughTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(clearcoatRoughTexInfo_pre.texCoord)}
					});
			}
			material_builder.extensions["KHR_materials_clearcoat"] = tinygltf::Value(KHR_materials_clearcoat_builder);
		}

		// IOR Extension (KHR_materials_ior)
		float ior_retrieve_phase1 = std::sqrt(material.reflectance);
		float ior_retrieve_phase2 = -(1+ior_retrieve_phase1)/(ior_retrieve_phase1-1);
		tinygltf::Value::Object KHR_materials_ior_builder = {
			{"ior",tinygltf::Value(double(ior_retrieve_phase2))}
		};
		material_builder.extensions["KHR_materials_ior"] = tinygltf::Value(KHR_materials_ior_builder);

		// Specular Extension (KHR_materials_specular)
		tinygltf::Value::Object KHR_materials_specular_builder = {
			{"specularFactor", tinygltf::Value(material.specularColor.w)},
			{"specularColorFactor",tinygltf::Value({
				tinygltf::Value(double(material.specularColor.x)),
				tinygltf::Value(double(material.specularColor.y)),
				tinygltf::Value(double(material.specularColor.z))
			})}
		};
		if(material.textures[wi::scene::MaterialComponent::SPECULARMAP].resource.IsValid())
		{
			auto specularTexInfo_pre = _ExportHelper_StoreMaterialTexture(
				state, 
				wi::helper::GetDirectoryFromPath(filename), 
				material,
				wi::scene::MaterialComponent::SPECULARMAP
			);
			KHR_materials_specular_builder["specularTexture"] = tinygltf::Value({
					{"index",tinygltf::Value(specularTexInfo_pre.index)},
					{"texCoord",tinygltf::Value(specularTexInfo_pre.texCoord)}
				});
			KHR_materials_specular_builder["specularColorTexture"] = tinygltf::Value({
					{"index",tinygltf::Value(specularTexInfo_pre.index)},
					{"texCoord",tinygltf::Value(specularTexInfo_pre.texCoord)}
				});
		}
		material_builder.extensions["KHR_materials_specular"] = tinygltf::Value(KHR_materials_specular_builder);

		if (material.shaderType == MaterialComponent::SHADERTYPE_PBR_ANISOTROPIC)
		{
			// Anisotropy Extension (KHR_materials_anisotropy)
			tinygltf::Value::Object KHR_materials_anisotropy_builder = {
				{"anisotropyStrength", tinygltf::Value(material.anisotropy_strength)},
				{"anisotropyRotation", tinygltf::Value(material.anisotropy_rotation)}
			};
			if (material.textures[wi::scene::MaterialComponent::ANISOTROPYMAP].resource.IsValid())
			{
				auto specularTexInfo_pre = _ExportHelper_StoreMaterialTexture(
					state,
					wi::helper::GetDirectoryFromPath(filename),
					material,
					wi::scene::MaterialComponent::ANISOTROPYMAP
				);
				KHR_materials_anisotropy_builder["anisotropyTexture"] = tinygltf::Value({
						{"index",tinygltf::Value(specularTexInfo_pre.index)},
						{"texCoord",tinygltf::Value(specularTexInfo_pre.texCoord)}
					});
			}
			material_builder.extensions["KHR_materials_anisotropy"] = tinygltf::Value(KHR_materials_anisotropy_builder);
		}

		state.gltfModel.materials.push_back(material_builder);
	}

	// Revert terrain texture remappings for residency tiles:
	for (size_t i = 0; i < scene.terrains.GetCount(); ++i)
	{
		using namespace wi::terrain;
		Terrain& terrain = scene.terrains[i];
		for (auto& it : terrain.chunks)
		{
			const Chunk& chunk = it.first;
			ChunkData& chunk_data = it.second;

			MaterialComponent* material = scene.materials.GetComponent(chunk_data.entity);
			if (material == nullptr)
				continue;

			VirtualTexture& vt = *chunk_data.vt;
			if (vt.residency == nullptr)
				continue;
			material->texMulAdd = XMFLOAT4(1, 1, 0, 0);
		}
	}

	// Write Meshes
	for(size_t m_id = 0; m_id < wiscene.meshes.GetCount(); ++m_id)
	{
		auto& mesh = wiscene.meshes[m_id];
		auto meshEntity = wiscene.meshes.GetEntity(m_id);
		auto nameComponent = wiscene.names.GetComponent(meshEntity);

		tinygltf::Mesh mesh_builder;
		mesh_builder.name = nameComponent->name;

		tinygltf::Buffer buffer_builder;
		int buffer_index = (int)state.gltfModel.buffers.size();

		size_t buf_idc_size = 0;
		size_t buf_d_vpos_size = 0;
		size_t buf_d_vnorm_size = 0;
		size_t buf_d_vtan_size = 0;
		size_t buf_d_uv0_size = 0;
		size_t buf_d_uv1_size = 0;
		size_t buf_d_joint_size = 0;
		size_t buf_d_weights_size = 0;
		size_t buf_d_col_size = 0;
		size_t buf_d_vpos_offset = 0;
		size_t buf_d_vnorm_offset = 0;
		size_t buf_d_vtan_offset = 0;
		size_t buf_d_uv0_offset = 0;
		size_t buf_d_uv1_offset = 0;
		size_t buf_d_joint_offset = 0;
		size_t buf_d_weights_offset = 0;
		size_t buf_d_col_offset = 0;

		// Write mesh data to buffer first and then figure things out...
		size_t buf_i = 0;

		// We reverse the indices' windings so that the face isn't flipped
		for (size_t i = 0; i < mesh.indices.size(); i += 3)
		{
			_ExportHelper_valuetobuf(mesh.indices[i + 0], buffer_builder, buf_i);
			_ExportHelper_valuetobuf(mesh.indices[i + 2], buffer_builder, buf_i);
			_ExportHelper_valuetobuf(mesh.indices[i + 1], buffer_builder, buf_i);
		}
		buf_idc_size = buf_i;

		// Write positions next
		buf_d_vpos_offset = buf_i;
		for(auto& m_position : mesh.vertex_positions)
		{
			_ExportHelper_valuetobuf(m_position, buffer_builder, buf_i);
		}
		buf_d_vpos_size = buf_i - buf_d_vpos_offset;

		// Write normals next
		buf_d_vnorm_offset = buf_i;
		for(auto& m_normal : mesh.vertex_normals)
		{
			XMVECTOR nor = XMLoadFloat3(&m_normal);
			nor = XMVector3Normalize(nor);
			XMStoreFloat3(&m_normal, nor);
			_ExportHelper_valuetobuf(m_normal, buffer_builder, buf_i);
		}
		buf_d_vnorm_size = buf_i - buf_d_vnorm_offset;

		// Write tangents next
		buf_d_vtan_offset = buf_i;
		for(auto& m_tangent : mesh.vertex_tangents)
		{
			float w = m_tangent.w;
			XMVECTOR tan = XMLoadFloat4(&m_tangent);
			tan = XMVector3Normalize(tan);
			XMStoreFloat4(&m_tangent, tan);
			m_tangent.w = w;
			_ExportHelper_valuetobuf(m_tangent, buffer_builder, buf_i);
		}
		buf_d_vtan_size = buf_i - buf_d_vtan_offset;

		// Write uvset 0 next
		buf_d_uv0_offset = buf_i;
		for(auto& m_uv0 : mesh.vertex_uvset_0)
		{
			_ExportHelper_valuetobuf(m_uv0, buffer_builder, buf_i);
		}
		buf_d_uv0_size = buf_i - buf_d_uv0_offset;

		// Write uvset 1 next
		buf_d_uv1_offset = buf_i;
		for(auto& m_uv1 : mesh.vertex_uvset_1)
		{
			_ExportHelper_valuetobuf(m_uv1, buffer_builder, buf_i);
		}
		buf_d_uv1_size = buf_i - buf_d_uv1_offset;

		// Write animation data - armature bone id
		buf_d_joint_offset = buf_i;
		for(auto& m_joint : mesh.vertex_boneindices)
		{
			auto m_joint_v = XMLoadUInt4(&m_joint);
			XMSHORT4 m_joint_s;
			XMStoreShort4(&m_joint_s, m_joint_v);
			_ExportHelper_valuetobuf(m_joint_s, buffer_builder, buf_i);
		}
		buf_d_joint_size = buf_i - buf_d_joint_offset;

		// Write animation data - armature weights id
		buf_d_weights_offset = buf_i;
		for(auto& m_bone_weights : mesh.vertex_boneweights)
		{
			_ExportHelper_valuetobuf(m_bone_weights, buffer_builder, buf_i);
		}
		buf_d_weights_size = buf_i - buf_d_weights_offset;

		// Write vertex colors
		buf_d_col_offset = buf_i;
		for(auto& m_col : mesh.vertex_colors)
		{
			_ExportHelper_valuetobuf(m_col, buffer_builder, buf_i);
		}
		buf_d_col_size = buf_i - buf_d_col_offset;

		// Mesh data
		tinygltf::BufferView vpos_bufferView_builder;
		int vpos_bufferView_index = (int)state.gltfModel.bufferViews.size();
		vpos_bufferView_builder.buffer = buffer_index;
		vpos_bufferView_builder.byteOffset = buf_d_vpos_offset;
		vpos_bufferView_builder.byteLength = buf_d_vpos_size;
		vpos_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
		state.gltfModel.bufferViews.push_back(vpos_bufferView_builder);

		tinygltf::Accessor vpos_accessor_builder;
		int vpos_accessor_index = (int)state.gltfModel.accessors.size();
		vpos_accessor_builder.bufferView = vpos_bufferView_index;
		vpos_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		vpos_accessor_builder.count = mesh.vertex_positions.size();
		vpos_accessor_builder.type = TINYGLTF_TYPE_VEC3;
		auto bound = wi::primitive::AABB(mesh.vertex_positions[0], mesh.vertex_positions[0]);
		for(auto& vpos : mesh.vertex_positions)
		{
			bound = wi::primitive::AABB::Merge(bound, wi::primitive::AABB(vpos, vpos));
		}
		auto bound_max = bound.getMax();
		auto bound_min = bound.getMin();
		vpos_accessor_builder.maxValues = {bound_max.x, bound_max.y, bound_max.z};
		vpos_accessor_builder.minValues = {bound_min.x, bound_min.y, bound_min.z};
		state.gltfModel.accessors.push_back(vpos_accessor_builder);

		tinygltf::BufferView vnorm_bufferView_builder;
		int vnorm_bufferView_index = (int)state.gltfModel.bufferViews.size();
		vnorm_bufferView_builder.buffer = buffer_index;
		vnorm_bufferView_builder.byteOffset = buf_d_vnorm_offset;
		vnorm_bufferView_builder.byteLength = buf_d_vnorm_size;
		vnorm_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
		state.gltfModel.bufferViews.push_back(vnorm_bufferView_builder);

		tinygltf::Accessor vnorm_accessor_builder;
		int vnorm_accessor_index = (int)state.gltfModel.accessors.size();
		vnorm_accessor_builder.bufferView = vnorm_bufferView_index;
		vnorm_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		vnorm_accessor_builder.count = mesh.vertex_normals.size();
		vnorm_accessor_builder.type = TINYGLTF_TYPE_VEC3;
		state.gltfModel.accessors.push_back(vnorm_accessor_builder);

		tinygltf::BufferView vtan_bufferView_builder;
		int vtan_bufferView_index = (int)state.gltfModel.bufferViews.size();
		vtan_bufferView_builder.buffer = buffer_index;
		vtan_bufferView_builder.byteOffset = buf_d_vtan_offset;
		vtan_bufferView_builder.byteLength = buf_d_vtan_size;
		vtan_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
		state.gltfModel.bufferViews.push_back(vtan_bufferView_builder);

		tinygltf::Accessor vtan_accessor_builder;
		int vtan_accessor_index = (int)state.gltfModel.accessors.size();
		vtan_accessor_builder.bufferView = vtan_bufferView_index;
		vtan_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		vtan_accessor_builder.count = mesh.vertex_tangents.size();
		vtan_accessor_builder.type = TINYGLTF_TYPE_VEC4;
		state.gltfModel.accessors.push_back(vtan_accessor_builder);

		int uv0_accessor_index = -1;
		if(buf_d_uv0_size > 0)
		{
			tinygltf::BufferView uv0_bufferView_builder;
			int uv0_bufferView_index = (int)state.gltfModel.bufferViews.size();
			uv0_bufferView_builder.buffer = buffer_index;
			uv0_bufferView_builder.byteOffset = buf_d_uv0_offset;
			uv0_bufferView_builder.byteLength = buf_d_uv0_size;
			uv0_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(uv0_bufferView_builder);

			tinygltf::Accessor uv0_accessor_builder;
			uv0_accessor_index = (int)state.gltfModel.accessors.size();
			uv0_accessor_builder.bufferView = uv0_bufferView_index;
			uv0_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			uv0_accessor_builder.count = mesh.vertex_uvset_0.size();
			uv0_accessor_builder.type = TINYGLTF_TYPE_VEC2;
			state.gltfModel.accessors.push_back(uv0_accessor_builder);
		}

		int uv1_accessor_index = -1;
		if(buf_d_uv1_size > 0)
		{
			tinygltf::BufferView uv1_bufferView_builder;
			int uv1_bufferView_index = (int)state.gltfModel.bufferViews.size();
			uv1_bufferView_builder.buffer = buffer_index;
			uv1_bufferView_builder.byteOffset = buf_d_uv1_offset;
			uv1_bufferView_builder.byteLength = buf_d_uv1_size;
			uv1_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(uv1_bufferView_builder);

			tinygltf::Accessor uv1_accessor_builder;
			uv1_accessor_index = (int)state.gltfModel.accessors.size();
			uv1_accessor_builder.bufferView = uv1_bufferView_index;
			uv1_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			uv1_accessor_builder.count = mesh.vertex_uvset_1.size();
			uv1_accessor_builder.type = TINYGLTF_TYPE_VEC2;
			state.gltfModel.accessors.push_back(uv1_accessor_builder);
		}

		int joint_accessor_index = -1;
		if(buf_d_joint_size > 0)
		{
			tinygltf::BufferView joint_bufferView_builder;
			int joint_bufferView_index = (int)state.gltfModel.bufferViews.size();
			joint_bufferView_builder.buffer = buffer_index;
			joint_bufferView_builder.byteOffset = buf_d_joint_offset;
			joint_bufferView_builder.byteLength = buf_d_joint_size;
			joint_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(joint_bufferView_builder);

			tinygltf::Accessor joint_accessor_builder;
			joint_accessor_index = (int)state.gltfModel.accessors.size();
			joint_accessor_builder.bufferView = joint_bufferView_index;
			joint_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
			joint_accessor_builder.count = mesh.vertex_boneindices.size();
			joint_accessor_builder.type = TINYGLTF_TYPE_VEC4;
			state.gltfModel.accessors.push_back(joint_accessor_builder);
		}

		int weight_accessor_index = -1;
		if(buf_d_weights_size > 0)
		{
			tinygltf::BufferView weight_bufferView_builder;
			int weight_bufferView_index = (int)state.gltfModel.bufferViews.size();
			weight_bufferView_builder.buffer = buffer_index;
			weight_bufferView_builder.byteOffset = buf_d_weights_offset;
			weight_bufferView_builder.byteLength = buf_d_weights_size;
			weight_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(weight_bufferView_builder);

			tinygltf::Accessor weight_accessor_builder;
			weight_accessor_index = (int)state.gltfModel.accessors.size();
			weight_accessor_builder.bufferView = weight_bufferView_index;
			weight_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			weight_accessor_builder.count = mesh.vertex_boneweights.size();
			weight_accessor_builder.type = TINYGLTF_TYPE_VEC4;
			state.gltfModel.accessors.push_back(weight_accessor_builder);
		}

		int color_accessor_index = -1;
		if(buf_d_col_size > 0)
		{
			tinygltf::BufferView color_bufferView_builder;
			int color_bufferView_index = (int)state.gltfModel.bufferViews.size();
			color_bufferView_builder.buffer = buffer_index;
			color_bufferView_builder.byteOffset = buf_d_col_offset;
			color_bufferView_builder.byteLength = buf_d_col_size;
			color_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(color_bufferView_builder);

			tinygltf::Accessor color_accessor_builder;
			color_accessor_index = (int)state.gltfModel.accessors.size();
			color_accessor_builder.bufferView = color_bufferView_index;
			color_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
			color_accessor_builder.count = mesh.vertex_colors.size();
			color_accessor_builder.type = TINYGLTF_TYPE_VEC4;
			state.gltfModel.accessors.push_back(color_accessor_builder);
		}

		// Morph targets

		// Prep up a zero value defaults for sparse morph target
		size_t buf_d_morph_def_offset, buf_d_morph_def_size;
		buf_d_morph_def_offset = buf_i;
		for(auto& m_position : mesh.vertex_positions)
		{
			_ExportHelper_valuetobuf(XMFLOAT3(), buffer_builder, buf_i);
		}
		buf_d_morph_def_size = buf_i - buf_d_morph_def_offset;

		tinygltf::BufferView morph_def_bufferView_builder;
		int morph_def_bufferView_index = (int)state.gltfModel.bufferViews.size();
		morph_def_bufferView_builder.buffer = buffer_index;
		morph_def_bufferView_builder.byteOffset = buf_d_morph_def_offset;
		morph_def_bufferView_builder.byteLength = buf_d_morph_def_size;
		morph_def_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
		state.gltfModel.bufferViews.push_back(morph_def_bufferView_builder);

		wi::vector<std::pair<size_t,bool>> morphs_pos_accessors;
		wi::vector<std::pair<size_t,bool>> morphs_norm_accessors;
		for(auto& m_morph : mesh.morph_targets)
		{
			size_t buf_d_morph_idc_pos_offset, buf_d_morph_idc_pos_size,
				buf_d_morph_idc_nor_offset, buf_d_morph_idc_nor_size,
				buf_d_morph_pos_size, buf_d_morph_pos_offset,
				buf_d_morph_norm_size, buf_d_morph_norm_offset;

			buf_d_morph_idc_pos_offset = buf_i;
			for(auto& m_morph_idc : m_morph.sparse_indices_positions)
			{
				_ExportHelper_valuetobuf(m_morph_idc, buffer_builder, buf_i);
			}
			buf_d_morph_idc_pos_size = buf_i - buf_d_morph_idc_pos_offset;

			buf_d_morph_idc_nor_offset = buf_i;
			for (auto& m_morph_idc : m_morph.sparse_indices_normals)
			{
				_ExportHelper_valuetobuf(m_morph_idc, buffer_builder, buf_i);
			}
			buf_d_morph_idc_nor_size = buf_i - buf_d_morph_idc_nor_offset;
			
			buf_d_morph_pos_offset = buf_i;
			for(auto& m_morph_pos : m_morph.vertex_positions)
			{
				_ExportHelper_valuetobuf(m_morph_pos, buffer_builder, buf_i);
			}
			buf_d_morph_pos_size = buf_i - buf_d_morph_pos_offset;

			buf_d_morph_norm_offset = buf_i;
			for(auto& m_morph_norm : m_morph.vertex_normals)
			{
				_ExportHelper_valuetobuf(m_morph_norm, buffer_builder, buf_i);
			}
			buf_d_morph_norm_size = buf_i - buf_d_morph_norm_offset;

			// Build accessors

			size_t morph_pos_accessor_index;
			if(buf_d_morph_pos_size > 0)
			{
				// Sparse accessor indices
				auto is_sparse = (m_morph.sparse_indices_positions.size() > 0);
				int morph_sparse_bufferView_index = 0;
				if (is_sparse)
				{
					tinygltf::BufferView morph_sparse_bufferView_builder;
					morph_sparse_bufferView_index = (int)state.gltfModel.bufferViews.size();
					morph_sparse_bufferView_builder.buffer = buffer_index;
					morph_sparse_bufferView_builder.byteOffset = buf_d_morph_idc_pos_offset;
					morph_sparse_bufferView_builder.byteLength = buf_d_morph_idc_pos_size;
					morph_sparse_bufferView_builder.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
					state.gltfModel.bufferViews.push_back(morph_sparse_bufferView_builder);
				}

				tinygltf::BufferView morph_pos_bufferView_builder;
				int morph_pos_bufferView_index = (int)state.gltfModel.bufferViews.size();
				morph_pos_bufferView_builder.buffer = buffer_index;
				morph_pos_bufferView_builder.byteOffset = buf_d_morph_pos_offset;
				morph_pos_bufferView_builder.byteLength = buf_d_morph_pos_size;
				morph_pos_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
				state.gltfModel.bufferViews.push_back(morph_pos_bufferView_builder);

				tinygltf::Accessor morph_pos_accessor_builder;
				morph_pos_accessor_index = state.gltfModel.accessors.size();
				morph_pos_accessor_builder.bufferView = (is_sparse) ? morph_def_bufferView_index : morph_pos_bufferView_index;
				morph_pos_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				morph_pos_accessor_builder.count = (is_sparse) ? mesh.vertex_positions.size() : m_morph.vertex_positions.size();
				morph_pos_accessor_builder.type = TINYGLTF_TYPE_VEC3;
				if(is_sparse)
				{
					auto& sparse = morph_pos_accessor_builder.sparse;
					sparse.isSparse = true;
					sparse.count = (int)m_morph.sparse_indices_positions.size();
					
					sparse.indices.bufferView = morph_sparse_bufferView_index;
					sparse.indices.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
					
					sparse.values.bufferView = morph_pos_bufferView_index;
				}
				state.gltfModel.accessors.push_back(morph_pos_accessor_builder);
			}

			size_t morph_norm_accessor_index;
			if(buf_d_morph_norm_size > 0)
			{
				// Sparse accessor indices
				auto is_sparse = (m_morph.sparse_indices_normals.size() > 0);
				int morph_sparse_bufferView_index = 0;
				if (is_sparse)
				{
					tinygltf::BufferView morph_sparse_bufferView_builder;
					morph_sparse_bufferView_index = (int)state.gltfModel.bufferViews.size();
					morph_sparse_bufferView_builder.buffer = buffer_index;
					morph_sparse_bufferView_builder.byteOffset = buf_d_morph_idc_nor_offset;
					morph_sparse_bufferView_builder.byteLength = buf_d_morph_idc_nor_size;
					morph_sparse_bufferView_builder.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
					state.gltfModel.bufferViews.push_back(morph_sparse_bufferView_builder);
				}

				tinygltf::BufferView morph_norm_bufferView_builder;
				int morph_norm_bufferView_index = (int)state.gltfModel.bufferViews.size();
				morph_norm_bufferView_builder.buffer = buffer_index;
				morph_norm_bufferView_builder.byteOffset = buf_d_morph_norm_offset;
				morph_norm_bufferView_builder.byteLength = buf_d_morph_norm_size;
				morph_norm_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
				state.gltfModel.bufferViews.push_back(morph_norm_bufferView_builder);

				tinygltf::Accessor morph_norm_accessor_builder;
				morph_norm_accessor_index = state.gltfModel.accessors.size();
				// morph_norm_accessor_builder.bufferView = (is_sparse) ? vnorm_bufferView_index : morph_norm_bufferView_index;
				morph_norm_accessor_builder.bufferView = (is_sparse) ? morph_def_bufferView_index : morph_norm_bufferView_index;
				morph_norm_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				// morph_norm_accessor_builder.count = (is_sparse) ? mesh.vertex_normals.size() : m_morph.vertex_normals.size();
				morph_norm_accessor_builder.count = (is_sparse) ? mesh.vertex_positions.size() : m_morph.vertex_normals.size();
				morph_norm_accessor_builder.type = TINYGLTF_TYPE_VEC3;
				if(is_sparse)
				{
					auto& sparse = morph_norm_accessor_builder.sparse;
					sparse.isSparse = true;
					sparse.count = (int)m_morph.sparse_indices_normals.size();
					
					sparse.indices.bufferView = morph_sparse_bufferView_index;
					sparse.indices.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
					
					sparse.values.bufferView = morph_norm_bufferView_index;
				}
				state.gltfModel.accessors.push_back(morph_norm_accessor_builder);
			}

			morphs_pos_accessors.push_back({morph_pos_accessor_index, buf_d_morph_pos_size > 0});
			morphs_norm_accessors.push_back({morph_norm_accessor_index, buf_d_morph_norm_size > 0});
		}

		// Store mesh indices by subset, which mapped to primitives
		uint32_t first_subset = 0;
		uint32_t last_subset = 0;
		mesh.GetLODSubsetRange(0, first_subset, last_subset); // GLTF doesn't have LODs, so export only LOD0
		for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
		{
			const MeshComponent::MeshSubset& subset = mesh.subsets[subsetIndex];
			if (subset.indexCount == 0)
				continue;

			// One primitive has one bufferview and accessor?
			tinygltf::BufferView indices_bufferView_builder;
			int indices_bufferView_index = (int)state.gltfModel.bufferViews.size();
			indices_bufferView_builder.buffer = buffer_index;
			indices_bufferView_builder.byteOffset = subset.indexOffset*sizeof(uint32_t);
			indices_bufferView_builder.byteLength = subset.indexCount*sizeof(uint32_t);
			indices_bufferView_builder.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(indices_bufferView_builder);

			tinygltf::Accessor indices_accessor_builder;
			int indices_accessor_index = (int)state.gltfModel.accessors.size();
			indices_accessor_builder.bufferView = indices_bufferView_index;
			indices_accessor_builder.byteOffset = 0;
			indices_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
			indices_accessor_builder.count = subset.indexCount;
			indices_accessor_builder.type = TINYGLTF_TYPE_SCALAR;
			state.gltfModel.accessors.push_back(indices_accessor_builder);
	
			tinygltf::Primitive primitive_builder;
			primitive_builder.indices = indices_accessor_index;
			primitive_builder.attributes["POSITION"] = vpos_accessor_index;
			primitive_builder.attributes["NORMAL"] = vnorm_accessor_index;
			primitive_builder.attributes["TANGENT"] = vtan_accessor_index;
			if(buf_d_uv0_size > 0)
				primitive_builder.attributes["TEXCOORD_0"] = uv0_accessor_index;
			if(buf_d_uv1_size > 0)
				primitive_builder.attributes["TEXCOORD_1"] = uv1_accessor_index;
			if(buf_d_joint_size > 0)
				primitive_builder.attributes["JOINTS_0"] = joint_accessor_index;
			if(buf_d_weights_size > 0)
				primitive_builder.attributes["WEIGHTS_0"] = weight_accessor_index;
			if(buf_d_col_size > 0)
				primitive_builder.attributes["COLOR_0"] = color_accessor_index;
			primitive_builder.material = std::max(0, std::min((int)wiscene.materials.GetIndex(subset.materialID), (int)wiscene.materials.GetCount() - 1));
			primitive_builder.mode = TINYGLTF_MODE_TRIANGLES;

			for(size_t msub_morph_id = 0; msub_morph_id < morphs_pos_accessors.size(); ++msub_morph_id)
			{
				std::map<std::string, int> morph_info;
				if(morphs_pos_accessors[msub_morph_id].second)
					morph_info["POSITION"] = (int)morphs_pos_accessors[msub_morph_id].first;
				if(morphs_norm_accessors[msub_morph_id].second)
					morph_info["NORMAL"] = (int)morphs_norm_accessors[msub_morph_id].first;
				primitive_builder.targets.push_back(morph_info);
			}

			mesh_builder.primitives.push_back(primitive_builder);
		}

		state.gltfModel.buffers.push_back(buffer_builder);
		state.gltfModel.meshes.push_back(mesh_builder);
	}

	// Write Lights
	for(size_t l_id = 0; l_id < wiscene.lights.GetCount(); ++l_id)
	{
		auto& light = wiscene.lights[l_id];
		auto lightEntity = wiscene.lights.GetEntity(l_id);

		auto nameComponent = wiscene.names.GetComponent(lightEntity);

		tinygltf::Light light_builder;

		if(nameComponent != nullptr)
			light_builder.name = nameComponent->name;

		light_builder.type = 
			(light.type == LightComponent::LightType::DIRECTIONAL) ? "directional" : 
			(light.type == LightComponent::LightType::SPOT) ? "spot" : "point";
		light_builder.color = {double(light.color.x), double(light.color.y), double(light.color.z)};
		light_builder.intensity = double(light.intensity);
		light_builder.range = double(light.range);
		light_builder.spot.outerConeAngle = double(light.outerConeAngle);
		light_builder.spot.innerConeAngle = double(light.innerConeAngle);

		state.gltfModel.lights.push_back(light_builder);
	}

	// Write Cameras
	for(size_t cam_id = 0; cam_id < wiscene.cameras.GetCount(); ++cam_id)
	{
		auto& camera = wiscene.cameras[cam_id];
		auto cameraEntity = wiscene.cameras.GetEntity(cam_id);

		auto nameComponent = wiscene.names.GetComponent(cameraEntity);
		
		tinygltf::Camera camera_builder;

		if(nameComponent != nullptr)
			camera_builder.name = nameComponent->name;

		camera_builder.type = "perspective";
		camera_builder.perspective.aspectRatio = camera.width/camera.height;
		camera_builder.perspective.yfov = camera.fov;
		camera_builder.perspective.zfar = camera.zFarP;
		camera_builder.perspective.znear = camera.zNearP;

		state.gltfModel.cameras.push_back(camera_builder);
	}

	tinygltf::Scene scene_builder;

	// Compose Node
	for(size_t t_id = 0; t_id < wiscene.transforms.GetCount(); ++t_id)
	{
		auto& transformComponent = wiscene.transforms[t_id];
		auto transformEntity = wiscene.transforms.GetEntity(t_id);
		auto nameComponent = wiscene.names.GetComponent(transformEntity);

		auto light_forward_flip = wiscene.lights.Contains(transformEntity);
		if(light_forward_flip)
			transformComponent.RotateRollPitchYaw(XMFLOAT3(-XM_PIDIV2,0,0));

		auto objectComponent = wiscene.objects.GetComponent(transformEntity);

		tinygltf::Node node_builder;
		int node_index = (int)t_id;
		
		if(nameComponent != nullptr)
			node_builder.name = nameComponent->name;

		node_builder.scale.push_back(transformComponent.scale_local.x);
		node_builder.scale.push_back(transformComponent.scale_local.y);
		node_builder.scale.push_back(transformComponent.scale_local.z);

		node_builder.rotation.push_back(transformComponent.rotation_local.x);
		node_builder.rotation.push_back(transformComponent.rotation_local.y);
		node_builder.rotation.push_back(transformComponent.rotation_local.z);
		node_builder.rotation.push_back(transformComponent.rotation_local.w);

		node_builder.translation.push_back(transformComponent.translation_local.x);
		node_builder.translation.push_back(transformComponent.translation_local.y);
		node_builder.translation.push_back(transformComponent.translation_local.z);

		if(light_forward_flip)
			transformComponent.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2,0,0));
		
		if(objectComponent != nullptr)
		{
			if(objectComponent->meshID != wi::ecs::INVALID_ENTITY)
			{
				node_builder.mesh = (int)wiscene.meshes.GetIndex(objectComponent->meshID);
				if(wiscene.meshes[node_builder.mesh].armatureID != wi::ecs::INVALID_ENTITY)
				{
					node_builder.skin = (int)wiscene.armatures.GetIndex(wiscene.meshes[node_builder.mesh].armatureID);
				}
			}
		}

		if(wiscene.lights.Contains(transformEntity))
		{
			tinygltf::Value::Object node_light_extension_builder;
			node_light_extension_builder["light"] = tinygltf::Value(int(wiscene.lights.GetIndex(transformEntity)));
			node_builder.extensions["KHR_lights_punctual"] = tinygltf::Value(node_light_extension_builder);
		}

		if(wiscene.cameras.Contains(transformEntity))
		{
			node_builder.camera = (int)wiscene.cameras.GetIndex(transformEntity);
		}
		
		state.nodeMap[transformEntity] = node_index;
		state.gltfModel.nodes.push_back(node_builder);
		scene_builder.nodes.push_back(node_index);
	}

	// Write Armature
	for(size_t arm_id = 0; arm_id < wiscene.armatures.GetCount(); ++arm_id)
	{
		auto& armatureComponent = wiscene.armatures[arm_id];
		auto armatureEntity = wiscene.armatures.GetEntity(arm_id);

		auto nameComponent = wiscene.names.GetComponent(armatureEntity);

		tinygltf::Skin skin_builder;
		
		if(nameComponent != nullptr)
			skin_builder.name = nameComponent->name;

		tinygltf::Buffer buffer_builder;
		int buffer_index = (int)state.gltfModel.buffers.size();

		size_t buf_i = 0;

		// Write Inverse Bind Matrices to buffer
		for(auto& arm_invBindMatrix : armatureComponent.inverseBindMatrices)
		{
			_ExportHelper_valuetobuf(arm_invBindMatrix, buffer_builder, buf_i);
		}
		state.gltfModel.buffers.push_back(buffer_builder);

		//// Inverse Bind Matrices data access
		//// Analysis prep
		//wi::jobsystem::context analysis_ctx;
		//std::mutex analysis_lock_sync;
		//uint32_t analysis_readCount = 16384;

		tinygltf::BufferView aibm_bufferView_builder;
		int aibm_bufferView_index = (int)state.gltfModel.bufferViews.size();
		aibm_bufferView_builder.buffer = buffer_index;
		// aibm_bufferView_builder.byteOffset = 0;
		aibm_bufferView_builder.byteLength = buf_i;
		// aibm_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
		state.gltfModel.bufferViews.push_back(aibm_bufferView_builder);

		tinygltf::Accessor aibm_accessor_builder;
		int aibm_accessor_index = (int)state.gltfModel.accessors.size();
		aibm_accessor_builder.bufferView = aibm_bufferView_index;
		aibm_accessor_builder.byteOffset = 0;
		aibm_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		aibm_accessor_builder.count = armatureComponent.inverseBindMatrices.size();
		aibm_accessor_builder.type = TINYGLTF_TYPE_MAT4;
		// _ExportHelper_AccessorAnalysis(
		// 	aibm_accessor_builder, armatureComponent.inverseBindMatrices,
		// 	0, armatureComponent.inverseBindMatrices.size(),
		// 	analysis_readCount);
		state.gltfModel.accessors.push_back(aibm_accessor_builder);

		skin_builder.inverseBindMatrices = aibm_accessor_index;

		for(auto& arm_bone_id : armatureComponent.boneCollection)
		{
			skin_builder.joints.push_back(state.nodeMap[arm_bone_id]);
		}
		
		state.gltfModel.skins.push_back(skin_builder);
	}

	// Write Animations
	wi::unordered_map<Entity, std::vector<size_t>> animation_datasets;
	if(wiscene.animations.GetCount() > 0)
	{
		// Find accessor types first!
		wi::unordered_map<Entity, size_t> animdata_vectype; 
		for(size_t anim_id = 0; anim_id < wiscene.animations.GetCount(); ++anim_id)
		{
			auto& animation = wiscene.animations[anim_id];
			for(auto& channel : animation.channels)
			{
				if(animdata_vectype.find(animation.samplers[channel.samplerIndex].data) == animdata_vectype.end())
					animdata_vectype[animation.samplers[channel.samplerIndex].data] = 
						(channel.path == AnimationComponent::AnimationChannel::Path::SCALE) ? TINYGLTF_TYPE_VEC3 :
						(channel.path == AnimationComponent::AnimationChannel::Path::ROTATION) ? TINYGLTF_TYPE_VEC4 :
						(channel.path == AnimationComponent::AnimationChannel::Path::TRANSLATION) ? TINYGLTF_TYPE_VEC3 :
						(channel.path == AnimationComponent::AnimationChannel::Path::WEIGHTS) ? TINYGLTF_TYPE_SCALAR : TINYGLTF_TYPE_SCALAR;
			}
		}

		// Store animations into a single buffer
		size_t buf_i = 0;
		tinygltf::Buffer buffer_builder;
		int buffer_index = (int)state.gltfModel.buffers.size();
		for(size_t animdata_id = 0; animdata_id < wiscene.animation_datas.GetCount(); ++animdata_id)
		{
			auto& animdata = wiscene.animation_datas[animdata_id];
			auto animdataEntity = wiscene.animation_datas.GetEntity(animdata_id);

			size_t buf_d_ftime_offset, buf_d_ftime_size, 
				buf_d_fdata_offset, buf_d_fdata_size;
			buf_d_ftime_offset = buf_i;
			for(auto& animdata_ftime : animdata.keyframe_times)
			{
				_ExportHelper_valuetobuf(animdata_ftime, buffer_builder, buf_i);
			}
			buf_d_ftime_size = buf_i - buf_d_ftime_offset;

			buf_d_fdata_offset = buf_i;
			for(auto& animdata_fdata : animdata.keyframe_data)
			{
				_ExportHelper_valuetobuf(animdata_fdata, buffer_builder, buf_i);
			}
			buf_d_fdata_size = buf_i - buf_d_fdata_offset;

			tinygltf::BufferView ftime_bufferView_builder;
			int ftime_bufferView_index = (int)state.gltfModel.bufferViews.size();
			ftime_bufferView_builder.buffer = buffer_index;
			ftime_bufferView_builder.byteOffset = buf_d_ftime_offset;
			ftime_bufferView_builder.byteLength = buf_d_ftime_size;
			// ftime_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(ftime_bufferView_builder);

			tinygltf::Accessor ftime_accessor_builder;
			int ftime_accessor_index = (int)state.gltfModel.accessors.size();
			ftime_accessor_builder.bufferView = ftime_bufferView_index;
			ftime_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			ftime_accessor_builder.count = animdata.keyframe_times.size();
			ftime_accessor_builder.type = TINYGLTF_TYPE_SCALAR;
			state.gltfModel.accessors.push_back(ftime_accessor_builder);

			tinygltf::BufferView fdata_bufferView_builder;
			int fdata_bufferView_index = (int)state.gltfModel.bufferViews.size();
			fdata_bufferView_builder.buffer = buffer_index;
			fdata_bufferView_builder.byteOffset = buf_d_fdata_offset;
			fdata_bufferView_builder.byteLength = buf_d_fdata_size;
			// fdata_bufferView_builder.target = TINYGLTF_TARGET_ARRAY_BUFFER;
			state.gltfModel.bufferViews.push_back(fdata_bufferView_builder);

			int anim_vectype = TINYGLTF_TYPE_SCALAR;
			size_t anim_sizedivider = 1;
			auto find_animdata_vectype = animdata_vectype.find(animdataEntity);
			if(find_animdata_vectype != animdata_vectype.end())
			{
				anim_vectype = (int)find_animdata_vectype->second;
				anim_sizedivider = (find_animdata_vectype->second == TINYGLTF_TYPE_SCALAR) ? 1 : find_animdata_vectype->second;
			}

			tinygltf::Accessor fdata_accessor_builder;
			int fdata_accessor_index = (int)state.gltfModel.accessors.size();
			fdata_accessor_builder.bufferView = fdata_bufferView_index;
			fdata_accessor_builder.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			fdata_accessor_builder.count = animdata.keyframe_data.size() / anim_sizedivider;
			fdata_accessor_builder.type = anim_vectype;
			state.gltfModel.accessors.push_back(fdata_accessor_builder);

			animation_datasets[animdataEntity] = {
				(size_t)ftime_bufferView_index,
				(size_t)ftime_accessor_index,
				(size_t)fdata_bufferView_index,
				(size_t)fdata_accessor_index
			};
		}
		state.gltfModel.buffers.push_back(buffer_builder);
	}
	for(size_t anim_id = 0; anim_id < wiscene.animations.GetCount(); ++anim_id)
	{
		auto& animation = wiscene.animations[anim_id];
		
		tinygltf::Animation animation_builder;

		for(auto& sampler : animation.samplers)
		{
			tinygltf::AnimationSampler sampler_builder;
			sampler_builder.input = (int)animation_datasets[sampler.data][1];
			sampler_builder.output = (int)animation_datasets[sampler.data][3];
			sampler_builder.interpolation = 
				(sampler.mode == AnimationComponent::AnimationSampler::Mode::CUBICSPLINE) ? "CUBICSPLINE" :
				(sampler.mode == AnimationComponent::AnimationSampler::Mode::STEP) ? "STEP" : "LINEAR";

			animation_builder.samplers.push_back(sampler_builder);
		}
		for(auto& channel : animation.channels)
		{
			tinygltf::AnimationChannel channel_builder;
			channel_builder.target_node = state.nodeMap[channel.target];
			channel_builder.sampler = (int)channel.samplerIndex;
			channel_builder.target_path = 
				(channel.path == AnimationComponent::AnimationChannel::Path::SCALE) ? "scale" :
				(channel.path == AnimationComponent::AnimationChannel::Path::ROTATION) ? "rotation" :
				(channel.path == AnimationComponent::AnimationChannel::Path::TRANSLATION) ? "translation" : "weights";
			
			animation_builder.channels.push_back(channel_builder);
		}
		state.gltfModel.animations.push_back(animation_builder);
	}

	// Compose hierarchy
	for(size_t h_id = 0; h_id < wiscene.hierarchy.GetCount(); ++h_id)
	{
		auto& hierarchyComponent = wiscene.hierarchy[h_id];
		auto hierarchyEntity = wiscene.hierarchy.GetEntity(h_id);
		if(wiscene.transforms.Contains(hierarchyComponent.parentID) && wiscene.transforms.Contains(hierarchyEntity))
		{
			int node_index = (int)wiscene.transforms.GetIndex(hierarchyEntity);
			size_t parent_node_index = wiscene.transforms.GetIndex(hierarchyComponent.parentID);
			state.gltfModel.nodes[parent_node_index].children.push_back(node_index);
		}
	}

	state.gltfModel.defaultScene = (int)state.gltfModel.scenes.size();
	state.gltfModel.scenes.push_back(scene_builder);
	state.gltfModel.asset.version = "2.0";
	state.gltfModel.asset.generator = "WickedEngine";

	auto file_extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename));
	if(file_extension == "GLB")
	{
		writer.WriteGltfSceneToFile(&state.gltfModel, filename, false, true, true, true);
	}
	else
	{
		writer.WriteGltfSceneToFile(&state.gltfModel, filename, false, false, true, false);
	}

	// Restore scene world orientation
	FlipZAxis(state);
	wiscene.Update(0.f);
}
