#pragma once
#include "CommonInclude.h"
#include "wiScene_Decl.h"
#include "wiScene_Components.h"
#include "wiNoise.h"
#include "wiECS.h"
#include "wiColor.h"
#include "wiHairParticle.h"
#include "wiVector.h"

#include <memory>
#include <algorithm>

namespace wi::terrain
{
	struct Chunk
	{
		union
		{
			struct
			{
				int32_t x, z;
			};
			uint64_t raw = 0;
		};
		constexpr bool operator==(const Chunk& other) const
		{
			return raw == other.raw;
		}
		constexpr uint64_t compute_hash() const
		{
			return raw;
		}
	};
}

namespace std
{
	template <>
	struct hash<wi::terrain::Chunk>
	{
		constexpr uint64_t operator()(const wi::terrain::Chunk& chunk) const
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
	enum
	{
		MATERIAL_BASE,
		MATERIAL_SLOPE,
		MATERIAL_LOW_ALTITUDE,
		MATERIAL_HIGH_ALTITUDE,
		MATERIAL_COUNT
	};

	struct VirtualTextureAtlas
	{
		struct Map
		{
			wi::graphics::Texture texture;
			wi::graphics::Texture texture_raw_block;
		};
		Map maps[4];
		wi::graphics::GPUBuffer tile_pool;

		uint8_t physical_tile_count_x = 0;
		uint8_t physical_tile_count_y = 0;

		struct Tile
		{
			uint8_t x = 0xFF;
			uint8_t y = 0xFF;
			uint8_t sub_shift = 0; // 0: whole physical tile, otherwise the physical tile is subdivided into (1 << sub_shift)^2 smaller tiles (used for far away chunk fallbacks)
			uint8_t sub_index = 0; // index of the subdivision slot within the physical tile
			constexpr bool IsValid() const
			{
				return x != 0xFF && y != 0xFF;
			}
			constexpr operator uint16_t() const { return uint16_t(uint16_t(x) | (uint16_t(y) << 8u)); }
		};
		wi::vector<Tile> free_tiles;

		static constexpr uint8_t SUB_SHIFT_MAX = 2;
		static constexpr uint32_t SUB_SLOT_MAX = 1u << (SUB_SHIFT_MAX * 2);

		struct PhysicalTile
		{
			const Tile* last_used = nullptr;
			uint64_t free_frames = 0;

			// Subdivision state, only valid while last_used == &sub_block:
			Tile sub_block; // owner of the whole physical tile when it is subdivided
			const Tile* sub_owners[SUB_SLOT_MAX] = {};
			uint32_t sub_free_frames[SUB_SLOT_MAX] = {};

			constexpr bool IsSubdivided(uint8_t shift) const
			{
				return last_used == &sub_block && sub_block.sub_shift == shift;
			}
		};
		wi::vector<PhysicalTile> physical_tiles;
		wi::vector<uint32_t> sub_blocks[SUB_SHIFT_MAX + 1]; // physical tile indices that are subdivided, per sub_shift

		// Size of one subdivision slot in texels including border, it is kept multiple of 4 for block compression:
		static constexpr uint32_t sub_tile_pitch(uint8_t shift)
		{
			return ((SVT_TILE_SIZE_PADDED / 4u) >> shift) * 4u;
		}
		// Usable resolution of a tile without the border:
		static constexpr uint32_t sub_tile_resolution(uint8_t shift)
		{
			return sub_tile_pitch(shift) - SVT_TILE_BORDER * 2;
		}
		// Upper left texel of a tile inside the atlas, including the border:
		static inline XMUINT2 sub_tile_offset(const Tile& tile)
		{
			const uint32_t pitch = sub_tile_pitch(tile.sub_shift);
			const uint32_t slots = 1u << tile.sub_shift;
			return XMUINT2(
				tile.x * SVT_TILE_SIZE_PADDED + (tile.sub_index % slots) * pitch,
				tile.y * SVT_TILE_SIZE_PADDED + (tile.sub_index / slots) * pitch
			);
		}
		// Texture coordinate transform to map a tile without mips into the atlas (for chunks without residency):
		XMFLOAT4 get_tile_texmuladd(const Tile& tile) const
		{
			const float width_rcp = 1.0f / float(maps[0].texture.desc.width);
			const float height_rcp = 1.0f / float(maps[0].texture.desc.height);
			const float resolution = float(sub_tile_resolution(tile.sub_shift));
			const XMUINT2 offset = sub_tile_offset(tile);
			return XMFLOAT4(
				resolution * width_rcp,
				resolution * height_rcp,
				float(offset.x + SVT_TILE_BORDER) * width_rcp,
				float(offset.y + SVT_TILE_BORDER) * height_rcp
			);
		}

		struct Residency
		{
			wi::graphics::Texture feedbackMap;
			wi::graphics::Texture residencyMap;
			wi::graphics::GPUBuffer requestBuffer;
			wi::graphics::GPUBuffer allocationBuffer;
			wi::graphics::GPUBuffer allocationBuffer_CPU_readback[wi::graphics::GraphicsDevice::GetBufferCount()];
			wi::graphics::GPUBuffer pageBuffer;
			wi::graphics::GPUBuffer pageBuffer_CPU_upload[wi::graphics::GraphicsDevice::GetBufferCount()];
			bool data_available_CPU[wi::graphics::GraphicsDevice::GetBufferCount()] = {};
			int16_t cpu_resource_id = 0;
			uint32_t resolution = 0;

			void init(uint32_t resolution);
			void reset();
		};
		wi::unordered_map<uint32_t, wi::vector<wi::allocator::shared_ptr<Residency>>> free_residencies; // per resolution residencies

		bool allocate_tile(Tile& tile)
		{
			if (free_tiles.empty())
				return false;
			tile = free_tiles.back();
			free_tiles.pop_back();
			PhysicalTile& physical_tile = physical_tiles[tile.x + tile.y * physical_tile_count_x];
			physical_tile.last_used = &tile;
			physical_tile.free_frames = 0;
			return true;
		}
		// Allocates a whole physical tile (sub_shift = 0) or a subdivision slot of a shared physical tile (sub_shift > 0)
		bool allocate_sub_tile(Tile& tile, uint8_t sub_shift)
		{
			if (sub_shift == 0)
				return allocate_tile(tile);
			sub_shift = std::min(sub_shift, SUB_SHIFT_MAX);
			const uint32_t slot_count = 1u << (sub_shift * 2);
			auto claim = [&](PhysicalTile& physical_tile, uint32_t slot) {
				physical_tile.sub_owners[slot] = &tile;
				physical_tile.sub_free_frames[slot] = 0;
				tile = physical_tile.sub_block;
				tile.sub_index = uint8_t(slot);
			};
			wi::vector<uint32_t>& blocks = sub_blocks[sub_shift];
			for (size_t i = 0; i < blocks.size();)
			{
				PhysicalTile& physical_tile = physical_tiles[blocks[i]];
				if (!physical_tile.IsSubdivided(sub_shift))
				{
					// physical tile was reclaimed for other use:
					blocks[i] = blocks.back();
					blocks.pop_back();
					continue;
				}
				// Only blocks kept resident in last frame can be used, otherwise they can be in free_tiles list:
				if (physical_tile.free_frames == 0)
				{
					for (uint32_t slot = 0; slot < slot_count; ++slot)
					{
						if (physical_tile.sub_owners[slot] == nullptr || physical_tile.sub_free_frames[slot] > 0)
						{
							claim(physical_tile, slot);
							return true;
						}
					}
				}
				i++;
			}
			// No free slot found, subdivide a new physical tile:
			Tile block;
			if (!allocate_tile(block))
				return false;
			const uint32_t index = block.x + block.y * physical_tile_count_x;
			PhysicalTile& physical_tile = physical_tiles[index];
			physical_tile.sub_block = block;
			physical_tile.sub_block.sub_shift = sub_shift;
			physical_tile.last_used = &physical_tile.sub_block;
			for (uint32_t slot = 0; slot < SUB_SLOT_MAX; ++slot)
			{
				physical_tile.sub_owners[slot] = nullptr;
				physical_tile.sub_free_frames[slot] = 0;
			}
			if (std::find(blocks.begin(), blocks.end(), index) == blocks.end())
			{
				blocks.push_back(index);
			}
			claim(physical_tile, 0);
			return true;
		}
		bool request_residency(const Tile& tile)
		{
			if (check_tile_resident(tile))
			{
				PhysicalTile& physical_tile = physical_tiles[tile.x + tile.y * physical_tile_count_x];
				physical_tile.free_frames = 0;
				if (tile.sub_shift > 0)
				{
					physical_tile.sub_free_frames[tile.sub_index] = 0;
				}
				return true;
			}
			return false;
		}
		bool check_tile_resident(const Tile& tile) const
		{
			if (!tile.IsValid())
				return false;
			const PhysicalTile& physical_tile = physical_tiles[tile.x + tile.y * physical_tile_count_x];
			if (tile.sub_shift > 0)
			{
				return physical_tile.IsSubdivided(tile.sub_shift) && physical_tile.sub_owners[tile.sub_index] == &tile;
			}
			return physical_tile.last_used == &tile;
		}
		uint64_t get_tile_frames(const Tile& tile) const
		{
			if (!tile.IsValid())
				return ~0ull;
			return physical_tiles[tile.x + tile.y * physical_tile_count_x].free_frames;
		}
		wi::allocator::shared_ptr<Residency> allocate_residency(uint32_t resolution)
		{
			if (free_residencies[resolution].empty())
			{
				wi::allocator::shared_ptr<Residency> residency = wi::allocator::make_shared<Residency>();
				residency->init(resolution);
				free_residencies[resolution].push_back(residency);
			}
			wi::allocator::shared_ptr<Residency> residency = free_residencies[resolution].back();
			free_residencies[resolution].pop_back();
			residency->reset();
			return residency;
		}
		void free_residency(wi::allocator::shared_ptr<Residency>& residency)
		{
			if (residency == nullptr)
				return;
			free_residencies[residency->resolution].push_back(residency);
			residency = {};
		}
		inline bool IsValid() const
		{
			return maps[0].texture.IsValid();
		}
	};

	struct VirtualTexture
	{
		wi::allocator::shared_ptr<VirtualTextureAtlas::Residency> residency;
		wi::vector<VirtualTextureAtlas::Tile> tiles;
		uint32_t lod_count = 0;
		uint32_t resolution = 0;
		uint8_t fallback_sub_shift = 0; // only for chunks without residency: subdivision of the physical tile that holds the single tile

		void init(VirtualTextureAtlas& atlas, uint resolution, uint8_t fallback_sub_shift = 0);

		void free(VirtualTextureAtlas& atlas)
		{
			tiles.clear();
			atlas.free_residency(residency);
		}

		void invalidate()
		{
			resolution = 0;
		}

		struct AllocationRequest
		{
			uint32_t x = 0;
			uint32_t y = 0;
			uint32_t lod = 0;
			uint32_t tile_index = 0;
		};
		wi::vector<AllocationRequest> allocation_requests;

		// Attach this data to Virtual Texture because we will record these by separate CPU thread:
		struct UpdateRequest
		{
			uint16_t x = 0;
			uint16_t y = 0;
			uint16_t lod = 0;
			uint8_t tile_x = 0;
			uint8_t tile_y = 0;
			uint8_t sub_shift = 0;
			uint8_t sub_index = 0;
		};
		mutable wi::vector<UpdateRequest> update_requests;
		wi::graphics::Texture blendmap;
	};

	struct BlendmapLayer
	{
		wi::vector<uint8_t> pixels;
	};

	struct ChunkData
	{
		wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity grass_entity = wi::ecs::INVALID_ENTITY; // single grass patch for earlier terrain version
		wi::ecs::Entity props_entity_old = wi::ecs::INVALID_ENTITY; // prop entity for earlier terrain version
		float prop_density_current = 1; // prop density that the prop placements were computed with
		float grass_density_current = 1;

		struct PropCell
		{
			wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY; // valid when objects are placed
			bool demote = false; // objects will be removed when the static impostors that include this cell are swapped in
			wi::vector<wi::vector<wi::scene::ImpostorComponent::StaticImpostor>> placements; // placement points per prop (not serialized)
		};
		wi::vector<PropCell> prop_cells;
		bool props_placed = false; // placements of prop_cells are computed (not serialized)
		bool prop_objects = false; // any of the prop_cells can have objects placed

		struct GrassPatch
		{
			wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
			uint32_t base_strand_count = 0;
			uint32_t x0 = 0;
			uint32_t z0 = 0;
			uint32_t x1 = 0;
			uint32_t z1 = 0;
			uint32_t region = MATERIAL_BASE;
		};
		wi::vector<GrassPatch> grass_patches;
		const XMFLOAT3* mesh_vertex_positions = nullptr;
		wi::vector<float> grass_region_lengths[MATERIAL_COUNT];
		wi::vector<BlendmapLayer> blendmap_layers;
		wi::vector<BlendmapLayer> spline_blendmap_layers;
		wi::graphics::Texture blendmap;
		wi::primitive::Sphere sphere;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		bool visible = true;
		bool invalidated = false;
		wi::allocator::shared_ptr<VirtualTexture> vt;
		wi::vector<uint16_t> heightmap_data;
		wi::graphics::Texture heightmap;

		void enable_blendmap_layer(size_t materialIndex)
		{
			while (blendmap_layers.size() < materialIndex + 1)
			{
				blendmap_layers.emplace_back().pixels.resize(vertexCount);
			}
		}
	};

	struct Prop
	{
		wi::vector<uint8_t> data; // serialized component data storage
		wi::ecs::Entity source_entity = wi::ecs::INVALID_ENTITY; // the original entity that this prop was serialized from
		float density = 10; // target count on one unscaled chunk
		float clumping = 0; // clumping of distribution
		float clump_spacing = 64; // spacing of clumps
		uint32_t seed = 0; // placement distribuiton seed
		int region = 0; // region selection in range [0,3] (0: base/grass, 1: slopes, 2: low altitude (bottom level-0), 3: high altitude (0-top level))
		float region_power = 1; // region weight affection power factor
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
			TESSELLATION = 1 << 6,
		};
		uint32_t _flags = CENTER_TO_CAM | REMOVAL | GRASS;

		wi::ecs::Entity terrainEntity = wi::ecs::INVALID_ENTITY;
		wi::ecs::Entity chunkGroupEntity = wi::ecs::INVALID_ENTITY;
		wi::scene::Scene* scene = nullptr;
		wi::vector<wi::ecs::Entity> materialEntities;
		wi::vector<wi::ecs::Entity> splineMaterialEntities;
		wi::ecs::Entity grassEntities[MATERIAL_COUNT] = {}; // grass per region index; base is [MATERIAL_BASE]
		wi::scene::WeatherComponent weather;
		wi::HairParticleSystem grass_properties; // base grass, edited by the length and distance sliders
		wi::scene::MaterialComponent grass_material;
		wi::HairParticleSystem grass_region_properties[MATERIAL_COUNT];
		wi::scene::MaterialComponent grass_region_materials[MATERIAL_COUNT];
		wi::unordered_map<Chunk, ChunkData> chunks;
		Chunk center_chunk;
		wi::noise::Perlin perlin_noise;
		wi::vector<Prop> props;
		int grass_chunk_dist = 1;

		// For generating scene on a background thread:
		float generation_time_budget_milliseconds = 8; // after this much time, the generation thread will start to exit. This can help avoid a very long running, resource consuming and slow cancellation generation
		wi::allocator::shared_ptr<Generator> generator;

		wi::vector<VirtualTexture*> virtual_textures_in_use;
		wi::graphics::Sampler sampler;
		VirtualTextureAtlas atlas;

		struct NoSparseCopy
		{
			const wi::graphics::Texture* texture_src = nullptr;
			const wi::graphics::Texture* texture_dst = nullptr;
			wi::graphics::Box srcbox = {};
		};
		mutable wi::vector<NoSparseCopy> nosparse_copies;

		wi::graphics::GPUBuffer chunk_buffer;
		int chunk_buffer_range = 3; // how many chunks to upload to GPU in X and Z directions

		constexpr bool IsCenterToCamEnabled() const { return _flags & CENTER_TO_CAM; }
		constexpr bool IsRemovalEnabled() const { return _flags & REMOVAL; }
		constexpr bool IsGrassEnabled() const { return _flags & GRASS; }
		constexpr bool IsGenerationStarted() const { return _flags & GENERATION_STARTED; }
		constexpr bool IsPhysicsEnabled() const { return _flags & PHYSICS; }
		constexpr bool IsTessellationEnabled() const { return _flags & TESSELLATION; }

		constexpr void SetCenterToCamEnabled(bool value) { if (value) { _flags |= CENTER_TO_CAM; } else { _flags &= ~CENTER_TO_CAM; } }
		constexpr void SetRemovalEnabled(bool value) { if (value) { _flags |= REMOVAL; } else { _flags &= ~REMOVAL; } }
		constexpr void SetGrassEnabled(bool value) { if (value) { _flags |= GRASS; } else { _flags &= ~GRASS; } }
		constexpr void SetGenerationStarted(bool value) { if (value) { _flags |= GENERATION_STARTED; } else { _flags &= ~GENERATION_STARTED; } }
		constexpr void SetPhysicsEnabled(bool value) { if (value) { _flags |= PHYSICS; } else { _flags &= ~PHYSICS; } }
		constexpr void SetTessellationEnabled(bool value) { if (value) { _flags |= TESSELLATION; } else { _flags &= ~TESSELLATION; } }

		float lod_bias = 0;
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

		wi::vector<wi::allocator::shared_ptr<Modifier>> modifiers;
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
		// Check if the terrain generation is currently busy (running in background)
		bool IsGenerationBusy() const;
		// Creates the textures for a chunk data
		void CreateChunkRegionTexture(ChunkData& chunk_data);

		void UpdateVirtualTexturesCPU();
		void UpdateVirtualTexturesGPU(wi::graphics::CommandList cmd) const;
		void CopyVirtualTexturePageStatusGPU(wi::graphics::CommandList cmd) const;
		void AllocateVirtualTextureTileRequestsGPU(wi::graphics::CommandList cmd) const;
		void WritebackTileRequestsGPU(wi::graphics::CommandList cmd) const;

		ShaderTerrain GetShaderTerrain() const;

		void InvalidateProps();

		void InvalidateChunksAtSpline(const wi::scene::SplineComponent& spline);

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

	private:
		wi::vector<wi::scene::MaterialComponent> materials; // temp storage allocation
		float chunk_scale_rcp = 1.0f / chunk_scale;
		float cam_height = 0;
		float cam_x = 0;
		float cam_z = 0;

		void ComputePropPlacements(const Chunk& chunk, ChunkData& chunk_data); // computes the prop placement points of the chunk's prop cells from the chunk surface
		wi::vector<wi::ecs::Entity> prop_entities_old; // prop entities of earlier terrain version, they are removed after loading

		// Prop static impostors:
		struct PropImpostorPart
		{
			wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;
			XMFLOAT4X4 matrix = wi::math::IDENTITY_MATRIX; // object matrix relative to prop placement
			bool has_impostor = false;
			uint32_t buffer_index = 0; // index into prop_impostor_buffers
		};
		struct PropImpostorCache
		{
			wi::vector<uint8_t> data; // copy of Prop::data that the parts were created from
			wi::vector<PropImpostorPart> parts;
		};
		wi::vector<PropImpostorCache> prop_impostor_cache;
		// Static impostor buffers per impostor mesh, double buffered: the front buffer is used by the ImpostorComponent, the back buffer is refilled on the generation thread
		struct PropImpostorBuffer
		{
			wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;
			wi::graphics::GPUBuffer buffers[2];
			uint32_t counts[2] = {};
			wi::graphics::GPUBuffer upload; // CPU writable staging buffer, copied into the back buffer with async copy
		};
		wi::vector<PropImpostorBuffer> prop_impostor_buffers;
		int prop_impostor_front = 0; // index of the double buffers that is in use by ImpostorComponents
		uint64_t prop_impostor_swap_frame = 0; // device frame when the double buffers were last swapped
		bool prop_impostor_back_free = true; // back buffer is not in use by GPU, it can be refilled, otherwise new back buffers are allocated
		bool prop_impostors_ready = false; // back buffer was refilled, waiting to be swapped
		bool prop_impostors_dirty = false;
		void RefreshPropImpostorParts();
		void UpdatePropStaticImpostors(); // fills the back buffers, runs on generation thread
		bool SwapPropStaticImpostors(); // swaps the double buffers if refilled and removes objects of demoted chunks (returns true if objects were removed), runs on main thread
		void ClearPropStaticImpostors(); // removes static impostors from ImpostorComponents immediately, runs on main thread
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

		// helpers for more user friendly setup with scaling in world space:
		constexpr void SetScale(float scale) { frequency = 1.0f / scale; }
		constexpr float GetScale() const { return 1.0f / frequency; }

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
			float weight = std::pow(1 - saturate((res.distance - shape) * fade), std::max(0.0001f, falloff));
			Blend(height, weight);
		}
	};
	struct HeightmapModifier : public Modifier
	{
		float amount = 1.0f; // multiplier for height values

		wi::vector<uint8_t> data;
		int width = 0;
		int height = 0;

		HeightmapModifier() { type = Type::Heightmap; SetScale(1.0f); weight = 1.0f; }
		void Apply(const XMFLOAT2& world_pos, float& height) override
		{
			XMFLOAT2 p = world_pos;
			p.x *= frequency;
			p.y *= frequency;
			const XMFLOAT2 pixel = XMFLOAT2(p.x + this->width * 0.5f, float(this->height - 1) - (p.y + this->height * 0.5f));
			if (pixel.x >= 0 && pixel.x < this->width - 1 && pixel.y >= 0 && pixel.y < this->height - 1)
			{
				const XMFLOAT2 pixel_frac = XMFLOAT2(frac(pixel.x), frac(pixel.y));
				const XMUINT2 pixel_tl = XMUINT2((uint32_t)std::floor(pixel.x), (uint32_t)std::floor(pixel.y));
				const XMUINT2 pixel_tr = XMUINT2((uint32_t)std::ceil(pixel.x), (uint32_t)std::floor(pixel.y));
				const XMUINT2 pixel_bl = XMUINT2((uint32_t)std::floor(pixel.x), (uint32_t)std::ceil(pixel.y));
				const XMUINT2 pixel_br = XMUINT2((uint32_t)std::ceil(pixel.x), (uint32_t)std::ceil(pixel.y));
				const bool is_8bit = data.size() == this->width * this->height * sizeof(uint8_t);
				XMFLOAT4 gather;
				if (is_8bit)
				{
					const float remap = 1.0f / 255.0f;
					const uint8_t* src = data.data();
					const float tl = (float)src[pixel_tl.x + pixel_tl.y * this->width] * remap;
					const float tr = (float)src[pixel_tr.x + pixel_tr.y * this->width] * remap;
					const float bl = (float)src[pixel_bl.x + pixel_bl.y * this->width] * remap;
					const float br = (float)src[pixel_br.x + pixel_br.y * this->width] * remap;
					gather = XMFLOAT4(bl, br, tr, tl);
				}
				else
				{
					const float remap = 1.0f / 65535.0f;
					const uint16_t* src = (const uint16_t*)data.data();
					const float tl = (float)src[pixel_tl.x + pixel_tl.y * this->width] * remap;
					const float tr = (float)src[pixel_tr.x + pixel_tr.y * this->width] * remap;
					const float bl = (float)src[pixel_bl.x + pixel_bl.y * this->width] * remap;
					const float br = (float)src[pixel_br.x + pixel_br.y * this->width] * remap;
					gather = XMFLOAT4(bl, br, tr, tl);
				}
				Blend(height, bilinear(gather, pixel_frac) * amount);
			}
		}
	};

}
