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

struct ChunkData
{
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	wi::HairParticleSystem grass;
	bool grass_exists = false;
	std::mt19937 prop_rand;
};

struct TerrainGenerator : public wi::gui::Window
{
	inline static const int width = 64 + 3; // + 3: filler vertices for lod apron and grid perimeter
	inline static const float half_width = (width - 1) * 0.5f;
	inline static const float width_rcp = 1.0f / (width - 1);
	inline static const uint32_t vertexCount = width * width;
	inline static const int max_lod = (int)std::log2(width - 3) + 1;
	wi::scene::Scene* scene = &wi::scene::GetScene(); // by default it uses the global scene, but this can be changed
	wi::ecs::Entity terrainEntity = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity materialEntity_Base = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity materialEntity_Slope = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity materialEntity_LowAltitude = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity materialEntity_HighAltitude = wi::ecs::INVALID_ENTITY;
	wi::scene::MaterialComponent material_Base;
	wi::scene::MaterialComponent material_Slope;
	wi::scene::MaterialComponent material_LowAltitude;
	wi::scene::MaterialComponent material_HighAltitude;
	wi::scene::MaterialComponent material_GrassParticle;
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
	wi::vector<wi::ecs::Entity> generation_entities; // because cannot parent to main scene on the background thread, parentable entities are collected

	wi::gui::CheckBox centerToCamCheckBox;
	wi::gui::CheckBox removalCheckBox;
	wi::gui::Slider lodSlider;
	wi::gui::Slider generationSlider;
	wi::gui::ComboBox presetCombo;
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
	void Generation_Update();
	// Tells the generation thread that it should be cancelled and blocks until that is confirmed
	void Generation_Cancel();

};
