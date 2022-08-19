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
	float prop_density_current = 1;
	wi::HairParticleSystem grass;
	float grass_density_current = 1;
	std::mt19937 prop_rand;
	wi::Color region_weights[vertexCount] = {};
	wi::graphics::Texture region_weights_texture;
	uint32_t required_texture_resolution = 0;
	wi::primitive::Sphere sphere;
	XMFLOAT3 position = XMFLOAT3(0, 0, 0);
};

struct TerrainGenerator : public wi::gui::Window
{
	static constexpr int EVENT_THEME_RESET = 12345;
	wi::ecs::Entity terrainEntity = wi::ecs::INVALID_ENTITY;
	wi::scene::Scene* scene = nullptr;
	wi::scene::MaterialComponent material_Base;
	wi::scene::MaterialComponent material_Slope;
	wi::scene::MaterialComponent material_LowAltitude;
	wi::scene::MaterialComponent material_HighAltitude;
	wi::scene::MaterialComponent material_GrassParticle;
	wi::HairParticleSystem grass_properties;
#ifdef PLATFORM_LINUX
	// TODO: investigate why wi::unordered_map is crashing terrain generator on Linux
	std::unordered_map<Chunk, ChunkData> chunks;
#else
	wi::unordered_map<Chunk, ChunkData> chunks;
#endif
	wi::vector<uint32_t> indices;
	struct LOD
	{
		uint32_t indexOffset = 0;
		uint32_t indexCount = 0;
	};
	wi::vector<LOD> lods;
	Chunk center_chunk = {};
	wi::noise::Perlin perlin_noise;

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
	wi::gui::Slider propDensitySlider;
	wi::gui::Slider grassDensitySlider;
	wi::gui::ComboBox presetCombo;
	wi::gui::Slider scaleSlider;
	wi::gui::Slider seedSlider;
	wi::gui::Slider bottomLevelSlider;
	wi::gui::Slider topLevelSlider;
	wi::gui::Button saveHeightmapButton;
	wi::gui::ComboBox addModifierCombo;

	wi::gui::Slider region1Slider;
	wi::gui::Slider region2Slider;
	wi::gui::Slider region3Slider;

	struct Modifier : public wi::gui::Window
	{
		wi::gui::ComboBox blendCombo;
		wi::gui::Slider blendSlider;
		wi::gui::Slider frequencySlider;

		enum class BlendMode
		{
			Normal,
			Multiply,
			Additive,
		};

		Modifier(const std::string& name)
		{
			wi::gui::Window::Create(name, wi::gui::Window::WindowControls::CLOSE_AND_COLLAPSE);
			SetSize(XMFLOAT2(200, 100));

			blendCombo.Create("Blend Mode: ");
			blendCombo.SetSize(XMFLOAT2(100, 20));
			blendCombo.AddItem("Normal", (uint64_t)BlendMode::Normal);
			blendCombo.AddItem("Additive", (uint64_t)BlendMode::Additive);
			blendCombo.AddItem("Multiply", (uint64_t)BlendMode::Multiply);
			AddWidget(&blendCombo);

			blendSlider.Create(0, 1, 0.5f, 10000, "Blend: ");
			blendSlider.SetSize(XMFLOAT2(100, 20));
			blendSlider.SetTooltip("Blending amount");
			AddWidget(&blendSlider);

			frequencySlider.Create(0.0001f, 0.01f, 0.0008f, 10000, "Frequency: ");
			frequencySlider.SetSize(XMFLOAT2(100, 20));
			frequencySlider.SetTooltip("Frequency for the tiling");
			AddWidget(&frequencySlider);
		}

		virtual void Seed(uint32_t seed) {}
		virtual void SetCallback(std::function<void(wi::gui::EventArgs args)> func)
		{
			blendCombo.OnSelect(func);
			blendSlider.OnSlide(func);
			frequencySlider.OnSlide(func);
		}
		virtual void Apply(const XMFLOAT2& world_pos, float& height) = 0;
		void Blend(float& height, float value)
		{
			switch ((BlendMode)blendCombo.GetItemUserData(blendCombo.GetSelected()))
			{
			default:
			case BlendMode::Normal:
				height = wi::math::Lerp(height, value, blendSlider.GetValue());
				break;
			case BlendMode::Multiply:
				height *= value * blendSlider.GetValue();
				break;
			case BlendMode::Additive:
				height += value * blendSlider.GetValue();
				break;
			}
		}
	};
	wi::vector<std::unique_ptr<Modifier>> modifiers;
	wi::vector<Modifier*> modifiers_to_remove;
	void AddModifier(Modifier* modifier);

	void Create();

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

	void ResizeLayout() override;
};
