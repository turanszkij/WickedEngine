#pragma once
#include "WickedEngine.h"

#include <random>
#include <string>

struct Chunk
{
	int x, z;
	constexpr bool operator==(const Chunk& other) const
	{
		return (x == other.x) && (z == other.z);
	}
};
namespace std
{
	template <>
	struct hash<Chunk>
	{
		inline size_t operator()(const Chunk& k) const
		{
			return ((hash<int>()(k.x) ^ (hash<int>()(k.z) << 1)) >> 1);
		}
	};
}

struct ChunkData
{
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	wi::HairParticleSystem grass;
};

struct TerrainGenerator : public wi::gui::Window
{
	inline static const int width = 64 + 3; // + 3: filler vertices for lod apron and grid perimeter
	inline static const float half_width = (width - 1) * 0.5f;
	inline static const float width_rcp = 1.0f / (width - 1);
	inline static const uint32_t vertexCount = width * width;
	inline static const int max_lod = (int)std::log2(width - 3) + 1;
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
	std::mt19937 prop_rand;

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

	// This needs to be called at least once before using the terrain generator
	void init();
	// Restarts the terrain generateion from scratch
	//	This will remove previously existing terrain
	void Generation_Restart();
	// This will run the actual generation tasks, call it once per frame
	//	budget_milliseconds : approximately this much time will be allowed for generating chunks. At minimum, one chunk will be always generated.
	void Generation_Update(int budget_milliseconds = 2);

};
