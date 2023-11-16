#include "wiTerrain.h"
#include "wiProfiler.h"
#include "wiTimer.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiScene.h"
#include "wiPhysics.h"
#include "wiImage.h"
#include "wiFont.h"
#include "wiTextureHelper.h"
#include "wiBacklog.h"

#include <mutex>
#include <string>
#include <atomic>
#include <random>

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
	inline ChunkIndices& chunk_indices()
	{
		static ChunkIndices state;
		return state;
	}

	struct Generator
	{
		wi::scene::Scene scene; // The background generation thread can safely add things to this, it will be merged into the main scene when it is safe to do so
		wi::jobsystem::context workload;
		std::atomic_bool cancelled{ false };
	};

	static std::mutex locker;


	void VirtualTextureAtlas::Residency::init(uint32_t resolution)
	{
		this->resolution = resolution;
		GraphicsDevice* device = GetDevice();

		uint32_t lod_count = 0;
		uint32_t tile_count = 0;
		uint32_t side = resolution;
		while (side >= SVT_TILE_SIZE)
		{
			tile_count += (side / SVT_TILE_SIZE) * (side / SVT_TILE_SIZE);
			side /= 2;
			lod_count++;
		}
		tile_count += 1; // packed mips

		if (resolution > SVT_TILE_SIZE)
		{
			uint32_t granularity = resolution / SVT_TILE_SIZE;

			TextureDesc td;
			td.width = granularity;
			td.height = granularity;

			td.format = Format::R32_UINT;
			td.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			td.usage = Usage::DEFAULT;
			td.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
			td.mip_levels = lod_count;
			bool success = device->CreateTexture(&td, nullptr, &residencyMap);
			assert(success);
			device->SetName(&residencyMap, "VirtualTexture::residencyMap");

			for (uint32_t i = 0; i < lod_count; ++i)
			{
				int subresource_index = device->CreateSubresource(&residencyMap, SubresourceType::UAV, 0, 1, i, 1);
				assert(subresource_index == i);
			}


			td.format = Format::R32_UINT; // shader atomic support needed
			td.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
			td.usage = Usage::DEFAULT;
			td.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
			td.mip_levels = 1;
			success = device->CreateTexture(&td, nullptr, &feedbackMap);
			assert(success);
			device->SetName(&feedbackMap, "VirtualTexture::feedbackMap");

			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::UNORDERED_ACCESS;
				bd.size = sizeof(uint32_t) * tile_count;
				success = device->CreateBuffer(&bd, nullptr, &requestBuffer);
				assert(success);
				device->SetName(&requestBuffer, "VirtualTexture::requestBuffer");
			}
			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::UNORDERED_ACCESS;
				bd.size = sizeof(uint32_t) * (tile_count + 1); // +1: atomic global counter
				success = device->CreateBuffer(&bd, nullptr, &allocationBuffer);
				assert(success);
				device->SetName(&allocationBuffer, "VirtualTexture::allocationBuffer");

				bd.misc_flags = {};
				bd.bind_flags = BindFlag::NONE;
				bd.usage = Usage::READBACK;
				for (int i = 0; i < arraysize(allocationBuffer_CPU_readback); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &allocationBuffer_CPU_readback[i]);
					assert(success);
					device->SetName(&allocationBuffer_CPU_readback[i], "VirtualTexture::allocationBuffer_CPU_readback[i]");
				}
			}
			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::SHADER_RESOURCE;
				bd.size = sizeof(uint32_t) * tile_count;
				success = device->CreateBuffer(&bd, nullptr, &pageBuffer);
				assert(success);
				device->SetName(&pageBuffer, "VirtualTexture::pageBuffer");

				bd.misc_flags = {};
				bd.usage = Usage::UPLOAD;
				bd.bind_flags = {};
				for (int i = 0; i < arraysize(pageBuffer_CPU_upload); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &pageBuffer_CPU_upload[i]);
					assert(success);
					device->SetName(&pageBuffer_CPU_upload[i], "VirtualTexture::pageBuffer_CPU_upload[i]");
				}
			}
		}
	}
	void VirtualTextureAtlas::Residency::reset()
	{
		for (int i = 0; i < arraysize(data_available_CPU); ++i)
		{
			data_available_CPU[i] = false;
		}
		cpu_resource_id = 0;
	}

	void VirtualTexture::init(VirtualTextureAtlas& atlas, uint resolution)
	{
		this->resolution = resolution;

		lod_count = 0;
		uint32_t tile_count = 0;
		uint32_t side = resolution;
		while (side >= SVT_TILE_SIZE)
		{
			tile_count += (side / SVT_TILE_SIZE) * (side / SVT_TILE_SIZE);
			side /= 2;
			lod_count++;
		}

		locker.lock();
		free(atlas);
		if (tile_count > 1)
		{
			// close by chunks have residency and packed mips
			residency = atlas.allocate_residency(resolution);
			tile_count += 1; // packed mip chain
			lod_count += 1; // packed mip chain
			tiles.resize(tile_count);
			tiles[tile_count - 1] = atlas.allocate_tile();
			tiles[tile_count - 2] = atlas.allocate_tile();
			if (tiles[tile_count - 1].IsValid())
			{
				// packed mip tail update:
				UpdateRequest& request = update_requests.emplace_back();
				request.lod = lod_count - 1;
				request.tile_x = tiles[tile_count - 1].x;
				request.tile_y = tiles[tile_count - 1].y;
				request.x = 0;
				request.y = 0;
			}
			if (tiles[tile_count - 2].IsValid())
			{
				// last nonpacked mip update:
				UpdateRequest& request = update_requests.emplace_back();
				request.lod = lod_count - 2;
				request.tile_x = tiles[tile_count - 2].x;
				request.tile_y = tiles[tile_count - 2].y;
				request.x = 0;
				request.y = 0;
			}
		}
		else
		{
			// far away chunks only have 1 tile
			tiles.resize(tile_count);
			tiles.back() = atlas.allocate_tile();
			if (tiles.back().IsValid())
			{
				// update the only tile there is:
				UpdateRequest& request = update_requests.emplace_back();
				request.lod = lod_count - 1;
				request.tile_x = tiles.back().x;
				request.tile_y = tiles.back().y;
				request.x = 0;
				request.y = 0;
			}
		}
		locker.unlock();

	}

	Terrain::Terrain()
	{
		weather.ambient = XMFLOAT3(0.4f, 0.4f, 0.4f);
		weather.SetRealisticSky(true);
		weather.SetRealisticSkyAerialPerspective(true);
		weather.SetVolumetricClouds(true);
		weather.volumetricCloudParameters.layerFirst.extinctionCoefficient = XMFLOAT3(0.71f * 0.05f, 0.86f * 0.05f, 1.0f * 0.05f);
		weather.volumetricCloudParameters.layerFirst.totalNoiseScale = 0.0009f;
		weather.volumetricCloudParameters.layerFirst.curlScale = 0.15f;
		weather.volumetricCloudParameters.layerFirst.detailNoiseModifier = 0.5;
		weather.volumetricCloudParameters.layerFirst.weatherScale = 0.000035f;
		weather.volumetricCloudParameters.layerFirst.coverageMinimum = 0.25f;
		weather.volumetricCloudParameters.layerFirst.typeAmount = 0.0f;
		weather.volumetricCloudParameters.layerFirst.typeMinimum = 0.0f;
		weather.volumetricCloudParameters.layerFirst.rainAmount = 0.9f;
		weather.volumetricCloudParameters.layerFirst.rainMinimum = 0.0f;
		weather.volumetricCloudParameters.layerFirst.gradientSmall = XMFLOAT4(0.01f, 0.07f, 0.08f, 0.14f);
		weather.volumetricCloudParameters.layerFirst.windSpeed = 20.0f;
		weather.volumetricCloudParameters.layerFirst.coverageWindSpeed = 35.0f;
		weather.volumetricCloudParameters.layerSecond.coverageAmount = 0.0f;
		weather.oceanParameters.waterHeight = -40;
		weather.oceanParameters.wave_amplitude = 120;
		weather.fogStart = 0;
		weather.fogDensity = 0.001f;
		weather.SetHeightFog(true);
		weather.fogHeightStart = 0;
		weather.fogHeightEnd = 500;
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


		for (auto it = chunks.begin(); it != chunks.end(); it++)
		{
			ChunkData& chunk_data = it->second;
			if (chunk_data.vt != nullptr)
			{
				chunk_data.vt->free(atlas);
			}
		}
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

	void Terrain::Generation_Update(const CameraComponent& camera)
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
			for (auto it = chunks.begin(); it != chunks.end(); it++)
			{
				ChunkData& chunk_data = it->second;
				if (chunk_data.vt != nullptr)
				{
					chunk_data.vt->free(atlas);
				}
			}
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
		GraphicsDevice* device = GetDevice();

		// Check whether there are any materials that would write to virtual textures:
		bool virtual_texture_any = false;
		bool virtual_texture_available[TEXTURESLOT_COUNT] = {};
		MaterialComponent* virtual_materials[4] = {
			&material_Base,
			&material_Slope,
			&material_LowAltitude,
			&material_HighAltitude,
		};
		for (auto& material : virtual_materials)
		{
			for (int i = 0; i < TEXTURESLOT_COUNT; ++i)
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
			chunk_data.visible = camera.frustum.CheckSphere(chunk_data.sphere.center, chunk_data.sphere.radius);
			const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

			if (wi::renderer::GetOcclusionCullingEnabled())
			{
				size_t object_index = scene->objects.GetIndex(chunk_data.entity);
				if (object_index < scene->occlusion_results_objects.size())
				{
					if (scene->occlusion_results_objects[object_index].IsOccluded())
					{
						chunk_data.visible = false;
					}
				}
			}

			// pointer refresh:
			MeshComponent* chunk_mesh = scene->meshes.GetComponent(chunk_data.entity);
			if (chunk_mesh != nullptr)
			{
				chunk_data.mesh_vertex_positions = chunk_mesh->vertex_positions.data();

				if (IsTessellationEnabled())
				{
					chunk_mesh->tessellationFactor = 32;
				}
				else
				{
					chunk_mesh->tessellationFactor = 0;
				}
				if (!chunk_mesh->bvh.IsValid())
				{
					chunk_mesh->SetBVHEnabled(true);
				}

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
					if (chunk_data.vt != nullptr)
					{
						chunk_data.vt->free(atlas);
					}
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
			if (chunk_data.visible && chunk_data.grass_entity != INVALID_ENTITY)
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

			it++;
		}

		if (virtual_texture_any)
		{
			UpdateVirtualTexturesCPU();
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
					mesh.SetQuantizedPositionsDisabled(true); // connecting meshes quantization is not correct because mismatching AABBs
					object.meshID = chunk_data.entity;
					mesh.indices = chunk_indices().indices;
					for (auto& lod : chunk_indices().lods)
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

					object.SetCastShadow(slope_cast_shadow.load());
					mesh.SetDoubleSidedShadow(slope_cast_shadow.load());

					wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
						mesh.CreateRenderData();
						chunk_data.sphere.center = mesh.aabb.getCenter();
						chunk_data.sphere.center.x += chunk_data.position.x;
						chunk_data.sphere.center.y += chunk_data.position.y;
						chunk_data.sphere.center.z += chunk_data.position.z;
						chunk_data.sphere.radius = mesh.aabb.getRadius();
						mesh.SetBVHEnabled(true);
					});

					// If there were any vertices in this chunk that could be valid for grass, store the grass particle system:
					if (grass_valid_vertex_count.load() > 0)
					{
						chunk_data.grass = std::move(grass); // the grass will be added to the scene later, only when the chunk is close to the camera (center chunk's neighbors)
						chunk_data.grass.meshID = chunk_data.entity;
						chunk_data.grass.strandCount = uint32_t(grass_valid_vertex_count.load() * 3 * chunk_scale * chunk_scale); // chunk_scale * chunk_scale : grass density increases with squared amount with chunk scale (x*z)
						chunk_data.grass.CreateFromMesh(mesh);
					}

					// Create the textures for virtual texture update:
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
							grass.CreateRenderData();
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

						if (prop_density > 0 && chunk_data.props_entity == INVALID_ENTITY && chunk_data.mesh_vertex_positions != nullptr)
						{
							chunk_data.props_entity = CreateEntity();
							generator->scene.transforms.Create(chunk_data.props_entity);
							generator->scene.names.Create(chunk_data.props_entity) = "props";
							generator->scene.Component_Attach(chunk_data.props_entity, chunk_data.entity, true);
							chunk_data.prop_density_current = prop_density;

							wi::random::RNG rng((uint32_t)chunk.compute_hash() ^ seed);

							for (const auto& prop : props)
							{
								if (prop.data.empty())
									continue;
								int gen_count = (int)rng.next_uint(
									uint32_t(prop.min_count_per_chunk * chunk_data.prop_density_current),
									std::max(1u, uint32_t(prop.max_count_per_chunk * chunk_data.prop_density_current))
								);
								for (int i = 0; i < gen_count; ++i)
								{
									uint32_t tri = rng.next_uint(0, chunk_indices().lods[0].indexCount / 3); // random triangle on the chunk mesh
									uint32_t ind0 = chunk_indices().indices[tri * 3 + 0];
									uint32_t ind1 = chunk_indices().indices[tri * 3 + 1];
									uint32_t ind2 = chunk_indices().indices[tri * 3 + 2];
									const XMFLOAT3& pos0 = chunk_data.mesh_vertex_positions[ind0];
									const XMFLOAT3& pos1 = chunk_data.mesh_vertex_positions[ind1];
									const XMFLOAT3& pos2 = chunk_data.mesh_vertex_positions[ind2];
									const XMFLOAT4 region0 = chunk_data.region_weights[ind0];
									const XMFLOAT4 region1 = chunk_data.region_weights[ind1];
									const XMFLOAT4 region2 = chunk_data.region_weights[ind2];
									// random barycentric coords on the triangle:
									float f = rng.next_float();
									float g = rng.next_float();
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
										transform->translation_local.y += wi::math::Lerp(prop.min_y_offset, prop.max_y_offset, rng.next_float());
										const float scaling = wi::math::Lerp(prop.min_size, prop.max_size, rng.next_float());
										transform->Scale(XMFLOAT3(scaling, scaling, scaling));
										transform->RotateRollPitchYaw(XMFLOAT3(0, XM_2PI * rng.next_float(), 0));
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

	void Terrain::CreateChunkRegionTexture(ChunkData& chunk_data)
	{
		if (!chunk_data.region_weights_texture.IsValid())
		{
			GraphicsDevice* device = GetDevice();
			TextureDesc desc;
			desc.width = (uint32_t)chunk_width;
			desc.height = (uint32_t)chunk_width;
			desc.format = Format::R8G8B8A8_UNORM;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			SubresourceData data;
			data.data_ptr = chunk_data.region_weights.data();
			data.row_pitch = chunk_width * sizeof(chunk_data.region_weights[0]);
			bool success = device->CreateTexture(&desc, &data, &chunk_data.region_weights_texture);
			assert(success);
		}
	}

	void Terrain::UpdateVirtualTexturesCPU()
	{
		GraphicsDevice* device = GetDevice();
		virtual_textures_in_use.clear();
		virtual_texture_barriers_before_update.clear();
		virtual_texture_barriers_after_update.clear();
		virtual_texture_barriers_before_allocation.clear();
		virtual_texture_barriers_after_allocation.clear();

		if (!sampler.IsValid())
		{
			SamplerDesc samplerDesc;
			samplerDesc.filter = Filter::ANISOTROPIC;
			samplerDesc.max_anisotropy = 8; // The atlas tile border can take up to 8x anisotropic
			samplerDesc.address_u = TextureAddressMode::CLAMP;
			samplerDesc.address_v = TextureAddressMode::CLAMP;
			samplerDesc.address_w = TextureAddressMode::CLAMP;
			bool success = device->CreateSampler(&samplerDesc, &sampler);
			assert(success);
		}

		wi::jobsystem::context ctx;
		for (auto& it : chunks)
		{
			const Chunk& chunk = it.first;
			ChunkData& chunk_data = it.second;
			const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

			MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);
			if (material == nullptr)
				continue;

			material->sampler_descriptor = device->GetDescriptorIndex(&sampler);

			// This should have been created on generation thread, but if not (serialized), create it last minute:
			CreateChunkRegionTexture(chunk_data);

			if (!atlas.IsValid())
			{
				const uint32_t physical_width = 16384u;
				const uint32_t physical_height = 8192u;
				GPUBufferDesc tile_pool_desc;

				for (uint32_t map_type = 0; map_type < arraysize(atlas.maps); ++map_type)
				{
					TextureDesc desc;
					desc.width = physical_width;
					desc.height = physical_height;
					desc.misc_flags = ResourceMiscFlag::SPARSE;
					desc.bind_flags = BindFlag::SHADER_RESOURCE;
					desc.mip_levels = 1;
					desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;

					TextureDesc desc_raw_block = desc;
					desc_raw_block.width /= 4;
					desc_raw_block.height /= 4;
					desc_raw_block.bind_flags = BindFlag::UNORDERED_ACCESS;
					desc_raw_block.layout = ResourceState::UNORDERED_ACCESS;

					switch (map_type)
					{
					default:
						desc.format = Format::BC1_UNORM_SRGB;
						desc_raw_block.format = Format::R32G32_UINT;
						break;
					case MaterialComponent::NORMALMAP:
						desc.format = Format::BC5_UNORM;
						desc_raw_block.format = Format::R32G32B32A32_UINT;
						desc.swizzle.r = ComponentSwizzle::R;
						desc.swizzle.g = ComponentSwizzle::G;
						desc.swizzle.b = ComponentSwizzle::ONE;
						desc.swizzle.a = ComponentSwizzle::ONE;
						break;
					case MaterialComponent::SURFACEMAP:
						desc.format = Format::BC3_UNORM;
						desc_raw_block.format = Format::R32G32B32A32_UINT;
						break;
					}

					bool success = device->CreateTexture(&desc, nullptr, &atlas.maps[map_type].texture);
					assert(success);

					success = device->CreateTexture(&desc_raw_block, nullptr, &atlas.maps[map_type].texture_raw_block);
					assert(success);

					assert(atlas.maps[map_type].texture.sparse_properties->total_tile_count == atlas.maps[map_type].texture_raw_block.sparse_properties->total_tile_count);
					assert(atlas.maps[map_type].texture.sparse_page_size == atlas.maps[map_type].texture_raw_block.sparse_page_size);

					tile_pool_desc.size += atlas.maps[map_type].texture.sparse_properties->total_tile_count * atlas.maps[map_type].texture.sparse_page_size;
					tile_pool_desc.alignment = std::max(tile_pool_desc.alignment, atlas.maps[map_type].texture.sparse_page_size);

					for (uint32_t i = 0; i < atlas.maps[map_type].texture_raw_block.desc.mip_levels; ++i)
					{
						int subresource_index = device->CreateSubresource(&atlas.maps[map_type].texture_raw_block, SubresourceType::UAV, 0, 1, i, 1);
						assert(subresource_index == i);
					}
				}

				tile_pool_desc.misc_flags = ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS;
				bool success = device->CreateBuffer(&tile_pool_desc, nullptr, &atlas.tile_pool);
				assert(success);

				const uint32_t physical_tile_count_x = physical_width / SVT_TILE_SIZE_PADDED;
				const uint32_t physical_tile_count_y = physical_height / SVT_TILE_SIZE_PADDED;

				atlas.free_tiles.clear();
				atlas.free_tiles.reserve(physical_tile_count_x * physical_tile_count_y);
				for (uint8_t y = 0; y < physical_tile_count_y; ++y)
				{
					for (uint8_t x = 0; x < physical_tile_count_x; ++x)
					{
						VirtualTextureAtlas::Tile& tile = atlas.free_tiles.emplace_back();
						tile.x = x;
						tile.y = y;
					}
				}

				uint32_t offset = 0;
				for (uint32_t map_type = 0; map_type < arraysize(atlas.maps); ++map_type)
				{
					// Sparse mapping for block compression aliasing:
					SparseUpdateCommand commands[2];
					commands[0].sparse_resource = &atlas.maps[map_type].texture;
					commands[0].tile_pool = &atlas.tile_pool;
					commands[0].num_resource_regions = 1;
					SparseResourceCoordinate coordinate;
					coordinate.x = 0;
					coordinate.y = 0;
					commands[0].coordinates = &coordinate;
					SparseRegionSize region;
					region.width = atlas.maps[map_type].texture.desc.width / atlas.maps[map_type].texture.sparse_properties->tile_width;
					region.height = atlas.maps[map_type].texture.desc.height / atlas.maps[map_type].texture.sparse_properties->tile_height;
					commands[0].sizes = &region;
					TileRangeFlags flags = {};
					commands[0].range_flags = &flags;
					commands[0].range_start_offsets = &offset;
					uint32_t count = atlas.maps[map_type].texture.sparse_properties->total_tile_count;
					commands[0].range_tile_counts = &count;
					commands[1] = commands[0];
					commands[1].sparse_resource = &atlas.maps[map_type].texture_raw_block;
					device->SparseUpdate(QUEUE_COPY, commands, arraysize(commands));
					offset += count;
				}
			}

			if (chunk_data.vt == nullptr)
			{
				chunk_data.vt = std::make_shared<VirtualTexture>();
			}
			VirtualTexture& vt = *chunk_data.vt;

			const uint32_t min_resolution = SVT_TILE_SIZE;
			const uint32_t max_resolution = 65536u;
			const uint32_t required_resolution = dist < 2 ? max_resolution : min_resolution;
			//const uint32_t required_resolution = std::max(min_resolution, max_resolution >> std::min(7, std::max(0, dist - 1)));

			if (vt.resolution != required_resolution)
			{
				vt.init(atlas, required_resolution);

				for (uint32_t map_type = 0; map_type < arraysize(atlas.maps); ++map_type)
				{
					material->textures[map_type].resource.SetTexture(atlas.maps[map_type].texture);
					if (vt.residency != nullptr)
					{
						material->texMulAdd = XMFLOAT4(1, 1, 0, 0);
						material->textures[map_type].sparse_residencymap_descriptor = device->GetDescriptorIndex(&vt.residency->residencyMap, SubresourceType::SRV);
						if (map_type == 0)
						{
							// Only first texture slot will write feedback
							material->textures[map_type].sparse_feedbackmap_descriptor = device->GetDescriptorIndex(&vt.residency->feedbackMap, SubresourceType::UAV);
						}
						else
						{
							material->textures[map_type].sparse_feedbackmap_descriptor = -1;
						}
					}
					else
					{
						// Simple chunks without residency only have 1 mapped tile, and no mips so they simply use a texMulAdd (todo)
						auto tile = vt.tiles.back();
						const float2 resolution_rcp = float2(
							1.0f / (float)atlas.maps[map_type].texture.desc.width,
							1.0f / (float)atlas.maps[map_type].texture.desc.height
						);
						if (map_type == 0)
						{
							material->texMulAdd.x = (float)SVT_TILE_SIZE * resolution_rcp.x;
							material->texMulAdd.y = (float)SVT_TILE_SIZE * resolution_rcp.y;
							material->texMulAdd.z = ((float)tile.x * (float)SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER) * resolution_rcp.x;
							material->texMulAdd.w = ((float)tile.y * (float)SVT_TILE_SIZE_PADDED + SVT_TILE_BORDER) * resolution_rcp.y;
						}
						material->textures[map_type].sparse_residencymap_descriptor = -1;
						material->textures[map_type].sparse_feedbackmap_descriptor = -1;
					}
					material->textures[map_type].lod_clamp = (float)vt.lod_count - 2;
				}
				vt.region_weights_texture = chunk_data.region_weights_texture;
			}

			virtual_textures_in_use.push_back(&vt);


			if (vt.residency == nullptr)
				continue;

			// Process each virtual texture on a background thread:
			wi::jobsystem::Execute(ctx, [this, &vt](wi::jobsystem::JobArgs args) {

				const uint32_t width = vt.residency->feedbackMap.desc.width;
				const uint32_t height = vt.residency->feedbackMap.desc.height;

				// We must only access persistently mapped resources by CPU that the GPU is not using currently:
				vt.residency->cpu_resource_id = (vt.residency->cpu_resource_id + 1) % arraysize(vt.residency->allocationBuffer_CPU_readback);
				const bool data_available_CPU = vt.residency->data_available_CPU[vt.residency->cpu_resource_id]; // indicates whether any GPU data is readable at this point or not
				vt.residency->data_available_CPU[vt.residency->cpu_resource_id] = true;

				// Perform the allocations and deallocations:
				//	GPU writes allocation requests by virtualTextureTileAllocateCS.hlsl compute shader
				if (data_available_CPU)
				{
					uint32_t page_count = 0;
					uint32_t lod_offsets[10] = {};
					for (uint32_t i = 0; i < vt.lod_count; ++i)
					{
						const uint32_t l_width = std::max(1u, width >> i);
						const uint32_t l_height = std::max(1u, height >> i);
						lod_offsets[i] = page_count;
						page_count += l_width * l_height;
					}

					uint32_t allocation_count = *(const uint32_t*)vt.residency->allocationBuffer_CPU_readback[vt.residency->cpu_resource_id].mapped_data;
					allocation_count = std::min(uint32_t(vt.tiles.size() - 1), allocation_count);
					//allocation_count = std::min(100u, allocation_count);
					const uint32_t* allocation_requests = ((const uint32_t*)vt.residency->allocationBuffer_CPU_readback[vt.residency->cpu_resource_id].mapped_data) + 1; // +1 offset of the allocation counter
					for (uint32_t i = 0; i < allocation_count; ++i)
					{
						const uint32_t allocation_request = allocation_requests[i];
						const uint8_t x = (allocation_request >> 24u) & 0xFF;
						const uint8_t y = (allocation_request >> 16u) & 0xFF;
						const uint8_t lod = (allocation_request >> 8u) & 0xFF;
						const bool allocate = allocation_request & 0x1;
						const bool must_be_always_resident = (int)lod >= ((int)vt.lod_count - 2);
						if (lod >= vt.lod_count)
							continue;
						const uint32_t l_offset = lod_offsets[lod];
						const uint32_t l_width = std::max(1u, width >> lod);
						const uint32_t l_height = std::max(1u, height >> lod);
						const uint32_t l_index = l_offset + x + y * l_width;
						if (x >= l_width || y >= l_height)
							continue;
						VirtualTextureAtlas::Tile& tile = vt.tiles[l_index];

						if (allocate)
						{
							if (tile.IsValid())
							{
								continue;
							}
							locker.lock();
							tile = atlas.allocate_tile();
							locker.unlock();

							if (tile.IsValid())
							{
								VirtualTexture::UpdateRequest& request = vt.update_requests.emplace_back();
								request.x = x;
								request.y = y;
								request.lod = lod;
								request.tile_x = tile.x;
								request.tile_y = tile.y;
							}
						}
						else if (!must_be_always_resident)
						{
							if (!tile.IsValid())
							{
								continue;
							}
							locker.lock();
							atlas.free_tile(tile);
							locker.unlock();
						}
					}
				}

				// Update page buffer for GPU:
				uint32_t* page_buffer = (uint32_t*)vt.residency->pageBuffer_CPU_upload[vt.residency->cpu_resource_id].mapped_data;
				for (size_t i = 0; i < vt.tiles.size(); ++i)
				{
					uint32_t page = uint32_t(uint32_t(vt.tiles[i].x) | (uint32_t(vt.tiles[i].y) << 8u));
					// force memcpy into uncached memory to avoid read stall by mistake:
					std::memcpy(page_buffer + i, &page, sizeof(page));
				}

			});

			virtual_texture_barriers_before_update.push_back(GPUBarrier::Buffer(&vt.residency->pageBuffer, ResourceState::COPY_DST, ResourceState::SHADER_RESOURCE_COMPUTE));
			virtual_texture_barriers_before_update.push_back(GPUBarrier::Image(&vt.residency->feedbackMap, vt.residency->feedbackMap.desc.layout, ResourceState::UNORDERED_ACCESS));
			virtual_texture_barriers_before_update.push_back(GPUBarrier::Image(&vt.residency->residencyMap, vt.residency->residencyMap.desc.layout, ResourceState::UNORDERED_ACCESS));
			virtual_texture_barriers_after_update.push_back(GPUBarrier::Image(&vt.residency->residencyMap, ResourceState::UNORDERED_ACCESS, vt.residency->residencyMap.desc.layout));
			virtual_texture_barriers_before_allocation.push_back(GPUBarrier::Image(&vt.residency->feedbackMap, ResourceState::UNORDERED_ACCESS, vt.residency->feedbackMap.desc.layout));
			virtual_texture_barriers_after_allocation.push_back(GPUBarrier::Buffer(&vt.residency->allocationBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SRC));
		}
		wi::jobsystem::Wait(ctx);
	}

	void Terrain::UpdateVirtualTexturesGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Terrain - UpdateVirtualTexturesGPU", cmd);
		auto range = wi::profiler::BeginRangeGPU("Terrain - UpdateVirtualTexturesGPU", cmd);

		device->Barrier(virtual_texture_barriers_before_update.data(), (uint32_t)virtual_texture_barriers_before_update.size(), cmd);

		device->EventBegin("Clear Metadata", cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->residency == nullptr)
				continue;
			device->ClearUAV(&vt->residency->feedbackMap, 0xFF, cmd);
			device->ClearUAV(&vt->residency->requestBuffer, 0xFF, cmd);
			device->ClearUAV(&vt->residency->allocationBuffer, 0, cmd);
		}
		device->EventEnd(cmd);

		device->EventBegin("Update Residency Maps", cmd);
		device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_VIRTUALTEXTURE_RESIDENCYUPDATE), cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->residency == nullptr)
				continue;
			VirtualTextureResidencyUpdateCB cb;
			cb.lodCount = vt->lod_count;
			cb.width = vt->residency->residencyMap.desc.width;
			cb.height = vt->residency->residencyMap.desc.height;
			cb.pageBufferRO = device->GetDescriptorIndex(&vt->residency->pageBuffer, SubresourceType::SRV);
			for (uint mip = 0; mip < arraysize(cb.residencyTextureRW_mips); ++mip)
			{
				if (mip < vt->lod_count)
				{
					cb.residencyTextureRW_mips[mip].x = device->GetDescriptorIndex(&vt->residency->residencyMap, SubresourceType::UAV, mip);
				}
				else
				{
					cb.residencyTextureRW_mips[mip].x = -1;
				}
			}
			device->BindDynamicConstantBuffer(cb, 0, cmd);

			device->Dispatch(
				(vt->residency->residencyMap.desc.width + 7u) / 8u,
				(vt->residency->residencyMap.desc.height + 7u) / 8u,
				1u,
				cmd
			);
		}
		device->EventEnd(cmd);

		device->EventBegin("Render Tile Regions", cmd);

		ShaderMaterial materials[4];
		material_Base.WriteShaderMaterial(&materials[0]);
		material_Slope.WriteShaderMaterial(&materials[1]);
		material_LowAltitude.WriteShaderMaterial(&materials[2]);
		material_HighAltitude.WriteShaderMaterial(&materials[3]);
		device->BindDynamicConstantBuffer(materials, 0, cmd);

		for (uint32_t map_type = 0; map_type < 3; map_type++)
		{
			switch (map_type)
			{
			case MaterialComponent::BASECOLORMAP:
				device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_BASECOLORMAP), cmd);
				device->BindUAV(&atlas.maps[MaterialComponent::BASECOLORMAP].texture_raw_block, 0, cmd);
				break;
			case MaterialComponent::NORMALMAP:
				device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_NORMALMAP), cmd);
				device->BindUAV(&atlas.maps[MaterialComponent::NORMALMAP].texture_raw_block, 0, cmd);
				break;
			case MaterialComponent::SURFACEMAP:
				device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_SURFACEMAP), cmd);
				device->BindUAV(&atlas.maps[MaterialComponent::SURFACEMAP].texture_raw_block, 0, cmd);
				break;
			default:
				assert(0);
				break;
			}

			for (const VirtualTexture* vt : virtual_textures_in_use)
			{
				for (auto& request : vt->update_requests)
				{
					uint request_lod_resolution = std::max(1u, vt->resolution >> request.lod);
					const uint2 write_offset_original = uint2(
						request.tile_x * SVT_TILE_SIZE_PADDED / 4,
						request.tile_y * SVT_TILE_SIZE_PADDED / 4
					);

					TerrainVirtualTexturePush push;
					push.offset = int2(
						int(request.x * SVT_TILE_SIZE) - int(SVT_TILE_BORDER),
						int(request.y * SVT_TILE_SIZE) - int(SVT_TILE_BORDER)
					);
					push.region_weights_textureRO = device->GetDescriptorIndex(&vt->region_weights_texture, SubresourceType::SRV);

					if (request_lod_resolution < SVT_TILE_SIZE)
					{
						// packed mips

						uint32_t tail_mip_idx = 0;
						while (request_lod_resolution >= 4u)
						{
							push.resolution_rcp = 1.0f / request_lod_resolution;
							push.write_offset = write_offset_original;
							push.write_offset.x += SVT_PACKED_MIP_OFFSETS[tail_mip_idx].x / 4;
							push.write_offset.y += SVT_PACKED_MIP_OFFSETS[tail_mip_idx].y / 4;
							push.write_size = (SVT_TILE_BORDER + request_lod_resolution + SVT_TILE_BORDER) / 4;
							device->PushConstants(&push, sizeof(push), cmd);

							device->Dispatch(
								(push.write_size + 7u) / 8u,
								(push.write_size + 7u) / 8u,
								1,
								cmd
							);

							request_lod_resolution /= 2u;
							tail_mip_idx++;
						}
					}
					else
					{
						push.resolution_rcp = 1.0f / request_lod_resolution;
						push.write_offset = write_offset_original;
						push.write_size = SVT_TILE_SIZE_PADDED / 4u;
						device->PushConstants(&push, sizeof(push), cmd);

						device->Dispatch(
							(push.write_size + 7u) / 8u,
							(push.write_size + 7u) / 8u,
							1,
							cmd
						);
					}
				}
			}
		}
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			vt->update_requests.clear();
		}
		device->EventEnd(cmd);

		device->Barrier(virtual_texture_barriers_after_update.data(), (uint32_t)virtual_texture_barriers_after_update.size(), cmd);

		wi::profiler::EndRange(range);
		device->EventEnd(cmd);
	}

	void Terrain::CopyVirtualTexturePageStatusGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Terrain - CopyVirtualTexturePageStatusGPU", cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->residency == nullptr)
				continue;
			device->CopyResource(&vt->residency->pageBuffer, &vt->residency->pageBuffer_CPU_upload[vt->residency->cpu_resource_id], cmd);
		}
		device->EventEnd(cmd);
	}

	void Terrain::AllocateVirtualTextureTileRequestsGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("Terrain - AllocateVirtualTextureTileRequestsGPU", cmd);
		device->Barrier(virtual_texture_barriers_before_allocation.data(), (uint32_t)virtual_texture_barriers_before_allocation.size(), cmd);

		{
			device->EventBegin("Tile Requests", cmd);
			device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_VIRTUALTEXTURE_TILEREQUESTS), cmd);
			for (const VirtualTexture* vt : virtual_textures_in_use)
			{
				if (vt->residency == nullptr)
					continue;
				VirtualTextureTileRequestsPush push;
				push.lodCount = vt->lod_count;
				push.width = vt->residency->feedbackMap.desc.width;
				push.height = vt->residency->feedbackMap.desc.height;
				push.feedbackTextureRO = device->GetDescriptorIndex(&vt->residency->feedbackMap, SubresourceType::SRV);
				push.requestBufferRW = device->GetDescriptorIndex(&vt->residency->requestBuffer, SubresourceType::UAV);
				device->PushConstants(&push, sizeof(push), cmd);

				device->Dispatch(
					(std::max(1u, vt->residency->feedbackMap.desc.width / 2) + 7u) / 8u,
					(std::max(1u, vt->residency->feedbackMap.desc.height / 2) + 7u) / 8u,
					1u,
					cmd
				);
			}
			device->EventEnd(cmd);
		}

		GPUBarrier memory_barrier = GPUBarrier::Memory();
		device->Barrier(&memory_barrier, 1, cmd);

		{
			device->EventBegin("Tile Allocation Requests", cmd);
			device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_VIRTUALTEXTURE_TILEALLOCATE), cmd);
			for (const VirtualTexture* vt : virtual_textures_in_use)
			{
				if (vt->residency == nullptr)
					continue;
				VirtualTextureTileAllocatePush push;
				push.threadCount = (uint)vt->tiles.size();
				push.lodCount = vt->lod_count;
				push.width = vt->residency->feedbackMap.desc.width;
				push.height = vt->residency->feedbackMap.desc.height;
				push.pageBufferRO = device->GetDescriptorIndex(&vt->residency->pageBuffer, SubresourceType::SRV);
				push.requestBufferRW = device->GetDescriptorIndex(&vt->residency->requestBuffer, SubresourceType::UAV);
				push.allocationBufferRW = device->GetDescriptorIndex(&vt->residency->allocationBuffer, SubresourceType::UAV);
				device->PushConstants(&push, sizeof(push), cmd);

				device->Dispatch(
					(push.threadCount + 63u) / 64u,
					1u,
					1u,
					cmd
				);
			}
			device->EventEnd(cmd);
		}

		device->Barrier(virtual_texture_barriers_after_allocation.data(), (uint32_t)virtual_texture_barriers_after_allocation.size(), cmd);

		device->EventEnd(cmd);
	}

	void Terrain::WritebackTileRequestsGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("Terrain - WritebackTileRequestsGPU", cmd);

		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->residency == nullptr)
				continue;
			device->CopyResource(&vt->residency->allocationBuffer_CPU_readback[vt->residency->cpu_resource_id], &vt->residency->allocationBuffer, cmd);
		}

		device->EventEnd(cmd);
	}

	void Terrain::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		Generation_Cancel();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> lod_multiplier;
			if (seri.GetVersion() < 3)
			{
				float texlod;
				archive >> texlod;
			}
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
			if (seri.GetVersion() >= 2 && seri.GetVersion() < 3)
			{
				uint32_t target_texture_resolution;
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
			if (seri.GetVersion() < 3)
			{
				float texlod = 1;
				archive << texlod;
			}
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
			if (seri.GetVersion() >= 2 && seri.GetVersion() < 3)
			{
				uint32_t target_texture_resolution = 1024;
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
