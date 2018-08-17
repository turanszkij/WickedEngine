#include "stdafx.h"
#include "ModelImporter.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include <fstream>

using namespace std;
using namespace wiGraphicsTypes;

namespace tinygltf
{
	bool LoadImageData(Image *image, std::string *err, std::string *warn,
		int req_width, int req_height, const unsigned char *bytes,
		int size, void *)
	{
		if (!image->uri.empty())
		{
			// external image will be loaded by resource manager
			return true;
		}
		else
		{
			// embedded image
			assert(0); // TODO
			return false;
		}
	}

	bool WriteImageData(const std::string *basepath, const std::string *filename,
		Image *image, bool embedImages, void *)
	{
		assert(0); // TODO
		return false;
	}
}

Model* ImportModel_GLTF(const std::string& fileName)
{
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(name));
	wiHelper::RemoveExtensionFromFileName(name);



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
		material->roughness = 0.2f;
		material->metalness = 0.0f;
		material->reflectance = 0.2f;
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
			material->textureName = directory + img.uri;
			if (!material->textureName.empty())
				material->texture = (Texture2D*)wiResourceManager::GetGlobal()->add(material->textureName);
		}
		if (normalTexture != x.additionalValues.end())
		{
			auto& tex = gltfModel.textures[normalTexture->second.TextureIndex()];
			auto& img = gltfModel.images[tex.source];
			material->normalMapName = directory + img.uri;
			if (!material->normalMapName.empty())
				material->normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->normalMapName);
		}
		if (emissiveTexture != x.additionalValues.end())
		{
			auto& tex = gltfModel.textures[emissiveTexture->second.TextureIndex()];
			auto& img = gltfModel.images[tex.source];
			material->surfaceMapName = directory + img.uri;
			if (!material->surfaceMapName.empty())
				material->surfaceMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->surfaceMapName);
		}

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
			material->alphaRef = static_cast<float>(alphaCutoff->second.Factor());
		}

	}

	vector<Armature*> armatureArray;
	for (auto& skin : gltfModel.skins)
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
			XMMATRIX& w =
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

		// Final hierarchy and extra matrices created here:
		armature->CreateFamily();

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

	vector<Mesh*> meshArray;
	for (auto& x : gltfModel.meshes)
	{
		Mesh* mesh = new Mesh(x.name);

		meshArray.push_back(mesh);

		mesh->renderable = true;

		if (!armatureArray.empty())
		{
			mesh->armature = armatureArray[0]; // How to resolve?
			mesh->armatureName = mesh->armature->name;
		}

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
					// reorder indices:
					mesh->indices[offset + i + 0] = data[i + 0];
					mesh->indices[offset + i + 1] = data[i + 2];
					mesh->indices[offset + i + 2] = data[i + 1];
				}
			}
			else if (stride == 2)
			{
				for (size_t i = 0; i < count; i += 3)
				{
					// reorder indices:
					mesh->indices[offset + i + 0] = ((uint16_t*)data)[i + 0];
					mesh->indices[offset + i + 1] = ((uint16_t*)data)[i + 2];
					mesh->indices[offset + i + 2] = ((uint16_t*)data)[i + 1];
				}
			}
			else if (stride == 4)
			{
				for (size_t i = 0; i < count; i += 3)
				{
					// reorder indices:
					mesh->indices[offset + i + 0] = ((uint32_t*)data)[i + 0];
					mesh->indices[offset + i + 1] = ((uint32_t*)data)[i + 2];
					mesh->indices[offset + i + 2] = ((uint32_t*)data)[i + 1];
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
				subset.material = new Material("gltfLoader-defaultMat");
			}

			mesh->subsets.push_back(subset);
			mesh->materialNames.push_back(subset.material->name);
		}

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
						const XMFLOAT3& pos = ((XMFLOAT3*)data)[i];

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
						mesh->vertices_FULL[offset + i].nor.z = nor.z;
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
	}

	// Object transformations and mesh links and cameras:
	int camID = 0;
	for (auto& node : gltfModel.nodes)
	{
		if (node.mesh >= 0)
		{
			Object* object = new Object(node.name);
			model->objects.insert(object);

			object->mesh = meshArray[node.mesh];
			object->meshName = object->mesh->name;

			if (!node.scale.empty())
			{
				object->scale_rest = XMFLOAT3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);
			}
			if (!node.rotation.empty())
			{
				object->rotation_rest = XMFLOAT4((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
			}
			if (!node.translation.empty())
			{
				object->translation_rest = XMFLOAT3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
			}
		}
		if (node.camera >= 0)
		{
			Camera* camera = new Camera;
			camera->SetUp((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0.1f, 800);
			model->cameras.push_back(camera);

			camera->name = gltfModel.cameras[node.camera].name;
			if (camera->name.empty())
			{
				stringstream ss("");
				ss << "cam" << camID++;
				camera->name = ss.str();
			}

			if (!node.rotation.empty())
			{
				camera->rotation_rest = XMFLOAT4((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
			}
			if (!node.translation.empty())
			{
				camera->translation_rest = XMFLOAT3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
			}

			camera->UpdateProps();
		}
	}

	model->FinishLoading();

	//XMMATRIX rightHandedToLeftHanded = XMMatrixRotationX(-XM_PIDIV2) * XMMatrixRotationY(XM_PIDIV2);
	//this->transform(rightHandedToLeftHanded);

	return model;
}
