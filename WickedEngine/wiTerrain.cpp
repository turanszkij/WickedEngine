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
#include <deque>

// This will do terrain rendering without sparse texture usage, with extra tile copies for block compression:
//#define NOSPARSE

#ifdef __APPLE__
// On Apple Metal API the sparse texture doesn't seem to work with block compression aliasing, so use this mode as workaround:
#define NOSPARSE
#endif // __APPLE__

// Logical texture resolution limits:
const uint32_t min_virtual_resolution = SVT_TILE_SIZE;
const uint32_t max_virtual_resolution = 65536u;

#ifdef NOSPARSE
// Try to account for memory increase in no-sparse mode, reduce atlas size:
const uint32_t atlas_physical_width = 16384u;
const uint32_t atlas_physical_height = 8192u;
#else
const uint32_t atlas_physical_width = 16384u;
const uint32_t atlas_physical_height = 16384u;
#endif // NOSPARSE

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
		wi::vector<SplineComponent> splines;
		wi::vector<wi::ecs::Entity> spline_entities;
		wi::vector<Chunk> removable_chunks; // chunks that were invalidated are regenerated on the generator thread. Before merging them with the scene, the previous version of them will need to be removed from the destination scene
		std::deque<Chunk> priority_invalidation; // to not let invalidation stuck at same chunks every frame while editing splines, for more appealing visual feedback
		wi::vector<Chunk> build_batch; // Reused each generation run (cleared, capacity kept) to avoid per-frame allocation
	};

	wi::jobsystem::context virtual_texture_ctx;

	static std::mutex locker;

	constexpr void weight_norm(XMFLOAT4& weights)
	{
		const float norm = 1.0f / (weights.x + weights.y + weights.z + weights.w);
		weights.x *= norm;
		weights.y *= norm;
		weights.z *= norm;
		weights.w *= norm;
	};

	inline int world_to_chunk_index(float world_pos, float scale, float scale_rcp)
	{
		return (int)std::floor((world_pos + chunk_half_width * scale) * chunk_width_rcp * scale_rcp);
	}

	inline void overlapped_chunk_range(float min_world, float max_world, float scale, float scale_rcp, int& index_min, int& index_max)
	{
		index_min = world_to_chunk_index(min_world, scale, scale_rcp);
		const int end_index = world_to_chunk_index(max_world, scale, scale_rcp);
		const float side = float(chunk_width - 1) * scale;
		const float end_min = (float(end_index) - 0.5f) * side;
		index_max = (max_world <= end_min) ? end_index - 1 : end_index;
		if (index_max < index_min)
		{
			index_max = index_min;
		}
	}


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

			td.format = Format::R8G8B8A8_UINT;
			td.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			td.usage = Usage::DEFAULT;
			td.layout = ResourceState::UNORDERED_ACCESS;
			td.mip_levels = lod_count;
			wi::vector<uint32_t> default_data(td.width * td.height, 0xff);
			wi::vector<SubresourceData> initdata(lod_count);
			for (uint32_t lod = 0; lod < lod_count; ++lod)
			{
				initdata[lod].data_ptr = default_data.data();
				initdata[lod].row_pitch = sizeof(uint32_t) * std::max(1u, td.width >> lod);
			}
			bool success = device->CreateTexture(&td, initdata.data(), &residencyMap);
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
			td.layout = ResourceState::UNORDERED_ACCESS;
			td.mip_levels = 1;
			success = device->CreateTexture(&td, initdata.data(), &feedbackMap);
			assert(success);
			device->SetName(&feedbackMap, "VirtualTexture::feedbackMap");

			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::UNORDERED_ACCESS;
				bd.size = sizeof(uint32_t) * tile_count;
				success = device->CreateBufferClearedWithType(&bd, 0x000000FFu, &requestBuffer);
				assert(success);
				device->SetName(&requestBuffer, "VirtualTexture::requestBuffer");
			}
			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::UNORDERED_ACCESS;
				bd.size = sizeof(uint32_t) * (tile_count + 1); // +1: atomic global counter
				wi::vector<uint32_t> data(tile_count + 1);
				success = device->CreateBuffer(&bd, data.data(), &allocationBuffer);
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
				bd.format = Format::R16_UINT;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::SHADER_RESOURCE;
				bd.size = GetFormatStride(bd.format) * tile_count;
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

		std::scoped_lock lck(locker);
		free(atlas);
		if (tile_count > 1)
		{
			// close by chunks have residency and packed mips
			residency = atlas.allocate_residency(resolution);
			tile_count += 1; // packed mip chain
			lod_count += 1; // packed mip chain
			tiles.resize(tile_count);
			if (atlas.allocate_tile(tiles[tile_count - 1]))
			{
				// packed mip tail update:
				UpdateRequest& request = update_requests.emplace_back();
				request.lod = lod_count - 1;
				request.tile_x = tiles[tile_count - 1].x;
				request.tile_y = tiles[tile_count - 1].y;
				request.x = 0;
				request.y = 0;
			}
			if (atlas.allocate_tile(tiles[tile_count - 2]))
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
			if (atlas.allocate_tile(tiles.back()))
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
		weather.oceanParameters.waterHeight = -10;
		weather.oceanParameters.wave_amplitude = 1000;
		weather.oceanParameters.patch_length = 20;
		weather.fogStart = 0;
		weather.fogDensity = 0.001f;
		weather.SetHeightFog(true);
		weather.fogHeightStart = 0;
		weather.fogHeightEnd = 500;
		weather.windDirection = XMFLOAT3(0.05f, 0.05f, 0.05f);
		weather.windSpeed = 4;
		weather.stars = 0.6f;

		grass_properties.viewDistance = chunk_width;

		generator = wi::allocator::make_shared_single<Generator>();

		materialEntities.resize(MATERIAL_COUNT);
	}
	Terrain::~Terrain()
	{
		Generation_Cancel();
		wi::jobsystem::Wait(virtual_texture_ctx);
	}

	void Terrain::Generation_Restart()
	{
		SetGenerationStarted(true);
		Generation_Cancel();
		generator->scene.Clear();

		// save material parameters:
		materials.resize(materialEntities.size());
		for (int i = 0; i < materialEntities.size(); ++i)
		{
			MaterialComponent* material = scene->materials.GetComponent(materialEntities[i]);
			if (material == nullptr)
				continue;
			materials[i] = *material;
			materials[i].SetDirty(false);
		}

		// save grass parameters:
		if (scene->hairs.Contains(grassEntity))
		{
			grass_properties = *scene->hairs.GetComponent(grassEntity);
			grass_properties.SetDirty(false);
		}
		if (scene->materials.Contains(grassEntity))
		{
			grass_material = *scene->materials.GetComponent(grassEntity);
			grass_material.SetDirty(false);
		}

		for (auto it = chunks.begin(); it != chunks.end(); it++)
		{
			ChunkData& chunk_data = it->second;
			if (chunk_data.vt != nullptr)
			{
				chunk_data.vt->free(atlas);
			}
		}
		chunks.clear();
		prop_chunks.clear();
		virtual_textures_in_use.clear();

		if (chunkGroupEntity != INVALID_ENTITY)
		{
			scene->Entity_Remove(chunkGroupEntity);
		}

		perlin_noise.init(seed);
		for (auto& modifier : modifiers)
		{
			modifier->Seed(seed);
		}

		// Add some nice weather and lighting if there are no weathers in the scene yet:
		bool created_terrain_weather = false;
		if (scene->weathers.GetCount() == 0)
		{
			if (!scene->weathers.Contains(terrainEntity))
			{
				scene->weathers.Create(terrainEntity);
				created_terrain_weather = true;
			}
		}
		if (created_terrain_weather)
		{
			*scene->weathers.GetComponent(terrainEntity) = weather;
		}
		if (!weather.IsOceanEnabled())
		{
			scene->ocean = {};
		}

		// Restore surface source materials:
		{
			// Clean up invalid material entities that no longer exist in the scene
			for (size_t i = 0; i < materialEntities.size(); )
			{
				if (materialEntities[i] != INVALID_ENTITY && !scene->materials.Contains(materialEntities[i]))
				{
					materialEntities.erase(materialEntities.begin() + i);
					if (i < materials.size())
					{
						materials.erase(materials.begin() + i);
					}
					continue;
				}
				++i;
			}

			// Restore valid materials
			for (size_t i = 0; i < materialEntities.size(); ++i)
			{
				if (materialEntities[i] == INVALID_ENTITY)
				{
					materialEntities[i] = CreateEntity();
				}
				scene->Component_Attach(materialEntities[i], terrainEntity);
				if (!scene->materials.Contains(materialEntities[i]))
				{
					scene->materials.Create(materialEntities[i]);
				}
				*scene->materials.GetComponent(materialEntities[i]) = materials[i];
			}
		}

		// Restore grass parameters:
		if (IsGrassEnabled()) {
			if (grassEntity == INVALID_ENTITY)
			{
				grassEntity = CreateEntity();
			}
			if (!scene->hairs.Contains(grassEntity))
			{
				scene->hairs.Create(grassEntity);
			}
			if (!scene->materials.Contains(grassEntity))
			{
				scene->materials.Create(grassEntity);
			}
			if (!scene->names.Contains(grassEntity))
			{
				scene->names.Create(grassEntity) = "grass";
			}
			*scene->hairs.GetComponent(grassEntity) = grass_properties;
			*scene->materials.GetComponent(grassEntity) = grass_material;
		}

		if (chunkGroupEntity == INVALID_ENTITY)
		{
			chunkGroupEntity = CreateEntity();
		}
		scene->Component_Attach(chunkGroupEntity, terrainEntity);
		if (!scene->names.Contains(chunkGroupEntity))
		{
			scene->names.Create(chunkGroupEntity) = "chunks";
		}
	}

	bool Terrain::RemovePropChunksOverlapping(const Chunk& terrain_chunk)
	{
		if (scene == nullptr)
		{
			return false;
		}
		const float half = chunk_half_width * chunk_scale;
		const float pos_x = float(terrain_chunk.x * (chunk_width - 1)) * chunk_scale;
		const float pos_z = float(terrain_chunk.z * (chunk_width - 1)) * chunk_scale;
		int x0, x1, z0, z1;
		overlapped_chunk_range(pos_x - half, pos_x + half, 1.0f, 1.0f, x0, x1);
		overlapped_chunk_range(pos_z - half, pos_z + half, 1.0f, 1.0f, z0, z1);
		bool removed = false;
		for (int z = z0; z <= z1; ++z)
		{
			for (int x = x0; x <= x1; ++x)
			{
				Chunk prop_chunk;
				prop_chunk.x = x;
				prop_chunk.z = z;
				auto it = prop_chunks.find(prop_chunk);
				if (it == prop_chunks.end())
				{
					continue;
				}
				if (it->second.entity != INVALID_ENTITY)
				{
					scene->Entity_Remove(it->second.entity);
					removed = true;
				}
				prop_chunks.erase(it);
			}
		}
		return removed;
	}

	bool Terrain::PlacePropChunk(const Chunk& prop_chunk)
	{
		if (!(prop_density > 0.0f) || props.empty())
		{
			return false;
		}
		{
			auto existing = prop_chunks.find(prop_chunk);
			if (existing != prop_chunks.end() && existing->second.entity != INVALID_ENTITY)
			{
				return false;
			}
		}

		// Prop gen is fixed at chunk_scale=1
		const float prop_pos_x = float(prop_chunk.x * (chunk_width - 1));
		const float prop_pos_z = float(prop_chunk.z * (chunk_width - 1));
		const float prop_min_x = prop_pos_x - chunk_half_width;
		const float prop_max_x = prop_pos_x + chunk_half_width;
		const float prop_min_z = prop_pos_z - chunk_half_width;
		const float prop_max_z = prop_pos_z + chunk_half_width;

		int tx0, tx1, tz0, tz1;
		overlapped_chunk_range(prop_min_x, prop_max_x, chunk_scale, chunk_scale_rcp, tx0, tx1);
		overlapped_chunk_range(prop_min_z, prop_max_z, chunk_scale, chunk_scale_rcp, tz0, tz1);
		for (int tz = tz0; tz <= tz1; ++tz)
		{
			for (int tx = tx0; tx <= tx1; ++tx)
			{
				Chunk terrain_chunk;
				terrain_chunk.x = tx;
				terrain_chunk.z = tz;
				for (const Chunk& removable : generator->removable_chunks)
				{
					if (removable == terrain_chunk)
					{
						return false; // this chunk's mesh is replaced before the generator scene is merged
					}
				}
				auto it = chunks.find(terrain_chunk);
				if (it == chunks.end())
				{
					return false;
				}
				const ChunkData& chunk_data = it->second;
				if (chunk_data.invalidated || chunk_data.mesh_vertex_positions == nullptr || chunk_data.blendmap_layers.size() < 4)
				{
					return false;
				}
				for (size_t layer = 0; layer < 4; ++layer)
				{
					if (chunk_data.blendmap_layers[layer].pixels.size() != vertexCount)
					{
						return false;
					}
				}
				for (const BlendmapLayer& layer : chunk_data.spline_blendmap_layers)
				{
					if (layer.pixels.size() != vertexCount)
					{
						return false;
					}
				}
			}
		}

		PropChunkData& prop_data = prop_chunks[prop_chunk];
		prop_data.entity = CreateEntity();
		prop_data.prop_density_current = prop_density;
		TransformComponent& prop_transform = generator->scene.transforms.Create(prop_data.entity);
		prop_transform.ClearTransform();
		prop_transform.Translate(XMFLOAT3(prop_pos_x, 0.0f, prop_pos_z));
		prop_transform.UpdateTransform();
		generator->scene.names.Create(prop_data.entity) = "props";
		generator->scene.Component_Attach(prop_data.entity, chunkGroupEntity);

		auto mix_hash = [](uint64_t x) -> uint64_t
		{
			x += 0x9E3779B97F4A7C15ull;
			x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
			x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
			return x ^ (x >> 31);
		};
		auto hash_u01 = [](uint64_t h) -> float
		{
			return float(h >> 40) * (1.0f / 16777216.0f);
		};
		auto region_at = [](const ChunkData& chunk_data, uint32_t index) -> XMFLOAT4
		{
			return wi::Color(
				chunk_data.blendmap_layers[0].pixels[index],
				chunk_data.blendmap_layers[1].pixels[index],
				chunk_data.blendmap_layers[2].pixels[index],
				chunk_data.blendmap_layers[3].pixels[index]
			);
		};
		auto spline_at = [](const ChunkData& chunk_data, uint32_t index) -> float
		{
			if (chunk_data.spline_blendmap_layers.empty())
			{
				return 0.0f;
			}
			float factor = 0.0f;
			for (auto& layer : chunk_data.spline_blendmap_layers)
			{
				factor += float(layer.pixels[index]) / 255.0f;
			}
			return factor / float(chunk_data.spline_blendmap_layers.size());
		};

		Chunk cached_chunk;
		cached_chunk.raw = 0;
		const ChunkData* cached = nullptr;
		bool cache_valid = false;

		for (size_t prop_index = 0; prop_index < props.size(); ++prop_index)
		{
			const Prop& prop = props[prop_index];
			if (prop.data.empty())
			{
				continue;
			}

			const float mean_count = prop.density * prop_data.prop_density_current;
			if (!(mean_count > 0.0f))
			{
				continue;
			}

			constexpr float chunk_side = float(chunk_width - 1);
			constexpr float cell = chunk_side / 8.0f;
			constexpr float cell_rcp = 1.0f / cell;
			const float clump = saturate(prop.clumping);
			const float spacing = std::max(prop.clump_spacing, 1.0f);
			const float spacing_rcp = 1.0f / spacing;
			const float radius_frac = lerp(1.0f, 0.28f, clump);
			const float keep = mean_count * (1.0f / 64.0f);
			int slot_count = 1;
			if (keep > 1.0f)
			{
				slot_count = (int)std::ceil(std::min(keep, 16.0f));
			}
			const int cx0 = (int)std::floor(prop_min_x * cell_rcp) - 1;
			const int cz0 = (int)std::floor(prop_min_z * cell_rcp) - 1;
			const int cx1 = (int)std::ceil(prop_max_x * cell_rcp);
			const int cz1 = (int)std::ceil(prop_max_z * cell_rcp);

			for (int cz = cz0; cz <= cz1; ++cz)
			{
				for (int cx = cx0; cx <= cx1; ++cx)
				{
					for (int slot = 0; slot < slot_count; ++slot)
					{
						uint64_t h = mix_hash(uint64_t(uint32_t(cx)) | (uint64_t(uint32_t(cz)) << 32));
						h = mix_hash(h ^ (uint64_t(prop.seed) * 0x9E3779B97F4A7C15ull + 1ull));
						h = mix_hash(h ^ (uint64_t(uint32_t(slot)) * 0x9E3779B97F4A7C15ull + 1ull));
						const float keep_roll = hash_u01(h);
						if (!(keep_roll < saturate(keep - float(slot))))
						{
							continue;
						}
						h = mix_hash(h);
						const float ju = hash_u01(h);
						h = mix_hash(h);
						const float jv = hash_u01(h);
						h = mix_hash(h);
						const float f0 = hash_u01(h);
						h = mix_hash(h);
						const float f1 = hash_u01(h);
						h = mix_hash(h);
						const float f2 = hash_u01(h);

						const float world_x = (float(cx) + ju) * cell;
						const float world_z = (float(cz) + jv) * cell;
						if (world_x < prop_min_x || world_z < prop_min_z || world_x >= prop_max_x || world_z >= prop_max_z)
						{
							continue;
						}
						if (clump > 0.0f)
						{
							h = mix_hash(h);
							const float edge = hash_u01(h);
							float best_t = 2.0f;
							const int grove_x = (int)std::floor(world_x * spacing_rcp);
							const int grove_z = (int)std::floor(world_z * spacing_rcp);
							int ring = (int)std::ceil(1.35f * radius_frac);
							if (ring < 1)
							{
								ring = 1;
							}
							for (int oz = -ring; oz <= ring && best_t > 0.65f; ++oz)
							{
								for (int ox = -ring; ox <= ring; ++ox)
								{
									const int gx = grove_x + ox;
									const int gz = grove_z + oz;
									uint64_t gh = mix_hash(uint64_t(uint32_t(gx)) | (uint64_t(uint32_t(gz)) << 32));
									gh = mix_hash(gh ^ (uint64_t(prop.seed) * 0x9E3779B97F4A7C15ull + 1ull));
									gh = mix_hash(gh ^ 0xD1B54A32D192ED03ull);
									if (hash_u01(gh) < 0.34f)
									{
										continue;
									}
									gh = mix_hash(gh);
									const float cu = hash_u01(gh);
									gh = mix_hash(gh);
									const float cv = hash_u01(gh);
									gh = mix_hash(gh);
									const float size_u = hash_u01(gh);
									gh = mix_hash(gh);
									const float ax = hash_u01(gh) * 2.0f - 1.0f;
									gh = mix_hash(gh);
									const float az = hash_u01(gh) * 2.0f - 1.0f;
									gh = mix_hash(gh);
									const float stretch = lerp(0.55f, 1.0f, hash_u01(gh));
									const float axis_len2 = ax * ax + az * az;
									float dir_x = 1.0f;
									float dir_z = 0.0f;
									if (axis_len2 > 1.0e-4f)
									{
										const float inv = 1.0f / std::sqrt(axis_len2);
										dir_x = ax * inv;
										dir_z = az * inv;
									}
									const float px = world_x - (float(gx) + cu) * spacing;
									const float pz = world_z - (float(gz) + cv) * spacing;
									const float lx = px * dir_x + pz * dir_z;
									const float lz = px * -dir_z + pz * dir_x;
									const float dist = std::sqrt(lx * lx + (lz / stretch) * (lz / stretch));
									const float t = dist / (spacing * radius_frac * lerp(0.62f, 1.0f, size_u));
									if (t < best_t)
									{
										best_t = t;
									}
									if (best_t <= 0.65f)
									{
										break;
									}
								}
							}
							float admit = 0.0f;
							if (best_t <= 0.65f)
							{
								admit = 1.0f;
							}
							else if (best_t < 1.0f)
							{
								admit = lerp(1.0f, 0.35f, (best_t - 0.65f) * (1.0f / 0.35f));
							}
							else if (best_t < 1.35f)
							{
								admit = lerp(0.35f, 0.0f, (best_t - 1.0f) * (1.0f / 0.35f)) * (1.0f - clump);
							}
							if (!(edge < admit))
							{
								continue;
							}
						}

						Chunk terrain_chunk;
						terrain_chunk.x = world_to_chunk_index(world_x, chunk_scale, chunk_scale_rcp);
						terrain_chunk.z = world_to_chunk_index(world_z, chunk_scale, chunk_scale_rcp);
						if (!cache_valid || !(cached_chunk == terrain_chunk))
						{
							auto it = chunks.find(terrain_chunk);
							cache_valid = it != chunks.end();
							cached_chunk = terrain_chunk;
							cached = cache_valid ? &it->second : nullptr;
						}
						if (!cache_valid || cached->mesh_vertex_positions == nullptr)
						{
							continue;
						}

						const float vx = (world_x - cached->position.x) * chunk_scale_rcp + chunk_half_width;
						const float vz = (world_z - cached->position.z) * chunk_scale_rcp + chunk_half_width;
						if (vx < 0.0f || vz < 0.0f || vx >= float(chunk_width - 1) || vz >= float(chunk_width - 1))
						{
							continue;
						}

						const int ix = std::min(int(std::floor(vx)), chunk_width - 2);
						const int iz = std::min(int(std::floor(vz)), chunk_width - 2);
						const float tx = vx - float(ix);
						const float tz = vz - float(iz);

						const uint32_t i00 = uint32_t(ix + iz * chunk_width);
						const uint32_t i10 = i00 + 1;
						const uint32_t i01 = i00 + uint32_t(chunk_width);
						const uint32_t i11 = i01 + 1;

						uint32_t i0 = i00;
						uint32_t i1 = i10;
						uint32_t i2 = i01;
						float w0 = 1.0f - tx - tz;
						float w1 = tx;
						float w2 = tz;
						if (tx + tz > 1.0f)
						{
							i0 = i10;
							i1 = i01;
							i2 = i11;
							w0 = 1.0f - tz;
							w1 = 1.0f - tx;
							w2 = tx + tz - 1.0f;
						}

						const XMFLOAT3& p0 = cached->mesh_vertex_positions[i0];
						const XMFLOAT3& p1 = cached->mesh_vertex_positions[i1];
						const XMFLOAT3& p2 = cached->mesh_vertex_positions[i2];
						const XMFLOAT3 vertex_pos = XMFLOAT3(
							p0.x * w0 + p1.x * w1 + p2.x * w2,
							p0.y * w0 + p1.y * w1 + p2.y * w2,
							p0.z * w0 + p1.z * w1 + p2.z * w2
						);

						const XMFLOAT4 r0 = region_at(*cached, i0);
						const XMFLOAT4 r1 = region_at(*cached, i1);
						const XMFLOAT4 r2 = region_at(*cached, i2);
						XMFLOAT4 region = XMFLOAT4(
							r0.x * w0 + r1.x * w1 + r2.x * w2,
							r0.y * w0 + r1.y * w1 + r2.y * w2,
							r0.z * w0 + r1.z * w1 + r2.z * w2,
							r0.w * w0 + r1.w * w1 + r2.w * w2
						);
						weight_norm(region);
						const float spline_factor = spline_at(*cached, i0) * w0 + spline_at(*cached, i1) * w1 + spline_at(*cached, i2) * w2;

						const float region_weight = ((float*)&region)[clamp(prop.region, 0, 3)];
						const float chance = (prop.region_power > 0 ? std::pow(region_weight, prop.region_power) : 1.0f) * (1.0f - saturate(spline_factor));
						if (chance >= 0.5f)
						{
							wi::Archive archive = wi::Archive(prop.data.data(), prop.data.size());
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
								name->name += std::to_string(cx) + "_" + std::to_string(cz);
								if (slot > 0)
								{
									name->name += "_" + std::to_string(slot);
								}
							}
							TransformComponent* transform = generator->scene.transforms.GetComponent(entity);
							if (transform == nullptr)
							{
								transform = &generator->scene.transforms.Create(entity);
							}
							transform->translation_local = XMFLOAT3(
								cached->position.x + vertex_pos.x - prop_pos_x,
								vertex_pos.y,
								cached->position.z + vertex_pos.z - prop_pos_z
							);
							transform->translation_local.y += lerp(prop.min_y_offset, prop.max_y_offset, f0);
							const float scaling = lerp(prop.min_size, prop.max_size, f1);
							transform->Scale(XMFLOAT3(scaling, scaling, scaling));
							transform->RotateRollPitchYaw(XMFLOAT3(0, XM_2PI * f2, 0));
							transform->SetDirty();
							transform->UpdateTransform();
							generator->scene.Component_Attach(entity, prop_data.entity, true);
						}
					}
				}
			}
		}
		return true;
	}

	void Terrain::Generation_Update(const CameraComponent& camera)
	{
		if (wi::jobsystem::IsBusy(generator->workload))
			return; // updating can't run while generation is running. Note: we could cancel here, but it could take long until cancel request is fulfilled (happened with physics mesh creations)

		bool restart_generation = false;
		if (!IsGenerationStarted())
		{
			restart_generation = true;
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
			restart_generation = true;
			modifiers_to_remove.clear();
		}
		for (wi::ecs::Entity entity : materialEntities)
		{
			MaterialComponent* material = scene->materials.GetComponent(entity);
			if (material == nullptr)
				continue;
			if (material->IsDirty())
			{
				restart_generation = true;
				break;
			}
		}
		splineMaterialEntities.clear();
		for (size_t i = 0; i < scene->splines.GetCount(); ++i)
		{
			const SplineComponent& spline = scene->splines[i];
			if (spline.terrain_modifier_amount > 0 || spline.prev_terrain_modifier_amount > 0)
			{
				bool spline_terrain_invalidation = false;
				if (spline.dirty_terrain)
				{
					spline.dirty_terrain = false;
					spline_terrain_invalidation = true;
					spline.prev_terrain_modifier_amount = spline.terrain_modifier_amount;
					spline.prev_terrain_pushdown = spline.terrain_pushdown;
					spline.prev_terrain_texture_falloff = spline.terrain_texture_falloff;
					spline.prev_terrain_generation_nodes = (int)spline.spline_node_entities.size();
				}
				spline_terrain_invalidation |= spline.materialEntity != spline.materialEntity_terrainPrev;
				const MaterialComponent* splineMaterial = scene->materials.GetComponent(spline.materialEntity);
				if (splineMaterial != nullptr)
				{
					spline_terrain_invalidation |= splineMaterial->IsDirty();
					splineMaterialEntities.push_back(spline.materialEntity);
				}
				spline.materialEntity_terrainPrev = spline.materialEntity;

				if (spline_terrain_invalidation)
				{
					InvalidateChunksAtSpline(spline);
				}
			}
		}

		if (restart_generation)
		{
			Generation_Restart();
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
			prop_chunks.clear();
			virtual_textures_in_use.clear();
			return;
		}

		if (chunkGroupEntity == INVALID_ENTITY)
		{
			chunkGroupEntity = terrainEntity;
		}

		WeatherComponent* weather_component = scene->weathers.GetComponent(terrainEntity);
		if (weather_component != nullptr)
		{
			weather = *weather_component; // feedback default weather
		}

		bool props_regenerated = false;

		// Invalidated chunks replacements, originals are removed before merging updated ones:
		for (Chunk chunk : generator->removable_chunks)
		{
			auto it = chunks.find(chunk);
			if (it != chunks.end())
			{
				ChunkData& chunk_data = it->second;
				scene->Entity_Remove(chunk_data.entity);
				chunk_data.props_entity_old = INVALID_ENTITY;
				props_regenerated |= RemovePropChunksOverlapping(chunk);
				if (chunk_data.vt != nullptr)
				{
					chunk_data.vt->invalidate();
				}
			}
		}
		generator->removable_chunks.clear();

		// What was generated, will be merged in to the main scene
		props_regenerated |= generator->scene.rigidbodies.GetCount() >= 1000;
		scene->MergeFastInternal(generator->scene);

		chunk_scale_rcp = 1.0f / chunk_scale;
		cam_height = camera.Eye.y;

		if (IsCenterToCamEnabled())
		{
			center_chunk.x = (int)std::floor((camera.Eye.x + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
			center_chunk.z = (int)std::floor((camera.Eye.z + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
			// Prop grid is chunk_scale 1, so the center uses the unscaled 66m formula.
			prop_center_chunk.x = (int)std::floor((camera.Eye.x + chunk_half_width) * chunk_width_rcp);
			prop_center_chunk.z = (int)std::floor((camera.Eye.z + chunk_half_width) * chunk_width_rcp);
		}

		const int removal_threshold = generation + 2;
		GraphicsDevice* device = GetDevice();

		// Check whether there are any materials that would write to virtual textures:
		bool virtual_texture_any = false;

		if (scene->materials.GetCount() > 0)
		{
			for (wi::ecs::Entity entity : materialEntities)
			{
				MaterialComponent* material = scene->materials.GetComponent(entity);
				if (material == nullptr)
					continue;

				for (int i = 0; i < TEXTURESLOT_COUNT && !virtual_texture_any; ++i)
				{
					switch (i)
					{
					case MaterialComponent::BASECOLORMAP:
					case MaterialComponent::NORMALMAP:
					case MaterialComponent::SURFACEMAP:
					case MaterialComponent::EMISSIVEMAP:
						if (material->textures[i].resource.IsValid())
						{
							virtual_texture_any = true;
						}
						break;
					default:
						break;
					}
				}

				if (virtual_texture_any)
					break;
			}

			if (grassEntity != INVALID_ENTITY)
			{
				MaterialComponent* material_grassparticle_in_scene = scene->materials.GetComponent(grassEntity);
				if (material_grassparticle_in_scene != nullptr)
				{
					grass_material = *material_grassparticle_in_scene;
				}
				HairParticleSystem* hair = scene->hairs.GetComponent(grassEntity);
				if (hair != nullptr)
				{
					grass_properties = *hair;
				}
			}
		}

		// Ensure that enough grass chunks are generated so that grass view distance will not cause popping:
		grass_chunk_dist = std::max(1, int(grass_properties.viewDistance / (float(chunk_width) * chunk_scale) + 0.5f));

		for (auto it = chunks.begin(); it != chunks.end();)
		{
			const Chunk& chunk = it->first;
			ChunkData& chunk_data = it->second;
			chunk_data.visible = camera.frustum.CheckSphere(chunk_data.sphere.center, chunk_data.sphere.radius);
			const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

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
			}
			else
			{
				chunk_data.mesh_vertex_positions = nullptr;
			}

			ObjectComponent* chunk_object = scene->objects.GetComponent(chunk_data.entity);
			if (chunk_object != nullptr)
			{
				if (wi::renderer::GetOcclusionCullingEnabled() && chunk_object->occlusion.IsOccluded())
				{
					chunk_data.visible = false;
				}
				chunk_object->SetWetmapEnabled(scene->IsWetmapProcessingRequired());
				chunk_object->emissiveColor.w = virtual_texture_any ? 1.0f : 0.0f; // without virtual textures, emissive will be disabled on the object-level (most likely missing content)
			}

			// old prop remove:
			if (chunk_data.props_entity_old != INVALID_ENTITY)
			{
				scene->Entity_Remove(chunk_data.props_entity_old);
				chunk_data.props_entity_old = INVALID_ENTITY;
				props_regenerated = true;
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
					props_regenerated |= RemovePropChunksOverlapping(chunk);
					scene->Entity_Remove(it->second.entity);
					it = chunks.erase(it);
					continue; // don't increment iterator
				}
				else
				{
					// Grass patch removal:
					if (chunk_data.grass_entity != INVALID_ENTITY && (dist > grass_chunk_dist || !IsGrassEnabled()))
					{
						scene->Entity_Remove(chunk_data.grass_entity);
						chunk_data.grass_entity = INVALID_ENTITY; // grass can be generated here by generation thread...
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
					grass->randomness = grass_properties.randomness;
					grass->randomSeed = grass_properties.randomSeed;
					grass->stiffness = grass_properties.stiffness;
					grass->viewDistance = grass_properties.viewDistance;
					grass->width = grass_properties.width;
					grass->uniformity = grass_properties.uniformity;
					grass->atlas_rects = grass_properties.atlas_rects;
					grass->segmentCount = grass_properties.segmentCount;
					grass->billboardCount = grass_properties.billboardCount;
					grass->drag = grass_properties.drag;
					grass->gravityPower = grass_properties.gravityPower;
				}

				MaterialComponent* chunkGrassMaterial = scene->materials.GetComponent(chunk_data.grass_entity);
				if (chunkGrassMaterial != nullptr)
				{
					*chunkGrassMaterial = grass_material;
				}
			}

			RigidBodyPhysicsComponent* rigidbody = scene->rigidbodies.GetComponent(chunk_data.entity);
			if (IsPhysicsEnabled())
			{
				const ObjectComponent* object = scene->objects.GetComponent(chunk_data.entity);

				if (dist < physics_generation)
				{
					if (rigidbody == nullptr)
					{
						RigidBodyPhysicsComponent& newrigidbody = scene->rigidbodies.Create(chunk_data.entity);
						newrigidbody.shape = RigidBodyPhysicsComponent::HEIGHTFIELD;
						newrigidbody.mass = 0; // terrain chunks are static
						newrigidbody.friction = 0.8f;
						//newrigidbody.mesh_lod = 2;
					}
					else
					{
						rigidbody->shape = RigidBodyPhysicsComponent::HEIGHTFIELD;
					}
				}
				else if(rigidbody != nullptr)
				{
					scene->rigidbodies.Remove(chunk_data.entity);
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

		// Prop chunks stream on the 66m grid, with the same distance and density test terrain chunks used at chunk_scale 1.
		if (IsRemovalEnabled())
		{
			for (auto prop_it = prop_chunks.begin(); prop_it != prop_chunks.end();)
			{
				const Chunk& prop_chunk = prop_it->first;
				PropChunkData& prop_data = prop_it->second;
				const int prop_dist = std::max(std::abs(prop_center_chunk.x - prop_chunk.x), std::abs(prop_center_chunk.z - prop_chunk.z));
				if (prop_dist > prop_generation || std::abs(prop_data.prop_density_current - prop_density) > std::numeric_limits<float>::epsilon())
				{
					if (prop_data.entity != INVALID_ENTITY)
					{
						scene->Entity_Remove(prop_data.entity);
						props_regenerated = true;
					}
					prop_it = prop_chunks.erase(prop_it);
					continue;
				}
				++prop_it;
			}
		}

		if (props_regenerated)
		{
			wi::physics::OptimizeBroadPhase(*scene);
		}

		if (virtual_texture_any)
		{
			UpdateVirtualTexturesCPU();
		}
		else
		{
			virtual_textures_in_use.clear();
		}

		const uint64_t required_chunk_buffer_size = sizeof(ShaderTerrainChunk) * (chunk_buffer_range * 2 + 1) * (chunk_buffer_range * 2 + 1);
		if (chunk_buffer.desc.size < required_chunk_buffer_size)
		{
			GPUBufferDesc desc;
			desc.usage = Usage::DEFAULT;
			desc.size = required_chunk_buffer_size;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			desc.stride = sizeof(ShaderTerrainChunk);
			bool success = device->CreateBuffer(&desc, nullptr, &chunk_buffer);
			assert(success);
			device->SetName(&chunk_buffer, "wi::terrain::Terrain::chunk_buffer");
		}

		// Start the generation on a background thread and keep it running until the next frame
		generator->cancelled.store(false);
		generator->workload.priority = wi::jobsystem::Priority::Low;
		size_t terrain_spline_count_scene = 0; // Count the current terrain modifier splines in scene
		for (size_t i = 0; i < scene->splines.GetCount(); ++i)
		{
			const SplineComponent& spline = scene->splines[i];
			if (spline.terrain_modifier_amount > 0)
			{
				terrain_spline_count_scene++;
			}
		}
		// Check mismatches between current and previous slines (removal, undo, etc.)
		for (size_t i = 0; i < generator->splines.size(); ++i)
		{
			const SplineComponent& spline = generator->splines[i];
			Entity entity = generator->spline_entities[i];
			bool invalidation_required = false;
			invalidation_required |= !scene->splines.Contains(entity); // no longer exists in scene, was deleted
			if (!invalidation_required)
			{
				// If spline exists in scene, need to compare for changes:
				const SplineComponent& other = *scene->splines.GetComponent(entity);
				invalidation_required |= spline.width != other.width;
				invalidation_required |= spline.rotation != other.rotation;
				invalidation_required |= spline.terrain_modifier_amount != other.terrain_modifier_amount;
				invalidation_required |= spline.terrain_pushdown != other.terrain_pushdown;
				invalidation_required |= spline.terrain_texture_falloff != other.terrain_texture_falloff;
				invalidation_required |= spline.spline_node_transforms.size() != other.spline_node_transforms.size();
				if (!invalidation_required)
				{
					// Last resort compare whole node transform array:
					invalidation_required |= std::memcmp(spline.spline_node_transforms.data(), other.spline_node_transforms.data(), spline.spline_node_transforms.size() * sizeof(TransformComponent)) != 0;
				}
			}
			if (invalidation_required)
			{
				InvalidateChunksAtSpline(spline);
			}
		}
		generator->splines.clear();
		generator->spline_entities.clear();
		for (size_t i = 0; i < scene->splines.GetCount(); ++i)
		{
			const SplineComponent& spline = scene->splines[i];
			if (spline.terrain_modifier_amount > 0)
			{
				generator->splines.push_back(spline);
				generator->spline_entities.push_back(scene->splines.GetEntity(i));
			}
		}
		wi::jobsystem::Execute(generator->workload, [this](wi::jobsystem::JobArgs a) {

			wi::Timer timer;
			bool generated_something = false;

			// Allocate the chunk's entity + components in the generator scene and size its
			// CPU buffers. This mutates the scene component managers, so it must run
			// serially (before the build_chunk() jobs, which only read those components).
			auto setup_chunk = [&](const Chunk& chunk)
			{
				{
					ChunkData& chunk_data = chunks[chunk];

					std::string chunk_name = "chunk_" + std::to_string(chunk.x) + "_" + std::to_string(chunk.z);
					if (chunk_data.entity == INVALID_ENTITY)
					{
						chunk_data.entity = generator->scene.Entity_CreateObject(chunk_name);
					}
					else
					{
						// replacement will be made instead of simple merge, entity ID can be reused:
						if (!generator->scene.names.Contains(chunk_data.entity)) generator->scene.names.Create(chunk_data.entity) = std::move(chunk_name);
						if (!generator->scene.layers.Contains(chunk_data.entity)) generator->scene.layers.Create(chunk_data.entity);
						if (!generator->scene.transforms.Contains(chunk_data.entity)) generator->scene.transforms.Create(chunk_data.entity);
						if (!generator->scene.objects.Contains(chunk_data.entity)) generator->scene.objects.Create(chunk_data.entity);
						generator->scene.materials.Remove(chunk_data.entity);
						generator->scene.meshes.Remove(chunk_data.entity);
						generator->removable_chunks.push_back(chunk);
						chunk_data.grass_entity = INVALID_ENTITY;
					}
					ObjectComponent& object = *generator->scene.objects.GetComponent(chunk_data.entity);
					object.lod_bias = lod_bias;
					object.filterMask |= wi::enums::FILTER_NAVIGATION_MESH;
					object.filterMask |= wi::enums::FILTER_TERRAIN;
					generator->scene.Component_Attach(chunk_data.entity, chunkGroupEntity);

					TransformComponent& transform = *generator->scene.transforms.GetComponent(chunk_data.entity);
					transform.ClearTransform();
					chunk_data.position = XMFLOAT3(float(chunk.x * (chunk_width - 1)) * chunk_scale, 0, float(chunk.z * (chunk_width - 1)) * chunk_scale);
					transform.Translate(chunk_data.position);
					transform.UpdateTransform();

					MaterialComponent& material = generator->scene.materials.Create(chunk_data.entity);
					// material params will be 1 because they will be created from only texture maps
					//	because region materials are blended together into one texture
					material.SetInternal();
					material.SetRoughness(1);
					material.SetMetalness(1);
					material.SetReflectance(1);
					material.SetEmissiveStrength(100);

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
					mesh.vertex_tangents.resize(vertexCount);
					mesh.vertex_uvset_0.resize(vertexCount);

					chunk_data.blendmap_layers.resize(4);
					for (auto& x : chunk_data.blendmap_layers)
					{
						x.pixels.resize(vertexCount);
					}

					chunk_data.spline_blendmap_layers.resize(splineMaterialEntities.size());
					for (auto& x : chunk_data.spline_blendmap_layers)
					{
						x.pixels.resize(vertexCount);
					}

					chunk_data.mesh_vertex_positions = mesh.vertex_positions.data();

					chunk_data.heightmap_data.resize(vertexCount);

					// Mark clean now that it is being (re)generated, so a
					// budget-limited placement pass can't leave a freshly built
					// chunk flagged for rebuild:
					chunk_data.invalidated = false;
				}
			};

			// Build the mesh, textures, grass and physics shape for a chunk
			// that setup_chunk() has already created. Heavy and
			// parallelism-friendly: it only reads shared generation parameters
			// and writes this chunk's own data, so many chunks can build
			// concurrently. No scene component is created here, so the
			// component managers stay stable while these jobs run.
			auto build_chunk = [&](const Chunk& chunk)
			{
				{
					ChunkData& chunk_data = chunks.find(chunk)->second;
					ObjectComponent& object = *generator->scene.objects.GetComponent(chunk_data.entity);
					TransformComponent& transform = *generator->scene.transforms.GetComponent(chunk_data.entity);
					MeshComponent& mesh = *generator->scene.meshes.GetComponent(chunk_data.entity);

					wi::HairParticleSystem grass = grass_properties;
					grass.vertex_lengths.resize(vertexCount);
					std::atomic<uint32_t> grass_valid_vertex_count{ 0 };

					// Shadow casting will only be enabled for sloped terrain chunks:
					std::atomic_bool slope_cast_shadow;
					slope_cast_shadow.store(false);

					// Do a parallel for loop over all the chunk's vertices and compute their properties:
					wi::jobsystem::context ctx;
					ctx.priority = wi::jobsystem::Priority::Low;

					// Preload height grid with padding, because neighbors will need to be accessed to determine slopes:
					constexpr int chunk_width_padded = chunk_width + 1;
					constexpr uint32_t vertexCount_padded = chunk_width_padded * chunk_width_padded;
					float heights_padded[chunk_width_padded * chunk_width_padded];
					const XMVECTOR UP = XMVectorSet(0, 1, 0, 0);
					wi::jobsystem::Dispatch(ctx, vertexCount_padded, chunk_width_padded * 4, [&](wi::jobsystem::JobArgs args) {
						const uint32_t index = args.jobIndex;
						const XMUINT2 coord = XMUINT2(index % chunk_width_padded, index / chunk_width_padded);
						const float x = (float(coord.x) - chunk_half_width) * chunk_scale;
						const float z = (float(coord.y) - chunk_half_width) * chunk_scale;

						float height = 0;
						const XMFLOAT2 world_pos = XMFLOAT2(chunk_data.position.x + x, chunk_data.position.z + z);
						for (auto& modifier : modifiers)
						{
							modifier->Apply(world_pos, height);
						}
						height = lerp(bottomLevel, topLevel, height);

						const bool is_real_vertex = coord.x < chunk_width && coord.y < chunk_width;
						const uint32_t real_index = coord.x + coord.y * chunk_width;

						// Apply splines to height only:
						const XMVECTOR P = XMVectorSet(world_pos.x, -100000, world_pos.y, 0);
						const wi::primitive::Ray ray(P, UP);
						int splinematerialcnt = -1;
						for (size_t j = 0; j < generator->splines.size(); ++j)
						{
							const SplineComponent& spline = generator->splines[j];
							if (spline.materialEntity != INVALID_ENTITY)
								splinematerialcnt++;
							if (!spline.bvh.IntersectsFirst(ray, [&](uint32_t index) { return spline.precomputed_aabbs[index].intersects(ray); }))
								continue;
							XMVECTOR S = spline.TraceSplinePlane(P, UP, 4);
							S = spline.ClosestPointOnSpline(S, 4);
							const float splineheight = XMVectorGetY(S);
							const float splinedist = wi::math::Distance(XMVectorSetY(P, splineheight), S);
							const float splinefactor = 1.0f - smoothstep(0.0f, 1.0f, saturate(splinedist * sqr(spline.terrain_modifier_amount)));
							if (is_real_vertex && spline.materialEntity != INVALID_ENTITY)
							{
								chunk_data.spline_blendmap_layers[splinematerialcnt].pixels[real_index] = uint8_t(smoothstep(clamp(spline.terrain_texture_falloff, 0.0f, 0.999f), 1.0f, splinefactor) * 255);
							}
							height = lerp(height, splineheight - spline.terrain_pushdown, splinefactor);
						}

						heights_padded[coord.x + coord.y * chunk_width_padded] = height;
					});
					wi::jobsystem::Wait(ctx);

					wi::jobsystem::Dispatch(ctx, vertexCount, chunk_width * 4, [&](wi::jobsystem::JobArgs args) {
						const uint32_t index = args.jobIndex;
						const XMUINT2 coord = XMUINT2(index % chunk_width, index / chunk_width);
						const float x = (float(coord.x) - chunk_half_width) * chunk_scale;
						const float z = (float(coord.y) - chunk_half_width) * chunk_scale;
						const float height = heights_padded[coord.x + coord.y * chunk_width_padded];
						const XMVECTOR corners[3] = {
							XMVectorSet(chunk_data.position.x + x, height, chunk_data.position.z + z, 0),
							XMVectorSet(chunk_data.position.x + x + chunk_scale, heights_padded[coord.x + 1 + coord.y * chunk_width_padded], chunk_data.position.z + z, 0),
							XMVectorSet(chunk_data.position.x + x, heights_padded[coord.x + (coord.y + 1) * chunk_width_padded], chunk_data.position.z + z + chunk_scale, 0),
						};
						const XMVECTOR T = XMVectorSubtract(corners[1], corners[2]);
						const XMVECTOR B = XMVectorSubtract(corners[0], corners[1]);
						const XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
						XMFLOAT3 normal;
						XMStoreFloat3(&normal, N);

						const float slope_amount = 1.0f - saturate(normal.y);
						if (slope_amount > 0.1f)
							slope_cast_shadow.store(true);

						float region_base = 1;
						float region_slope = region1 == 0 ? 1 : smoothstep(0.0f, region1, slope_amount);
						float region_low_altitude = region2 == 0 ? 1 : smoothstep(0.0f, region2, wi::math::InverseLerp(0, bottomLevel, height));
						float region_high_altitude = region3 == 0 ? 1 : smoothstep(0.0f, region3, wi::math::InverseLerp(0, topLevel, height));

						region_low_altitude = saturate(region_low_altitude - region_slope);
						region_high_altitude = saturate(region_high_altitude - region_slope);

						XMFLOAT4 materialBlendWeights(region_base, region_slope, region_low_altitude, region_high_altitude);

						chunk_data.blendmap_layers[0].pixels[index] = uint8_t(materialBlendWeights.x * 255);
						chunk_data.blendmap_layers[1].pixels[index] = uint8_t(materialBlendWeights.y * 255);
						chunk_data.blendmap_layers[2].pixels[index] = uint8_t(materialBlendWeights.z * 255);
						chunk_data.blendmap_layers[3].pixels[index] = uint8_t(materialBlendWeights.w * 255);

						// Normalize after store, blending shader wants unnormalized!
						weight_norm(materialBlendWeights);

						mesh.vertex_positions[index] = XMFLOAT3(x, height, z);
						mesh.vertex_normals[index] = normal;
						XMStoreFloat4(&mesh.vertex_tangents[index], T);
						mesh.vertex_tangents[index].w = 1;
						const XMFLOAT2 uv = XMFLOAT2(x * chunk_scale_rcp * chunk_width_rcp + 0.5f, z * chunk_scale_rcp * chunk_width_rcp + 0.5f);
						mesh.vertex_uvset_0[index] = uv;

						XMFLOAT3 vertex_pos(chunk_data.position.x + x, height, chunk_data.position.z + z);

						float spline_factor = 0;
						if (!chunk_data.spline_blendmap_layers.empty())
						{
							for (auto& y : chunk_data.spline_blendmap_layers)
							{
								spline_factor += float(y.pixels[index]) / 255.0f;
							}
							spline_factor /= float(chunk_data.spline_blendmap_layers.size());
						}

						const float grass_noise_frequency = 0.1f;
						const float grass_noise = perlin_noise.compute(vertex_pos.x * grass_noise_frequency, vertex_pos.y * grass_noise_frequency, vertex_pos.z * grass_noise_frequency) * 0.5f + 0.5f;
						const float region_grass = std::pow(materialBlendWeights.x * (1 - materialBlendWeights.w), 8.0f) * grass_noise * (1 - saturate(spline_factor));
						if (region_grass > 0.1f)
						{
							grass_valid_vertex_count.fetch_add(1);
							grass.vertex_lengths[index] = region_grass;
						}
						else
						{
							grass.vertex_lengths[index] = 0;
						}
						chunk_data.heightmap_data[index] = uint16_t(inverse_lerp(bottomLevel, topLevel, height) * 65535);
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

					if (grass_valid_vertex_count.load() > 0)
					{
						wi::jobsystem::Execute(
							ctx,
							[&](wi::jobsystem::JobArgs args)
						{
							chunk_data.grass = std::move(grass); // the grass will be added to the scene later, only when the chunk is close to the camera (center chunk's neighbors)
							chunk_data.grass.meshID = chunk_data.entity;
							chunk_data.grass.strandCount = uint32_t(grass_valid_vertex_count.load() * 3 * chunk_scale * chunk_scale); // chunk_scale * chunk_scale : grass density increases with squared amount with chunk scale (x*z)
							chunk_data.grass.CreateFromMesh(mesh);
						});
					}
					else
					{
						chunk_data.grass.meshID = INVALID_ENTITY;
					}

					if (IsPhysicsEnabled())
					{
						wi::jobsystem::Execute(
							ctx,
							[&](wi::jobsystem::JobArgs args)
						{
							RigidBodyPhysicsComponent& newrigidbody = mesh.precomputed_rigidbody_physics_shape;
							newrigidbody.shape = RigidBodyPhysicsComponent::HEIGHTFIELD;
							newrigidbody.mass = 0; // terrain chunks are static
							newrigidbody.friction = 0.8f;
							//newrigidbody.mesh_lod = 2;
							wi::physics::CreateRigidBodyShape(newrigidbody, transform.scale_local, &mesh);
						});
					}

					chunk_data.heightmap = {};
					chunk_data.blendmap  = {};
					CreateChunkRegionTexture(chunk_data);

					wi::jobsystem::Wait(ctx);
				}
			};

			// Place grass on a finished chunk and clear its invalidated flag.
			// Mutates the generator scene (creates grass entities), so it runs
			// serially after the chunk's build_chunk() job has completed.
			// Props are placed on their own grid, not here.
			auto place_chunk = [&](const Chunk& chunk)
			{
				const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));
				auto it = chunks.find(chunk);

				// Grass patch placement:
				if (dist <= grass_chunk_dist && IsGrassEnabled())
				{
					it = chunks.find(chunk);
					if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
					{
						ChunkData& chunk_data = it->second;
						if ((chunk_data.grass_entity == INVALID_ENTITY || chunk_data.invalidated) && chunk_data.grass.meshID != INVALID_ENTITY && (cam_height - grass_properties.viewDistance < chunk_data.sphere.center.y + chunk_data.sphere.radius))
						{
							// add patch for this chunk
							if (chunk_data.grass_entity == INVALID_ENTITY)
							{
								chunk_data.grass_entity = CreateEntity();
							}
							wi::HairParticleSystem& grass = generator->scene.hairs.Create(chunk_data.grass_entity);
							grass = chunk_data.grass;
							chunk_data.grass_density_current = grass_density;
							grass.strandCount = uint32_t(grass.strandCount * chunk_data.grass_density_current);
							grass.CreateRenderData();
							generator->scene.materials.Create(chunk_data.grass_entity) = grass_material;
							generator->scene.transforms.Create(chunk_data.grass_entity);
							generator->scene.names.Create(chunk_data.grass_entity) = "grass";
							generator->scene.Component_Attach(chunk_data.grass_entity, chunk_data.entity, true);
							generated_something = true;
						}
					}
				}


				it = chunks.find(chunk); // re-query!
				if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
				{
					ChunkData& chunk_data = it->second;
					chunk_data.invalidated = false;
				}
			};

			// Generate chunks in batches so several build concurrently across
			//  the job pool. setup_chunk() (which mutates the scene) runs
			//  serially up front for the whole batch, so the component managers
			//  stay stable while the build jobs run; the batch is then placed
			//  (grass) before moving on.
			//  generator->build_batch is reused across runs: clear() keeps its
			//  capacity, so after the first run it never reallocates.
			const uint32_t batch_capacity = std::max(
				1u,
				wi::jobsystem::GetThreadCount(wi::jobsystem::Priority::Low)
			);
			generator->build_batch.clear();
			generator->build_batch.reserve(batch_capacity);

			wi::jobsystem::context build_ctx;
			build_ctx.priority = wi::jobsystem::Priority::Low;

			// Set up and build every chunk queued in the batch, then clear it.
			// Returns false if the per-frame time budget was exceeded (the
			// caller should stop).
			auto flush_build_batch = [&]() -> bool
			{
				if (generator->build_batch.empty())
				{
					return true;
				}

				for (const Chunk& chunk : generator->build_batch)
				{
					setup_chunk(chunk);
				}

				wi::jobsystem::Dispatch(
					build_ctx,
					uint32_t(generator->build_batch.size()),
					1,
					[&](wi::jobsystem::JobArgs args)
				{
					build_chunk(generator->build_batch[args.jobIndex]);
				});

				wi::jobsystem::Wait(build_ctx);
				generated_something = true;

				// Place grass on the chunks we just built:
				for (const Chunk& chunk : generator->build_batch)
				{
					place_chunk(chunk);
				}

				generator->build_batch.clear();

				if (timer.elapsed_milliseconds() > generation_time_budget_milliseconds)
				{
					generator->cancelled.store(true);

					return false;
				}

				return true;
			};

			// Visit one chunk position. If it still needs (re)generation, add
			// it to the build batch (flushed, and then placed, when the batch
			// is full). Otherwise it is already built, so place its
			// grass now. Returns false if the per-frame time budget was
			// exceeded (the caller should stop).
			auto request_chunk = [&](int offset_x, int offset_z) -> bool
			{
				Chunk chunk = center_chunk;
				chunk.x += offset_x;
				chunk.z += offset_z;
				auto it = chunks.find(chunk);

				if (it == chunks.end()
					|| it->second.entity == INVALID_ENTITY
					|| it->second.invalidated
				) {
					for (const Chunk& queued : generator->build_batch)
					{
						if (queued == chunk)
						{
							return true;
						}
					}
					generator->build_batch.push_back(chunk);

					if (generator->build_batch.size() >= batch_capacity)
					{
						return flush_build_batch();
					}

					return true;
				}

				// Already generated: place grass now, within the time budget.
				place_chunk(chunk);
				if (generated_something && timer.elapsed_milliseconds() > generation_time_budget_milliseconds)
				{
					generator->cancelled.store(true);

					return false;
				}

				return true;
			};

			// Walk chunk positions (priority invalidations first, then center,
			// then outward spiral) queueing generation. Stops early when the
			// time budget is hit.
			auto request_all_chunks = [&]()
			{
				// priority invalidation queue:
				//	This doesn't necessarily finish every frame, that's why it's a queue, next frame will pick up earlier requests before newer ones
				while (!generator->priority_invalidation.empty())
				{
					Chunk chunk = generator->priority_invalidation.front();
					generator->priority_invalidation.pop_front();
					auto it = chunks.find(chunk);
					if (it != chunks.end() && it->second.invalidated) // Check here too in this special case, because multiple of the same entries can easily exist on the queue. Already refreshes chunks will not be refreshed again
					{
						if (!request_chunk(chunk.x - center_chunk.x, chunk.z - center_chunk.z))
						{
							return;
						}
					}
				}

				// generate center chunk first:
				if (!request_chunk(0, 0))
				{
					return;
				}

				// then generate neighbor chunks in outward spiral:
				for (int growth = 0; growth < generation; ++growth)
				{
					const int side = 2 * (growth + 1);
					int x = -growth - 1;
					int z = -growth - 1;
					for (int i = 0; i < side; ++i)
					{
						if (!request_chunk(x, z))
						{
							return;
						}

						x++;
					}
					for (int i = 0; i < side; ++i)
					{
						if (!request_chunk(x, z))
						{
							return;
						}

						z++;
					}
					for (int i = 0; i < side; ++i)
					{
						if (!request_chunk(x, z))
						{
							return;
						}

						x--;
					}
					for (int i = 0; i < side; ++i)
					{
						if (!request_chunk(x, z))
						{
							return;
						}

						z--;
					}
				}
			};

			auto request_all_prop_chunks = [&]()
			{
				if (!(prop_density > 0.0f) || props.empty())
				{
					return;
				}

				auto visit = [&](int offset_x, int offset_z) -> bool
				{
					Chunk prop_chunk = prop_center_chunk;
					prop_chunk.x += offset_x;
					prop_chunk.z += offset_z;
					if (PlacePropChunk(prop_chunk))
					{
						generated_something = true;
						if (timer.elapsed_milliseconds() > generation_time_budget_milliseconds)
						{
							generator->cancelled.store(true);
							return false;
						}
					}
					return !generator->cancelled.load();
				};

				if (!visit(0, 0))
				{
					return;
				}

				for (int growth = 0; growth < prop_generation; ++growth)
				{
					const int side = 2 * (growth + 1);
					int x = -growth - 1;
					int z = -growth - 1;
					for (int i = 0; i < side; ++i)
					{
						if (!visit(x, z))
						{
							return;
						}
						x++;
					}
					for (int i = 0; i < side; ++i)
					{
						if (!visit(x, z))
						{
							return;
						}
						z++;
					}
					for (int i = 0; i < side; ++i)
					{
						if (!visit(x, z))
						{
							return;
						}
						x--;
					}
					for (int i = 0; i < side; ++i)
					{
						if (!visit(x, z))
						{
							return;
						}
						z--;
					}
				}
			};

			request_all_prop_chunks();

			if (!generator->cancelled.load())
			{
				request_all_chunks();
			}

			// Build (and place) any chunks left in a partial batch, unless we
			//  already stopped for the time budget — those are picked up on a
			//  later frame:
			if (!generator->cancelled.load())
			{
				flush_build_batch();
			}

			// Pick up prop chunks whose terrain was built earlier in this job.
			if (!generator->cancelled.load())
			{
				request_all_prop_chunks();
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

	bool Terrain::IsGenerationBusy() const
	{
		if (generator == nullptr)
			return false;
		return wi::jobsystem::IsBusy(generator->workload);
	}

	void Terrain::CreateChunkRegionTexture(ChunkData& chunk_data)
	{
		GraphicsDevice* device = GetDevice();

		if (!chunk_data.heightmap.IsValid() && !chunk_data.heightmap_data.empty())
		{
			TextureDesc desc;
			desc.width = (uint32_t)chunk_width;
			desc.height = (uint32_t)chunk_width;
			desc.format = Format::R16_UNORM;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;

			SubresourceData data;
			data.data_ptr = chunk_data.heightmap_data.data();
			data.row_pitch = chunk_width * sizeof(uint16_t);

			bool success = device->CreateTexture(&desc, &data, &chunk_data.heightmap);
			assert(success);
			device->SetName(&chunk_data.heightmap, "wi::terrain::ChunkData::heightmap");
		}

		const uint32_t required_layers = uint32_t(chunk_data.blendmap_layers.size() + chunk_data.spline_blendmap_layers.size());
		if (!chunk_data.blendmap.IsValid() || chunk_data.blendmap.desc.array_size != required_layers)
		{
			TextureDesc desc;
			desc.width = (uint32_t)chunk_width;
			desc.height = (uint32_t)chunk_width;
			desc.array_size = required_layers;
			desc.format = Format::R8_UNORM;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;

			wi::vector<SubresourceData> data(required_layers);
			for (size_t i = 0; i < chunk_data.blendmap_layers.size(); ++i)
			{
				data[i].data_ptr = chunk_data.blendmap_layers[i].pixels.data();
				data[i].row_pitch = chunk_width * sizeof(uint8_t);
			}
			const size_t offset = chunk_data.blendmap_layers.size();
			for (size_t i = 0; i < chunk_data.spline_blendmap_layers.size(); ++i)
			{
				data[offset + i].data_ptr = chunk_data.spline_blendmap_layers[i].pixels.data();
				data[offset + i].row_pitch = chunk_width * sizeof(uint8_t);
			}

			bool success = device->CreateTexture(&desc, data.data(), &chunk_data.blendmap);
			assert(success);
			device->SetName(&chunk_data.blendmap, "wi::terrain::ChunkData::blendmap");
		}
	}

	void Terrain::UpdateVirtualTexturesCPU()
	{
		wi::jobsystem::Wait(virtual_texture_ctx);
		virtual_texture_ctx.priority = wi::jobsystem::Priority::Low;

		GraphicsDevice* device = GetDevice();
		virtual_textures_in_use.clear();

		if (!sampler.IsValid())
		{
			SamplerDesc samplerDesc;
			samplerDesc.filter = Filter::ANISOTROPIC;
			samplerDesc.max_anisotropy = 4;
			// Note: using wrap mode by intention!
			//	Terrain itself doesn't need wrap mode, but decals will reuse the base material's sampler
			//	and decals can use wrapped textures (texmuladd)
			samplerDesc.address_u = TextureAddressMode::WRAP;
			samplerDesc.address_v = TextureAddressMode::WRAP;
			samplerDesc.address_w = TextureAddressMode::WRAP;
			bool success = device->CreateSampler(&samplerDesc, &sampler);
			assert(success);
		}

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
				GPUBufferDesc tile_pool_desc;

				for (uint32_t map_type = 0; map_type < arraysize(atlas.maps); ++map_type)
				{
					TextureDesc desc;
					desc.width = atlas_physical_width;
					desc.height = atlas_physical_height;
#ifndef NOSPARSE
					desc.misc_flags = ResourceMiscFlag::SPARSE;
#endif // NOSPARSE
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
					case MaterialComponent::BASECOLORMAP:
					case MaterialComponent::EMISSIVEMAP:
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
					device->SetName(&atlas.maps[map_type].texture, "VirtualTextureAtlas::texture");

					success = device->CreateTexture(&desc_raw_block, nullptr, &atlas.maps[map_type].texture_raw_block);
					assert(success);
					device->SetName(&atlas.maps[map_type].texture_raw_block, "VirtualTextureAtlas::texture_raw_block");

#ifndef NOSPARSE
					assert(atlas.maps[map_type].texture.sparse_properties->total_tile_count == atlas.maps[map_type].texture_raw_block.sparse_properties->total_tile_count);
					assert(atlas.maps[map_type].texture.sparse_page_size == atlas.maps[map_type].texture_raw_block.sparse_page_size);

					tile_pool_desc.size += atlas.maps[map_type].texture.sparse_properties->total_tile_count * atlas.maps[map_type].texture.sparse_page_size;
					tile_pool_desc.alignment = std::max(tile_pool_desc.alignment, atlas.maps[map_type].texture.sparse_page_size);
#endif // NOSPARSE

					for (uint32_t i = 0; i < atlas.maps[map_type].texture_raw_block.desc.mip_levels; ++i)
					{
						int subresource_index = device->CreateSubresource(&atlas.maps[map_type].texture_raw_block, SubresourceType::UAV, 0, 1, i, 1);
						assert(subresource_index == i);
					}
				}

#ifndef NOSPARSE
				tile_pool_desc.misc_flags = ResourceMiscFlag::ALIASING_TEXTURE_NON_RT_DS;
				bool success = device->CreateBuffer(&tile_pool_desc, nullptr, &atlas.tile_pool);
				assert(success);
#endif // NOSPARSE

				atlas.physical_tile_count_x = uint8_t(atlas_physical_width / SVT_TILE_SIZE_PADDED);
				atlas.physical_tile_count_y = uint8_t(atlas_physical_height / SVT_TILE_SIZE_PADDED);
				atlas.physical_tiles.resize(size_t(atlas.physical_tile_count_x) * size_t(atlas.physical_tile_count_y));

				uint64_t init_frames = 0;
				atlas.free_tiles.clear();
				atlas.free_tiles.reserve(atlas.physical_tile_count_x * atlas.physical_tile_count_y);
				for (uint8_t y = 0; y < atlas.physical_tile_count_y; ++y)
				{
					for (uint8_t x = 0; x < atlas.physical_tile_count_x; ++x)
					{
						VirtualTextureAtlas::Tile& tile = atlas.free_tiles.emplace_back();
						tile.x = x;
						tile.y = y;
						atlas.physical_tiles[x + y * atlas.physical_tile_count_x].free_frames = ++init_frames;
					}
				}

#ifndef NOSPARSE
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
					device->SparseUpdate(QUEUE_COMPUTE, commands, arraysize(commands));
					offset += count;
				}
#endif // NOSPARSE
			}

			if (chunk_data.vt == nullptr)
			{
				chunk_data.vt = wi::allocator::make_shared<VirtualTexture>();
			}
			VirtualTexture& vt = *chunk_data.vt;

			const uint32_t required_resolution = dist < 2 ? max_virtual_resolution : min_virtual_resolution;
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
						material->textures[map_type].virtual_anisotropy = sampler.desc.max_anisotropy;
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
						material->textures[map_type].resource.SetTextureVirtual(atlas.tile_pool, vt.residency->residencyMap, vt.residency->feedbackMap);
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
						material->textures[map_type].resource.SetTextureVirtual(atlas.tile_pool, Texture(), Texture());
					}
					material->textures[map_type].lod_clamp = (float)vt.lod_count - 2;
				}
				vt.blendmap = chunk_data.blendmap;
			}

			virtual_textures_in_use.push_back(&vt);

			if (vt.residency == nullptr)
				continue;
		}

		wi::jobsystem::Execute(virtual_texture_ctx, [this](wi::jobsystem::JobArgs args) {

			// Update state of physical tiles:
			//	Potentially each physical tile is getting marked as unused here (free_frames > 0), unless GPU requested them to be resident
			for (auto& x : atlas.physical_tiles)
			{
				x.free_frames++;
			}

			// Process GPU allocation requests from last frame:
			//	GPU writes allocation requests by virtualTextureTileAllocateCS.hlsl compute shader
			for (VirtualTexture* vt : virtual_textures_in_use)
			{
				// Last two tiles (last non-packed and packed mips) always kept resident:
				if (vt->tiles.size() > 0)
				{
					atlas.request_residency(vt->tiles[vt->tiles.size() - 1]);
					if (vt->tiles.size() > 1)
					{
						atlas.request_residency(vt->tiles[vt->tiles.size() - 2]);
					}
				}
				if (vt->residency == nullptr)
					continue;

				const uint32_t width = vt->residency->feedbackMap.desc.width;
				const uint32_t height = vt->residency->feedbackMap.desc.height;

				// We must only access persistently mapped resources by CPU that the GPU is not using currently:
				vt->residency->cpu_resource_id = (vt->residency->cpu_resource_id + 1) % arraysize(vt->residency->allocationBuffer_CPU_readback);
				const bool data_available_CPU = vt->residency->data_available_CPU[vt->residency->cpu_resource_id]; // indicates whether any GPU data is readable at this point or not
				vt->residency->data_available_CPU[vt->residency->cpu_resource_id] = true;

				if (data_available_CPU)
				{
					uint32_t page_count = 0;
					uint32_t lod_offsets[10] = {};
					for (uint32_t i = 0; i < vt->lod_count; ++i)
					{
						const uint32_t l_width = std::max(1u, width >> i);
						const uint32_t l_height = std::max(1u, height >> i);
						lod_offsets[i] = page_count;
						page_count += l_width * l_height;
					}

					uint32_t allocation_count = *(const uint32_t*)vt->residency->allocationBuffer_CPU_readback[vt->residency->cpu_resource_id].mapped_data;
					allocation_count = std::min(uint32_t(vt->tiles.size() - 1), allocation_count);
					const uint32_t* allocation_requests = ((const uint32_t*)vt->residency->allocationBuffer_CPU_readback[vt->residency->cpu_resource_id].mapped_data) + 1; // +1 offset of the allocation counter
					for (uint32_t i = 0; i < allocation_count; ++i)
					{
						const uint32_t allocation_request = allocation_requests[i];
						const uint8_t x = (allocation_request >> 24u) & 0xFF;
						const uint8_t y = (allocation_request >> 16u) & 0xFF;
						const uint8_t lod = (allocation_request >> 8u) & 0xFF;
						if (lod >= vt->lod_count)
							continue;
						const uint32_t l_offset = lod_offsets[lod];
						const uint32_t l_width = std::max(1u, width >> lod);
						const uint32_t l_height = std::max(1u, height >> lod);
						const uint32_t l_index = l_offset + x + y * l_width;
						if (x >= l_width || y >= l_height)
							continue;

						VirtualTextureAtlas::Tile& tile = vt->tiles[l_index];
						if (!atlas.request_residency(tile))
						{
							// The request_residency returned false, meaning it is not resident, so it will need to be allocated
							//	Allocation can be made only after all GPU requests have been processed, because some of them will
							//	remain resident, and those occupied tiles shoudln't be given out right now
							VirtualTexture::AllocationRequest& request = vt->allocation_requests.emplace_back();
							request.x = x;
							request.y = y;
							request.lod = lod;
							request.tile_index = l_index;
						}
					}
				}
			}

			// Determine reusable tiles:
			//	After all GPU requests were processed, we can make the unused tiles available for allocations
			atlas.free_tiles.clear();
			for (uint8_t y = 0; y < atlas.physical_tile_count_y; ++y)
			{
				for (uint8_t x = 0; x < atlas.physical_tile_count_x; ++x)
				{
					if (atlas.physical_tiles[x + y * atlas.physical_tile_count_x].free_frames > 0)
					{
						VirtualTextureAtlas::Tile& tile = atlas.free_tiles.emplace_back();
						tile.x = x;
						tile.y = y;
					}
				}
			}
			// Sort them by unused frame counts, this will make them be given out in least recently used (LRU) order:
			std::sort(atlas.free_tiles.begin(), atlas.free_tiles.end(), [&](const VirtualTextureAtlas::Tile& a, const VirtualTextureAtlas::Tile& b) {
				return atlas.get_tile_frames(a) < atlas.get_tile_frames(b);
			});

			// Fulfill new allocation requests:
			for (VirtualTexture* vt : virtual_textures_in_use)
			{
				for (auto& alloc_request : vt->allocation_requests)
				{
					VirtualTextureAtlas::Tile& tile = vt->tiles[alloc_request.tile_index];
					if (atlas.allocate_tile(tile))
					{
						VirtualTexture::UpdateRequest& request = vt->update_requests.emplace_back();
						request.x = (uint16_t)alloc_request.x;
						request.y = (uint16_t)alloc_request.y;
						request.lod = (uint16_t)alloc_request.lod;
						request.tile_x = tile.x;
						request.tile_y = tile.y;
					}
				}
				vt->allocation_requests.clear();
			}

			for (VirtualTexture* vt : virtual_textures_in_use)
			{
				if (vt->residency == nullptr)
					continue;

				// Update page buffer for GPU:
				//	This must be after VirtualTextureAtlas::physical_tiles is no longer being updated
				uint16_t* page_buffer = (uint16_t*)vt->residency->pageBuffer_CPU_upload[vt->residency->cpu_resource_id].mapped_data;
				for (size_t i = 0; i < vt->tiles.size(); ++i)
				{
					if (!atlas.check_tile_resident(vt->tiles[i]))
					{
						vt->tiles[i] = {};
					}
					uint16_t page = vt->tiles[i];
					// force memcpy into uncached memory to avoid read stall by mistake:
					std::memcpy(page_buffer + i, &page, sizeof(page));
				}
			}
		});
	}

	void Terrain::UpdateVirtualTexturesGPU(CommandList cmd) const
	{
		wi::jobsystem::Wait(virtual_texture_ctx);
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Terrain - UpdateVirtualTexturesGPU", cmd);
		auto range = wi::profiler::BeginRangeGPU("Terrain - UpdateVirtualTexturesGPU", cmd);

		device->Barrier(GPUBarrier::Memory(), cmd); // on Apple this fixes corruption so better be safe on all platforms, do not remove

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
		wi::renderer::BindCommonResources(cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->update_requests.empty())
				continue;
			for (uint32_t map_type = 0; map_type < arraysize(atlas.maps); map_type++)
			{
				TerrainVirtualTexturePush push = {};

				switch (map_type)
				{
				case MaterialComponent::BASECOLORMAP:
					device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_BASECOLORMAP), cmd);
					push.output_texture = device->GetDescriptorIndex(&atlas.maps[MaterialComponent::BASECOLORMAP].texture_raw_block, SubresourceType::UAV);
					break;
				case MaterialComponent::NORMALMAP:
					device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_NORMALMAP), cmd);
					push.output_texture = device->GetDescriptorIndex(&atlas.maps[MaterialComponent::NORMALMAP].texture_raw_block, SubresourceType::UAV);
					break;
				case MaterialComponent::SURFACEMAP:
					device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_SURFACEMAP), cmd);
					push.output_texture = device->GetDescriptorIndex(&atlas.maps[MaterialComponent::SURFACEMAP].texture_raw_block, SubresourceType::UAV);
					break;
				case MaterialComponent::EMISSIVEMAP:
					device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_EMISSIVEMAP), cmd);
					push.output_texture = device->GetDescriptorIndex(&atlas.maps[MaterialComponent::EMISSIVEMAP].texture_raw_block, SubresourceType::UAV);
					break;
				default:
					assert(0);
					break;
				}
				if (push.output_texture < 0)
					continue;

				push.blendmap_texture = device->GetDescriptorIndex(&vt->blendmap, SubresourceType::SRV);
				push.blendmap_layers = vt->blendmap.desc.array_size;
				if (push.blendmap_texture < 0)
					continue;

				auto mem = device->AllocateGPU(sizeof(uint) * push.blendmap_layers, cmd);
				const uint splineMaterialCount = (uint)splineMaterialEntities.size();
				const uint baseMaterialCount = push.blendmap_layers - splineMaterialCount;
				for (uint i = 0; i < splineMaterialCount; ++i)
				{
					const Entity entity = splineMaterialEntities[i];
					const uint material_index = (uint)scene->materials.GetIndex(entity);
					std::memcpy((uint*)mem.data + baseMaterialCount + i, &material_index, sizeof(uint)); // force memcpy to avoid uncached read from GPU pointer!
				}
				for (uint i = 0; i < baseMaterialCount; ++i)
				{
					const Entity entity = materialEntities[i];
					const uint material_index = (uint)scene->materials.GetIndex(entity);
					std::memcpy((uint*)mem.data + i, &material_index, sizeof(uint)); // force memcpy to avoid uncached read from GPU pointer!
				}

				push.blendmap_buffer = device->GetDescriptorIndex(&mem.buffer, SubresourceType::SRV);
				if (push.blendmap_buffer < 0)
					continue;
				push.blendmap_buffer_offset = (uint)mem.offset;

				for (auto& request : vt->update_requests)
				{
					uint request_lod_resolution = std::max(1u, vt->resolution >> request.lod);
					const uint2 write_offset_original = uint2(
						request.tile_x * SVT_TILE_SIZE_PADDED / 4,
						request.tile_y * SVT_TILE_SIZE_PADDED / 4
					);

					push.offset = int2(
						int(request.x * SVT_TILE_SIZE) - int(SVT_TILE_BORDER),
						int(request.y * SVT_TILE_SIZE) - int(SVT_TILE_BORDER)
					);

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

#ifdef NOSPARSE
					push.write_offset = write_offset_original;
					push.write_size = SVT_TILE_SIZE_PADDED / 4u;
					NoSparseCopy nosparse;
					nosparse.texture_src = &atlas.maps[map_type].texture_raw_block;
					nosparse.texture_dst = &atlas.maps[map_type].texture;
					nosparse.srcbox.left = push.write_offset.x;
					nosparse.srcbox.right = nosparse.srcbox.left + push.write_size;
					nosparse.srcbox.top = push.write_offset.y;
					nosparse.srcbox.bottom = nosparse.srcbox.top + push.write_size;
					nosparse.srcbox.back = 1;
					nosparse_copies.push_back(nosparse);
#endif // NOSPARSE
				}
			}
			vt->update_requests.clear();
		}
		device->Barrier(GPUBarrier::Memory(), cmd);
		device->EventEnd(cmd);

#ifdef NOSPARSE
		device->EventBegin("Terrain - Nosparse Copies", cmd);
		// Batch all of the Block compression copies after the region updating when sparse texture is disabled:
		for (auto& map : atlas.maps)
		{
			if (!map.texture.IsValid())
				continue;
			wi::renderer::PushBarrier(GPUBarrier::Image(&map.texture, ResourceState::SHADER_RESOURCE, ResourceState::COPY_DST));
			wi::renderer::PushBarrier(GPUBarrier::Image(&map.texture_raw_block, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SRC));
		}
		wi::renderer::FlushBarriers(cmd);

		for (auto& x : nosparse_copies)
		{
			device->CopyTexture(x.texture_dst, x.srcbox.left * 4, x.srcbox.top * 4, 0, 0, 0, x.texture_src, 0, 0, cmd, &x.srcbox);
		}
		nosparse_copies.clear();

		for (auto& map : atlas.maps)
		{
			if (!map.texture.IsValid())
				continue;
			wi::renderer::PushBarrier(GPUBarrier::Image(&map.texture, ResourceState::COPY_DST, ResourceState::SHADER_RESOURCE));
			wi::renderer::PushBarrier(GPUBarrier::Image(&map.texture_raw_block, ResourceState::COPY_SRC, ResourceState::UNORDERED_ACCESS));
		}
		wi::renderer::FlushBarriers(cmd);
		device->EventEnd(cmd);
#endif // NOSPARSE

		wi::profiler::EndRange(range);
		device->EventEnd(cmd);
	}

	void Terrain::CopyVirtualTexturePageStatusGPU(CommandList cmd) const
	{
		wi::jobsystem::Wait(virtual_texture_ctx);
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Terrain - CopyVirtualTexturePageStatusGPU", cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->residency == nullptr)
				continue;
			device->CopyResource(&vt->residency->pageBuffer, &vt->residency->pageBuffer_CPU_upload[vt->residency->cpu_resource_id], cmd);
		}
		device->EventEnd(cmd);

		if (chunk_buffer.IsValid())
		{
			device->EventBegin("Update chunk buffer", cmd);
			auto mem = device->AllocateGPU(chunk_buffer.desc.size, cmd);
			ShaderTerrainChunk* shader_chunks = (ShaderTerrainChunk*)mem.data;
			for (int y = -chunk_buffer_range; y <= chunk_buffer_range; ++y)
			{
				for (int x = -chunk_buffer_range; x <= chunk_buffer_range; ++x)
				{
					ShaderTerrainChunk shader_chunk;
					shader_chunk.init();

					auto chunk = center_chunk;
					chunk.x += x;
					chunk.z += y;
					auto it = chunks.find(chunk);
					if (it != chunks.end())
					{
						const auto& chunk_data = it->second;
						shader_chunk.heightmap = device->GetDescriptorIndex(&chunk_data.heightmap, SubresourceType::SRV);
						shader_chunk.materialID = (uint)scene->materials.GetIndex(chunk_data.entity);
					}

					const uint idx = (x + chunk_buffer_range) + (y + chunk_buffer_range) * (chunk_buffer_range * 2 + 1);
					std::memcpy(shader_chunks + idx, &shader_chunk, sizeof(shader_chunk));
				}
			}
			device->CopyBuffer(&chunk_buffer, 0, &mem.buffer, mem.offset, chunk_buffer.desc.size, cmd);
			device->EventEnd(cmd);
		}
	}

	void Terrain::AllocateVirtualTextureTileRequestsGPU(CommandList cmd) const
	{
		wi::jobsystem::Wait(virtual_texture_ctx);
		GraphicsDevice* device = GetDevice();

		device->EventBegin("Terrain - AllocateVirtualTextureTileRequestsGPU", cmd);

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
				push.feedbackTextureRW = device->GetDescriptorIndex(&vt->residency->feedbackMap, SubresourceType::UAV);
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

		device->Barrier(GPUBarrier::Memory(), cmd);

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

		device->EventEnd(cmd);
	}

	void Terrain::WritebackTileRequestsGPU(CommandList cmd) const
	{
		wi::jobsystem::Wait(virtual_texture_ctx);
		GraphicsDevice* device = GetDevice();

		device->EventBegin("Terrain - WritebackTileRequestsGPU", cmd);

		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->residency == nullptr)
				continue;
			device->CopyResource(&vt->residency->allocationBuffer_CPU_readback[vt->residency->cpu_resource_id], &vt->residency->allocationBuffer, cmd);
			wi::renderer::PushBarrier(GPUBarrier::Buffer(&vt->residency->allocationBuffer, ResourceState::COPY_SRC, ResourceState::COPY_DST));
		}

		wi::renderer::FlushBarriers(cmd);

		device->EventEnd(cmd);

		device->EventBegin("Clear Metadata", cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (vt->residency == nullptr)
				continue;

			// Note: instead of ClearUAV, we use copies from default initialized resources
			//	With this it can be put on the async copy queue
			//	Also, using the ClearUAV was having a very bad performance especially with DX12
			static wi::unordered_map<uint32_t, VirtualTextureAtlas::Residency> clear_residencies;
			VirtualTextureAtlas::Residency& clear = clear_residencies[vt->resolution];
			if (clear.resolution != vt->resolution)
			{
				clear.init(vt->resolution);
			}
			device->CopyResource(&vt->residency->feedbackMap, &clear.feedbackMap, cmd);
			device->CopyResource(&vt->residency->requestBuffer, &clear.requestBuffer, cmd);
			device->CopyResource(&vt->residency->allocationBuffer, &clear.allocationBuffer, cmd);

			wi::renderer::PushBarrier(GPUBarrier::Image(&vt->residency->residencyMap, ResourceState::COPY_DST, ResourceState::COPY_SRC));
		}
		wi::renderer::FlushBarriers(cmd);
		device->EventEnd(cmd);
	}

	ShaderTerrain Terrain::GetShaderTerrain() const
	{
		GraphicsDevice* device = GetDevice();

		ShaderTerrain shader_terrain;
		shader_terrain.init();

		auto it = chunks.find(center_chunk);
		if (it != chunks.end())
		{
			const auto& chunk_data = it->second;
			shader_terrain.chunk_size = chunk_half_width * 2 * chunk_scale;
			shader_terrain.center_chunk_pos.x = center_chunk.x * shader_terrain.chunk_size - shader_terrain.chunk_size * 0.5f;
			shader_terrain.center_chunk_pos.y = 0;
			shader_terrain.center_chunk_pos.z = center_chunk.z * shader_terrain.chunk_size - shader_terrain.chunk_size * 0.5f;
			shader_terrain.chunk_buffer = device->GetDescriptorIndex(&chunk_buffer, SubresourceType::SRV);
			shader_terrain.chunk_buffer_range = chunk_buffer_range;
			shader_terrain.min_height = bottomLevel;
			shader_terrain.max_height = topLevel;
		}

		return shader_terrain;
	}

	void Terrain::InvalidateProps()
	{
		for (auto it = prop_chunks.begin(); it != prop_chunks.end(); ++it)
		{
			it->second.prop_density_current = 0;
		}
	}

	void Terrain::InvalidateChunksAtSpline(const wi::scene::SplineComponent& spline)
	{
		for (auto it = chunks.begin(); it != chunks.end(); it++)
		{
			ChunkData& chunk_data = it->second;
			if (chunk_data.invalidated)
				continue;
			BoundingBox bb(chunk_data.sphere.center, XMFLOAT3(chunk_data.sphere.radius, 1000000, chunk_data.sphere.radius));
			if (spline.bvh.IntersectsFirst(bb, [&](uint32_t index) { return spline.precomputed_obbs[index].Intersects(bb); }))
			{
				chunk_data.invalidated = true;
				generator->priority_invalidation.push_back(it->first);
			}
		}
	}

	void Terrain::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		Generation_Cancel();

		// Note: separate component types serialized within terrain must NOT use the version of the terrain, but their own!
		ComponentLibrary& library = *seri.componentlibrary;
		const uint64_t terrain_version = seri.GetVersion();
		const uint64_t grass_version = seri.GetVersion("wi::scene::Scene::hairs");
		const uint64_t material_version = seri.GetVersion("wi::scene::Scene::materials");
		const uint64_t weather_version = seri.GetVersion("wi::scene::Scene::weathers");

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> lod_bias;
			if (terrain_version < 3)
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
			if (terrain_version < 7)
			{
				// Older scenes stored props on terrain chunks. Place the new chunk grid on that center until the camera takes over.
				const float anchor_x = float(center_chunk.x * (chunk_width - 1)) * chunk_scale;
				const float anchor_z = float(center_chunk.z * (chunk_width - 1)) * chunk_scale;
				prop_center_chunk.x = world_to_chunk_index(anchor_x, 1.0f, 1.0f);
				prop_center_chunk.z = world_to_chunk_index(anchor_z, 1.0f, 1.0f);
			}

			if (terrain_version >= 1)
			{
				archive >> physics_generation;
			}
			if (terrain_version >= 2 && terrain_version < 3)
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
				if (terrain_version >= 1)
				{
					archive >> prop.data;

					if (!prop.data.empty())
					{
						// Serialize the prop data in read mode and remap internal entity references:
						Scene tmp_scene;
						wi::Archive tmp_archive = wi::Archive(prop.data.data(), prop.data.size());
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
				if (terrain_version >= 7)
				{
					archive >> prop.density;
					archive >> prop.clumping;
					archive >> prop.clump_spacing;
					archive >> prop.seed;
					archive >> prop.region;
					archive >> prop.region_power;
				}
				else
				{
					int min_count = 0;
					int max_count = 10;
					float noise_frequency = 1;
					float noise_power = 1;
					float threshold = 0.5f;
					archive >> min_count;
					archive >> max_count;
					archive >> prop.region;
					archive >> prop.region_power;
					archive >> noise_frequency;
					archive >> noise_power;
					archive >> threshold;
					prop.density = float(min_count + max_count) * 0.5f;
					prop.seed = 0;
					prop.region_power *= 2;

					const float freq = noise_frequency > 1.0e-6f ? noise_frequency : 1.0e-6f;
					const float wavelength = 1.0f / freq;
					const float scale = saturate((wavelength - 8.0f) / 192.0f);
					const float peakiness = saturate((noise_power - 1.0f) / 3.0f);
					const float pickiness = saturate(threshold);
					prop.clumping = saturate(scale * 0.5f + peakiness * 0.35f + pickiness * 0.15f);
					prop.clump_spacing = 64;
				}
				archive >> prop.min_size;
				archive >> prop.max_size;
				archive >> prop.min_y_offset;
				archive >> prop.max_y_offset;
				if (terrain_version >= 6)
				{
					SerializeEntity(archive, prop.source_entity, seri);
				}
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
				SerializeEntity(archive, chunk_data.props_entity_old, seri);
				archive >> chunk_data.prop_density_current;

				seri.version = grass_version;
				chunk_data.grass.Serialize(archive, seri);
				seri.version = terrain_version;

				archive >> chunk_data.grass_density_current;
				if (terrain_version >= 5)
				{
					size_t blendmapCount = 0;
					archive >> blendmapCount;
					chunk_data.blendmap_layers.resize(blendmapCount);
					for (size_t i = 0; i < chunk_data.blendmap_layers.size(); ++i)
					{
						archive >> chunk_data.blendmap_layers[i].pixels;
					}
					archive >> chunk_data.heightmap_data;
				}
				else
				{
					wi::vector<wi::Color> blendmap4;
					archive >> blendmap4;
					chunk_data.blendmap_layers.resize(4);
					for (size_t i = 0; i < 4; ++i)
					{
						chunk_data.blendmap_layers[i].pixels.resize(blendmap4.size());
					}
					for (size_t i = 0; i < blendmap4.size(); ++i)
					{
						chunk_data.blendmap_layers[0].pixels[i] = blendmap4[i].getR();
						chunk_data.blendmap_layers[1].pixels[i] = blendmap4[i].getG();
						chunk_data.blendmap_layers[2].pixels[i] = blendmap4[i].getB();
						chunk_data.blendmap_layers[3].pixels[i] = blendmap4[i].getA();
					}
				}
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
						wi::allocator::shared_ptr<PerlinModifier> modifier = wi::allocator::make_shared_single<PerlinModifier>();
						modifiers[i] = modifier;
						archive >> modifier->octaves;
						archive >> modifier->seed;
						modifier->perlin_noise.Serialize(archive);
					}
					break;
				case Modifier::Type::Voronoi:
					{
						wi::allocator::shared_ptr<VoronoiModifier> modifier = wi::allocator::make_shared_single<VoronoiModifier>();
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
						wi::allocator::shared_ptr<HeightmapModifier> modifier = wi::allocator::make_shared_single<HeightmapModifier>();
						modifiers[i] = modifier;
						archive >> modifier->amount;
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
			archive << lod_bias;
			if (terrain_version < 3)
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

			if (terrain_version >= 1)
			{
				archive << physics_generation;
			}
			if (terrain_version >= 2 && terrain_version < 3)
			{
				uint32_t target_texture_resolution = 1024;
				archive << target_texture_resolution;
			}

			archive << props.size();
			for (size_t i = 0; i < props.size(); ++i)
			{
				Prop& prop = props[i];
				archive << prop.data;
				archive << prop.density;
				archive << prop.clumping;
				archive << prop.clump_spacing;
				archive << prop.seed;
				archive << prop.region;
				archive << prop.region_power;
				archive << prop.min_size;
				archive << prop.max_size;
				archive << prop.min_y_offset;
				archive << prop.max_y_offset;
				SerializeEntity(archive, prop.source_entity, seri);
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
				SerializeEntity(archive, chunk_data.props_entity_old, seri);
				archive << chunk_data.prop_density_current;

				seri.version = grass_version;
				chunk_data.grass.Serialize(archive, seri);
				seri.version = terrain_version;

				archive << chunk_data.grass_density_current;
				archive << chunk_data.blendmap_layers.size();
				for (size_t i = 0; i < chunk_data.blendmap_layers.size(); ++i)
				{
					archive << chunk_data.blendmap_layers[i].pixels;
				}
				archive << chunk_data.heightmap_data;
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
					archive << ((HeightmapModifier*)modifier.get())->amount;
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

		// Caution: seri.version changes must be handled carefully!

		if (terrain_version >= 5)
		{
			if (archive.IsReadMode())
			{
				SerializeEntity(archive, chunkGroupEntity, seri);
				size_t materialCount = 0;
				archive >> materialCount;
				materialEntities.resize(materialCount);
				for (size_t i = 0; i < materialEntities.size(); ++i)
				{
					SerializeEntity(archive, materialEntities[i], seri);
				}
				SerializeEntity(archive, grassEntity, seri);
			}
			else
			{
				SerializeEntity(archive, chunkGroupEntity, seri);
				archive << materialEntities.size();
				for (size_t i = 0; i < materialEntities.size(); ++i)
				{
					SerializeEntity(archive, materialEntities[i], seri);
				}
				SerializeEntity(archive, grassEntity, seri);
			}
		}
		else if (terrain_version >= 4)
		{
			SerializeEntity(archive, chunkGroupEntity, seri);
			for (size_t i = 0; i < MATERIAL_COUNT; ++i)
			{
				SerializeEntity(archive, materialEntities[i], seri);
			}
			SerializeEntity(archive, grassEntity, seri);
		}
		else
		{
			// Convert terrain version below 4 to newer version:
			seri.version = material_version;
			wi::scene::MaterialComponent materials[MATERIAL_COUNT];
			materials[MATERIAL_BASE].Serialize(archive, seri);
			materials[MATERIAL_SLOPE].Serialize(archive, seri);
			materials[MATERIAL_LOW_ALTITUDE].Serialize(archive, seri);
			materials[MATERIAL_HIGH_ALTITUDE].Serialize(archive, seri);
			grass_material.Serialize(archive, seri);
			wi::jobsystem::Wait(seri.ctx); // wait for material CreateRenderData() that was asynchronously launched in Serialize()!

			materialEntities[MATERIAL_BASE] = CreateEntity();
			materialEntities[MATERIAL_SLOPE] = CreateEntity();
			materialEntities[MATERIAL_LOW_ALTITUDE] = CreateEntity();
			materialEntities[MATERIAL_HIGH_ALTITUDE] = CreateEntity();
			grassEntity = CreateEntity();

			ComponentManager<MaterialComponent>* scene_materials = library.Get<MaterialComponent>("wi::scene::Scene::materials");
			scene_materials->Create(materialEntities[MATERIAL_BASE]) = materials[MATERIAL_BASE];
			scene_materials->Create(materialEntities[MATERIAL_SLOPE]) = materials[MATERIAL_SLOPE];
			scene_materials->Create(materialEntities[MATERIAL_LOW_ALTITUDE]) = materials[MATERIAL_LOW_ALTITUDE];
			scene_materials->Create(materialEntities[MATERIAL_HIGH_ALTITUDE]) = materials[MATERIAL_HIGH_ALTITUDE];
			scene_materials->Create(grassEntity) = grass_material;

			ComponentManager<NameComponent>* scene_names = library.Get<NameComponent>("wi::scene::Scene::names");
			scene_names->Create(materialEntities[MATERIAL_BASE]).name = "material_base";
			scene_names->Create(materialEntities[MATERIAL_SLOPE]).name = "material_slope";
			scene_names->Create(materialEntities[MATERIAL_LOW_ALTITUDE]).name = "material_low_altitude";
			scene_names->Create(materialEntities[MATERIAL_HIGH_ALTITUDE]).name = "material_high_altitude";
			scene_names->Create(grassEntity).name = "grass";
		}

		seri.version = weather_version;
		weather.Serialize(archive, seri);

		if (terrain_version < 4)
		{
			seri.version = grass_version;
			grass_properties.Serialize(archive, seri);
			seri.version = terrain_version;

			ComponentManager<wi::HairParticleSystem>* scene_hairs = library.Get<wi::HairParticleSystem>("wi::scene::Scene::hairs");
			scene_hairs->Create(grassEntity) = grass_properties;
		}

		perlin_noise.Serialize(archive);

		if (terrain_version >= 7)
		{
			if (archive.IsReadMode())
			{
				archive >> prop_center_chunk.x;
				archive >> prop_center_chunk.z;
				size_t prop_chunk_count = 0;
				archive >> prop_chunk_count;
				prop_chunks.clear();
				prop_chunks.reserve(prop_chunk_count);
				for (size_t i = 0; i < prop_chunk_count; ++i)
				{
					Chunk prop_chunk;
					archive >> prop_chunk.x;
					archive >> prop_chunk.z;
					PropChunkData& prop_data = prop_chunks[prop_chunk];
					SerializeEntity(archive, prop_data.entity, seri);
					archive >> prop_data.prop_density_current;
				}
			}
			else
			{
				archive << prop_center_chunk.x;
				archive << prop_center_chunk.z;
				archive << prop_chunks.size();
				for (auto& it : prop_chunks)
				{
					archive << it.first.x;
					archive << it.first.z;
					SerializeEntity(archive, it.second.entity, seri);
					archive << it.second.prop_density_current;
				}
			}
		}
	}

}
