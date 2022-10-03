#include "wiTerrain.h"
#include "wiProfiler.h"
#include "wiTimer.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiScene.h"
#include "wiPhysics.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

namespace wi::terrain
{
	struct ChunkIndices
	{
		wi::vector<uint32_t> indices;
		struct LOD
		{
			uint32_t indexOffset = 0;
			uint32_t indexCount = 0;
		};
		wi::vector<LOD> lods;

		ChunkIndices()
		{
			const int max_lod = (int)std::log2(chunk_width - 3) + 1;
			lods.resize(max_lod);
			for (int lod = 0; lod < max_lod; ++lod)
			{
				lods[lod].indexOffset = (uint32_t)indices.size();

				if (lod == 0)
				{
					for (int x = 0; x < chunk_width - 1; x++)
					{
						for (int z = 0; z < chunk_width - 1; z++)
						{
							int lowerLeft = x + z * chunk_width;
							int lowerRight = (x + 1) + z * chunk_width;
							int topLeft = x + (z + 1) * chunk_width;
							int topRight = (x + 1) + (z + 1) * chunk_width;

							indices.push_back(topLeft);
							indices.push_back(lowerLeft);
							indices.push_back(lowerRight);

							indices.push_back(topLeft);
							indices.push_back(lowerRight);
							indices.push_back(topRight);
						}
					}
				}
				else
				{
					const int step = 1 << lod;
					// inner grid:
					for (int x = 1; x < chunk_width - 2; x += step)
					{
						for (int z = 1; z < chunk_width - 2; z += step)
						{
							int lowerLeft = x + z * chunk_width;
							int lowerRight = (x + step) + z * chunk_width;
							int topLeft = x + (z + step) * chunk_width;
							int topRight = (x + step) + (z + step) * chunk_width;

							indices.push_back(topLeft);
							indices.push_back(lowerLeft);
							indices.push_back(lowerRight);

							indices.push_back(topLeft);
							indices.push_back(lowerRight);
							indices.push_back(topRight);
						}
					}
					// bottom border:
					for (int x = 0; x < chunk_width - 1; ++x)
					{
						const int z = 0;
						int current = x + z * chunk_width;
						int neighbor = x + 1 + z * chunk_width;
						int connection = 1 + ((x + (step + 1) / 2 - 1) / step) * step + (z + 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(neighbor);
						indices.push_back(connection);

						if (((x - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = 1 + (((x - 1) + (step + 1) / 2 - 1) / step) * step + (z + 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection);
							indices.push_back(connection1);
						}
					}
					// top border:
					for (int x = 0; x < chunk_width - 1; ++x)
					{
						const int z = chunk_width - 1;
						int current = x + z * chunk_width;
						int neighbor = x + 1 + z * chunk_width;
						int connection = 1 + ((x + (step + 1) / 2 - 1) / step) * step + (z - 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(connection);
						indices.push_back(neighbor);

						if (((x - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = 1 + (((x - 1) + (step + 1) / 2 - 1) / step) * step + (z - 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection1);
							indices.push_back(connection);
						}
					}
					// left border:
					for (int z = 0; z < chunk_width - 1; ++z)
					{
						const int x = 0;
						int current = x + z * chunk_width;
						int neighbor = x + (z + 1) * chunk_width;
						int connection = x + 1 + (((z + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(connection);
						indices.push_back(neighbor);

						if (((z - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = x + 1 + ((((z - 1) + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection1);
							indices.push_back(connection);
						}
					}
					// right border:
					for (int z = 0; z < chunk_width - 1; ++z)
					{
						const int x = chunk_width - 1;
						int current = x + z * chunk_width;
						int neighbor = x + (z + 1) * chunk_width;
						int connection = x - 1 + (((z + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(neighbor);
						indices.push_back(connection);

						if (((z - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = x - 1 + ((((z - 1) + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection);
							indices.push_back(connection1);
						}
					}
				}

				lods[lod].indexCount = (uint32_t)indices.size() - lods[lod].indexOffset;
			}
		}
	};
	static ChunkIndices chunk_indices;

	struct Generator
	{
		wi::scene::Scene scene; // The background generation thread can safely add things to this, it will be merged into the main scene when it is safe to do so
		wi::jobsystem::context workload;
		std::atomic_bool cancelled{ false };
	};

	Terrain::Terrain()
	{
		weather.ambient = XMFLOAT3(0.4f, 0.4f, 0.4f);
		weather.SetRealisticSky(true);
		weather.SetVolumetricClouds(true);
		weather.volumetricCloudParameters.CoverageAmount = 0.65f;
		weather.volumetricCloudParameters.CoverageMinimum = 0.15f;
		weather.oceanParameters.waterHeight = -40;
		weather.oceanParameters.wave_amplitude = 120;
		weather.fogStart = 300;
		weather.fogEnd = 100000;
		weather.SetHeightFog(true);
		weather.fogHeightStart = 0;
		weather.fogHeightEnd = 100;
		weather.windDirection = XMFLOAT3(0.05f, 0.05f, 0.05f);
		weather.windSpeed = 4;
		weather.stars = 0.6f;

		grass_properties.viewDistance = chunk_width;

		generator = std::make_shared<Generator>();
	}
	Terrain::~Terrain()
	{
		Generation_Cancel();
	}

	void Terrain::Generation_Restart()
	{
		SetGenerationStarted(true);
		Generation_Cancel();
		generator->scene.Clear();

		chunks.clear();

		wi::vector<Entity> entities_to_remove;
		for (size_t i = 0; i < scene->hierarchy.GetCount(); ++i)
		{
			const HierarchyComponent& hier = scene->hierarchy[i];
			if (hier.parentID == terrainEntity)
			{
				entities_to_remove.push_back(scene->hierarchy.GetEntity(i));
			}
		}
		for (Entity x : entities_to_remove)
		{
			scene->Entity_Remove(x);
		}

		perlin_noise.init(seed);
		for (auto& modifier : modifiers)
		{
			modifier->Seed(seed);
		}

		// Add some nice weather and lighting:
		if (!scene->weathers.Contains(terrainEntity))
		{
			scene->weathers.Create(terrainEntity);
		}
		*scene->weathers.GetComponent(terrainEntity) = weather;
		if (!weather.IsOceanEnabled())
		{
			scene->ocean = {};
		}

		{
			Entity sunEntity = scene->Entity_CreateLight("terrainSun");
			scene->Component_Attach(sunEntity, terrainEntity);
			LightComponent& light = *scene->lights.GetComponent(sunEntity);
			light.SetType(LightComponent::LightType::DIRECTIONAL);
			light.intensity = 16;
			light.SetCastShadow(true);
			TransformComponent& transform = *scene->transforms.GetComponent(sunEntity);
			transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV4, 0, XM_PIDIV4));
			transform.Translate(XMFLOAT3(0, 4, 0));
		}
	}

	void Terrain::Generation_Update(const wi::scene::CameraComponent& camera)
	{
		// The generation task is always cancelled every frame so we are sure that generation is not running at this point
		Generation_Cancel();

		if (!IsGenerationStarted())
		{
			Generation_Restart();
		}

		// Check whether any modifiers need to be removed, and we will really remove them here if so:
		if (!modifiers_to_remove.empty())
		{
			for (auto& modifier : modifiers_to_remove)
			{
				for (auto it = modifiers.begin(); it != modifiers.end(); ++it)
				{
					if (it->get() == modifier)
					{
						modifiers.erase(it);
						break;
					}
				}
			}
			Generation_Restart();
			modifiers_to_remove.clear();
		}

		if (terrainEntity == INVALID_ENTITY)
		{
			chunks.clear();
			return;
		}

		WeatherComponent* weather_component = scene->weathers.GetComponent(terrainEntity);
		if (weather_component != nullptr)
		{
			weather = *weather_component; // feedback default weather
		}

		// What was generated, will be merged in to the main scene
		scene->Merge(generator->scene);

		const float chunk_scale_rcp = 1.0f / chunk_scale;

		if (IsCenterToCamEnabled())
		{
			center_chunk.x = (int)std::floor((camera.Eye.x + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
			center_chunk.z = (int)std::floor((camera.Eye.z + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
		}

		const int removal_threshold = generation + 2;
		const float texlodMultiplier = texlod;
		GraphicsDevice* device = GetDevice();
		virtual_texture_updates.clear();
		virtual_texture_barriers_begin.clear();
		virtual_texture_barriers_end.clear();

		// Check whether there are any materials that would write to virtual textures:
		bool virtual_texture_any = false;
		virtual_texture_available[MaterialComponent::TEXTURESLOT_COUNT] = {};
		MaterialComponent* virtual_materials[4] = {
			&material_Base,
			&material_Slope,
			&material_LowAltitude,
			&material_HighAltitude,
		};
		for (auto& material : virtual_materials)
		{
			for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
			{
				virtual_texture_available[i] = false;
				switch (i)
				{
				case MaterialComponent::BASECOLORMAP:
				case MaterialComponent::NORMALMAP:
				case MaterialComponent::SURFACEMAP:
					if (material->textures[i].resource.IsValid())
					{
						virtual_texture_available[i] = true;
						virtual_texture_any = true;
					}
					break;
				default:
					break;
				}
			}
		}
		virtual_texture_available[MaterialComponent::SURFACEMAP] = true; // this is always needed to bake individual material properties

		for (auto it = chunks.begin(); it != chunks.end();)
		{
			const Chunk& chunk = it->first;
			ChunkData& chunk_data = it->second;
			bool chunk_visible = camera.frustum.CheckSphere(chunk_data.sphere.center, chunk_data.sphere.radius);
			const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

			if (wi::renderer::GetOcclusionCullingEnabled())
			{
				size_t object_index = scene->objects.GetIndex(chunk_data.entity);
				if (object_index < scene->occlusion_results_objects.size())
				{
					if (scene->occlusion_results_objects[object_index].IsOccluded())
					{
						chunk_visible = false;
					}
				}
			}

			// pointer refresh:
			MeshComponent* chunk_mesh = scene->meshes.GetComponent(chunk_data.entity);
			if (chunk_mesh != nullptr)
			{
				chunk_data.mesh_vertex_positions = chunk_mesh->vertex_positions.data();

#if 0
				// Test: remove off screen chunks
				//	But the problem is that shadow casters shouldn't be removed outside view
				if (chunk_visible && !chunk_mesh->generalBuffer.IsValid())
				{
					chunk_mesh->CreateRenderData();
				}
				else if (!chunk_visible && chunk_mesh->generalBuffer.IsValid())
				{
					chunk_mesh->DeleteRenderData();
				}
#endif
			}
			else
			{
				chunk_data.mesh_vertex_positions = nullptr;
			}

			// chunk removal:
			if (IsRemovalEnabled())
			{
				if (dist > removal_threshold)
				{
					scene->Entity_Remove(it->second.entity);
					it = chunks.erase(it);
					continue; // don't increment iterator
				}
				else
				{
					// Grass patch removal:
					if (chunk_data.grass_entity != INVALID_ENTITY && (dist > 1 || !IsGrassEnabled()))
					{
						scene->Entity_Remove(chunk_data.grass_entity);
						chunk_data.grass_entity = INVALID_ENTITY; // grass can be generated here by generation thread...
					}

					// Prop removal:
					if (chunk_data.props_entity != INVALID_ENTITY && (dist > prop_generation || std::abs(chunk_data.prop_density_current - prop_density) > std::numeric_limits<float>::epsilon()))
					{
						scene->Entity_Remove(chunk_data.props_entity);
						chunk_data.props_entity = INVALID_ENTITY; // prop can be generated here by generation thread...
					}
				}
			}

			// Grass property update:
			if (chunk_visible && chunk_data.grass_entity != INVALID_ENTITY)
			{
				wi::HairParticleSystem* grass = scene->hairs.GetComponent(chunk_data.grass_entity);
				if (grass != nullptr)
				{
					chunk_data.grass_density_current = grass_density;
					grass->strandCount = uint32_t(chunk_data.grass.strandCount * chunk_data.grass_density_current);
					grass->length = grass_properties.length;
					grass->viewDistance = grass_properties.viewDistance;
				}
			}

			RigidBodyPhysicsComponent* rigidbody = scene->rigidbodies.GetComponent(chunk_data.entity);
			if (IsPhysicsEnabled())
			{
				const ObjectComponent* object = scene->objects.GetComponent(chunk_data.entity);
				const int lod_required = object == nullptr ? 0 : object->lod;

				if (rigidbody != nullptr)
				{
					if (rigidbody->mesh_lod != lod_required)
					{
						rigidbody->mesh_lod = lod_required;
						rigidbody->physicsobject = {}; // will be recreated
					}
					else
					{
						if (dist < physics_generation)
						{
							wi::physics::SetActivationState(*rigidbody, wi::physics::ActivationState::Active);
						}
						else
						{
							//scene->rigidbodies.Remove(chunk_data.entity);
							wi::physics::SetActivationState(*rigidbody, wi::physics::ActivationState::Inactive);
						}
					}
				}
				else
				{
					if (dist < physics_generation)
					{
						RigidBodyPhysicsComponent& newrigidbody = scene->rigidbodies.Create(chunk_data.entity);
						newrigidbody.shape = RigidBodyPhysicsComponent::TRIANGLE_MESH;
						//newrigidbody.SetDisableDeactivation(true);
						newrigidbody.SetKinematic(true);
						newrigidbody.mesh_lod = lod_required;
					}
				}
			}
			else
			{
				if (rigidbody != nullptr)
				{
					scene->rigidbodies.Remove(chunk_data.entity);
				}
			}

			// Collect virtual texture update requests:
			if (virtual_texture_any)
			{
				if (chunk_visible)
				{
					uint32_t texture_lod = 0;
					const float distsq = wi::math::DistanceSquared(camera.Eye, chunk_data.sphere.center);
					const float radius = chunk_data.sphere.radius;
					const float radiussq = radius * radius;
					if (distsq < radiussq)
					{
						texture_lod = 0;
					}
					else
					{
						const float dist = std::sqrt(distsq);
						const float dist_to_sphere = dist - radius;
						texture_lod = uint32_t(dist_to_sphere * texlodMultiplier);
					}
					chunk_data.required_texture_resolution = uint32_t(target_texture_resolution / std::pow(2.0f, (float)std::max(0u, texture_lod)));
					chunk_data.required_texture_resolution = AlignTo(chunk_data.required_texture_resolution, 8u);
					chunk_data.required_texture_resolution = std::max(8u, chunk_data.required_texture_resolution);
				}
				else
				{
					chunk_data.required_texture_resolution = 8u;
				}
				MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);

				if (material != nullptr)
				{
					bool need_update = false;
					for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
					{
						if (virtual_texture_available[i])
						{
							uint32_t current_resolution = 0;
							if (material->textures[i].resource.IsValid())
							{
								current_resolution = material->textures[i].resource.GetTexture().GetDesc().width;
							}

							if (current_resolution != chunk_data.required_texture_resolution)
							{
								need_update = true;
								TextureDesc desc;
								desc.width = chunk_data.required_texture_resolution;
								desc.height = chunk_data.required_texture_resolution;
								if (i == MaterialComponent::TEXTURESLOT::NORMALMAP)
								{
									desc.format = Format::R8G8_UNORM;
								}
								else
								{
									desc.format = Format::R8G8B8A8_UNORM;
								}
								desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
								desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
								desc.mip_levels = 4;
								Texture texture;
								bool success = device->CreateTexture(&desc, nullptr, &texture);
								assert(success);

								if (desc.mip_levels > 1)
								{
									for (uint32_t i = 0; i < texture.desc.mip_levels; ++i)
									{
										int subresource_index = device->CreateSubresource(&texture, SubresourceType::UAV, 0, 1, i, 1);
										assert(subresource_index == i);
									}
								}

								material->textures[i].resource.SetTexture(texture);
								virtual_texture_barriers_begin.push_back(GPUBarrier::Image(&material->textures[i].resource.GetTexture(), desc.layout, ResourceState::UNORDERED_ACCESS));
								virtual_texture_barriers_end.push_back(GPUBarrier::Image(&material->textures[i].resource.GetTexture(), ResourceState::UNORDERED_ACCESS, desc.layout));
							}
						}
					}

					if (need_update)
					{
						// Shrink the uvs to avoid wrap sampling across edge by object rendering shaders:
						float virtual_texture_resolution_rcp = 1.0f / float(chunk_data.required_texture_resolution);
						material->texMulAdd.x = float(chunk_data.required_texture_resolution - 1) * virtual_texture_resolution_rcp;
						material->texMulAdd.y = float(chunk_data.required_texture_resolution - 1) * virtual_texture_resolution_rcp;
						material->texMulAdd.z = 0.5f * virtual_texture_resolution_rcp;
						material->texMulAdd.w = 0.5f * virtual_texture_resolution_rcp;

						virtual_texture_updates.push_back(chunk);
					}

				}
			}

			it++;
		}

		// Start the generation on a background thread and keep it running until the next frame
		wi::jobsystem::Execute(generator->workload, [=](wi::jobsystem::JobArgs args) {

			wi::Timer timer;
			bool generated_something = false;

			auto request_chunk = [&](int offset_x, int offset_z)
			{
				Chunk chunk = center_chunk;
				chunk.x += offset_x;
				chunk.z += offset_z;
				auto it = chunks.find(chunk);
				if (it == chunks.end() || it->second.entity == INVALID_ENTITY)
				{
					// Generate a new chunk:
					ChunkData& chunk_data = chunks[chunk];

					chunk_data.entity = generator->scene.Entity_CreateObject("chunk_" + std::to_string(chunk.x) + "_" + std::to_string(chunk.z));
					ObjectComponent& object = *generator->scene.objects.GetComponent(chunk_data.entity);
					object.lod_distance_multiplier = lod_multiplier;
					object.filterMask |= wi::enums::FILTER_NAVIGATION_MESH;
					generator->scene.Component_Attach(chunk_data.entity, terrainEntity);

					TransformComponent& transform = *generator->scene.transforms.GetComponent(chunk_data.entity);
					transform.ClearTransform();
					chunk_data.position = XMFLOAT3(float(chunk.x * (chunk_width - 1)) * chunk_scale, 0, float(chunk.z * (chunk_width - 1)) * chunk_scale);
					transform.Translate(chunk_data.position);
					transform.UpdateTransform();

					MaterialComponent& material = generator->scene.materials.Create(chunk_data.entity);
					// material params will be 1 because they will be created from only texture maps
					//	because region materials are blended together into one texture
					material.SetRoughness(1);
					material.SetMetalness(1);
					material.SetReflectance(1);

					MeshComponent& mesh = generator->scene.meshes.Create(chunk_data.entity);
					object.meshID = chunk_data.entity;
					mesh.indices = chunk_indices.indices;
					for (auto& lod : chunk_indices.lods)
					{
						mesh.subsets.emplace_back();
						mesh.subsets.back().materialID = chunk_data.entity;
						mesh.subsets.back().indexCount = lod.indexCount;
						mesh.subsets.back().indexOffset = lod.indexOffset;
					}
					mesh.subsets_per_lod = 1;
					mesh.vertex_positions.resize(vertexCount);
					mesh.vertex_normals.resize(vertexCount);
					mesh.vertex_uvset_0.resize(vertexCount);
					chunk_data.region_weights.resize(vertexCount);

					chunk_data.mesh_vertex_positions = mesh.vertex_positions.data();

					wi::HairParticleSystem grass = grass_properties;
					grass.vertex_lengths.resize(vertexCount);
					std::atomic<uint32_t> grass_valid_vertex_count{ 0 };

					// Shadow casting will only be enabled for sloped terrain chunks:
					std::atomic_bool slope_cast_shadow;
					slope_cast_shadow.store(false);

					// Do a parallel for loop over all the chunk's vertices and compute their properties:
					wi::jobsystem::context ctx;
					wi::jobsystem::Dispatch(ctx, vertexCount, chunk_width, [&](wi::jobsystem::JobArgs args) {
						uint32_t index = args.jobIndex;
						const float x = (float(index % chunk_width) - chunk_half_width) * chunk_scale;
						const float z = (float(index / chunk_width) - chunk_half_width) * chunk_scale;
						XMVECTOR corners[3];
						XMFLOAT2 corner_offsets[3] = {
							XMFLOAT2(0, 0),
							XMFLOAT2(1, 0),
							XMFLOAT2(0, 1),
						};
						for (int i = 0; i < arraysize(corners); ++i)
						{
							float height = 0;
							const XMFLOAT2 world_pos = XMFLOAT2(chunk_data.position.x + x + corner_offsets[i].x, chunk_data.position.z + z + corner_offsets[i].y);
							for (auto& modifier : modifiers)
							{
								modifier->Apply(world_pos, height);
							}
							height = wi::math::Lerp(bottomLevel, topLevel, height);
							corners[i] = XMVectorSet(world_pos.x, height, world_pos.y, 0);
						}
						const float height = XMVectorGetY(corners[0]);
						const XMVECTOR T = XMVectorSubtract(corners[2], corners[1]);
						const XMVECTOR B = XMVectorSubtract(corners[1], corners[0]);
						const XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
						XMFLOAT3 normal;
						XMStoreFloat3(&normal, N);

						const float slope_amount = 1.0f - wi::math::saturate(normal.y);
						if (slope_amount > 0.1f)
							slope_cast_shadow.store(true);

						const float region_base = 1;
						const float region_slope = std::pow(slope_amount, region1);
						const float region_low_altitude = bottomLevel == 0 ? 0 : std::pow(wi::math::saturate(wi::math::InverseLerp(0, bottomLevel, height)), region2);
						const float region_high_altitude = topLevel == 0 ? 0 : std::pow(wi::math::saturate(wi::math::InverseLerp(0, topLevel, height)), region3);

						XMFLOAT4 materialBlendWeights(region_base, 0, 0, 0);
						materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 1, 0, 0), region_slope);
						materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 0, 1, 0), region_low_altitude);
						materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 0, 0, 1), region_high_altitude);
						const float weight_norm = 1.0f / (materialBlendWeights.x + materialBlendWeights.y + materialBlendWeights.z + materialBlendWeights.w);
						materialBlendWeights.x *= weight_norm;
						materialBlendWeights.y *= weight_norm;
						materialBlendWeights.z *= weight_norm;
						materialBlendWeights.w *= weight_norm;

						chunk_data.region_weights[index] = wi::Color::fromFloat4(materialBlendWeights);

						mesh.vertex_positions[index] = XMFLOAT3(x, height, z);
						mesh.vertex_normals[index] = normal;
						const XMFLOAT2 uv = XMFLOAT2(x * chunk_scale_rcp * chunk_width_rcp + 0.5f, z * chunk_scale_rcp * chunk_width_rcp + 0.5f);
						mesh.vertex_uvset_0[index] = uv;

						XMFLOAT3 vertex_pos(chunk_data.position.x + x, height, chunk_data.position.z + z);

						const float grass_noise_frequency = 0.1f;
						const float grass_noise = perlin_noise.compute(vertex_pos.x * grass_noise_frequency, vertex_pos.y * grass_noise_frequency, vertex_pos.z * grass_noise_frequency) * 0.5f + 0.5f;
						const float region_grass = std::pow(materialBlendWeights.x * (1 - materialBlendWeights.w), 8.0f) * grass_noise;
						if (region_grass > 0.1f)
						{
							grass_valid_vertex_count.fetch_add(1);
							grass.vertex_lengths[index] = region_grass;
						}
						else
						{
							grass.vertex_lengths[index] = 0;
						}
						});
					wi::jobsystem::Wait(ctx); // wait until chunk's vertex buffer is fully generated

					material.SetCastShadow(slope_cast_shadow.load());

					wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
						mesh.CreateRenderData();
						chunk_data.sphere.center = mesh.aabb.getCenter();
						chunk_data.sphere.center.x += chunk_data.position.x;
						chunk_data.sphere.center.y += chunk_data.position.y;
						chunk_data.sphere.center.z += chunk_data.position.z;
						chunk_data.sphere.radius = mesh.aabb.getRadius();
						});

					// If there were any vertices in this chunk that could be valid for grass, store the grass particle system:
					if (grass_valid_vertex_count.load() > 0)
					{
						chunk_data.grass = std::move(grass); // the grass will be added to the scene later, only when the chunk is close to the camera (center chunk's neighbors)
						chunk_data.grass.meshID = chunk_data.entity;
						chunk_data.grass.strandCount = uint32_t(grass_valid_vertex_count.load() * 3 * chunk_scale * chunk_scale); // chunk_scale * chunk_scale : grass density increases with squared amount with chunk scale (x*z)
					}

					// Create the blend weights texture for virtual texture update:
					CreateChunkRegionTexture(chunk_data);

					wi::jobsystem::Wait(ctx); // wait until mesh.CreateRenderData() async task finishes
					generated_something = true;
				}

				const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

				// Grass patch placement:
				if (dist <= 1 && IsGrassEnabled())
				{
					it = chunks.find(chunk);
					if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
					{
						ChunkData& chunk_data = it->second;
						if (chunk_data.grass_entity == INVALID_ENTITY && chunk_data.grass.meshID != INVALID_ENTITY)
						{
							// add patch for this chunk
							chunk_data.grass_entity = CreateEntity();
							wi::HairParticleSystem& grass = generator->scene.hairs.Create(chunk_data.grass_entity);
							grass = chunk_data.grass;
							chunk_data.grass_density_current = grass_density;
							grass.strandCount = uint32_t(grass.strandCount * chunk_data.grass_density_current);
							generator->scene.materials.Create(chunk_data.grass_entity) = material_GrassParticle;
							generator->scene.transforms.Create(chunk_data.grass_entity);
							generator->scene.names.Create(chunk_data.grass_entity) = "grass";
							generator->scene.Component_Attach(chunk_data.grass_entity, chunk_data.entity, true);
							generated_something = true;
						}
					}
				}

				// Prop placement:
				if (dist <= prop_generation)
				{
					it = chunks.find(chunk);
					if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
					{
						ChunkData& chunk_data = it->second;

						if (chunk_data.props_entity == INVALID_ENTITY && chunk_data.mesh_vertex_positions != nullptr)
						{
							chunk_data.props_entity = CreateEntity();
							generator->scene.transforms.Create(chunk_data.props_entity);
							generator->scene.names.Create(chunk_data.props_entity) = "props";
							generator->scene.Component_Attach(chunk_data.props_entity, chunk_data.entity, true);
							chunk_data.prop_density_current = prop_density;

							std::mt19937 prop_rand;
							prop_rand.seed((uint32_t)chunk.compute_hash() ^ seed);

							for (const auto& prop : props)
							{
								if (prop.data.empty())
									continue;
								std::uniform_int_distribution<uint32_t> gen_distr(
									uint32_t(prop.min_count_per_chunk * chunk_data.prop_density_current),
									uint32_t(prop.max_count_per_chunk * chunk_data.prop_density_current)
								);
								int gen_count = gen_distr(prop_rand);
								for (int i = 0; i < gen_count; ++i)
								{
									std::uniform_real_distribution<float> float_distr(0.0f, 1.0f);
									std::uniform_int_distribution<uint32_t> ind_distr(0, chunk_indices.lods[0].indexCount / 3 - 1);
									uint32_t tri = ind_distr(prop_rand); // random triangle on the chunk mesh
									uint32_t ind0 = chunk_indices.indices[tri * 3 + 0];
									uint32_t ind1 = chunk_indices.indices[tri * 3 + 1];
									uint32_t ind2 = chunk_indices.indices[tri * 3 + 2];
									const XMFLOAT3& pos0 = chunk_data.mesh_vertex_positions[ind0];
									const XMFLOAT3& pos1 = chunk_data.mesh_vertex_positions[ind1];
									const XMFLOAT3& pos2 = chunk_data.mesh_vertex_positions[ind2];
									const XMFLOAT4 region0 = chunk_data.region_weights[ind0];
									const XMFLOAT4 region1 = chunk_data.region_weights[ind1];
									const XMFLOAT4 region2 = chunk_data.region_weights[ind2];
									// random barycentric coords on the triangle:
									float f = float_distr(prop_rand);
									float g = float_distr(prop_rand);
									if (f + g > 1)
									{
										f = 1 - f;
										g = 1 - g;
									}
									XMFLOAT3 vertex_pos;
									vertex_pos.x = pos0.x + f * (pos1.x - pos0.x) + g * (pos2.x - pos0.x);
									vertex_pos.y = pos0.y + f * (pos1.y - pos0.y) + g * (pos2.y - pos0.y);
									vertex_pos.z = pos0.z + f * (pos1.z - pos0.z) + g * (pos2.z - pos0.z);
									XMFLOAT4 region;
									region.x = region0.x + f * (region1.x - region0.x) + g * (region2.x - region0.x);
									region.y = region0.y + f * (region1.y - region0.y) + g * (region2.y - region0.y);
									region.z = region0.z + f * (region1.z - region0.z) + g * (region2.z - region0.z);
									region.w = region0.w + f * (region1.w - region0.w) + g * (region2.w - region0.w);

									const float noise = std::pow(perlin_noise.compute((vertex_pos.x + chunk_data.position.x) * prop.noise_frequency, vertex_pos.y * prop.noise_frequency, (vertex_pos.z + chunk_data.position.z) * prop.noise_frequency) * 0.5f + 0.5f, prop.noise_power);
									const float chance = std::pow(((float*)&region)[prop.region], prop.region_power) * noise;
									if (chance > prop.threshold)
									{
										wi::Archive archive = wi::Archive(prop.data.data());
										EntitySerializer seri;
										Entity entity = generator->scene.Entity_Serialize(
											archive,
											seri,
											INVALID_ENTITY,
											wi::scene::Scene::EntitySerializeFlags::RECURSIVE |
											wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
										);
										NameComponent* name = generator->scene.names.GetComponent(entity);
										if (name != nullptr)
										{
											name->name += std::to_string(i);
										}
										TransformComponent* transform = generator->scene.transforms.GetComponent(entity);
										if (transform == nullptr)
										{
											transform = &generator->scene.transforms.Create(entity);
										}
										transform->translation_local = vertex_pos;
										transform->translation_local.y += wi::math::Lerp(prop.min_y_offset, prop.max_y_offset, float_distr(prop_rand));
										const float scaling = wi::math::Lerp(prop.min_size, prop.max_size, float_distr(prop_rand));
										transform->Scale(XMFLOAT3(scaling, scaling, scaling));
										transform->RotateRollPitchYaw(XMFLOAT3(0, XM_2PI * float_distr(prop_rand), 0));
										transform->SetDirty();
										transform->UpdateTransform();
										generator->scene.Component_Attach(entity, chunk_data.props_entity, true);
										generated_something = true;
									}
								}
							}
							if (!IsPhysicsEnabled())
							{
								generator->scene.rigidbodies.Clear();
							}
						}
					}
				}

				if (generated_something && timer.elapsed_milliseconds() > generation_time_budget_milliseconds)
				{
					generator->cancelled.store(true);
				}

			};

			// generate center chunk first:
			request_chunk(0, 0);
			if (generator->cancelled.load()) return;

			// then generate neighbor chunks in outward spiral:
			for (int growth = 0; growth < generation; ++growth)
			{
				const int side = 2 * (growth + 1);
				int x = -growth - 1;
				int z = -growth - 1;
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					x++;
				}
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					z++;
				}
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					x--;
				}
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					z--;
				}
			}

			});

	}

	void Terrain::Generation_Cancel()
	{
		if (generator == nullptr)
			return;
		generator->cancelled.store(true); // tell the generation thread that work must be stopped
		wi::jobsystem::Wait(generator->workload); // waits until generation thread exits
		generator->cancelled.store(false); // the next generation can run
	}

	void Terrain::UpdateVirtualTextures(CommandList cmd) const
	{
		if (virtual_texture_updates.empty())
			return;

		GraphicsDevice* device = GetDevice();
		device->EventBegin("TerrainVirtualTextureUpdate", cmd);
		auto range = wi::profiler::BeginRangeGPU("TerrainVirtualTextureUpdate", cmd);
		device->Barrier(virtual_texture_barriers_begin.data(), (uint32_t)virtual_texture_barriers_begin.size(), cmd);

		device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE), cmd);

		ShaderMaterial materials[4];
		material_Base.WriteShaderMaterial(&materials[0]);
		material_Slope.WriteShaderMaterial(&materials[1]);
		material_LowAltitude.WriteShaderMaterial(&materials[2]);
		material_HighAltitude.WriteShaderMaterial(&materials[3]);
		device->BindDynamicConstantBuffer(materials, 0, cmd);

		for (auto& chunk : virtual_texture_updates)
		{
			auto it = chunks.find(chunk);
			if (it == chunks.end())
				continue;
			const ChunkData& chunk_data = it->second;

			const GPUResource* res[] = {
				&chunk_data.region_weights_texture,
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			const MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);
			if (material != nullptr)
			{
				for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
				{
					if (virtual_texture_available[i])
					{
						const Texture& texture = material->textures[i].resource.GetTexture();

						device->BindUAV(&texture, i * 4 + 0, cmd, 0);
						device->BindUAV(&texture, i * 4 + 1, cmd, 1);
						device->BindUAV(&texture, i * 4 + 2, cmd, 2);
						device->BindUAV(&texture, i * 4 + 3, cmd, 3);
					}
				}
			}

			device->Dispatch(chunk_data.required_texture_resolution / 8u, chunk_data.required_texture_resolution / 8u, 1, cmd);
		}

		device->Barrier(virtual_texture_barriers_end.data(), (uint32_t)virtual_texture_barriers_end.size(), cmd);

		wi::profiler::EndRange(range);
		device->EventEnd(cmd);
	}

	void Terrain::BakeVirtualTexturesToFiles()
	{
		if (terrainEntity == INVALID_ENTITY)
		{
			return;
		}

		wi::jobsystem::context ctx;

		static const std::string extension = "PNG";

		for (auto it = chunks.begin(); it != chunks.end(); it++)
		{
			const Chunk& chunk = it->first;
			ChunkData& chunk_data = it->second;
			MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);
			if (material != nullptr)
			{
				for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
				{
					auto& tex = material->textures[i];
					switch (i)
					{
					case MaterialComponent::BASECOLORMAP:
					case MaterialComponent::SURFACEMAP:
					case MaterialComponent::NORMALMAP:
						if (tex.GetGPUResource() != nullptr)
						{
							wi::vector<uint8_t> filedata;
							if (wi::helper::saveTextureToMemory(tex.resource.GetTexture(), filedata))
							{
								tex.resource.SetFileData(std::move(filedata));
								wi::jobsystem::Execute(ctx, [i, &tex, chunk](wi::jobsystem::JobArgs args) {
									wi::vector<uint8_t> filedata_ktx2;
									if (wi::helper::saveTextureToMemoryFile(tex.resource.GetFileData(), tex.resource.GetTexture().desc, extension, filedata_ktx2))
									{
										tex.name = std::to_string(chunk.x) + "_" + std::to_string(chunk.z);
										switch (i)
										{
										case MaterialComponent::BASECOLORMAP:
											tex.name += "_basecolormap";
											break;
										case MaterialComponent::SURFACEMAP:
											tex.name += "_surfacemap";
											break;
										case MaterialComponent::NORMALMAP:
											tex.name += "_normalmap";
											break;
										default:
											break;
										}
										tex.name += "." + extension;
										tex.resource = wi::resourcemanager::Load(tex.name, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA, filedata_ktx2.data(), filedata_ktx2.size());
									}
									});
							}
						}
						break;
					default:
						break;
					}
				}
			}
		}

		wi::helper::messageBox("Baking terrain virtual textures to static textures, this could take a while!", "Attention!");

		wi::jobsystem::Wait(ctx);

		for (auto it = chunks.begin(); it != chunks.end(); it++)
		{
			const Chunk& chunk = it->first;
			ChunkData& chunk_data = it->second;
			MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);
			if (material != nullptr)
			{
				material->CreateRenderData();
			}
		}
	}

	void Terrain::CreateChunkRegionTexture(ChunkData& chunk_data)
	{
		TextureDesc desc;
		desc.width = (uint32_t)chunk_width;
		desc.height = (uint32_t)chunk_width;
		desc.format = Format::R8G8B8A8_UNORM;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		SubresourceData data;
		data.data_ptr = chunk_data.region_weights.data();
		data.row_pitch = chunk_width * sizeof(chunk_data.region_weights[0]);
		bool success = GetDevice()->CreateTexture(&desc, &data, &chunk_data.region_weights_texture);
		assert(success);
	}

	void Terrain::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		Generation_Cancel();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> lod_multiplier;
			archive >> texlod;
			archive >> generation;
			archive >> prop_generation;
			archive >> prop_density;
			archive >> grass_density;
			archive >> chunk_scale;
			archive >> seed;
			archive >> bottomLevel;
			archive >> topLevel;
			archive >> region1;
			archive >> region2;
			archive >> region3;

			archive >> center_chunk.x;
			archive >> center_chunk.z;

			if (seri.GetVersion() >= 1)
			{
				archive >> physics_generation;
			}
			if (seri.GetVersion() >= 2)
			{
				archive >> target_texture_resolution;
			}

			size_t count = 0;
			archive >> count;
			props.resize(count);
			for (size_t i = 0; i < props.size(); ++i)
			{
				Prop& prop = props[i];
				if (seri.GetVersion() >= 1)
				{
					archive >> prop.data;

					if (!prop.data.empty())
					{
						// Serialize the prop data in read mode and remap internal entity references:
						Scene tmp_scene;
						wi::Archive tmp_archive = wi::Archive(prop.data.data());
						Entity entity = tmp_scene.Entity_Serialize(
							tmp_archive,
							seri,
							INVALID_ENTITY,
							wi::scene::Scene::EntitySerializeFlags::RECURSIVE
						);

						// Serialize again with the remapped references:
						wi::Archive remapped_archive;
						remapped_archive.SetReadModeAndResetPos(false);
						tmp_scene.Entity_Serialize(
							remapped_archive,
							seri,
							entity,
							wi::scene::Scene::EntitySerializeFlags::RECURSIVE
						);

						// Replace original data with remapped references for current session:
						remapped_archive.WriteData(prop.data);
					}
				}
				else
				{
					// Back compat reading of terrain version 0:
					std::string name;
					Entity mesh_entity;
					ObjectComponent object;
					archive >> name;
					SerializeEntity(archive, mesh_entity, seri);
					object.Serialize(archive, seri);
					Scene tmp_scene;
					Entity object_entity = CreateEntity();
					tmp_scene.names.Create(object_entity) = name;
					tmp_scene.objects.Create(object_entity) = object;
					tmp_scene.transforms.Create(object_entity);
					wi::Archive archive;
					EntitySerializer seri;
					tmp_scene.Entity_Serialize(
						archive,
						seri,
						object_entity,
						wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
					);
					archive.WriteData(prop.data);
				}
				archive >> prop.min_count_per_chunk;
				archive >> prop.max_count_per_chunk;
				archive >> prop.region;
				archive >> prop.region_power;
				archive >> prop.noise_frequency;
				archive >> prop.noise_power;
				archive >> prop.threshold;
				archive >> prop.min_size;
				archive >> prop.max_size;
				archive >> prop.min_y_offset;
				archive >> prop.max_y_offset;
			}

			archive >> count;
			chunks.reserve(count);
			for (size_t i = 0; i < count; ++i)
			{
				Chunk chunk;
				archive >> chunk.x;
				archive >> chunk.z;
				ChunkData& chunk_data = chunks[chunk];
				SerializeEntity(archive, chunk_data.entity, seri);
				SerializeEntity(archive, chunk_data.grass_entity, seri);
				SerializeEntity(archive, chunk_data.props_entity, seri);
				archive >> chunk_data.prop_density_current;
				chunk_data.grass.Serialize(archive, seri);
				archive >> chunk_data.grass_density_current;
				archive >> chunk_data.region_weights;
				CreateChunkRegionTexture(chunk_data);
				archive >> chunk_data.sphere.center;
				archive >> chunk_data.sphere.radius;
				archive >> chunk_data.position;
			}

			archive >> count;
			modifiers.resize(count);
			for (size_t i = 0; i < modifiers.size(); ++i)
			{
				uint32_t value;
				archive >> value;
				Modifier::Type type = (Modifier::Type)value;

				switch (type)
				{
				default:
				case Modifier::Type::Perlin:
					{
						std::shared_ptr<PerlinModifier> modifier = std::make_shared<PerlinModifier>();
						modifiers[i] = modifier;
						archive >> modifier->octaves;
						archive >> modifier->seed;
						modifier->perlin_noise.Serialize(archive);
					}
					break;
				case Modifier::Type::Voronoi:
					{
						std::shared_ptr<VoronoiModifier> modifier = std::make_shared<VoronoiModifier>();
						modifiers[i] = modifier;
						archive >> modifier->fade;
						archive >> modifier->shape;
						archive >> modifier->falloff;
						archive >> modifier->perturbation;
						archive >> modifier->seed;
						modifier->perlin_noise.Serialize(archive);
					}
					break;
				case Modifier::Type::Heightmap:
					{
						std::shared_ptr<HeightmapModifier> modifier = std::make_shared<HeightmapModifier>();
						modifiers[i] = modifier;
						archive >> modifier->scale;
						archive >> modifier->data;
						archive >> modifier->width;
						archive >> modifier->height;
					}
					break;
				}

				Modifier* modifier = modifiers[i].get();
				archive >> value;
				modifier->blend = (Modifier::BlendMode)value;
				archive >> modifier->weight;
				archive >> modifier->frequency;
			}

		}
		else
		{
			archive << _flags;
			archive << lod_multiplier;
			archive << texlod;
			archive << generation;
			archive << prop_generation;
			archive << prop_density;
			archive << grass_density;
			archive << chunk_scale;
			archive << seed;
			archive << bottomLevel;
			archive << topLevel;
			archive << region1;
			archive << region2;
			archive << region3;

			archive << center_chunk.x;
			archive << center_chunk.z;

			if (seri.GetVersion() >= 1)
			{
				archive << physics_generation;
			}
			if (seri.GetVersion() >= 2)
			{
				archive << target_texture_resolution;
			}

			archive << props.size();
			for (size_t i = 0; i < props.size(); ++i)
			{
				Prop& prop = props[i];
				archive << prop.data;
				archive << prop.min_count_per_chunk;
				archive << prop.max_count_per_chunk;
				archive << prop.region;
				archive << prop.region_power;
				archive << prop.noise_frequency;
				archive << prop.noise_power;
				archive << prop.threshold;
				archive << prop.min_size;
				archive << prop.max_size;
				archive << prop.min_y_offset;
				archive << prop.max_y_offset;
			}

			archive << chunks.size();
			for (auto& it : chunks)
			{
				const Chunk& chunk = it.first;
				archive << chunk.x;
				archive << chunk.z;
				ChunkData& chunk_data = it.second;
				SerializeEntity(archive, chunk_data.entity, seri);
				SerializeEntity(archive, chunk_data.grass_entity, seri);
				SerializeEntity(archive, chunk_data.props_entity, seri);
				archive << chunk_data.prop_density_current;
				chunk_data.grass.Serialize(archive, seri);
				archive << chunk_data.grass_density_current;
				archive << chunk_data.region_weights;
				archive << chunk_data.sphere.center;
				archive << chunk_data.sphere.radius;
				archive << chunk_data.position;
			}

			archive << modifiers.size();
			for (auto& modifier : modifiers)
			{
				archive << (uint32_t)modifier->type;
				switch (modifier->type)
				{
				default:
				case Modifier::Type::Perlin:
					archive << ((PerlinModifier*)modifier.get())->octaves;
					archive << ((PerlinModifier*)modifier.get())->seed;
					((PerlinModifier*)modifier.get())->perlin_noise.Serialize(archive);
					break;
				case Modifier::Type::Voronoi:
					archive << ((VoronoiModifier*)modifier.get())->fade;
					archive << ((VoronoiModifier*)modifier.get())->shape;
					archive << ((VoronoiModifier*)modifier.get())->falloff;
					archive << ((VoronoiModifier*)modifier.get())->perturbation;
					archive << ((VoronoiModifier*)modifier.get())->seed;
					((VoronoiModifier*)modifier.get())->perlin_noise.Serialize(archive);
					break;
				case Modifier::Type::Heightmap:
					archive << ((HeightmapModifier*)modifier.get())->scale;
					archive << ((HeightmapModifier*)modifier.get())->data;
					archive << ((HeightmapModifier*)modifier.get())->width;
					archive << ((HeightmapModifier*)modifier.get())->height;
					break;
				}

				archive << (uint32_t)modifier->blend;
				archive << modifier->weight;
				archive << modifier->frequency;
			}
		}

		material_Base.Serialize(archive, seri);
		material_Slope.Serialize(archive, seri);
		material_LowAltitude.Serialize(archive, seri);
		material_HighAltitude.Serialize(archive, seri);
		material_GrassParticle.Serialize(archive, seri);
		weather.Serialize(archive, seri);
		grass_properties.Serialize(archive, seri);
		perlin_noise.Serialize(archive);
	}

}
