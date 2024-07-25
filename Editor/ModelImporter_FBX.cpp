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

void FlipZAxis(Scene& scene, Entity rootEntity, wi::unordered_map<size_t, TransformComponent>& transforms_original);
void Import_Mixamo_Bone(Scene& scene, Entity rootEntity, Entity boneEntity);

// File callbacks are implemented for platforms that don't support default c++ filesystem correctly (like UWP)
struct FileDataStream
{
	wi::vector<uint8_t> data;
	size_t offset = 0;
};
size_t read_cb(void* user, void* data, size_t size)
{
	FileDataStream* datastream = (FileDataStream*)user;
	uint8_t* src = datastream->data.data() + datastream->offset;
	size = std::min(size, datastream->data.size() - datastream->offset);
	std::memcpy(data, src, size);
	datastream->offset += size;
	return size;
}
bool skip_cb(void* user, size_t size)
{
	FileDataStream* datastream = (FileDataStream*)user;
	size = std::min(size, datastream->data.size() - datastream->offset);
	datastream->offset += size;
	return true;
}
void close_cb(void* user)
{
	FileDataStream* datastream = (FileDataStream*)user;
	delete datastream;
}
bool open_file_cb(void* user, ufbx_stream* stream, const char* path, size_t path_len, const ufbx_open_file_info* info)
{
	wi::vector<uint8_t> filedata;
	if (wi::helper::FileRead(path, filedata))
	{
		FileDataStream* datastream = new FileDataStream;
		datastream->data = std::move(filedata);
		stream->user = datastream;
		stream->read_fn = read_cb;
		stream->skip_fn = skip_cb;
		stream->close_fn = close_cb;
		return true;
	}
	return false;
}

void ImportModel_FBX(const std::string& filename, wi::scene::Scene& scene)
{
	ufbx_load_opts opts = {};
	//opts.target_axes = ufbx_axes_left_handed_y_up;
	opts.target_unit_meters = 1.0f;
	//opts.target_camera_axes = ufbx_axes_left_handed_y_up;
	//opts.target_light_axes = ufbx_axes_left_handed_y_up;
	//opts.space_conversion = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT;
	//opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	opts.generate_missing_normals = true;
	opts.open_file_cb.fn = open_file_cb;
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
	wi::unordered_map<const ufbx_skin_deformer*, Entity> skin_lookup;
	wi::unordered_map<const ufbx_mesh*, Entity> mesh_lookup;
	wi::unordered_map<const ufbx_node*, Entity> node_lookup;

	Entity rootEntity = CreateEntity();
	{
		scene.names.Create(rootEntity) = wi::helper::GetFileNameFromPath(filename);
		const ufbx_node* node = fbxscene->root_node;
		node_lookup[node] = rootEntity;
		TransformComponent& transform = scene.transforms.Create(rootEntity);
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
	}

	wi::vector<wi::Resource> embedded_resources;
	auto preload_embedded_texture = [&](ufbx_texture* texture) {
		if (texture->content.data == nullptr)
			return;

		std::string filename = texture->filename.data;
		if (filename.empty())
		{
			// Force some image resource name:
			do {
				filename.clear();
				filename += "fbximport_" + std::to_string(wi::random::GetRandom(std::numeric_limits<uint32_t>::max())) + ".png";
			} while (wi::resourcemanager::Contains(filename)); // this is to avoid overwriting an existing imported image
		}

		auto resource = wi::resourcemanager::Load(
			filename,
			wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA | wi::resourcemanager::Flags::IMPORT_DELAY,
			(const uint8_t*)texture->content.data,
			texture->content.size
		);

		if (!resource.IsValid())
			return;
		embedded_resources.push_back(resource); //retain embedded resource for whole import session
	};

	for (const ufbx_material* material : fbxscene->materials)
	{
		Entity entity = CreateEntity();
		scene.Component_Attach(entity, rootEntity);
		material_lookup[material] = entity;
		if (material->name.length > 0)
		{
			scene.names.Create(entity) = material->name.data;
		}
		MaterialComponent& materialcomponent = scene.materials.Create(entity);
		materialcomponent.SetDoubleSided(material->features.double_sided.enabled);
		materialcomponent.baseColor = XMFLOAT4(material->pbr.base_color.value_vec4.v);
		if (material->pbr.base_color.texture != nullptr)
		{
			materialcomponent.textures[MaterialComponent::BASECOLORMAP].name = material->pbr.base_color.texture->filename.data;
			preload_embedded_texture(material->pbr.base_color.texture);
		}
		if (material->pbr.normal_map.texture != nullptr)
		{
			materialcomponent.textures[MaterialComponent::NORMALMAP].name = material->pbr.normal_map.texture->filename.data;
			preload_embedded_texture(material->pbr.normal_map.texture);
		}
		if (material->fbx.specular_factor.texture != nullptr) // not from pbr.
		{
			materialcomponent.textures[MaterialComponent::SPECULARMAP].name = material->fbx.specular_factor.texture->filename.data;
			preload_embedded_texture(material->fbx.specular_factor.texture);
		}
		if (material->pbr.displacement_map.texture != nullptr)
		{
			materialcomponent.textures[MaterialComponent::DISPLACEMENTMAP].name = material->pbr.displacement_map.texture->filename.data;
			preload_embedded_texture(material->pbr.displacement_map.texture);
			if (material->pbr.displacement_map.has_value)
			{
				materialcomponent.displacementMapping = material->pbr.displacement_map.value_real;
			}
		}
		if (material->pbr.ambient_occlusion.texture != nullptr)
		{
			materialcomponent.textures[MaterialComponent::OCCLUSIONMAP].name = material->pbr.ambient_occlusion.texture->filename.data;
			preload_embedded_texture(material->pbr.ambient_occlusion.texture);
			materialcomponent.SetOcclusionEnabled_Secondary(true);
		}
		if (material->pbr.emission_color.texture != nullptr)
		{
			materialcomponent.textures[MaterialComponent::EMISSIVEMAP].name = material->pbr.emission_color.texture->filename.data;
			preload_embedded_texture(material->pbr.emission_color.texture);
		}

		if (
			material->shader_type == UFBX_SHADER_3DS_MAX_PBR_METAL_ROUGH ||
			material->shader_type == UFBX_SHADER_GLTF_MATERIAL
			)
		{
			materialcomponent.roughness = material->pbr.roughness.value_real;
			materialcomponent.metalness = material->pbr.metalness.value_real;
			materialcomponent.reflectance = material->pbr.specular_ior.value_real;
		}

		if (
			material->shader_type == UFBX_SHADER_3DS_MAX_PBR_SPEC_GLOSS
			)
		{
			materialcomponent.SetUseSpecularGlossinessWorkflow(true);
		}

		if (material->pbr.emission_color.has_value)
		{
			materialcomponent.emissiveColor = XMFLOAT4(material->pbr.emission_color.value_vec4.v);
		}
		if (material->pbr.emission_factor.has_value)
		{
			materialcomponent.emissiveColor.w = material->pbr.emission_factor.value_real;
		}

		materialcomponent.CreateRenderData();
	}

	if (fbxscene->materials.count == 0)
	{
		scene.Entity_CreateMaterial("fbximport_defaultmaterial");
	}

	for (const ufbx_skin_deformer* skin : fbxscene->skin_deformers)
	{
		Entity entity = CreateEntity();
		scene.Component_Attach(entity, rootEntity);
		skin_lookup[skin] = entity;
		if (skin->name.length > 0)
		{
			scene.names.Create(entity) = skin->name.data;
		}
		scene.armatures.Create(entity);
		scene.transforms.Create(entity);
	}

	for (const ufbx_mesh* mesh : fbxscene->meshes)
	{
		Entity entity = CreateEntity();
		scene.Component_Attach(entity, rootEntity);
		mesh_lookup[mesh] = entity;
		if (mesh->name.length > 0)
		{
			scene.names.Create(entity) = mesh->name.data;
		}
		MeshComponent& meshcomponent = scene.meshes.Create(entity);
		uint32_t vertexOffset = 0;
		wi::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);
		const ufbx_skin_deformer* skin = nullptr;
		if (mesh->skin_deformers.count > 0)
		{
			skin = mesh->skin_deformers[0];
			meshcomponent.armatureID = skin_lookup[skin];
		}
		for (const ufbx_blend_deformer* deformer : mesh->blend_deformers)
		{
			meshcomponent.morph_targets.resize(deformer->channels.count);
			for (size_t i = 0; i < deformer->channels.count; ++i)
			{
				ufbx_blend_channel* channel = deformer->channels[i];
				if (channel == nullptr)
					continue;
				meshcomponent.morph_targets[i].weight = channel->weight;
			}
		}
		for (const ufbx_mesh_part& part : mesh->material_parts)
		{
			wi::vector<XMFLOAT3> positions;
			wi::vector<XMFLOAT3> normals;
			wi::vector<XMFLOAT2> uvset0;
			wi::vector<XMFLOAT2> uvset1;
			wi::vector<wi::Color> colors;
			wi::vector<XMUINT4> boneindices;
			wi::vector<XMFLOAT4> boneweights;
			wi::vector<XMUINT4> boneindices2;
			wi::vector<XMFLOAT4> boneweights2;
			wi::vector<MeshComponent::MorphTarget> morphs(meshcomponent.morph_targets.size());

			for (uint32_t face_index : part.face_indices)
			{
				ufbx_face face = mesh->faces[face_index];
				uint32_t num_tris = ufbx_triangulate_face(tri_indices.data(), tri_indices.size(), mesh, face);
				for (size_t i = 0; i < num_tris * 3; i++)
				{
					const uint32_t index = tri_indices[i];
					const uint32_t vertex = mesh->vertex_indices[index];

					if (mesh->vertex_position.exists)
					{
						positions.push_back(XMFLOAT3(mesh->vertex_position[index].v));
					}
					if (mesh->vertex_normal.exists)
					{
						normals.push_back(XMFLOAT3(mesh->vertex_normal[index].v));
					}
					if (mesh->vertex_uv.exists)
					{
						uvset0.push_back(XMFLOAT2(mesh->vertex_uv[index].v));
						uvset0.back().y = 1 - uvset0.back().y;
					}
					if (mesh->uv_sets.count > 1)
					{
						uvset1.push_back(XMFLOAT2(mesh->uv_sets[1].vertex_uv[index].v));
						uvset1.back().y = 1 - uvset1.back().y;
					}
					if (mesh->vertex_color.exists)
					{
						colors.push_back(wi::Color::fromFloat4(XMFLOAT4(mesh->vertex_color[index].v)));
					}

					if (skin)
					{
						boneindices.emplace_back();
						boneweights.emplace_back();

						ufbx_skin_vertex skin_vertex = skin->vertices[vertex];
						uint32_t num_weights = skin_vertex.num_weights;
						num_weights = std::min(num_weights, 8u);

						if (num_weights > 4)
						{
							boneindices2.resize(boneindices.size());
							boneweights2.resize(boneweights.size());
						}

						for (uint32_t i = 0; i < num_weights; ++i)
						{
							ufbx_skin_weight skin_weight = skin->weights[skin_vertex.weight_begin + i];
							if (i < 4)
							{
								(&boneindices.back().x)[i] = skin_weight.cluster_index;
								(&boneweights.back().x)[i] = skin_weight.weight;
							}
							else
							{
								(&boneindices2.back().x)[i] = skin_weight.cluster_index;
								(&boneweights2.back().x)[i] = skin_weight.weight;
							}
						}
						// Note: normalization of bone weights will be done in MeshComponent::CreateRenderData()
					}

					for (const ufbx_blend_deformer* deformer : mesh->blend_deformers)
					{
						for (size_t i = 0; i < deformer->channels.count; ++i)
						{
							ufbx_blend_channel* channel = deformer->channels[i];
							if (channel == nullptr)
								continue;
							ufbx_blend_shape* shape = channel->target_shape;
							if (shape == nullptr)
								continue;
							ufbx_vec3 vertex_offset = ufbx_get_blend_shape_vertex_offset(shape, vertex);
							morphs[i].vertex_positions.push_back(XMFLOAT3(vertex_offset.v));
						}
					}
				}
			}
			assert(positions.size() == part.num_triangles * 3);

			wi::vector<uint32_t> indices;
			indices.resize(part.num_triangles * 3);

			wi::vector<ufbx_vertex_stream> streams;
			if (!positions.empty())
			{
				streams.push_back({ positions.data(), positions.size(), sizeof(positions[0]) });
			}
			if (!normals.empty())
			{
				streams.push_back({ normals.data(), normals.size(), sizeof(normals[0]) });
			}
			if (!uvset0.empty())
			{
				streams.push_back({ uvset0.data(), uvset0.size(), sizeof(uvset0[0]) });
			}
			if (!uvset1.empty())
			{
				streams.push_back({ uvset1.data(), uvset1.size(), sizeof(uvset1[0]) });
			}
			if (!colors.empty())
			{
				streams.push_back({ colors.data(), colors.size(), sizeof(colors[0]) });
			}
			if (!boneindices.empty())
			{
				streams.push_back({ boneindices.data(), boneindices.size(), sizeof(boneindices[0]) });
			}
			if (!boneweights.empty())
			{
				streams.push_back({ boneweights.data(), boneweights.size(), sizeof(boneweights[0]) });
			}
			assert(boneindices.size() == boneweights.size());
			if (!boneindices2.empty())
			{
				boneindices2.resize(boneindices.size()); // ensure same size as first bone stream, we only resized so far on demand
				streams.push_back({ boneindices2.data(), boneindices2.size(), sizeof(boneindices2[0]) });
			}
			if (!boneweights2.empty())
			{
				boneweights2.resize(boneweights.size()); // ensure same size as first bone stream, we only resized so far on demand
				streams.push_back({ boneweights2.data(), boneweights2.size(), sizeof(boneweights2[0]) });
			}
			assert(boneindices2.size() == boneweights2.size());
			for (MeshComponent::MorphTarget& morph : morphs)
			{
				streams.push_back({ morph.vertex_positions.data(), morph.vertex_positions.size(), sizeof(morph.vertex_positions[0]) });
			}

			const size_t num_vertices = ufbx_generate_indices(streams.data(), streams.size(), indices.data(), indices.size(), nullptr, nullptr);

			MeshComponent::MeshSubset& subset = meshcomponent.subsets.emplace_back();
			subset.indexOffset = (uint32_t)meshcomponent.indices.size();
			subset.indexCount = uint32_t(indices.size());
			if (material_lookup.empty())
			{
				if (scene.materials.GetCount() > 0)
				{
					subset.materialID = scene.materials.GetEntity(0);
				}
			}
			else
			{
				subset.materialID = material_lookup[mesh->materials[part.index]];
			}

			for (uint32_t index : indices)
			{
				meshcomponent.indices.push_back(vertexOffset + index);
			}
			vertexOffset += (uint32_t)num_vertices;

			if (!positions.empty())
			{
				positions.resize(num_vertices);
				meshcomponent.vertex_positions.insert(meshcomponent.vertex_positions.end(), positions.begin(), positions.end());
			}
			if (!normals.empty())
			{
				normals.resize(num_vertices);
				meshcomponent.vertex_normals.insert(meshcomponent.vertex_normals.end(), normals.begin(), normals.end());
			}
			if (!uvset0.empty())
			{
				uvset0.resize(num_vertices);
				meshcomponent.vertex_uvset_0.insert(meshcomponent.vertex_uvset_0.end(), uvset0.begin(), uvset0.end());
			}
			if (!uvset1.empty())
			{
				uvset1.resize(num_vertices);
				meshcomponent.vertex_uvset_1.insert(meshcomponent.vertex_uvset_1.end(), uvset1.begin(), uvset1.end());
			}
			if (!colors.empty())
			{
				colors.resize(num_vertices);
				meshcomponent.vertex_colors.insert(meshcomponent.vertex_colors.end(), colors.begin(), colors.end());
			}
			if (!boneindices.empty())
			{
				boneindices.resize(num_vertices);
				meshcomponent.vertex_boneindices.insert(meshcomponent.vertex_boneindices.end(), boneindices.begin(), boneindices.end());
			}
			if (!boneweights.empty())
			{
				boneweights.resize(num_vertices);
				meshcomponent.vertex_boneweights.insert(meshcomponent.vertex_boneweights.end(), boneweights.begin(), boneweights.end());
			}
			if (!boneindices2.empty())
			{
				boneindices2.resize(num_vertices);
				meshcomponent.vertex_boneindices2.insert(meshcomponent.vertex_boneindices2.end(), boneindices2.begin(), boneindices2.end());
			}
			if (!boneweights2.empty())
			{
				boneweights2.resize(num_vertices);
				meshcomponent.vertex_boneweights2.insert(meshcomponent.vertex_boneweights2.end(), boneweights2.begin(), boneweights2.end());
			}
			for (size_t i = 0; i < morphs.size(); ++i)
			{
				morphs[i].vertex_positions.resize(num_vertices);
				meshcomponent.morph_targets[i].vertex_positions.insert(meshcomponent.morph_targets[i].vertex_positions.end(), morphs[i].vertex_positions.begin(), morphs[i].vertex_positions.end());
			}
		}

		meshcomponent.CreateRenderData();
	}

	for (const ufbx_node* node : fbxscene->nodes)
	{
		if (node->is_root)
			continue;
		Entity entity = CreateEntity();
		node_lookup[node] = entity;
		if (node->name.length > 0)
		{
			scene.names.Create(entity) = node->name.data;
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
		if (node->light)
		{
			LightComponent& light = scene.lights.Create(entity);
			light.SetCastShadow(node->light->cast_shadows);
			light.color = XMFLOAT3(node->light->color.v);
			light.direction = XMFLOAT3(node->light->local_direction.v);
			light.innerConeAngle = node->light->inner_angle;
			light.outerConeAngle = node->light->outer_angle;
			switch (node->light->type)
			{
			default:
			case UFBX_LIGHT_POINT:
				light.SetType(LightComponent::LightType::POINT);
				break;
			case UFBX_LIGHT_SPOT:
				light.SetType(LightComponent::LightType::SPOT);
				break;
			case UFBX_LIGHT_DIRECTIONAL:
				light.SetType(LightComponent::LightType::DIRECTIONAL);
				break;
			}
		}
		if (node->camera)
		{
			CameraComponent& camera = scene.cameras.Create(entity);
			camera.zNearP = node->camera->near_plane * 0.01f;
			camera.zFarP = node->camera->far_plane * 0.01f;
			camera.fov = wi::math::DegreesToRadians(node->camera->field_of_view_deg.y);
		}
	}

	// After all nodes were iterated, determine hierarchy:
	for (const ufbx_node* node : fbxscene->nodes)
	{
		if (node->is_root)
			continue;
		Entity entity = node_lookup[node];
		if (node->parent)
		{
			Entity parent = node_lookup[node->parent];
			scene.Component_Attach(entity, parent, true);
		}
		else
		{
			scene.Component_Attach(entity, rootEntity, true);
		}
	}

	// After nodes are complete, fill bones:
	for (const ufbx_skin_deformer* skin : fbxscene->skin_deformers)
	{
		Entity entity = skin_lookup[skin];
		ArmatureComponent* armature = scene.armatures.GetComponent(entity);
		if (armature == nullptr)
			continue;
		for (const ufbx_skin_cluster* cluster : skin->clusters)
		{
			Entity boneEntity = node_lookup[cluster->bone_node];
			armature->boneCollection.push_back(boneEntity);

			XMFLOAT4X4 inverseBindMatrix = wi::math::IDENTITY_MATRIX;
			inverseBindMatrix._11 = cluster->geometry_to_bone.m00;
			inverseBindMatrix._12 = cluster->geometry_to_bone.m10;
			inverseBindMatrix._13 = cluster->geometry_to_bone.m20;

			inverseBindMatrix._21 = cluster->geometry_to_bone.m01;
			inverseBindMatrix._22 = cluster->geometry_to_bone.m11;
			inverseBindMatrix._23 = cluster->geometry_to_bone.m21;

			inverseBindMatrix._31 = cluster->geometry_to_bone.m02;
			inverseBindMatrix._32 = cluster->geometry_to_bone.m12;
			inverseBindMatrix._33 = cluster->geometry_to_bone.m22;

			inverseBindMatrix._41 = cluster->geometry_to_bone.m03;
			inverseBindMatrix._42 = cluster->geometry_to_bone.m13;
			inverseBindMatrix._43 = cluster->geometry_to_bone.m23;

			armature->inverseBindMatrices.push_back(inverseBindMatrix);

			Import_Mixamo_Bone(scene, rootEntity, boneEntity);
		}
	}

	for (const ufbx_anim_stack* stack : fbxscene->anim_stacks)
	{
		ufbx_baked_anim* bake = ufbx_bake_anim(fbxscene, stack->anim, nullptr, nullptr);
		assert(bake != nullptr);
		if (bake == nullptr)
			continue;

		Entity entity = CreateEntity();
		scene.Component_Attach(entity, rootEntity);
		AnimationComponent& animcomponent = scene.animations.Create(entity);
		animcomponent.start = (float)bake->playback_time_begin;
		animcomponent.end = (float)bake->playback_time_end;

		for (const ufbx_baked_node& bake_node : bake->nodes)
		{
			ufbx_node* scene_node = fbxscene->nodes[bake_node.typed_id];
			Entity target = node_lookup[scene_node];

			// Translation:
			{
				Entity animDataEntity = CreateEntity();
				AnimationDataComponent& animationdata = scene.animation_datas.Create(animDataEntity);
				scene.Component_Attach(animDataEntity, entity);
				for (const ufbx_baked_vec3& keyframe : bake_node.translation_keys)
				{
					animationdata.keyframe_times.push_back((float)keyframe.time);
					animationdata.keyframe_data.push_back(keyframe.value.x);
					animationdata.keyframe_data.push_back(keyframe.value.y);
					animationdata.keyframe_data.push_back(keyframe.value.z);
				}
				AnimationComponent::AnimationChannel& channel = animcomponent.channels.emplace_back();
				channel.target = target;
				channel.path = AnimationComponent::AnimationChannel::Path::TRANSLATION;
				channel.samplerIndex = (int)animcomponent.samplers.size();
				AnimationComponent::AnimationSampler& sampler = animcomponent.samplers.emplace_back();
				sampler.mode = AnimationComponent::AnimationSampler::Mode::LINEAR;
				sampler.data = animDataEntity;
			}

			// Rotation:
			{
				Entity animDataEntity = CreateEntity();
				AnimationDataComponent& animationdata = scene.animation_datas.Create(animDataEntity);
				scene.Component_Attach(animDataEntity, entity);
				for (const ufbx_baked_quat& keyframe : bake_node.rotation_keys)
				{
					animationdata.keyframe_times.push_back((float)keyframe.time);
					animationdata.keyframe_data.push_back(keyframe.value.x);
					animationdata.keyframe_data.push_back(keyframe.value.y);
					animationdata.keyframe_data.push_back(keyframe.value.z);
					animationdata.keyframe_data.push_back(keyframe.value.w);
				}
				AnimationComponent::AnimationChannel& channel = animcomponent.channels.emplace_back();
				channel.target = target;
				channel.path = AnimationComponent::AnimationChannel::Path::ROTATION;
				channel.samplerIndex = (int)animcomponent.samplers.size();
				AnimationComponent::AnimationSampler& sampler = animcomponent.samplers.emplace_back();
				sampler.mode = AnimationComponent::AnimationSampler::Mode::LINEAR;
				sampler.data = animDataEntity;
			}

			// Scale:
			{
				Entity animDataEntity = CreateEntity();
				AnimationDataComponent& animationdata = scene.animation_datas.Create(animDataEntity);
				scene.Component_Attach(animDataEntity, entity);
				for (const ufbx_baked_vec3& keyframe : bake_node.scale_keys)
				{
					animationdata.keyframe_times.push_back((float)keyframe.time);
					animationdata.keyframe_data.push_back(keyframe.value.x);
					animationdata.keyframe_data.push_back(keyframe.value.y);
					animationdata.keyframe_data.push_back(keyframe.value.z);
				}
				AnimationComponent::AnimationChannel& channel = animcomponent.channels.emplace_back();
				channel.target = target;
				channel.path = AnimationComponent::AnimationChannel::Path::SCALE;
				channel.samplerIndex = (int)animcomponent.samplers.size();
				AnimationComponent::AnimationSampler& sampler = animcomponent.samplers.emplace_back();
				sampler.mode = AnimationComponent::AnimationSampler::Mode::LINEAR;
				sampler.data = animDataEntity;
			}

		}

		ufbx_free_baked_anim(bake);
	}

	ufbx_free_scene(fbxscene);

	// Note: we use custom FlipZAxis function for transforming to left-handed space
	//	because it works better than ufbx library's space conversion helpers, especially for ragdolls that will be generated
	wi::unordered_map<size_t, TransformComponent> transforms_original; // original transform states, can be used to restore after flip (like GLTF export)
	FlipZAxis(scene, rootEntity, transforms_original);

	scene.Update(0);
}

void FlipZAxis(Scene& scene, Entity rootEntity, wi::unordered_map<size_t, TransformComponent>& transforms_original)
{
	// Flip mesh data first
	for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	{
		auto& mesh = scene.meshes[i];
		for (auto& v_pos : mesh.vertex_positions)
		{
			v_pos.z *= -1.f;
		}
		for (auto& v_norm : mesh.vertex_normals)
		{
			v_norm.z *= -1.f;
		}
		for (auto& v_tan : mesh.vertex_tangents)
		{
			v_tan.z *= -1.f;
		}
		for (auto& v_morph : mesh.morph_targets)
		{
			for (auto& v_morph_norm : v_morph.vertex_normals)
			{
				v_morph_norm.z *= -1.f;
			}
			for (auto& v_morph_pos : v_morph.vertex_positions)
			{
				v_morph_pos.z *= -1.f;
			}
		}
		mesh.CreateRenderData();
	}

	// Flip scene's transformComponents
	bool state_restore = (transforms_original.size() > 0);
	if (!state_restore)
	{
		wi::unordered_map<wi::ecs::Entity, wi::ecs::Entity> hierarchy_list;
		wi::unordered_map<size_t, wi::scene::TransformComponent> correction_queue;

		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			auto transformEntity = scene.transforms.GetEntity(i);
			if (transformEntity == rootEntity)
				continue;

			correction_queue[i] = scene.transforms[i];

			auto hierarchy = scene.hierarchy.GetComponent(transformEntity);
			if (transformEntity != rootEntity && hierarchy != nullptr)
			{
				hierarchy_list[transformEntity] = hierarchy->parentID;
				scene.Component_Detach(transformEntity);
			}
		}
		transforms_original.insert(correction_queue.begin(), correction_queue.end());
		for (auto& correction_pair : correction_queue)
		{
			auto& transform = scene.transforms[correction_pair.first];
			auto& transform_original = correction_pair.second;

			XMVECTOR V_S, V_R, V_T;
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
		for (auto& hierarchy_pair : hierarchy_list)
		{
			scene.Component_Attach(hierarchy_pair.first, hierarchy_pair.second);
		}
	}
	else
	{
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			auto transform_original_find = transforms_original.find(i);
			if (transform_original_find != transforms_original.end())
			{
				auto& transform = scene.transforms[i];
				transform = transform_original_find->second;
			}
		}
		transforms_original.clear();
	}

	// Flip armature's bind pose
	for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
	{
		auto& armature = scene.armatures[i];
		for (int i = 0; i < armature.inverseBindMatrices.size(); ++i)
		{
			auto& bind = armature.inverseBindMatrices[i];

			XMVECTOR V_S, V_R, V_T;
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
	for (size_t i = 0; i < scene.animations.GetCount(); ++i)
	{
		auto& animation = scene.animations[i];

		for (auto& channel : animation.channels)
		{
			auto data = scene.animation_datas.GetComponent(animation.samplers[channel.samplerIndex].data);

			if (channel.path == wi::scene::AnimationComponent::AnimationChannel::Path::TRANSLATION)
			{
				for (size_t k = 0; k < data->keyframe_data.size() / 3; ++k)
				{
					data->keyframe_data[k * 3 + 2] *= -1.f;
				}
			}
			if (channel.path == wi::scene::AnimationComponent::AnimationChannel::Path::ROTATION)
			{
				for (size_t k = 0; k < data->keyframe_data.size() / 4; ++k)
				{
					data->keyframe_data[k * 4] *= -1.f;
					data->keyframe_data[k * 4 + 1] *= -1.f;
				}
			}
		}
	}
}

void Import_Mixamo_Bone(Scene& scene, Entity rootEntity, Entity boneEntity)
{
	auto get_humanoid = [&]() -> HumanoidComponent& {
		if (scene.humanoids.GetCount() == 0)
		{
			scene.humanoids.Create(rootEntity);
		}
		// Note: it seems there are multiple armatures for multiple body parts in some FBX from Mixamo,
		//	but there should be only one humanoid, so we don't create humanoids for each armature
		HumanoidComponent& component = scene.humanoids[0];
		return component;
	};

	const NameComponent* namecomponent = scene.names.GetComponent(boneEntity);
	if (namecomponent == nullptr)
		return;
	const std::string& name = namecomponent->name;

	if (!name.compare("mixamorig:Hips"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Hips)] = boneEntity;
	}
	else if (!name.compare("mixamorig:Spine"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Spine)] = boneEntity;
	}
	else if (!name.compare("mixamorig:Spine1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Chest)] = boneEntity;
	}
	else if (!name.compare("mixamorig:Spine2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::UpperChest)] = boneEntity;
	}
	else if (!name.compare("mixamorig:Neck"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Neck)] = boneEntity;
	}
	else if (!name.compare("mixamorig:Head"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::Head)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftShoulder"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftShoulder)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightShoulder"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightShoulder)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperArm)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightUpperArm)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftForeArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerArm)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightForeArm"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLowerArm)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHand"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftHand)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHand"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightHand)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandThumb1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbMetacarpal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandThumb1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightThumbMetacarpal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandThumb2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandThumb2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightThumbProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandThumb3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftThumbDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandThumb3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightThumbDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandIndex1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandIndex1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightIndexProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandIndex2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandIndex2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightIndexIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandIndex3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftIndexDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandIndex3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightIndexDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandMiddle1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandMiddle1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandMiddle2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandMiddle2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandMiddle3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftMiddleDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandMiddle3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightMiddleDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandRing1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftRingProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandRing1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightRingProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandRing2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftRingIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandRing2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightRingIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandRing3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftRingDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandRing3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightRingDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandPinky1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandPinky1"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLittleProximal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandPinky2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandPinky2"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLittleIntermediate)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftHandPinky3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLittleDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightHandPinky3"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLittleDistal)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftUpLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftUpperLeg)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightUpLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightUpperLeg)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftLowerLeg)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightLeg"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightLowerLeg)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftFoot"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftFoot)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightFoot"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightFoot)] = boneEntity;
	}
	else if (!name.compare("mixamorig:LeftToeBase"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::LeftToes)] = boneEntity;
	}
	else if (!name.compare("mixamorig:RightToeBase"))
	{
		get_humanoid().bones[size_t(HumanoidComponent::HumanoidBone::RightToes)] = boneEntity;
	}
}
