#include "stdafx.h"
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
using namespace wiSceneComponents;


// Transform the data from glTF space to engine-space:
const bool transform_to_LH = true;


namespace tinygltf
{
	bool LoadImageData(Image *image, std::string *err, std::string *warn,
		int req_width, int req_height, const unsigned char *bytes,
		int size, void *)
	{
		(void)warn;

		const int requiredComponents = 4;

		int w, h, comp;
		// if image cannot be decoded, ignore parsing and keep it by its path
		// don't break in this case
		// FIXME we should only enter this function if the image is embedded. If
		// image->uri references
		// an image file, it should be left as it is. Image loading should not be
		// mandatory (to support other formats)
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
		//image->component = comp;
		image->component = requiredComponents;
		image->image.resize(static_cast<size_t>(w * h * image->component));
		std::copy(data, data + w * h * image->component, image->image.begin());

		free(data);

		return true;


		//if (!image->uri.empty())
		//{
		//	// external image will be loaded by resource manager
		//	return true;
		//}
		//else
		//{
		//	// embedded image

		//	// We will load the texture2d by hand here and register to the resource manager
		//	{
		//		// png, tga, jpg, etc. loader:

		//		const int channelCount = 4;
		//		int width, height, bpp;
		//		unsigned char* rgb = stbi_load_from_memory(bytes, size, &width, &height, &bpp, channelCount);

		//		if (rgb != nullptr)
		//		{
		//			TextureDesc desc;
		//			desc.ArraySize = 1;
		//			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		//			desc.CPUAccessFlags = 0;
		//			desc.Format = FORMAT_R8G8B8A8_UNORM;
		//			desc.Height = static_cast<uint32_t>(height);
		//			desc.Width = static_cast<uint32_t>(width);
		//			desc.MipLevels = (UINT)log2(max(width, height));
		//			desc.MiscFlags = 0;
		//			desc.Usage = USAGE_DEFAULT;

		//			UINT mipwidth = width;
		//			SubresourceData* InitData = new SubresourceData[desc.MipLevels];
		//			for (UINT mip = 0; mip < desc.MipLevels; ++mip)
		//			{
		//				InitData[mip].pSysMem = rgb;
		//				InitData[mip].SysMemPitch = static_cast<UINT>(mipwidth * channelCount);
		//				mipwidth = max(1, mipwidth / 2);
		//			}

		//			Texture2D* tex = new Texture2D;
		//			tex->RequestIndependentShaderResourcesForMIPs(true);
		//			tex->RequestIndependentUnorderedAccessResourcesForMIPs(true);
		//			HRESULT hr = wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData, &tex);
		//			assert(SUCCEEDED(hr));

		//			if (tex != nullptr)
		//			{
		//				wiRenderer::AddDeferredMIPGen(tex);

		//				if (image->name.empty())
		//				{
		//					static UINT imgcounter = 0;
		//					stringstream ss("");
		//					ss << "gltfLoader_embedded_image" << imgcounter++;
		//					image->name = ss.str();
		//				}
		//				// We loaded the texture2d, so register to the resource manager to be retrieved later:
		//				wiResourceManager::GetGlobal()->Register(image->name, tex, wiResourceManager::IMAGE);
		//			}
		//		}

		//		free(rgb);
		//	}

		//	return true;
		//}

		//return false;
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

				if (image->name.empty())
				{
					static UINT imgcounter = 0;
					stringstream ss("");
					ss << "gltfLoader_image" << imgcounter++;
					image->name = ss.str();
				}
				// We loaded the texture2d, so register to the resource manager to be retrieved later:
				wiResourceManager::GetGlobal()->Register(image->name, tex, wiResourceManager::IMAGE);
			}
		}
	}
}

void LoadNode(tinygltf::Node* node, tinygltf::Node* parent, Model* model, tinygltf::Model& gltfModel, vector<Mesh*>& meshArray, vector<Armature*>& armatureArray)
{
	if (node == nullptr)
	{
		return;
	}

	Transform transform;
	if (!node->scale.empty())
	{
		transform.scale_rest = XMFLOAT3((float)node->scale[0], (float)node->scale[1], (float)node->scale[2]);
	}
	if (!node->rotation.empty())
	{
		transform.rotation_rest = XMFLOAT4((float)node->rotation[0], (float)node->rotation[1], (float)node->rotation[2], (float)node->rotation[3]);
	}
	if (!node->translation.empty())
	{
		transform.translation_rest = XMFLOAT3((float)node->translation[0], (float)node->translation[1], (float)node->translation[2]);
	}
	transform.UpdateTransform();

	if (parent != nullptr)
	{
		transform.parentName = parent->name;
	}

	if(node->mesh >= 0)
	{
		Object* object = new Object(node->name);
		model->objects.insert(object);

		*(Transform*)object = transform;

		if (node->mesh < meshArray.size())
		{
			object->mesh = meshArray[node->mesh];
			object->meshName = object->mesh->name;
		}
		else
		{
			auto& x = gltfModel.meshes[node->mesh];
			Mesh* mesh = new Mesh(x.name);
			meshArray.push_back(mesh);

			object->mesh = mesh;
			object->meshName = mesh->name;

			mesh->renderable = true;

			XMFLOAT3 min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
			XMFLOAT3 max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			for (auto& prim : x.primitives)
			{
				assert(prim.indices >= 0);

				// Fill indices:
				const tinygltf::Accessor& accessor = gltfModel.accessors[prim.indices];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				int stride = accessor.ByteStride(bufferView);
				size_t count = accessor.count;

				size_t offset = mesh->indices.size();
				mesh->indices.resize(offset + count);

				const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				if (stride == 1)
				{
					for (size_t i = 0; i < count; i += 3)
					{
						mesh->indices[offset + i + 0] = data[i + 0];
						mesh->indices[offset + i + 1] = data[i + 1];
						mesh->indices[offset + i + 2] = data[i + 2];
					}
				}
				else if (stride == 2)
				{
					for (size_t i = 0; i < count; i += 3)
					{
						mesh->indices[offset + i + 0] = ((uint16_t*)data)[i + 0];
						mesh->indices[offset + i + 1] = ((uint16_t*)data)[i + 1];
						mesh->indices[offset + i + 2] = ((uint16_t*)data)[i + 2];
					}
				}
				else if (stride == 4)
				{
					for (size_t i = 0; i < count; i += 3)
					{
						mesh->indices[offset + i + 0] = ((uint32_t*)data)[i + 0];
						mesh->indices[offset + i + 1] = ((uint32_t*)data)[i + 1];
						mesh->indices[offset + i + 2] = ((uint32_t*)data)[i + 2];
					}
				}
				else
				{
					assert(0 && "unsupported index stride!");
				}


				// Create mesh subset:
				MeshSubset subset;

				if (prim.material >= 0)
				{
					const string& mat_name = gltfModel.materials[prim.material].name;
					auto& found_mat = model->materials.find(mat_name);
					if (found_mat != model->materials.end())
					{
						subset.material = found_mat->second;
					}
				}

				if (subset.material == nullptr)
				{
					subset.material = new Material("gltfLoader_defaultMat");
				}

				mesh->subsets.push_back(subset);
				mesh->materialNames.push_back(subset.material->name);
			}

			bool hasBoneWeights = false;
			bool hasBoneIndices = false;

			int matIndex = -1;
			for (auto& prim : x.primitives)
			{
				matIndex++;
				size_t offset = mesh->vertices_FULL.size();

				for (auto& attr : prim.attributes)
				{
					const string& attr_name = attr.first;
					int attr_data = attr.second;

					const tinygltf::Accessor& accessor = gltfModel.accessors[attr_data];
					const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

					int stride = accessor.ByteStride(bufferView);
					size_t count = accessor.count;

					if (mesh->vertices_FULL.size() == offset)
					{
						mesh->vertices_FULL.resize(offset + count);
					}

					const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

					if (!attr_name.compare("POSITION"))
					{
						assert(stride == 12);
						for (size_t i = 0; i < count; ++i)
						{
							XMFLOAT3 pos = ((XMFLOAT3*)data)[i];

							if (transform_to_LH)
							{
								pos.z = -pos.z;
							}

							mesh->vertices_FULL[offset + i].pos = XMFLOAT4(pos.x, pos.y, pos.z, 0);

							min = wiMath::Min(min, pos);
							max = wiMath::Max(max, pos);
						}
					}
					else if (!attr_name.compare("NORMAL"))
					{
						assert(stride == 12);
						for (size_t i = 0; i < count; ++i)
						{
							const XMFLOAT3& nor = ((XMFLOAT3*)data)[i];

							mesh->vertices_FULL[offset + i].nor.x = nor.x;
							mesh->vertices_FULL[offset + i].nor.y = nor.y;
							mesh->vertices_FULL[offset + i].nor.z = -nor.z;
						}
					}
					else if (!attr_name.compare("TEXCOORD_0"))
					{
						assert(stride == 8);
						for (size_t i = 0; i < count; ++i)
						{
							const XMFLOAT2& tex = ((XMFLOAT2*)data)[i];

							mesh->vertices_FULL[offset + i].tex.x = tex.x;
							mesh->vertices_FULL[offset + i].tex.y = tex.y;
							mesh->vertices_FULL[offset + i].tex.z = (float)matIndex /*prim.material*/;
						}
					}
					else if (!attr_name.compare("JOINTS_0"))
					{
						if (stride == 4)
						{
							hasBoneIndices = true;
							struct JointTmp
							{
								uint8_t ind[4];
							};

							for (size_t i = 0; i < count; ++i)
							{
								const JointTmp& joint = ((JointTmp*)data)[i];

								mesh->vertices_FULL[offset + i].ind.x = (float)joint.ind[0];
								mesh->vertices_FULL[offset + i].ind.y = (float)joint.ind[1];
								mesh->vertices_FULL[offset + i].ind.z = (float)joint.ind[2];
								mesh->vertices_FULL[offset + i].ind.w = (float)joint.ind[3];
							}
						}
						else if (stride == 8)
						{
							hasBoneIndices = true;
							struct JointTmp
							{
								uint16_t ind[4];
							};

							for (size_t i = 0; i < count; ++i)
							{
								const JointTmp& joint = ((JointTmp*)data)[i];

								mesh->vertices_FULL[offset + i].ind.x = (float)joint.ind[0];
								mesh->vertices_FULL[offset + i].ind.y = (float)joint.ind[1];
								mesh->vertices_FULL[offset + i].ind.z = (float)joint.ind[2];
								mesh->vertices_FULL[offset + i].ind.w = (float)joint.ind[3];
							}
						}
						else
						{
							assert(0);
						}
					}
					else if (!attr_name.compare("WEIGHTS_0"))
					{
						hasBoneWeights = true;
						assert(stride == 16);
						for (size_t i = 0; i < count; ++i)
						{
							mesh->vertices_FULL[offset + i].wei = ((XMFLOAT4*)data)[i];
						}
					}

				}

			}

			mesh->aabb.create(min, max);

			model->meshes.insert(make_pair(mesh->name, mesh));


			if (!armatureArray.empty() && hasBoneIndices && hasBoneWeights)
			{
				mesh->armature = armatureArray[0]; // How to resolve?
				mesh->armatureName = mesh->armature->name;
			}

		}
	}

	if (node->camera >= 0)
	{
		Camera* camera = new Camera;
		camera->SetUp((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 800);
		model->cameras.push_back(camera);

		*(Transform*)camera = transform;

		camera->name = gltfModel.cameras[node->camera].name;
		if (camera->name.empty())
		{
			static int camID = 0;
			stringstream ss("");
			ss << "cam" << camID++;
			camera->name = ss.str();
		}
		node->name = camera->name;

		camera->UpdateProps();
	}

	if (!node->children.empty())
	{
		for (int child : node->children)
		{
			LoadNode(&gltfModel.nodes[child], node, model, gltfModel, meshArray, armatureArray);
		}
	}
}

Model* ImportModel_GLTF(const std::string& fileName)
{
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(name));
	wiHelper::RemoveExtensionFromFileName(name);


	vector<Armature*> armatureArray;
	vector<Mesh*> meshArray;


	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
	loader.SetImageWriter(tinygltf::WriteImageData, nullptr);

	bool ret;
	if (!extension.compare("GLTF"))
	{
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, fileName);
	}
	else
	{
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, fileName); // for binary glTF(.glb) 
	}
	if (!ret) {
		wiHelper::messageBox(err, "GLTF error!");
		return nullptr;
	}

	Model* model = new Model;
	model->name = name;

	for (auto& x : gltfModel.materials)
	{
		Material* material = new Material(x.name);
		model->materials.insert(make_pair(material->name, material));

		material->baseColor = XMFLOAT3(1, 1, 1);
		material->roughness = 1.0f;
		material->metalness = 1.0f;
		material->reflectance = 0.02f;
		material->emissive = 0;

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
			auto& tex = gltfModel.textures[baseColorTexture->second.TextureIndex()];
			auto& img = gltfModel.images[tex.source];
			RegisterTexture2D(&img);
			material->textureName = img.name;
		}
		else if(!gltfModel.images.empty())
		{
			// For some reason, we don't have diffuse texture, but have other textures
			//	I have a problem, because one model viewer displays textures on a model which has no basecolor set in its material...
			//	This is probably not how it should be (todo)
			RegisterTexture2D(&gltfModel.images[0]);
			material->textureName = gltfModel.images[0].name;
		}

		tinygltf::Image* img_nor = nullptr;
		tinygltf::Image* img_met_rough = nullptr;
		tinygltf::Image* img_emissive = nullptr;

		if (normalTexture != x.additionalValues.end())
		{
			auto& tex = gltfModel.textures[normalTexture->second.TextureIndex()];
			img_nor = &gltfModel.images[tex.source];
		}
		if (metallicRoughnessTexture != x.values.end())
		{
			auto& tex = gltfModel.textures[metallicRoughnessTexture->second.TextureIndex()];
			img_met_rough = &gltfModel.images[tex.source];
		}
		if (emissiveTexture != x.additionalValues.end())
		{
			auto& tex = gltfModel.textures[emissiveTexture->second.TextureIndex()];
			img_emissive = &gltfModel.images[tex.source];
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
			material->normalMapName = img_nor->name;
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
			material->surfaceMapName = img_met_rough->name;
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
				material->surfaceMapName = img_emissive->name;
			}
		}

		// Retrieve textures by name:
		if (!material->textureName.empty())
			material->texture = (Texture2D*)wiResourceManager::GetGlobal()->add(material->textureName);
		if (!material->normalMapName.empty())
			material->normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->normalMapName);
		if (!material->surfaceMapName.empty())
			material->surfaceMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->surfaceMapName);

		if (baseColorFactor != x.values.end())
		{
			material->baseColor.x = static_cast<float>(baseColorFactor->second.ColorFactor()[0]);
			material->baseColor.y = static_cast<float>(baseColorFactor->second.ColorFactor()[1]);
			material->baseColor.z = static_cast<float>(baseColorFactor->second.ColorFactor()[2]);
		}
		if (roughnessFactor != x.values.end())
		{
			material->roughness = static_cast<float>(roughnessFactor->second.Factor());
		}
		if (metallicFactor != x.values.end())
		{
			material->metalness = static_cast<float>(metallicFactor->second.Factor());
		}
		if (emissiveFactor != x.additionalValues.end())
		{
			material->emissive = static_cast<float>(emissiveFactor->second.ColorFactor()[0]);
		}
		if (alphaCutoff != x.additionalValues.end())
		{
			material->alphaRef = 1 - static_cast<float>(alphaCutoff->second.Factor());
		}

	}

	for(auto& skin : gltfModel.skins)
	{
		Armature* armature = new Armature(skin.name);
		model->armatures.insert(armature);

		armatureArray.push_back(armature);

		const tinygltf::Node& skeleton_node = gltfModel.nodes[skin.skeleton];

		const size_t jointCount = skin.joints.size();

		armature->boneCollection.resize(jointCount);

		// Create bone collection:
		for (size_t i = 0; i < jointCount; ++i)
		{
			int jointIndex = skin.joints[i];
			const tinygltf::Node& joint_node = gltfModel.nodes[jointIndex];

			Bone* bone = new Bone(joint_node.name);
			if (bone->name.empty())
			{
				// GLTF might not contain bone names...
				stringstream ss("");
				ss << "Bone_" << i;
				bone->name = ss.str();
			}

			armature->boneCollection[i] = bone;

			if (!joint_node.scale.empty())
			{
				bone->scale_rest = XMFLOAT3((float)joint_node.scale[0], (float)joint_node.scale[1], (float)joint_node.scale[2]);
			}
			if (!joint_node.rotation.empty())
			{
				bone->rotation_rest = XMFLOAT4((float)joint_node.rotation[0], (float)joint_node.rotation[1], (float)joint_node.rotation[2], (float)joint_node.rotation[3]);
			}
			if (!joint_node.translation.empty())
			{
				bone->translation_rest = XMFLOAT3((float)joint_node.translation[0], (float)joint_node.translation[1], (float)joint_node.translation[2]);
			}

			XMVECTOR s = XMLoadFloat3(&bone->scale_rest);
			XMVECTOR r = XMLoadFloat4(&bone->rotation_rest);
			XMVECTOR t = XMLoadFloat3(&bone->translation_rest);
			XMMATRIX w =
				XMMatrixScalingFromVector(s)*
				XMMatrixRotationQuaternion(r)*
				XMMatrixTranslationFromVector(t)
				;
			XMStoreFloat4x4(&bone->world_rest, w);
		}

		// Create bone name hierarchy:
		for (size_t i = 0; i < jointCount; ++i)
		{
			int jointIndex = skin.joints[i];
			const tinygltf::Node& joint_node = gltfModel.nodes[jointIndex];

			for (int childJointIndex : joint_node.children)
			{
				for (size_t j = 0; j < jointCount; ++j)
				{
					if (skin.joints[j] == childJointIndex)
					{
						armature->boneCollection[j]->parentName = armature->boneCollection[i]->name;
						break;
					}
				}
			}
		}

		if (transform_to_LH)
		{
			XMStoreFloat4x4(&armature->skinningRemap, XMMatrixScaling(1, 1, -1));
		}

		// Final hierarchy and extra matrices created here:
		armature->CreateFamily();

	}

	const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene];
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		LoadNode(&gltfModel.nodes[scene.nodes[i]], nullptr, model, gltfModel, meshArray, armatureArray);
	}

	int animID = 0;
	for (auto& anim : gltfModel.animations)
	{
		if (armatureArray.empty())
		{
			break;
		}
		Armature* armature = armatureArray[0];

		for (Bone* bone : armature->boneCollection)
		{
			bone->actionFrames.push_back(ActionFrames());
		}

		Action action;
		action.name = anim.name;
		if (action.name.empty())
		{
			stringstream ss("");
			ss << "Action_" << animID++;
			action.name = ss.str();
		}

		for (auto& channel : anim.channels)
		{
			const tinygltf::Node& target_node = gltfModel.nodes[channel.target_node];
			const tinygltf::AnimationSampler& sam = anim.samplers[channel.sampler];

			Bone* bone = nullptr;

			// Search for the armature + bone this animation belongs to:
			{
				const auto& skin = gltfModel.skins[0];

				const size_t jointCount = skin.joints.size();
				assert(armature->boneCollection.size() == jointCount);

				for (size_t i = 0; i < jointCount; ++i)
				{
					int jointIndex = skin.joints[i];

					if (jointIndex == channel.target_node)
					{
						bone = armature->boneCollection[i];
						break;
					}
				}
			}

			if (bone == nullptr)
			{
				assert(0 && "Corresponding bone not found!");
				continue;
			}


			vector<KeyFrame> keyframes;

			// AnimationSampler input = keyframe times
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[sam.input];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				int stride = accessor.ByteStride(bufferView);
				size_t count = accessor.count;

				keyframes.resize(count);

				const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				int firstFrame = INT_MAX;

				assert(stride == 4);
				for (size_t i = 0; i < count; ++i)
				{
					keyframes[i].frameI = (int)(((float*)data)[i] * 60); // !!! converting from time-base to frame-based !!!

					action.frameCount = max(action.frameCount, keyframes[i].frameI);
					firstFrame = min(firstFrame, keyframes[i].frameI);
				}

				// Cut out the empty part of the animation at the beginning:
				firstFrame = min(firstFrame, action.frameCount);
				for (size_t i = 0; i < count; ++i)
				{
					keyframes[i].frameI -= firstFrame;
				}
				action.frameCount -= firstFrame;

			}

			// AnimationSampler output = keyframe data
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[sam.output];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				int stride = accessor.ByteStride(bufferView);
				size_t count = accessor.count;

				// Unfortunately, GLTF stores absolute values for animation nodes, but the engine needs relative
				//	Absolute = animation * rest (so the rest matrix is baked into animation, this can't be blended like we do now)
				//	Relative = animation (so we can blend all animation tracks however we want, then post multiply with the rest matrix after blending)
				const XMMATRIX invRest = XMMatrixInverse(nullptr, XMLoadFloat4x4(&bone->world_rest));

				const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				if (!channel.target_path.compare("scale"))
				{
					assert(stride == sizeof(XMFLOAT3));
					for (size_t i = 0; i < count; ++i)
					{
						const XMFLOAT3& sca = ((XMFLOAT3*)data)[i];
						//keyframes[i].data = XMFLOAT4(sca.x, sca.y, sca.z, 0);

						// Remove rest matrix from animation track:
						XMMATRIX mat = XMMatrixScalingFromVector(XMLoadFloat3(&sca));
						mat = mat * invRest;
						XMVECTOR s, r, t;
						XMMatrixDecompose(&s, &r, &t, mat);
						XMStoreFloat4(&keyframes[i].data, s);
					}
					bone->actionFrames.back().keyframesSca.insert(bone->actionFrames.back().keyframesSca.end(), keyframes.begin(), keyframes.end());
				}
				else if (!channel.target_path.compare("rotation"))
				{
					assert(stride == sizeof(XMFLOAT4));
					for (size_t i = 0; i < count; ++i)
					{
						const XMFLOAT4& rot = ((XMFLOAT4*)data)[i];
						//keyframes[i].data = rot;

						// Remove rest matrix from animation track:
						XMMATRIX mat = XMMatrixRotationQuaternion(XMLoadFloat4(&rot));
						mat = mat * invRest;
						XMVECTOR s, r, t;
						XMMatrixDecompose(&s, &r, &t, mat);
						XMStoreFloat4(&keyframes[i].data, r);
					}
					bone->actionFrames.back().keyframesRot.insert(bone->actionFrames.back().keyframesRot.end(), keyframes.begin(), keyframes.end());
				}
				else if (!channel.target_path.compare("translation"))
				{
					assert(stride == sizeof(XMFLOAT3));
					for (size_t i = 0; i < count; ++i)
					{
						const XMFLOAT3& tra = ((XMFLOAT3*)data)[i];
						//keyframes[i].data = XMFLOAT4(tra.x, tra.y, tra.z, 1);

						// Remove rest matrix from animation track:
						XMMATRIX mat = XMMatrixTranslationFromVector(XMLoadFloat3(&tra));
						mat = mat * invRest;
						XMVECTOR s, r, t;
						XMMatrixDecompose(&s, &r, &t, mat);
						XMStoreFloat4(&keyframes[i].data, t);
					}
					bone->actionFrames.back().keyframesPos.insert(bone->actionFrames.back().keyframesPos.end(), keyframes.begin(), keyframes.end());
				}
				else
				{
					assert(0);
				}
			}


		}

		armature->actions.push_back(action);

	}

	model->FinishLoading();

	return model;
}
