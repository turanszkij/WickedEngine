#pragma once
#include "WickedEngine.h"

#include <random>
#include <string>
#include <atomic>

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
namespace std
{
	template <>
	struct hash<Chunk>
	{
		inline size_t operator()(const Chunk& chunk) const
		{
			return chunk.compute_hash();
		}
	};
}

inline static const int chunk_width = 64 + 3; // + 3: filler vertices for lod apron and grid perimeter
inline static const float chunk_half_width = (chunk_width - 1) * 0.5f;
inline static const float chunk_width_rcp = 1.0f / (chunk_width - 1);
inline static const uint32_t vertexCount = chunk_width * chunk_width;
inline static const int max_lod = (int)std::log2(chunk_width - 3) + 1;
struct ChunkData
{
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity grass_entity = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity props_entity = wi::ecs::INVALID_ENTITY;
	const XMFLOAT3* mesh_vertex_positions = nullptr;
	wi::HairParticleSystem grass;
	std::mt19937 prop_rand;
	wi::Color region_weights[vertexCount] = {};
	wi::graphics::Texture region_weights_texture;
	uint32_t virtual_texture_resolution = 0;
	wi::primitive::Sphere sphere;
	XMFLOAT3 position = XMFLOAT3(0, 0, 0);
};

struct TerrainGenerator : public wi::gui::Window
{
	wi::ecs::Entity terrainEntity = wi::ecs::INVALID_ENTITY;
	wi::scene::Scene* scene = nullptr;
	wi::scene::MaterialComponent material_Base;
	wi::scene::MaterialComponent material_Slope;
	wi::scene::MaterialComponent material_LowAltitude;
	wi::scene::MaterialComponent material_HighAltitude;
	wi::scene::MaterialComponent material_GrassParticle;
	wi::HairParticleSystem grass_properties;
	wi::unordered_map<Chunk, ChunkData> chunks;
	wi::vector<uint32_t> indices;
	struct LOD
	{
		uint32_t indexOffset = 0;
		uint32_t indexCount = 0;
	};
	wi::vector<LOD> lods;
	wi::noise::Perlin perlin;
	Chunk center_chunk = {};

	struct HeightmapTexture
	{
		wi::vector<uint8_t> data;
		int width = 0;
		int height = 0;
	} heightmap;

	struct Prop
	{
		std::string name = "prop";
		wi::ecs::Entity mesh_entity = wi::ecs::INVALID_ENTITY;
		wi::scene::ObjectComponent object;
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
	wi::vector<Prop> props;

	// For generating scene on a background thread:
	wi::scene::Scene generation_scene; // The background generation thread can safely add things to this, it will be merged into the main scene when it is safe to do so
	wi::jobsystem::context generation_workload;
	std::atomic_bool generation_cancelled;
	float generation_time_budget_milliseconds = 12; // after this much time, the generation thread will exit. This can help avoid a very long running, resource consuming and slow cancellation generation

	// Virtual texture updates will be batched like:
	//	1) Execute all barriers (dst: UNORDERED_ACCESS)
	//	2) Execute all compute shaders
	//	3) Execute all barriers (dst: SHADER_RESOURCE)
	wi::vector<Chunk> virtual_texture_updates;
	wi::vector<wi::graphics::GPUBarrier> virtual_texture_barriers_begin;
	wi::vector<wi::graphics::GPUBarrier> virtual_texture_barriers_end;

	wi::gui::CheckBox centerToCamCheckBox;
	wi::gui::CheckBox removalCheckBox;
	wi::gui::CheckBox grassCheckBox;
	wi::gui::Slider lodSlider;
	wi::gui::Slider texlodSlider;
	wi::gui::Slider generationSlider;
	wi::gui::Slider propSlider;
	wi::gui::ComboBox presetCombo;
	wi::gui::Slider scaleSlider;
	wi::gui::Slider propDensitySlider;
	wi::gui::Slider seedSlider;
	wi::gui::Slider bottomLevelSlider;
	wi::gui::Slider topLevelSlider;
	wi::gui::Slider perlinBlendSlider;
	wi::gui::Slider perlinFrequencySlider;
	wi::gui::Slider perlinOctavesSlider;
	wi::gui::Slider voronoiBlendSlider;
	wi::gui::Slider voronoiFrequencySlider;
	wi::gui::Slider voronoiFadeSlider;
	wi::gui::Slider voronoiShapeSlider;
	wi::gui::Slider voronoiFalloffSlider;
	wi::gui::Slider voronoiPerturbationSlider;
	wi::gui::Button saveHeightmapButton;
	wi::gui::Button heightmapButton;
	wi::gui::Slider heightmapBlendSlider;

	wi::gui::Slider region1Slider;
	wi::gui::Slider region2Slider;
	wi::gui::Slider region3Slider;

	// This needs to be called at least once before using the terrain generator
	void init();

	// Restarts the terrain generation from scratch
	//	This will remove previously existing terrain
	void Generation_Restart();
	// This will run the actual generation tasks, call it once per frame
	void Generation_Update(const wi::scene::CameraComponent& camera);
	// Tells the generation thread that it should be cancelled and blocks until that is confirmed
	void Generation_Cancel();
	// The virtual textures will be compressed and saved into resources. They can be serialized from there
	void BakeVirtualTexturesToFiles();
};
