#pragma once
#include "CommonInclude.h"
#include "wiScene_Decl.h"
#include "wiScene_Components.h"
#include "wiNoise.h"
#include "wiECS.h"
#include "wiColor.h"
#include "wiHairParticle.h"

#include <random>
#include <string>
#include <atomic>

namespace wi::terrain
{
	struct Chunk
	{
		int x, z;
		constexpr bool operator==(const Chunk& other) const
		{
			return (x == other.x) && (z == other.z);
		}
		inline size_t compute_hash() const
		{
			return ((std::hash<int>()(x) ^ (std::hash<int>()(z) << 1)) >> 1);
		}
	};
}

namespace std
{
	template <>
	struct hash<wi::terrain::Chunk>
	{
		inline size_t operator()(const wi::terrain::Chunk& chunk) const
		{
			return chunk.compute_hash();
		}
	};
}

namespace wi::terrain
{
	static constexpr int chunk_width = 64 + 3; // + 3: filler vertices for lod apron and grid perimeter
	static constexpr float chunk_half_width = (chunk_width - 1) * 0.5f;
	static constexpr float chunk_width_rcp = 1.0f / (chunk_width - 1);
	static constexpr uint32_t vertexCount = chunk_width * chunk_width;

	struct GPUPageAllocator
	{
		wi::graphics::GPUBuffer buffer;

		struct Page
		{
			uint16_t index = 0xFFFF;
			constexpr bool IsValid() const { return index < 0xFFFF; }
		};
		wi::vector<Page> pages;

		void init(size_t budget, size_t page_size);
		Page allocate();
		void free(Page page);
	};
	struct VirtualTexture
	{
		wi::graphics::Texture texture;
		wi::graphics::Texture residencyMap;
		wi::graphics::Texture feedbackMap;
		wi::graphics::Texture residencyMap_CPU[wi::graphics::GraphicsDevice::GetBufferCount() + 1];
		wi::graphics::Texture feedbackMap_CPU[wi::graphics::GraphicsDevice::GetBufferCount() + 1];
		bool data_available_CPU[wi::graphics::GraphicsDevice::GetBufferCount() + 1] = {};
		int cpu_resource_id = 0;

		void init(const wi::graphics::TextureDesc& desc);

		struct LOD
		{
			uint32_t width = 0;
			uint32_t height = 0;
			wi::vector<GPUPageAllocator::Page> pages;
			wi::vector<uint8_t> page_dependecies;

			void free(GPUPageAllocator& page_allocator)
			{
				for (auto& tile : pages)
				{
					page_allocator.free(tile);
					tile = {};
				}
			}
		};
		wi::vector<LOD> lods;
		wi::vector<uint8_t> tile_residency;
		uint8_t residentMaxLod = 0xFF;	// fully resident maximum LOD

		void free(GPUPageAllocator& page_allocator)
		{
			for (auto& lod : lods)
			{
				lod.free(page_allocator);
			}
		}

		void DrawDebug(wi::graphics::CommandList cmd);
	};
	struct SparseUpdateBatcher
	{
		wi::vector<wi::graphics::SparseUpdateCommand> commands;
		struct CommandArrays
		{
			wi::vector<wi::graphics::SparseResourceCoordinate> sparse_coordinate;
			wi::vector<wi::graphics::SparseRegionSize> sparse_size;
			wi::vector<wi::graphics::TileRangeFlags> tile_range_flags;
			wi::vector<uint32_t> tile_range_offset;
			wi::vector<uint32_t> tile_range_count;
		};
		wi::vector<CommandArrays> command_arrays;

		void Flush(wi::graphics::QUEUE_TYPE queue);
	};

	struct ChunkData
	{
		wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity grass_entity = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity props_entity = wi::ecs::INVALID_ENTITY;
		const XMFLOAT3* mesh_vertex_positions = nullptr;
		float prop_density_current = 1;
		wi::HairParticleSystem grass;
		float grass_density_current = 1;
		wi::vector<wi::Color> region_weights;
		wi::graphics::Texture region_weights_texture;
		wi::primitive::Sphere sphere;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		bool visible = true;

		VirtualTexture vt[3];
	};

	struct Prop
	{
		wi::vector<uint8_t> data; // serialized component data storage
		int min_count_per_chunk = 0; // a chunk will try to generate min this many props of this type
		int max_count_per_chunk = 10; // a chunk will try to generate max this many props of this type
		int region = 0; // region selection in range [0,3] (0: base/grass, 1: slopes, 2: low altitude (bottom level-0), 3: high altitude (0-top level))
		float region_power = 1; // region weight affection power factor
		float noise_frequency = 1; // perlin noise's frequency for placement factor
		float noise_power = 1; // perlin noise's power
		float threshold = 0.5f; // the chance of placement (higher is less chance)
		float min_size = 1; // scaling randomization range min
		float max_size = 1; // scaling randomization range max
		float min_y_offset = 0; // min randomized offset on Y axis
		float max_y_offset = 0; // max randomized offset on Y axis
	};

	struct Modifier;
	struct Generator;

	struct Terrain
	{
		enum FLAGS
		{
			EMPTY = 0,
			CENTER_TO_CAM = 1 << 0,
			REMOVAL = 1 << 1,
			GRASS = 1 << 2,
			GENERATION_STARTED = 1 << 4,
			PHYSICS = 1 << 5,
		};
		uint32_t _flags = CENTER_TO_CAM | REMOVAL | GRASS;

		wi::ecs::Entity terrainEntity = wi::ecs::INVALID_ENTITY;
		wi::scene::Scene* scene = nullptr;
		wi::scene::MaterialComponent material_Base;
		wi::scene::MaterialComponent material_Slope;
		wi::scene::MaterialComponent material_LowAltitude;
		wi::scene::MaterialComponent material_HighAltitude;
		wi::scene::MaterialComponent material_GrassParticle;
		wi::scene::WeatherComponent weather;
		wi::HairParticleSystem grass_properties;
		wi::unordered_map<Chunk, ChunkData> chunks;
		Chunk center_chunk = {};
		wi::noise::Perlin perlin_noise;
		wi::vector<Prop> props;

		// For generating scene on a background thread:
		std::shared_ptr<Generator> generator;
		float generation_time_budget_milliseconds = 12; // after this much time, the generation thread will exit. This can help avoid a very long running, resource consuming and slow cancellation generation

		struct VirtualTextureUpdateRequest
		{
			uint32_t lod = 0;
			uint32_t tile_x = 0;
			uint32_t tile_y = 0;
			uint32_t map_type = 0;
			wi::graphics::Texture texturemap;
			wi::graphics::Texture region_weights_texture;
		};
		mutable wi::vector<VirtualTextureUpdateRequest> virtual_texture_updates;
		mutable wi::vector<wi::graphics::GPUBarrier> virtual_texture_barriers;
		GPUPageAllocator page_allocator;
		SparseUpdateBatcher sparse_batcher;

		constexpr bool IsCenterToCamEnabled() const { return _flags & CENTER_TO_CAM; }
		constexpr bool IsRemovalEnabled() const { return _flags & REMOVAL; }
		constexpr bool IsGrassEnabled() const { return _flags & GRASS; }
		constexpr bool IsGenerationStarted() const { return _flags & GENERATION_STARTED; }
		constexpr bool IsPhysicsEnabled() const { return _flags & PHYSICS; }

		constexpr void SetCenterToCamEnabled(bool value) { if (value) { _flags |= CENTER_TO_CAM; } else { _flags &= ~CENTER_TO_CAM; } }
		constexpr void SetRemovalEnabled(bool value) { if (value) { _flags |= REMOVAL; } else { _flags &= ~REMOVAL; } }
		constexpr void SetGrassEnabled(bool value) { if (value) { _flags |= GRASS; } else { _flags &= ~GRASS; } }
		constexpr void SetGenerationStarted(bool value) { if (value) { _flags |= GENERATION_STARTED; } else { _flags &= ~GENERATION_STARTED; } }
		constexpr void SetPhysicsEnabled(bool value) { if (value) { _flags |= PHYSICS; } else { _flags &= ~PHYSICS; } }

		float lod_multiplier = 0.005f;
		float texlod = 0.01f;
		int generation = 12;
		int prop_generation = 10;
		int physics_generation = 3;
		float prop_density = 1;
		float grass_density = 1;
		float chunk_scale = 1;
		uint32_t seed = 3926;
		float bottomLevel = -60;
		float topLevel = 380;
		float region1 = 1;
		float region2 = 2;
		float region3 = 8;
		uint32_t target_texture_resolution = 1024;

		wi::vector<std::shared_ptr<Modifier>> modifiers;
		wi::vector<Modifier*> modifiers_to_remove;

		Terrain();
		~Terrain();

		// Restarts the terrain generation from scratch
		//	This will remove previously existing terrain
		void Generation_Restart();
		// This will run the actual generation tasks, call it once per frame
		void Generation_Update(const wi::scene::CameraComponent& camera);
		// Tells the generation thread that it should be cancelled and blocks until that is confirmed
		void Generation_Cancel();
		// The virtual textures will be compressed and saved into resources. They can be serialized from there
		void BakeVirtualTexturesToFiles();
		// Creates the blend weight texture for a chunk data
		void CreateChunkRegionTexture(ChunkData& chunk_data);

		// Evaluate tile request, allocate tiles, map, create update reques
		void UpdateVirtualTexturesCPU(ChunkData& chunk_data);
		// Updates the virtual textures on GPU by compute shaders
		void UpdateVirtualTexturesGPU(wi::graphics::CommandList cmd) const;
		// Feedback maps are copied to CPU readable resources
		void WritebackTileRequestsGPU(wi::graphics::CommandList cmd) const;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};

	struct Modifier
	{
		virtual ~Modifier() = default;

		enum class Type
		{
			Perlin,
			Voronoi,
			Heightmap,
		} type = Type::Perlin;

		enum class BlendMode
		{
			Normal,
			Additive,
			Multiply,
		} blend = BlendMode::Normal;

		float weight = 0.5f;
		float frequency = 0.0008f;

		virtual void Seed(uint32_t seed) {}
		virtual void Apply(const XMFLOAT2& world_pos, float& height) = 0;
		constexpr void Blend(float& height, float value)
		{
			switch (blend)
			{
			default:
			case BlendMode::Normal:
				height = wi::math::Lerp(height, value, weight);
				break;
			case BlendMode::Multiply:
				height *= value * weight;
				break;
			case BlendMode::Additive:
				height += value * weight;
				break;
			}
		}
	};
	struct PerlinModifier : public Modifier
	{
		int octaves = 6;
		uint32_t seed = 0;
		wi::noise::Perlin perlin_noise;

		PerlinModifier() { type = Type::Perlin; }
		void Seed(uint32_t seed) override
		{
			this->seed = seed;
			perlin_noise.init(seed);
		}
		void Apply(const XMFLOAT2& world_pos, float& height) override
		{
			XMFLOAT2 p = world_pos;
			p.x *= frequency;
			p.y *= frequency;
			Blend(height, perlin_noise.compute(p.x, p.y, 0, octaves) * 0.5f + 0.5f);
		}
	};
	struct VoronoiModifier : public Modifier
	{
		float fade = 2.59f;
		float shape = 0.7f;
		float falloff = 6;
		float perturbation = 0.1f;
		uint32_t seed = 0;
		wi::noise::Perlin perlin_noise;

		VoronoiModifier() { type = Type::Voronoi; }
		void Seed(uint32_t seed) override
		{
			this->seed = seed;
			perlin_noise.init(seed);
		}
		void Apply(const XMFLOAT2& world_pos, float& height) override
		{
			XMFLOAT2 p = world_pos;
			p.x *= frequency;
			p.y *= frequency;
			if (perturbation > 0)
			{
				const float angle = perlin_noise.compute(p.x, p.y, 0, 6) * XM_2PI;
				p.x += std::sin(angle) * perturbation;
				p.y += std::cos(angle) * perturbation;
			}
			wi::noise::voronoi::Result res = wi::noise::voronoi::compute(p.x, p.y, (float)seed);
			float weight = std::pow(1 - wi::math::saturate((res.distance - shape) * fade), std::max(0.0001f, falloff));
			Blend(height, weight);
		}
	};
	struct HeightmapModifier : public Modifier
	{
		float scale = 0.1f;

		wi::vector<uint8_t> data;
		int width = 0;
		int height = 0;

		HeightmapModifier() { type = Type::Heightmap; }
		void Apply(const XMFLOAT2& world_pos, float& height) override
		{
			XMFLOAT2 p = world_pos;
			p.x *= frequency;
			p.y *= frequency;
			XMFLOAT2 pixel = XMFLOAT2(p.x + width * 0.5f, p.y + this->height * 0.5f);
			if (pixel.x >= 0 && pixel.x < width && pixel.y >= 0 && pixel.y < this->height)
			{
				const int idx = int(pixel.x) + int(pixel.y) * width;
				Blend(height, ((float)data[idx] / 255.0f) * scale);
			}
		}
	};

}
