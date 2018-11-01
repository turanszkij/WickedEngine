#include "stdafx.h"
#include "wiSceneSystem.h"
#include "ModelImporter.h"

#include "Utility/stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiSceneSystem;
using namespace wiECS;


// Transform the data from glTF space to engine-space:
static const bool transform_to_LH = true;


namespace tinygltf
{
	bool LoadImageData(Image *image, std::string *err, std::string *warn,
		int req_width, int req_height, const unsigned char *bytes,
		int size, void *)
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


void RegisterTexture2D(tinygltf::Image *image)
{
	// We will load the texture2d by hand here and register to the resource manager
	{
		int width = image->width;
		int height = image->height;
		int channelCount = image->component;
		const unsigned char* rgb = image->image.data();

		if (rgb != nullptr)
		{
			TextureDesc desc;
			desc.ArraySize = 1;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.Format = FORMAT_R8G8B8A8_UNORM;
			desc.Height = static_cast<uint32_t>(height);
			desc.Width = static_cast<uint32_t>(width);
			desc.MipLevels = (UINT)log2(max(width, height));
			desc.MiscFlags = 0;
			desc.Usage = USAGE_DEFAULT;

			UINT mipwidth = width;
			SubresourceData* InitData = new SubresourceData[desc.MipLevels];
			for (UINT mip = 0; mip < desc.MipLevels; ++mip)
			{
				InitData[mip].pSysMem = rgb;
				InitData[mip].SysMemPitch = static_cast<UINT>(mipwidth * channelCount);
				mipwidth = max(1, mipwidth / 2);
			}

			Texture2D* tex = new Texture2D;
			tex->RequestIndependentShaderResourcesForMIPs(true);
			tex->RequestIndependentUnorderedAccessResourcesForMIPs(true);
			HRESULT hr = wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData, &tex);
			assert(SUCCEEDED(hr));

			if (tex != nullptr)
			{
				wiRenderer::AddDeferredMIPGen(tex);

				if (image->uri.empty())
				{
					static UINT imgcounter = 0;
					stringstream ss("");
					ss << "gltfLoader_image" << imgcounter++;
					image->uri = ss.str();
				}

				// We loaded the texture2d, so register to the resource manager to be retrieved later:
				wiResourceManager::GetGlobal().Register(image->uri, tex, wiResourceManager::IMAGE);
			}
		}
	}
}


struct LoaderState
{
	tinygltf::Model gltfModel;
	Scene scene;
	unordered_map<const tinygltf::Node*, Entity> entityMap;
	vector<Entity> materialArray;
	vector<Entity> meshArray;
	vector<Entity> armatureArray;
};

// Recursively loads nodes and resolves hierarchy:
void LoadNode(tinygltf::Node* node, Entity parent, LoaderState& state)
{
	if (node == nullptr)
	{
		return;
	}

	Entity entity = INVALID_ENTITY;

	if(node->mesh >= 0)
	{
		entity = state.scene.Entity_CreateObject(node->name);
		ObjectComponent& object = *state.scene.objects.GetComponent(entity);

		if (node->mesh < (int)state.meshArray.size())
		{
			object.meshID = state.meshArray[node->mesh];

			if (node->skin >= 0)
			{
				MeshComponent& mesh = *state.scene.meshes.GetComponent(object.meshID);
				assert(!mesh.vertex_boneindices.empty());
				mesh.armatureID = state.armatureArray[node->skin];
			}
		}
		else
		{
			assert(0);
		}
	}
	else if (node->camera >= 0)
	{
		if (node->name.empty())
		{
			static int camID = 0;
			stringstream ss("");
			ss << "cam" << camID++;
			node->name = ss.str();
		}

		entity = state.scene.Entity_CreateCamera(node->name, (float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 800);
	}

	if (entity == INVALID_ENTITY)
	{
		entity = CreateEntity();
		state.scene.transforms.Create(entity);
	}

	state.entityMap[node] = entity;

	TransformComponent& transform = *state.scene.transforms.GetComponent(entity);
	if (!node->scale.empty())
	{
		transform.scale_local = XMFLOAT3((float)node->scale[0], (float)node->scale[1], (float)node->scale[2]);
	}
	if (!node->rotation.empty())
	{
		transform.rotation_local = XMFLOAT4((float)node->rotation[0], (float)node->rotation[1], (float)node->rotation[2], (float)node->rotation[3]);
	}
	if (!node->translation.empty())
	{
		transform.translation_local = XMFLOAT3((float)node->translation[0], (float)node->translation[1], (float)node->translation[2]);
	}
	if (!node->matrix.empty())
	{
		transform.world._11 = (float)node->matrix[0];
		transform.world._12 = (float)node->matrix[1];
		transform.world._13 = (float)node->matrix[2];
		transform.world._14 = (float)node->matrix[3];
		transform.world._21 = (float)node->matrix[4];
		transform.world._22 = (float)node->matrix[5];
		transform.world._23 = (float)node->matrix[6];
		transform.world._24 = (float)node->matrix[7];
		transform.world._31 = (float)node->matrix[8];
		transform.world._32 = (float)node->matrix[9];
		transform.world._33 = (float)node->matrix[10];
		transform.world._34 = (float)node->matrix[11];
		transform.world._41 = (float)node->matrix[12];
		transform.world._42 = (float)node->matrix[13];
		transform.world._43 = (float)node->matrix[14];
		transform.world._44 = (float)node->matrix[15];
		transform.ApplyTransform(); // this creates S, R, T vectors from world matrix
	}

	// Important:
	//	Do NOT call UpdateTransform, because Attach will query parent world matrix, and invert it for bind matrix
	//	But here we load everything in bind space (relative to parent) already, so it must be IDENTITY!
	transform.world = IDENTITYMATRIX;

	if (parent != INVALID_ENTITY)
	{
		state.scene.Component_Attach(entity, parent);
	}

	if (!node->children.empty())
	{
		for (int child : node->children)
		{
			LoadNode(&state.gltfModel.nodes[child], entity, state);
		}
	}
}

void ImportModel_GLTF(const std::string& fileName)
{
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(name));
	wiHelper::RemoveExtensionFromFileName(name);


	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
	loader.SetImageWriter(tinygltf::WriteImageData, nullptr);
	
	LoaderState state;

	bool ret;
	if (!extension.compare("GLTF"))
	{
		ret = loader.LoadASCIIFromFile(&state.gltfModel, &err, &warn, fileName);
	}
	else
	{
		ret = loader.LoadBinaryFromFile(&state.gltfModel, &err, &warn, fileName); // for binary glTF(.glb) 
	}
	if (!ret) {
		wiHelper::messageBox(err, "GLTF error!");
	}

	Entity rootEntity = CreateEntity();
	state.scene.transforms.Create(rootEntity);

	// Create materials:
	for (auto& x : state.gltfModel.materials)
	{
		Entity materialEntity = state.scene.Entity_CreateMaterial(x.name);

		state.materialArray.push_back(materialEntity);

		MaterialComponent& material = *state.scene.materials.GetComponent(materialEntity);

		material.baseColor = XMFLOAT4(1, 1, 1, 1);
		material.roughness = 1.0f;
		material.metalness = 1.0f;
		material.reflectance = 0.02f;
		material.emissive = 0;

		auto& baseColorTexture = x.values.find("baseColorTexture");
		auto& metallicRoughnessTexture = x.values.find("metallicRoughnessTexture");
		auto& normalTexture = x.additionalValues.find("normalTexture");
		auto& emissiveTexture = x.additionalValues.find("emissiveTexture");
		auto& occlusionTexture = x.additionalValues.find("occlusionTexture");

		auto& baseColorFactor = x.values.find("baseColorFactor");
		auto& roughnessFactor = x.values.find("roughnessFactor");
		auto& metallicFactor = x.values.find("metallicFactor");
		auto& emissiveFactor = x.additionalValues.find("emissiveFactor");
		auto& alphaCutoff = x.additionalValues.find("alphaCutoff");

		if (baseColorTexture != x.values.end())
		{
			auto& tex = state.gltfModel.textures[baseColorTexture->second.TextureIndex()];
			auto& img = state.gltfModel.images[tex.source];
			RegisterTexture2D(&img);
			material.baseColorMapName = img.uri;
		}
		else if(!state.gltfModel.images.empty())
		{
			// For some reason, we don't have diffuse texture, but have other textures
			//	I have a problem, because one model viewer displays textures on a model which has no basecolor set in its material...
			//	This is probably not how it should be (todo)
			RegisterTexture2D(&state.gltfModel.images[0]);
			material.baseColorMapName = state.gltfModel.images[0].uri;
		}

		tinygltf::Image* img_nor = nullptr;
		tinygltf::Image* img_met_rough = nullptr;
		tinygltf::Image* img_emissive = nullptr;

		if (normalTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[normalTexture->second.TextureIndex()];
			img_nor = &state.gltfModel.images[tex.source];
		}
		if (metallicRoughnessTexture != x.values.end())
		{
			auto& tex = state.gltfModel.textures[metallicRoughnessTexture->second.TextureIndex()];
			img_met_rough = &state.gltfModel.images[tex.source];
		}
		if (emissiveTexture != x.additionalValues.end())
		{
			auto& tex = state.gltfModel.textures[emissiveTexture->second.TextureIndex()];
			img_emissive = &state.gltfModel.images[tex.source];
		}

		// Now we will begin interleaving texture data to match engine layout:

		if (img_nor != nullptr)
		{
			uint32_t* data32_roughness = nullptr;
			if (img_met_rough != nullptr && img_met_rough->width == img_nor->width && img_met_rough->height == img_nor->height)
			{
				data32_roughness = (uint32_t*)img_met_rough->image.data();
			}
			else if (img_met_rough != nullptr)
			{
				wiBackLog::post("[gltf] Warning: there is a normalmap and roughness texture, but not the same size! Roughness will not be baked in!");
			}

			// Convert normal map:
			uint32_t* data32 = (uint32_t*)img_nor->image.data();
			for (int i = 0; i < img_nor->width * img_nor->height; ++i)
			{
				uint32_t pixel = data32[i];
				float r = ((pixel >> 0)  & 255) / 255.0f;
				float g = ((pixel >> 8)  & 255) / 255.0f;
				float b = ((pixel >> 16) & 255) / 255.0f;
				float a = ((pixel >> 24) & 255) / 255.0f;

				// swap normal y direction:
				g = 1 - g;

				// reset roughness:
				a = 1;

				if (data32_roughness != nullptr)
				{
					// add roughness from texture (G):
					a = ((data32_roughness[i] >> 8) & 255) / 255.0f;
					a = max(1.0f / 255.0f, a); // disallow 0 roughness (but is it really a good idea to do it here???)
				}

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}

			RegisterTexture2D(img_nor);
			material.normalMapName = img_nor->uri;
		}

		if (img_met_rough != nullptr)
		{
			uint32_t* data32_emissive = nullptr;
			if (img_emissive != nullptr && img_emissive->width == img_met_rough->width && img_emissive->height == img_met_rough->height)
			{
				data32_emissive = (uint32_t*)img_emissive->image.data();
			}

			uint32_t* data32 = (uint32_t*)img_met_rough->image.data();
			for (int i = 0; i < img_met_rough->width * img_met_rough->height; ++i)
			{
				uint32_t pixel = data32[i];
				float r = ((pixel >> 0) & 255) / 255.0f;
				float g = ((pixel >> 8) & 255) / 255.0f;
				float b = ((pixel >> 16) & 255) / 255.0f;
				float a = ((pixel >> 24) & 255) / 255.0f;

				float reflectance = 1;
				float metalness = b;
				float emissive = 0;
				float sss = 1;

				if (data32_emissive != nullptr)
				{
					// add emissive from texture (R):
					//	(Currently only supporting single channel emissive)
					emissive = ((data32_emissive[i] >> 0) & 255) / 255.0f;
				}

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(reflectance * 255.0f) << 0;
				rgba8 |= (uint32_t)(metalness * 255.0f) << 8;
				rgba8 |= (uint32_t)(emissive * 255.0f) << 16;
				rgba8 |= (uint32_t)(sss * 255.0f) << 24;

				data32[i] = rgba8;
			}

			RegisterTexture2D(img_met_rough);
			material.surfaceMapName = img_met_rough->uri;
		}
		else if (img_emissive != nullptr)
		{
			// No metalness texture, just emissive...
			uint32_t* data32 = (uint32_t*)img_emissive->image.data();

			if (data32 != nullptr)
			{
				for (int i = 0; i < img_emissive->width * img_emissive->height; ++i)
				{
					uint32_t pixel = data32[i];
					float r = ((pixel >> 0) & 255) / 255.0f;
					float g = ((pixel >> 8) & 255) / 255.0f;
					float b = ((pixel >> 16) & 255) / 255.0f;
					float a = ((pixel >> 24) & 255) / 255.0f;

					float reflectance = 1;
					float metalness = 1;
					float emissive = r;
					float sss = 1;

					uint32_t rgba8 = 0;
					rgba8 |= (uint32_t)(reflectance * 255.0f) << 0;
					rgba8 |= (uint32_t)(metalness * 255.0f) << 8;
					rgba8 |= (uint32_t)(emissive * 255.0f) << 16;
					rgba8 |= (uint32_t)(sss * 255.0f) << 24;

					data32[i] = rgba8;
				}

				RegisterTexture2D(img_emissive);
				material.surfaceMapName = img_emissive->uri;
			}
		}

		// Retrieve textures by name:
		if (!material.baseColorMapName.empty())
			material.baseColorMap = (Texture2D*)wiResourceManager::GetGlobal().add(material.baseColorMapName);
		if (!material.normalMapName.empty())
			material.normalMap = (Texture2D*)wiResourceManager::GetGlobal().add(material.normalMapName);
		if (!material.surfaceMapName.empty())
			material.surfaceMap = (Texture2D*)wiResourceManager::GetGlobal().add(material.surfaceMapName);

		if (baseColorFactor != x.values.end())
		{
			material.baseColor.x = static_cast<float>(baseColorFactor->second.ColorFactor()[0]);
			material.baseColor.y = static_cast<float>(baseColorFactor->second.ColorFactor()[1]);
			material.baseColor.z = static_cast<float>(baseColorFactor->second.ColorFactor()[2]);
		}
		if (roughnessFactor != x.values.end())
		{
			material.roughness = static_cast<float>(roughnessFactor->second.Factor());
		}
		if (metallicFactor != x.values.end())
		{
			material.metalness = static_cast<float>(metallicFactor->second.Factor());
		}
		if (emissiveFactor != x.additionalValues.end())
		{
			material.emissive = static_cast<float>(emissiveFactor->second.ColorFactor()[0]);
		}
		if (alphaCutoff != x.additionalValues.end())
		{
			material.alphaRef = 1 - static_cast<float>(alphaCutoff->second.Factor());
		}

	}

	if (state.materialArray.empty())
	{
		Entity materialEntity = state.scene.Entity_CreateMaterial("GLTFImport_defaultMaterial");
		state.materialArray.push_back(materialEntity);
	}

	// Create meshes:
	for (auto& x : state.gltfModel.meshes)
	{
		Entity meshEntity = state.scene.Entity_CreateMesh(x.name);

		state.meshArray.push_back(meshEntity);

		MeshComponent& mesh = *state.scene.meshes.GetComponent(meshEntity);

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

			mesh.subsets.back().materialID = state.materialArray[max(0, prim.material)];

			uint32_t vertexOffset = (uint32_t)mesh.vertex_positions.size();

			const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

			if (stride == 1)
			{
				for (size_t i = 0; i < indexCount; i += 3)
				{
					mesh.indices[indexOffset + i + 0] = vertexOffset + data[i + 0];
					mesh.indices[indexOffset + i + 1] = vertexOffset + data[i + 1];
					mesh.indices[indexOffset + i + 2] = vertexOffset + data[i + 2];
				}
			}
			else if (stride == 2)
			{
				for (size_t i = 0; i < indexCount; i += 3)
				{
					mesh.indices[indexOffset + i + 0] = vertexOffset + ((uint16_t*)data)[i + 0];
					mesh.indices[indexOffset + i + 1] = vertexOffset + ((uint16_t*)data)[i + 1];
					mesh.indices[indexOffset + i + 2] = vertexOffset + ((uint16_t*)data)[i + 2];
				}
			}
			else if (stride == 4)
			{
				for (size_t i = 0; i < indexCount; i += 3)
				{
					mesh.indices[indexOffset + i + 0] = vertexOffset + ((uint32_t*)data)[i + 0];
					mesh.indices[indexOffset + i + 1] = vertexOffset + ((uint32_t*)data)[i + 1];
					mesh.indices[indexOffset + i + 2] = vertexOffset + ((uint32_t*)data)[i + 2];
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
				else if (!attr_name.compare("TEXCOORD_0"))
				{
					mesh.vertex_texcoords.resize(vertexOffset + vertexCount);
					assert(stride == 8);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						const XMFLOAT2& tex = ((XMFLOAT2*)data)[i];

						mesh.vertex_texcoords[vertexOffset + i].x = tex.x;
						mesh.vertex_texcoords[vertexOffset + i].y = tex.y;
					}
				}
				else if (!attr_name.compare("TEXCOORD_1"))
				{
					mesh.vertex_atlas.resize(vertexOffset + vertexCount);
					assert(stride == 8);
					for (size_t i = 0; i < vertexCount; ++i)
					{
						const XMFLOAT2& tex = ((XMFLOAT2*)data)[i];

						mesh.vertex_atlas[vertexOffset + i].x = tex.x;
						mesh.vertex_atlas[vertexOffset + i].y = tex.y;
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
		state.scene.names.Create(armatureEntity) = skin.name;
		state.scene.layers.Create(armatureEntity);
		TransformComponent& transform = state.scene.transforms.Create(armatureEntity);
		ArmatureComponent& armature = state.scene.armatures.Create(armatureEntity);

		state.armatureArray.push_back(armatureEntity);

		if (transform_to_LH)
		{
			XMStoreFloat4x4(&armature.remapMatrix, XMMatrixScaling(1, 1, -1));
		}

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
		LoadNode(&state.gltfModel.nodes[gltfScene.nodes[i]], rootEntity, state);
	}

	// Create armature-bone mappings:
	int armatureIndex = 0;
	for (auto& skin : state.gltfModel.skins)
	{
		Entity entity = state.armatureArray[armatureIndex++];
		ArmatureComponent& armature = *state.scene.armatures.GetComponent(entity);

		const size_t jointCount = skin.joints.size();

		armature.boneCollection.resize(jointCount);

		// Create bone collection:
		for (size_t i = 0; i < jointCount; ++i)
		{
			int jointIndex = skin.joints[i];
			const tinygltf::Node& joint_node = state.gltfModel.nodes[jointIndex];

			Entity boneEntity = state.entityMap[&joint_node];

			armature.boneCollection[i] = boneEntity;
		}
	}

	// Create animations:
	for (auto& anim : state.gltfModel.animations)
	{
		Entity entity = CreateEntity();
		state.scene.names.Create(entity) = anim.name;
		AnimationComponent& animationcomponent = state.scene.animations.Create(entity);
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

			// AnimationSampler input = keyframe times
			{
				const tinygltf::Accessor& accessor = state.gltfModel.accessors[sam.input];
				const tinygltf::BufferView& bufferView = state.gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = state.gltfModel.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				int stride = accessor.ByteStride(bufferView);
				size_t count = accessor.count;

				animationcomponent.samplers[i].keyframe_times.resize(count);

				const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				assert(stride == 4);

				for (size_t j = 0; j < count; ++j)
				{
					float time = ((float*)data)[j];
					animationcomponent.samplers[i].keyframe_times[j] = time;
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
					animationcomponent.samplers[i].keyframe_data.resize(count * 3);
					for (size_t j = 0; j < count; ++j)
					{
						((XMFLOAT3*)animationcomponent.samplers[i].keyframe_data.data())[j] = ((XMFLOAT3*)data)[j];
					}
				}
				break;
				case TINYGLTF_TYPE_VEC4:
				{
					assert(stride == sizeof(XMFLOAT4));
					animationcomponent.samplers[i].keyframe_data.resize(count * 4);
					for (size_t j = 0; j < count; ++j)
					{
						((XMFLOAT4*)animationcomponent.samplers[i].keyframe_data.data())[j] = ((XMFLOAT4*)data)[j];
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

			animationcomponent.channels[i].target = state.entityMap[&state.gltfModel.nodes[channel.target_node]];
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
		TransformComponent& transform = *state.scene.transforms.GetComponent(rootEntity);
		transform.scale_local.z = -transform.scale_local.z;
		transform.SetDirty();
	}

	// We parented everything to a root transform, but we actually don't need that after loading model.
	//	Apply every transformation according to root transform, then remove root all together. 
	//	We could also keep it, but right now, it seems better to delete and have less hierarchy
	state.scene.Update(0);
	state.scene.Component_DetachChildren(rootEntity);
	state.scene.Entity_Remove(rootEntity);

	wiRenderer::GetScene().Merge(state.scene);

}
