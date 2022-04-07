#include "stdafx.h"
#include "MeshWindow.h"
#include "Editor.h"

#include "Utility/stb_image.h"

#include "meshoptimizer/meshoptimizer.h"

#include <string>
#include <random>

using namespace wi::ecs;
using namespace wi::scene;

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
	wi::ecs::Entity entity = INVALID_ENTITY;
	wi::HairParticleSystem grass;
};

struct TerraGen : public wi::gui::Window
{
	const int width = 64 + 3; // + 3: filler vertices for lod apron and grid perimeter
	const float half_width = (width - 1) * 0.5f;
	const float width_rcp = 1.0f / (width - 1);
	const uint32_t vertexCount = width * width;
	const int max_lod = (int)std::log2(width - 3) + 1;
	Entity terrainEntity = INVALID_ENTITY;
	Entity materialEntity_Base = INVALID_ENTITY;
	Entity materialEntity_Slope = INVALID_ENTITY;
	Entity materialEntity_LowAltitude = INVALID_ENTITY;
	Entity materialEntity_HighAltitude = INVALID_ENTITY;
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
		Entity mesh_entity = INVALID_ENTITY;
		ObjectComponent object;
		int min_count_per_chunk = 0; // a chunk will try to generate min this many props of this type
		int max_count_per_chunk = 10; // a chunk will try to generate max this many props of this type
		int region = 0; // region selection in range [0,3]
		float region_power = 1; // region weight affection power factor
		float noise_frequency = 1.0f; // perlin noise's frequency for placement factor
		float threshold = 0.5f; // the chance of placement (higher is less chance)
		float min_size = 1; // scaling randomization range min
		float max_size = 1; // scaling randomization range max
		float min_y_offset = 0; // min randomized offset on Y axis
		float max_y_offset = 0; // max randomized offset on Y axis
	};
	wi::vector<Prop> props;
	std::mt19937 prop_rand;

	wi::gui::CheckBox generationCheckBox;
	wi::gui::CheckBox removalCheckBox;
	wi::gui::Slider seedSlider;
	wi::gui::Slider lodSlider;
	wi::gui::Slider generationSlider;
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

	void init()
	{
		indices.clear();
		lods.clear();
		lods.resize(max_lod);
		for (int lod = 0; lod < max_lod; ++lod)
		{
			lods[lod].indexOffset = (uint32_t)indices.size();

			if (lod == 0)
			{
				for (int x = 0; x < width - 1; x++)
				{
					for (int z = 0; z < width - 1; z++)
					{
						int lowerLeft = x + z * width;
						int lowerRight = (x + 1) + z * width;
						int topLeft = x + (z + 1) * width;
						int topRight = (x + 1) + (z + 1) * width;

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
				for (int x = 1; x < width - 2; x += step)
				{
					for (int z = 1; z < width - 2; z += step)
					{
						int lowerLeft = x + z * width;
						int lowerRight = (x + step) + z * width;
						int topLeft = x + (z + step) * width;
						int topRight = (x + step) + (z + step) * width;

						indices.push_back(topLeft);
						indices.push_back(lowerLeft);
						indices.push_back(lowerRight);

						indices.push_back(topLeft);
						indices.push_back(lowerRight);
						indices.push_back(topRight);
					}
				}
				// bottom border:
				for (int x = 0; x < width - 1; ++x)
				{
					const int z = 0;
					int current = x + z * width;
					int neighbor = x + 1 + z * width;
					int connection = 1 + ((x + (step + 1) / 2 - 1) / step) * step + (z + 1) * width;

					indices.push_back(current);
					indices.push_back(neighbor);
					indices.push_back(connection);

					if (((x - 1) % (step)) == step / 2) // halfway fill triangle
					{
						int connection1 = 1 + (((x - 1) + (step + 1) / 2 - 1) / step) * step + (z + 1) * width;

						indices.push_back(current);
						indices.push_back(connection);
						indices.push_back(connection1);
					}
				}
				// top border:
				for (int x = 0; x < width - 1; ++x)
				{
					const int z = width - 1;
					int current = x + z * width;
					int neighbor = x + 1 + z * width;
					int connection = 1 + ((x + (step + 1) / 2 - 1) / step) * step + (z - 1) * width;

					indices.push_back(current);
					indices.push_back(connection);
					indices.push_back(neighbor);

					if (((x - 1) % (step)) == step / 2) // halfway fill triangle
					{
						int connection1 = 1 + (((x - 1) + (step + 1) / 2 - 1) / step) * step + (z - 1) * width;

						indices.push_back(current);
						indices.push_back(connection1);
						indices.push_back(connection);
					}
				}
				// left border:
				for (int z = 0; z < width - 1; ++z)
				{
					const int x = 0;
					int current = x + z * width;
					int neighbor = x + (z + 1) * width;
					int connection = x + 1 + (((z + (step + 1) / 2 - 1) / step) * step + 1) * width;

					indices.push_back(current);
					indices.push_back(connection);
					indices.push_back(neighbor);

					if (((z - 1) % (step)) == step / 2) // halfway fill triangle
					{
						int connection1 = x + 1 + ((((z - 1) + (step + 1) / 2 - 1) / step) * step + 1) * width;

						indices.push_back(current);
						indices.push_back(connection1);
						indices.push_back(connection);
					}
				}
				// right border:
				for (int z = 0; z < width - 1; ++z)
				{
					const int x = width - 1;
					int current = x + z * width;
					int neighbor = x + (z + 1) * width;
					int connection = x - 1 + (((z + (step + 1) / 2 - 1) / step) * step + 1) * width;

					indices.push_back(current);
					indices.push_back(neighbor);
					indices.push_back(connection);

					if (((z - 1) % (step)) == step / 2) // halfway fill triangle
					{
						int connection1 = x - 1 + ((((z - 1) + (step + 1) / 2 - 1) / step) * step + 1) * width;

						indices.push_back(current);
						indices.push_back(connection);
						indices.push_back(connection1);
					}
				}
			}

			lods[lod].indexCount = (uint32_t)indices.size() - lods[lod].indexOffset;
		}

		wi::gui::Window::Create("TerraGen");
		SetSize(XMFLOAT2(420, 460));

		float xx = 150;
		float yy = 0;
		float stepstep = 25;
		float heihei = 20;

		generationCheckBox.Create("Generation: ");
		generationCheckBox.SetTooltip("Automatically generate chunks around camera. This sets the center chunk to camera position.");
		generationCheckBox.SetSize(XMFLOAT2(heihei, heihei));
		generationCheckBox.SetPos(XMFLOAT2(xx, yy += stepstep));
		generationCheckBox.SetCheck(true);
		AddWidget(&generationCheckBox);

		removalCheckBox.Create("Removal: ");
		removalCheckBox.SetTooltip("Automatically remove chunks that are farther than generation distance around center chunk.");
		removalCheckBox.SetSize(XMFLOAT2(heihei, heihei));
		removalCheckBox.SetPos(XMFLOAT2(xx + 100, yy));
		removalCheckBox.SetCheck(true);
		AddWidget(&removalCheckBox);

		seedSlider.Create(1, 12345, 3926, 12344, "Seed: ");
		seedSlider.SetTooltip("Seed for terrain randomness");
		seedSlider.SetSize(XMFLOAT2(200, heihei));
		seedSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&seedSlider);

		lodSlider.Create(0.0001f, 0.01f, 0.004f, 10000, "LOD Distance: ");
		lodSlider.SetTooltip("Set the LOD (Level Of Detail) distance multiplier.\nLow values increase LOD detail in distance");
		lodSlider.SetSize(XMFLOAT2(200, heihei));
		lodSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		lodSlider.OnSlide([this](wi::gui::EventArgs args) {
			for (auto& it : chunks)
			{
				const ChunkData& chunk_data = it.second;
				if (chunk_data.entity != INVALID_ENTITY)
				{
					ObjectComponent* object = GetScene().objects.GetComponent(chunk_data.entity);
					if (object != nullptr)
					{
						object->lod_distance_multiplier = args.fValue;
					}
				}
			}
			});
		AddWidget(&lodSlider);

		generationSlider.Create(0, 16, 12, 16, "Generation Distance: ");
		generationSlider.SetTooltip("How far out chunks will be generated (value is in number of chunks)");
		generationSlider.SetSize(XMFLOAT2(200, heihei));
		generationSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&generationSlider);

		bottomLevelSlider.Create(-100, 0, -10, 10000, "Bottom Level: ");
		bottomLevelSlider.SetTooltip("Terrain mesh grid lowest level");
		bottomLevelSlider.SetSize(XMFLOAT2(200, heihei));
		bottomLevelSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&bottomLevelSlider);

		topLevelSlider.Create(0, 10000, 1000, 10000, "Top Level: ");
		topLevelSlider.SetTooltip("Terrain mesh grid topmost level");
		topLevelSlider.SetSize(XMFLOAT2(200, heihei));
		topLevelSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&topLevelSlider);

		perlinBlendSlider.Create(0, 1, 0.5f, 10000, "Perlin Blend: ");
		perlinBlendSlider.SetTooltip("Amount of perlin noise to use");
		perlinBlendSlider.SetSize(XMFLOAT2(200, heihei));
		perlinBlendSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&perlinBlendSlider);

		perlinFrequencySlider.Create(0.0001f, 0.01f, 0.005f, 10000, "Perlin Frequency: ");
		perlinFrequencySlider.SetTooltip("Frequency for the perlin noise");
		perlinFrequencySlider.SetSize(XMFLOAT2(200, heihei));
		perlinFrequencySlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&perlinFrequencySlider);

		perlinOctavesSlider.Create(1, 8, 6, 7, "Perlin Detail: ");
		perlinOctavesSlider.SetTooltip("Octave count for the perlin noise");
		perlinOctavesSlider.SetSize(XMFLOAT2(200, heihei));
		perlinOctavesSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&perlinOctavesSlider);

		voronoiBlendSlider.Create(0, 1, 0.5f, 10000, "Voronoi Blend: ");
		voronoiBlendSlider.SetTooltip("Amount of voronoi to use for elevation");
		voronoiBlendSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiBlendSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiBlendSlider);

		voronoiFrequencySlider.Create(0.0001f, 0.01f, 0.001f, 10000, "Voronoi Frequency: ");
		voronoiFrequencySlider.SetTooltip("Voronoi can create distinctly elevated areas, the more cells there are, smaller the consecutive areas");
		voronoiFrequencySlider.SetSize(XMFLOAT2(200, heihei));
		voronoiFrequencySlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiFrequencySlider);

		voronoiFadeSlider.Create(0, 100, 2.1f, 10000, "Voronoi Fade: ");
		voronoiFadeSlider.SetTooltip("Fade out voronoi regions by distance from cell's center");
		voronoiFadeSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiFadeSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiFadeSlider);

		voronoiShapeSlider.Create(0, 1, 0.351f, 10000, "Voronoi Shape: ");
		voronoiShapeSlider.SetTooltip("How much the voronoi shape will be kept");
		voronoiShapeSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiShapeSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiShapeSlider);

		voronoiFalloffSlider.Create(0, 8, 3.3f, 10000, "Voronoi Falloff: ");
		voronoiFalloffSlider.SetTooltip("Controls the falloff of the voronoi distance fade effect");
		voronoiFalloffSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiFalloffSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiFalloffSlider);

		voronoiPerturbationSlider.Create(0, 1, 0.1f, 10000, "Voronoi Perturbation: ");
		voronoiPerturbationSlider.SetTooltip("Controls the random look of voronoi region edges");
		voronoiPerturbationSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiPerturbationSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiPerturbationSlider);


		heightmapButton.Create("Load Heightmap...");
		heightmapButton.SetTooltip("Load a heightmap texture, where the red channel corresponds to terrain height and the resolution to dimensions.\nThe heightmap will be placed in the world center.");
		heightmapButton.SetSize(XMFLOAT2(200, heihei));
		heightmapButton.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&heightmapButton);

		heightmapBlendSlider.Create(0, 1, 1, 10000, "Heightmap Blend: ");
		heightmapBlendSlider.SetTooltip("Amount of displacement coming from the heightmap texture");
		heightmapBlendSlider.SetSize(XMFLOAT2(200, heihei));
		heightmapBlendSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&heightmapBlendSlider);



		auto generate_callback = [=](wi::gui::EventArgs args) {
			Generation_Restart();
		};
		seedSlider.OnSlide(generate_callback);
		bottomLevelSlider.OnSlide(generate_callback);
		topLevelSlider.OnSlide(generate_callback);
		perlinFrequencySlider.OnSlide(generate_callback);
		perlinBlendSlider.OnSlide(generate_callback);
		perlinOctavesSlider.OnSlide(generate_callback);
		voronoiBlendSlider.OnSlide(generate_callback);
		voronoiFrequencySlider.OnSlide(generate_callback);
		voronoiFadeSlider.OnSlide(generate_callback);
		voronoiShapeSlider.OnSlide(generate_callback);
		voronoiFalloffSlider.OnSlide(generate_callback);
		voronoiPerturbationSlider.OnSlide(generate_callback);
		heightmapBlendSlider.OnSlide(generate_callback);

		heightmapButton.OnClick([=](wi::gui::EventArgs args) {

			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

					heightmap = {};
					int bpp = 0;
					stbi_uc* rgba = stbi_load(fileName.c_str(), &heightmap.width, &heightmap.height, &bpp, 1);
					if (rgba != nullptr)
					{
						heightmap.data.resize(heightmap.width * heightmap.height);
						for (int i = 0; i < heightmap.width * heightmap.height; ++i)
						{
							heightmap.data[i] = rgba[i];
						}
						stbi_image_free(rgba);
						Generation_Restart();
					}
					});
				});
			});

		heightmap = {};
	}

	void Generation_Restart()
	{
		Scene& scene = wi::scene::GetScene();

		// If these already exist, save them before recreating:
		//	(This is helpful if eg: someone edits them in the editor and then regenerates the terrain)
		if (materialEntity_Base != INVALID_ENTITY)
		{
			MaterialComponent* material = scene.materials.GetComponent(materialEntity_Base);
			material_Base = *material;
		}
		if (materialEntity_Slope != INVALID_ENTITY)
		{
			MaterialComponent* material = scene.materials.GetComponent(materialEntity_Slope);
			material_Slope = *material;
		}
		if (materialEntity_LowAltitude != INVALID_ENTITY)
		{
			MaterialComponent* material = scene.materials.GetComponent(materialEntity_LowAltitude);
			material_LowAltitude = *material;
		}
		if (materialEntity_HighAltitude != INVALID_ENTITY)
		{
			MaterialComponent* material = scene.materials.GetComponent(materialEntity_HighAltitude);
			material_HighAltitude = *material;
		}

		chunks.clear();

		scene.Entity_Remove(terrainEntity);
		terrainEntity = CreateEntity();
		scene.transforms.Create(terrainEntity);
		scene.names.Create(terrainEntity) = "terrain";

		materialEntity_Base = scene.Entity_CreateMaterial("terrainMaterial_Base");
		materialEntity_Slope = scene.Entity_CreateMaterial("terrainMaterial_Slope");
		materialEntity_LowAltitude = scene.Entity_CreateMaterial("terrainMaterial_LowAltitude");
		materialEntity_HighAltitude = scene.Entity_CreateMaterial("terrainMaterial_HighAltitude");
		scene.Component_Attach(materialEntity_Base, terrainEntity);
		scene.Component_Attach(materialEntity_Slope, terrainEntity);
		scene.Component_Attach(materialEntity_LowAltitude, terrainEntity);
		scene.Component_Attach(materialEntity_HighAltitude, terrainEntity);
		// init/restore materials:
		*scene.materials.GetComponent(materialEntity_Base) = material_Base;
		*scene.materials.GetComponent(materialEntity_Slope) = material_Slope;
		*scene.materials.GetComponent(materialEntity_LowAltitude) = material_LowAltitude;
		*scene.materials.GetComponent(materialEntity_HighAltitude) = material_HighAltitude;

		const uint32_t seed = (uint32_t)seedSlider.GetValue();
		perlin.init(seed);
		prop_rand.seed(seed);

		// Add some nice weather and lighting if there is none yet:
		if (scene.weathers.GetCount() == 0)
		{
			Entity weatherEntity = CreateEntity();
			WeatherComponent& weather = scene.weathers.Create(weatherEntity);
			scene.names.Create(weatherEntity) = "terrainWeather";
			scene.Component_Attach(weatherEntity, terrainEntity);
			weather.SetRealisticSky(true);
			weather.SetVolumetricClouds(true);
			weather.volumetricCloudParameters.CoverageAmount = 0.95f;
			weather.volumetricCloudParameters.CoverageMinimum = 1.383f;
			//weather.SetOceanEnabled(true);
			weather.oceanParameters.waterHeight = -5;
			weather.oceanParameters.wave_amplitude = 120;
			weather.fogStart = 10;
			weather.fogEnd = 100000;
			weather.SetHeightFog(true);
			weather.fogHeightStart = 0;
			weather.fogHeightEnd = 100;
			weather.windDirection = XMFLOAT3(0.05f, 0.05f, 0.05f);
			weather.windSpeed = 4;
		}
		if (scene.lights.GetCount() == 0)
		{
			Entity sunEntity = scene.Entity_CreateLight("terrainSun");
			scene.Component_Attach(sunEntity, terrainEntity);
			LightComponent& light = *scene.lights.GetComponent(sunEntity);
			light.SetType(LightComponent::LightType::DIRECTIONAL);
			light.energy = 6;
			light.SetCastShadow(true);
			//light.SetVolumetricsEnabled(true);
			TransformComponent& transform = *scene.transforms.GetComponent(sunEntity);
			transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV4, 0, XM_PIDIV4));
		}
	}

	void Generation_Update(int allocated_timeframe_milliseconds = 2)
	{
		if (terrainEntity == INVALID_ENTITY)
			return;

		Scene& scene = wi::scene::GetScene();
		if (!scene.transforms.Contains(terrainEntity))
		{
			terrainEntity = INVALID_ENTITY;
			chunks.clear();
			return;
		}

		wi::Timer timer;
		const float lodMultiplier = lodSlider.GetValue();
		const int generation = (int)generationSlider.GetValue();
		const float bottomLevel = bottomLevelSlider.GetValue();
		const float topLevel = topLevelSlider.GetValue();
		const float heightmapBlend = heightmapBlendSlider.GetValue();
		const float perlinBlend = perlinBlendSlider.GetValue();
		const uint32_t seed = (uint32_t)seedSlider.GetValue();
		const int perlinOctaves = (int)perlinOctavesSlider.GetValue();
		const float perlinFrequency = perlinFrequencySlider.GetValue();
		const float voronoiBlend = voronoiBlendSlider.GetValue();
		const float voronoiFrequency = voronoiFrequencySlider.GetValue();
		const float voronoiFade = voronoiFadeSlider.GetValue();
		const float voronoiShape = voronoiShapeSlider.GetValue();
		const float voronoiFalloff = voronoiFalloffSlider.GetValue();
		const float voronoiPerturbation = voronoiPerturbationSlider.GetValue();

		if (generationCheckBox.GetCheck())
		{
			const CameraComponent& camera = GetCamera();
			center_chunk.x = (int)std::floor((camera.Eye.x + half_width) * width_rcp);
			center_chunk.z = (int)std::floor((camera.Eye.z + half_width) * width_rcp);
		}

		// Chunk removal checks:
		if (removalCheckBox.GetCheck())
		{
			const int removal_threshold = generation + 2;
			for (auto& it : chunks)
			{
				const Chunk& chunk = it.first;
				if (std::abs(center_chunk.x - chunk.x) > removal_threshold || std::abs(center_chunk.z - chunk.z) > removal_threshold)
				{
					scene.Entity_Remove(it.second.entity);
					it.second = {};
				}
			}
		}

		bool should_exit = false;
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

				chunk_data.entity = scene.Entity_CreateObject("chunk_" + std::to_string(chunk.x) + "_" + std::to_string(chunk.z));
				ObjectComponent& object = *scene.objects.GetComponent(chunk_data.entity);
				object.lod_distance_multiplier = lodMultiplier;
				scene.Component_Attach(chunk_data.entity, terrainEntity);

				TransformComponent& transform = *scene.transforms.GetComponent(chunk_data.entity);
				transform.ClearTransform();
				const XMFLOAT3 chunk_pos = XMFLOAT3(float(chunk.x * (width - 1)), 0, float(chunk.z * (width - 1)));
				transform.Translate(chunk_pos);
				transform.UpdateTransform();

				MeshComponent& mesh = scene.meshes.Create(chunk_data.entity);
				object.meshID = chunk_data.entity;
				mesh.indices = indices;
				mesh.SetTerrain(true);
				mesh.terrain_material1 = materialEntity_Slope;
				mesh.terrain_material2 = materialEntity_LowAltitude;
				mesh.terrain_material3 = materialEntity_HighAltitude;
				for (auto& lod : lods)
				{
					mesh.subsets.emplace_back();
					mesh.subsets.back().materialID = materialEntity_Base;
					mesh.subsets.back().indexCount = lod.indexCount;
					mesh.subsets.back().indexOffset = lod.indexOffset;
				}
				mesh.subsets_per_lod = 1;
				mesh.vertex_positions.resize(vertexCount);
				mesh.vertex_normals.resize(vertexCount);
				mesh.vertex_colors.resize(vertexCount);
				mesh.vertex_uvset_0.resize(vertexCount);
				mesh.vertex_uvset_1.resize(vertexCount);
				mesh.vertex_atlas.resize(vertexCount);

				wi::HairParticleSystem grass;
				grass.vertex_lengths.resize(vertexCount);
				std::atomic<uint32_t> grass_valid_vertex_count{ 0 };

				wi::jobsystem::context ctx;
				wi::jobsystem::Dispatch(ctx, vertexCount, width, [&](wi::jobsystem::JobArgs args) {
					uint32_t index = args.jobIndex;
					const float x = float(index % width) - half_width;
					const float z = float(index / width) - half_width;
					XMVECTOR corners[3];
					XMFLOAT2 corner_offsets[3] = {
						XMFLOAT2(0, 0),
						XMFLOAT2(1, 0),
						XMFLOAT2(0, 1),
					};
					for (int i = 0; i < arraysize(corners); ++i)
					{
						float height = 0;
						const XMFLOAT2 world_pos = XMFLOAT2(chunk_pos.x + x + corner_offsets[i].x, chunk_pos.z + z + corner_offsets[i].y);
						if (perlinBlend > 0)
						{
							XMFLOAT2 p = world_pos;
							p.x *= perlinFrequency;
							p.y *= perlinFrequency;
							height += (perlin.compute(p.x, p.y, 0, perlinOctaves) * 0.5f + 0.5f) * perlinBlend;
						}
						if (voronoiBlend > 0)
						{
							XMFLOAT2 p = world_pos;
							p.x *= voronoiFrequency;
							p.y *= voronoiFrequency;
							if (voronoiPerturbation > 0)
							{
								const float angle = perlin.compute(p.x, p.y, 0, 6) * XM_2PI;
								p.x += std::sin(angle) * voronoiPerturbation;
								p.y += std::cos(angle) * voronoiPerturbation;
							}
							wi::noise::voronoi::Result res = wi::noise::voronoi::compute(p.x, p.y, (float)seed);
							float weight = std::pow(1 - wi::math::saturate((res.distance - voronoiShape) * voronoiFade), std::max(0.0001f, voronoiFalloff));
							height *= weight * voronoiBlend;
						}
						if (!heightmap.data.empty())
						{
							XMFLOAT2 pixel = XMFLOAT2(world_pos.x + heightmap.width * 0.5f, world_pos.y + heightmap.height * 0.5f);
							if (pixel.x >= 0 && pixel.x < heightmap.width && pixel.y >= 0 && pixel.y < heightmap.height)
							{
								const int idx = int(pixel.x) + int(pixel.y) * heightmap.width;
								height = ((float)heightmap.data[idx] / 255.0f) * heightmapBlend;
							}
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

					const float region_base = 1;
					const float region_slope = 1 - std::pow(wi::math::saturate(normal.y), 2.0f);
					const float region_low_altitude = std::pow(wi::math::saturate(wi::math::InverseLerp(0, bottomLevel, height)), 2.0f);
					const float region_high_altitude = std::pow(wi::math::saturate(wi::math::InverseLerp(0, topLevel * 0.25f, height)), 4.0f);

					XMFLOAT4 materialBlendWeights(1, 0, 0, 0);
					materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 1, 0, 0), region_slope);
					materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 0, 1, 0), region_low_altitude);
					materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 0, 0, 1), region_high_altitude);
					const float weight_norm = 1.0f / (materialBlendWeights.x + materialBlendWeights.y + materialBlendWeights.z + materialBlendWeights.w);
					materialBlendWeights.x *= weight_norm;
					materialBlendWeights.y *= weight_norm;
					materialBlendWeights.z *= weight_norm;
					materialBlendWeights.w *= weight_norm;

					mesh.vertex_positions[index] = XMFLOAT3(x, height, z);
					mesh.vertex_normals[index] = normal;
					mesh.vertex_colors[index] = wi::Color::fromFloat4(materialBlendWeights);
					const XMFLOAT2 uv = XMFLOAT2(x * width_rcp, z * width_rcp);
					mesh.vertex_uvset_0[index] = uv;
					mesh.vertex_uvset_1[index] = uv;
					mesh.vertex_atlas[index] = uv;

					XMFLOAT3 vertex_pos(chunk_pos.x + x, height, chunk_pos.z + z);

					const float grass_noise_frequency = 0.1f;
					const float grass_noise = perlin.compute(vertex_pos.x * grass_noise_frequency, vertex_pos.y * grass_noise_frequency, vertex_pos.z * grass_noise_frequency) * 0.5f + 0.5f;
					const float region_grass = std::pow(materialBlendWeights.x * (1 - materialBlendWeights.w), 4.0f) * grass_noise;
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
				wi::jobsystem::Wait(ctx);

				mesh.CreateRenderData();

				if (grass_valid_vertex_count.load() > 0)
				{
					grass.meshID = chunk_data.entity;
					grass.length = 5;
					grass.strandCount = grass_valid_vertex_count.load() * 3;
					grass.viewDistance = 80;
					grass.frameCount = 2;
					grass.framesX = 1;
					grass.framesY = 2;
					grass.frameStart = 0;
					scene.materials.Create(chunk_data.entity) = material_GrassParticle;
					chunk_data.grass = std::move(grass);
				}

				for (auto& prop : props)
				{
					std::uniform_int_distribution<uint32_t> gen_distr(prop.min_count_per_chunk, prop.max_count_per_chunk);
					int gen_count = gen_distr(prop_rand);
					for (int i = 0; i < gen_count; ++i)
					{
						std::uniform_real_distribution<float> float_distr(0.0f, 1.0f);
						std::uniform_int_distribution<uint32_t> ind_distr(0, lods[0].indexCount / 3 - 1);
						uint32_t tri = ind_distr(prop_rand);
						uint32_t ind0 = mesh.indices[tri * 3 + 0];
						uint32_t ind1 = mesh.indices[tri * 3 + 1];
						uint32_t ind2 = mesh.indices[tri * 3 + 2];
						const XMFLOAT3& pos0 = mesh.vertex_positions[ind0];
						const XMFLOAT3& pos1 = mesh.vertex_positions[ind1];
						const XMFLOAT3& pos2 = mesh.vertex_positions[ind2];
						const uint32_t& col0 = mesh.vertex_colors[ind0];
						const uint32_t& col1 = mesh.vertex_colors[ind1];
						const uint32_t& col2 = mesh.vertex_colors[ind2];
						const XMFLOAT4 region0 = wi::Color(col0).toFloat4();
						const XMFLOAT4 region1 = wi::Color(col1).toFloat4();
						const XMFLOAT4 region2 = wi::Color(col2).toFloat4();
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
						vertex_pos.x += chunk_pos.x;
						vertex_pos.z += chunk_pos.z;
						XMFLOAT4 region;
						region.x = region0.x + f * (region1.x - region0.x) + g * (region2.x - region0.x);
						region.y = region0.y + f * (region1.y - region0.y) + g * (region2.y - region0.y);
						region.z = region0.z + f * (region1.z - region0.z) + g * (region2.z - region0.z);
						region.w = region0.w + f * (region1.w - region0.w) + g * (region2.w - region0.w);

						const float noise = perlin.compute(vertex_pos.x * prop.noise_frequency, vertex_pos.y * prop.noise_frequency, vertex_pos.z * prop.noise_frequency) * 0.5f + 0.5f;
						const float chance = std::pow(((float*)&region)[prop.region], prop.region_power) * noise;
						if (chance > prop.threshold)
						{
							Entity entity = scene.Entity_CreateObject(prop.name + std::to_string(i));
							ObjectComponent* object = scene.objects.GetComponent(entity);
							*object = prop.object;
							TransformComponent* transform = scene.transforms.GetComponent(entity);
							XMFLOAT3 offset = vertex_pos;
							offset.y += wi::math::Lerp(prop.min_y_offset, prop.max_y_offset, float_distr(prop_rand));
							transform->Translate(offset);
							const float scaling = wi::math::Lerp(prop.min_size, prop.max_size, float_distr(prop_rand));
							transform->Scale(XMFLOAT3(scaling, scaling, scaling));
							transform->RotateRollPitchYaw(XMFLOAT3(0, XM_2PI * float_distr(prop_rand), 0));
							transform->UpdateTransform();
							scene.Component_Attach(entity, chunk_data.entity);
						}
					}
				}

				if (timer.elapsed_milliseconds() > allocated_timeframe_milliseconds) // approximately this much time is allowed for generation
					should_exit = true;
			}

			// Grass patch placement:
			it = chunks.find(chunk);
			if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
			{
				ChunkData& chunk_data = it->second;
				if (chunk_data.grass.meshID != INVALID_ENTITY)
				{
					const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));
					if (dist <= 1)
					{
						if (!scene.hairs.Contains(chunk_data.entity))
						{
							// add patch for this chunk
							wi::HairParticleSystem& grass = scene.hairs.Create(chunk_data.entity);
							grass = chunk_data.grass;
							const MeshComponent* mesh = scene.meshes.GetComponent(chunk_data.entity);
							if (mesh != nullptr)
							{
								grass.CreateRenderData(*mesh);
							}
						}
					}
					else
					{
						wi::HairParticleSystem* grass = scene.hairs.GetComponent(chunk_data.entity);
						if (grass != nullptr)
						{
							// remove this chunk's grass patch from the scene
							scene.hairs.Remove(chunk_data.entity);
						}
					}
				}
			}

		};

		// generate center chunk first:
		request_chunk(0, 0);
		if (should_exit) return;

		// then generate neighbor chunks in outward spiral:
		for (int growth = 0; growth < generation; ++growth)
		{
			const int side = 2 * (growth + 1);
			int x = -growth - 1;
			int z = -growth - 1;
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (should_exit) return;
				x++;
			}
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (should_exit) return;
				z++;
			}
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (should_exit) return;
				x--;
			}
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (should_exit) return;
				z--;
			}
		}
	}

} terragen;

void MeshWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Mesh Window");
	SetSize(XMFLOAT2(580, 600));

	float x = 150;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	meshInfoLabel.Create("Mesh Info");
	meshInfoLabel.SetPos(XMFLOAT2(x - 50, y += step));
	meshInfoLabel.SetSize(XMFLOAT2(450, 190));
	meshInfoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&meshInfoLabel);

	// Left side:
	y = meshInfoLabel.GetScale().y + 5;

	subsetComboBox.Create("Selected subset: ");
	subsetComboBox.SetSize(XMFLOAT2(40, hei));
	subsetComboBox.SetPos(XMFLOAT2(x, y += step));
	subsetComboBox.SetEnabled(false);
	subsetComboBox.OnSelect([=](wi::gui::EventArgs args) {
		Scene& scene = wi::scene::GetScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			subset = args.iValue;
			if (!editor->translator.selected.empty())
			{
				editor->translator.selected.back().subsetIndex = subset;
			}
		}
	});
	subsetComboBox.SetTooltip("Select a subset. A subset can also be selected by picking it in the 3D scene.");
	AddWidget(&subsetComboBox);

	terrainCheckBox.Create("Terrain: ");
	terrainCheckBox.SetTooltip("If enabled, the mesh will use multiple materials and blend between them based on vertex colors.");
	terrainCheckBox.SetSize(XMFLOAT2(hei, hei));
	terrainCheckBox.SetPos(XMFLOAT2(x, y += step));
	terrainCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->SetTerrain(args.bValue);
			if (args.bValue && mesh->vertex_colors.empty())
			{
				mesh->vertex_colors.resize(mesh->vertex_positions.size());
				std::fill(mesh->vertex_colors.begin(), mesh->vertex_colors.end(), wi::Color::Red().rgba); // fill red (meaning only blend base material)
				mesh->CreateRenderData();

				for (auto& subset : mesh->subsets)
				{
					MaterialComponent* material = wi::scene::GetScene().materials.GetComponent(subset.materialID);
					if (material != nullptr)
					{
						material->SetUseVertexColors(true);
					}
				}
			}
			SetEntity(entity, subset); // refresh information label
		}
		});
	AddWidget(&terrainCheckBox);

	doubleSidedCheckBox.Create("Double Sided: ");
	doubleSidedCheckBox.SetTooltip("If enabled, the inside of the mesh will be visible.");
	doubleSidedCheckBox.SetSize(XMFLOAT2(hei, hei));
	doubleSidedCheckBox.SetPos(XMFLOAT2(x, y += step));
	doubleSidedCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->SetDoubleSided(args.bValue);
		}
	});
	AddWidget(&doubleSidedCheckBox);

	softbodyCheckBox.Create("Soft body: ");
	softbodyCheckBox.SetTooltip("Enable soft body simulation. Tip: Use the Paint Tool to control vertex pinning.");
	softbodyCheckBox.SetSize(XMFLOAT2(hei, hei));
	softbodyCheckBox.SetPos(XMFLOAT2(x, y += step));
	softbodyCheckBox.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = wi::scene::GetScene();
		SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(entity);

		if (args.bValue)
		{
			if (physicscomponent == nullptr)
			{
				SoftBodyPhysicsComponent& softbody = scene.softbodies.Create(entity);
				softbody.friction = frictionSlider.GetValue();
				softbody.restitution = restitutionSlider.GetValue();
				softbody.mass = massSlider.GetValue();
			}
		}
		else
		{
			if (physicscomponent != nullptr)
			{
				scene.softbodies.Remove(entity);
			}
		}

	});
	AddWidget(&softbodyCheckBox);

	massSlider.Create(0, 10, 1, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.SetSize(XMFLOAT2(100, hei));
	massSlider.SetPos(XMFLOAT2(x, y += step));
	massSlider.OnSlide([&](wi::gui::EventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->mass = args.fValue;
		}
	});
	AddWidget(&massSlider);

	frictionSlider.Create(0, 1, 0.5f, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.SetSize(XMFLOAT2(100, hei));
	frictionSlider.SetPos(XMFLOAT2(x, y += step));
	frictionSlider.OnSlide([&](wi::gui::EventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->friction = args.fValue;
		}
	});
	AddWidget(&frictionSlider);

	restitutionSlider.Create(0, 1, 0, 100000, "Restitution: ");
	restitutionSlider.SetTooltip("Set the restitution amount for the physics engine.");
	restitutionSlider.SetSize(XMFLOAT2(100, hei));
	restitutionSlider.SetPos(XMFLOAT2(x, y += step));
	restitutionSlider.OnSlide([&](wi::gui::EventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->restitution = args.fValue;
		}
		});
	AddWidget(&restitutionSlider);

	impostorCreateButton.Create("Create Impostor");
	impostorCreateButton.SetTooltip("Create an impostor image of the mesh. The mesh will be replaced by this image when far away, to render faster.");
	impostorCreateButton.SetSize(XMFLOAT2(200, hei));
	impostorCreateButton.SetPos(XMFLOAT2(x - 50, y += step));
	impostorCreateButton.OnClick([&](wi::gui::EventArgs args) {
	    Scene& scene = wi::scene::GetScene();
		ImpostorComponent* impostor = scene.impostors.GetComponent(entity);
	    if (impostor == nullptr)
		{
		    impostorCreateButton.SetText("Delete Impostor");
			scene.impostors.Create(entity).swapInDistance = impostorDistanceSlider.GetValue();
		}
	    else
		{
			impostorCreateButton.SetText("Create Impostor");
			scene.impostors.Remove(entity);
		}
	});
	AddWidget(&impostorCreateButton);

	impostorDistanceSlider.Create(0, 1000, 100, 10000, "Impostor Distance: ");
	impostorDistanceSlider.SetTooltip("Assign the distance where the mesh geometry should be switched to the impostor image.");
	impostorDistanceSlider.SetSize(XMFLOAT2(100, hei));
	impostorDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	impostorDistanceSlider.OnSlide([&](wi::gui::EventArgs args) {
		ImpostorComponent* impostor = wi::scene::GetScene().impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostor->swapInDistance = args.fValue;
		}
	});
	AddWidget(&impostorDistanceSlider);

	tessellationFactorSlider.Create(0, 100, 0, 10000, "Tessellation Factor: ");
	tessellationFactorSlider.SetTooltip("Set the dynamic tessellation amount. Tessellation should be enabled in the Renderer window and your GPU must support it!");
	tessellationFactorSlider.SetSize(XMFLOAT2(100, hei));
	tessellationFactorSlider.SetPos(XMFLOAT2(x, y += step));
	tessellationFactorSlider.OnSlide([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->tessellationFactor = args.fValue;
		}
	});
	AddWidget(&tessellationFactorSlider);

	flipCullingButton.Create("Flip Culling");
	flipCullingButton.SetTooltip("Flip faces to reverse triangle culling order.");
	flipCullingButton.SetSize(XMFLOAT2(200, hei));
	flipCullingButton.SetPos(XMFLOAT2(x - 50, y += step));
	flipCullingButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipCulling();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&flipCullingButton);

	flipNormalsButton.Create("Flip Normals");
	flipNormalsButton.SetTooltip("Flip surface normals.");
	flipNormalsButton.SetSize(XMFLOAT2(200, hei));
	flipNormalsButton.SetPos(XMFLOAT2(x - 50, y += step));
	flipNormalsButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipNormals();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&flipNormalsButton);

	computeNormalsSmoothButton.Create("Compute Normals [SMOOTH]");
	computeNormalsSmoothButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per vertex. This can reduce vertex count, but is slow.");
	computeNormalsSmoothButton.SetSize(XMFLOAT2(200, hei));
	computeNormalsSmoothButton.SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsSmoothButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH);
			SetEntity(entity, subset);
		}
	});
	AddWidget(&computeNormalsSmoothButton);

	computeNormalsHardButton.Create("Compute Normals [HARD]");
	computeNormalsHardButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per face. This can increase vertex count.");
	computeNormalsHardButton.SetSize(XMFLOAT2(200, hei));
	computeNormalsHardButton.SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsHardButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_HARD);
			SetEntity(entity, subset);
		}
	});
	AddWidget(&computeNormalsHardButton);

	recenterButton.Create("Recenter");
	recenterButton.SetTooltip("Recenter mesh to AABB center.");
	recenterButton.SetSize(XMFLOAT2(200, hei));
	recenterButton.SetPos(XMFLOAT2(x - 50, y += step));
	recenterButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->Recenter();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&recenterButton);

	recenterToBottomButton.Create("RecenterToBottom");
	recenterToBottomButton.SetTooltip("Recenter mesh to AABB bottom.");
	recenterToBottomButton.SetSize(XMFLOAT2(200, hei));
	recenterToBottomButton.SetPos(XMFLOAT2(x - 50, y += step));
	recenterToBottomButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->RecenterToBottom();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&recenterToBottomButton);

	mergeButton.Create("Merge Selected");
	mergeButton.SetTooltip("Merges selected objects/meshes into one.");
	mergeButton.SetSize(XMFLOAT2(200, hei));
	mergeButton.SetPos(XMFLOAT2(x - 50, y += step));
	mergeButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = wi::scene::GetScene();
		MeshComponent merged_mesh;
		bool valid_normals = false;
		bool valid_uvset_0 = false;
		bool valid_uvset_1 = false;
		bool valid_atlas = false;
		bool valid_boneindices = false;
		bool valid_boneweights = false;
		bool valid_colors = false;
		bool valid_windweights = false;
		wi::unordered_set<Entity> entities_to_remove;
		Entity prev_subset_material = INVALID_ENTITY;
		for (auto& picked : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(picked.entity);
			if (object == nullptr)
				continue;
			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				continue;
			const TransformComponent* transform = scene.transforms.GetComponent(picked.entity);
			XMMATRIX W = XMLoadFloat4x4(&transform->world);
			uint32_t vertexOffset = (uint32_t)merged_mesh.vertex_positions.size();
			uint32_t indexOffset = (uint32_t)merged_mesh.indices.size();
			for (auto& ind : mesh->indices)
			{
				merged_mesh.indices.push_back(vertexOffset + ind);
			}
			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			mesh->GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
				if (subset.materialID != prev_subset_material)
				{
					// new subset
					prev_subset_material = subset.materialID;
					merged_mesh.subsets.push_back(subset);
					merged_mesh.subsets.back().indexOffset += indexOffset;
				}
				else
				{
					// append to previous subset
					merged_mesh.subsets.back().indexCount += subset.indexCount;
				}
			}
			for (size_t i = 0; i < mesh->vertex_positions.size(); ++i)
			{
				merged_mesh.vertex_positions.push_back(mesh->vertex_positions[i]);
				XMStoreFloat3(&merged_mesh.vertex_positions.back(), XMVector3Transform(XMLoadFloat3(&merged_mesh.vertex_positions.back()), W));

				if (mesh->vertex_normals.empty())
				{
					merged_mesh.vertex_normals.emplace_back();
				}
				else
				{
					valid_normals = true;
					merged_mesh.vertex_normals.push_back(mesh->vertex_normals[i]);
					XMStoreFloat3(&merged_mesh.vertex_normals.back(), XMVector3TransformNormal(XMLoadFloat3(&merged_mesh.vertex_normals.back()), W));
				}

				if (mesh->vertex_uvset_0.empty())
				{
					merged_mesh.vertex_uvset_0.emplace_back();
				}
				else
				{
					valid_uvset_0 = true;
					merged_mesh.vertex_uvset_0.push_back(mesh->vertex_uvset_0[i]);
				}

				if (mesh->vertex_uvset_1.empty())
				{
					merged_mesh.vertex_uvset_1.emplace_back();
				}
				else
				{
					valid_uvset_1 = true;
					merged_mesh.vertex_uvset_1.push_back(mesh->vertex_uvset_1[i]);
				}

				if (mesh->vertex_atlas.empty())
				{
					merged_mesh.vertex_atlas.emplace_back();
				}
				else
				{
					valid_atlas = true;
					merged_mesh.vertex_atlas.push_back(mesh->vertex_atlas[i]);
				}

				if (mesh->vertex_boneindices.empty())
				{
					merged_mesh.vertex_boneindices.emplace_back();
				}
				else
				{
					valid_boneindices = true;
					merged_mesh.vertex_boneindices.push_back(mesh->vertex_boneindices[i]);
				}

				if (mesh->vertex_boneweights.empty())
				{
					merged_mesh.vertex_boneweights.emplace_back();
				}
				else
				{
					valid_boneweights = true;
					merged_mesh.vertex_boneweights.push_back(mesh->vertex_boneweights[i]);
				}

				if (mesh->vertex_colors.empty())
				{
					merged_mesh.vertex_colors.push_back(~0u);
				}
				else
				{
					valid_colors = true;
					merged_mesh.vertex_colors.push_back(mesh->vertex_colors[i]);
				}

				if (mesh->vertex_windweights.empty())
				{
					merged_mesh.vertex_windweights.emplace_back();
				}
				else
				{
					valid_windweights = true;
					merged_mesh.vertex_windweights.push_back(mesh->vertex_windweights[i]);
				}
			}
			if (merged_mesh.armatureID == INVALID_ENTITY)
			{
				merged_mesh.armatureID = mesh->armatureID;
			}
			entities_to_remove.insert(object->meshID);
			entities_to_remove.insert(picked.entity);
		}

		if (!merged_mesh.vertex_positions.empty())
		{
			if (!valid_normals)
				merged_mesh.vertex_normals.clear();
			if (!valid_uvset_0)
				merged_mesh.vertex_uvset_0.clear();
			if (!valid_uvset_1)
				merged_mesh.vertex_uvset_1.clear();
			if (!valid_atlas)
				merged_mesh.vertex_atlas.clear();
			if (!valid_boneindices)
				merged_mesh.vertex_boneindices.clear();
			if (!valid_boneweights)
				merged_mesh.vertex_boneweights.clear();
			if (!valid_colors)
				merged_mesh.vertex_colors.clear();
			if (!valid_windweights)
				merged_mesh.vertex_windweights.clear();

			Entity merged_object_entity = scene.Entity_CreateObject("mergedObject");
			Entity merged_mesh_entity = scene.Entity_CreateMesh("mergedMesh");
			ObjectComponent* object = scene.objects.GetComponent(merged_object_entity);
			object->meshID = merged_mesh_entity;
			MeshComponent* mesh = scene.meshes.GetComponent(merged_mesh_entity);
			*mesh = std::move(merged_mesh);
			mesh->CreateRenderData();
		}

		for (auto& x : entities_to_remove)
		{
			scene.Entity_Remove(x);
		}
		
	});
	AddWidget(&mergeButton);

	optimizeButton.Create("Optimize");
	optimizeButton.SetTooltip("Run the meshoptimizer library.");
	optimizeButton.SetSize(XMFLOAT2(200, hei));
	optimizeButton.SetPos(XMFLOAT2(x - 50, y += step));
	optimizeButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			// https://github.com/zeux/meshoptimizer#vertex-cache-optimization

			size_t index_count = mesh->indices.size();
			size_t vertex_count = mesh->vertex_positions.size();

			wi::vector<uint32_t> indices(index_count);
			meshopt_optimizeVertexCache(indices.data(), mesh->indices.data(), index_count, vertex_count);

			mesh->indices = indices;

			mesh->CreateRenderData();
			SetEntity(entity, subset);
		}
		});
	AddWidget(&optimizeButton);


	// Right side:

	x = 150;
	y = meshInfoLabel.GetScale().y + 5;

	subsetMaterialComboBox.Create("Subset Material: ");
	subsetMaterialComboBox.SetSize(XMFLOAT2(200, hei));
	subsetMaterialComboBox.SetPos(XMFLOAT2(x + 180, y += step));
	subsetMaterialComboBox.SetEnabled(false);
	subsetMaterialComboBox.OnSelect([&](wi::gui::EventArgs args) {
		Scene& scene = wi::scene::GetScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr && subset >= 0 && subset < mesh->subsets.size())
		{
			MeshComponent::MeshSubset& meshsubset = mesh->subsets[subset];
			if (args.iValue == 0)
			{
				meshsubset.materialID = INVALID_ENTITY;
			}
			else
			{
				MeshComponent::MeshSubset& meshsubset = mesh->subsets[subset];
				meshsubset.materialID = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	subsetMaterialComboBox.SetTooltip("Set the base material of the selected MeshSubset");
	AddWidget(&subsetMaterialComboBox);

	terrainMat1Combo.Create("Terrain Material 1: ");
	terrainMat1Combo.SetSize(XMFLOAT2(200, hei));
	terrainMat1Combo.SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat1Combo.SetEnabled(false);
	terrainMat1Combo.OnSelect([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material1 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wi::scene::GetScene();
				mesh->terrain_material1 = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	terrainMat1Combo.SetTooltip("Choose a sub terrain blend material. (GREEN vertex color mask)");
	AddWidget(&terrainMat1Combo);

	terrainMat2Combo.Create("Terrain Material 2: ");
	terrainMat2Combo.SetSize(XMFLOAT2(200, hei));
	terrainMat2Combo.SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat2Combo.SetEnabled(false);
	terrainMat2Combo.OnSelect([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material2 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wi::scene::GetScene();
				mesh->terrain_material2 = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	terrainMat2Combo.SetTooltip("Choose a sub terrain blend material. (BLUE vertex color mask)");
	AddWidget(&terrainMat2Combo);

	terrainMat3Combo.Create("Terrain Material 3: ");
	terrainMat3Combo.SetSize(XMFLOAT2(200, hei));
	terrainMat3Combo.SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat3Combo.SetEnabled(false);
	terrainMat3Combo.OnSelect([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material3 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wi::scene::GetScene();
				mesh->terrain_material3 = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	terrainMat3Combo.SetTooltip("Choose a sub terrain blend material. (ALPHA vertex color mask)");
	AddWidget(&terrainMat3Combo);

	terrainGenButton.Create("Generate Terrain...");
	terrainGenButton.SetTooltip("Generate terrain meshes.");
	terrainGenButton.SetSize(XMFLOAT2(200, hei));
	terrainGenButton.SetPos(XMFLOAT2(x + 180, y += step));
	terrainGenButton.OnClick([=](wi::gui::EventArgs args) {

		if (!terragen_initialized)
		{
			terragen_initialized = true;

			// Customize terrain generator:
			terragen.material_Base.SetUseVertexColors(true);
			terragen.material_Base.SetRoughness(1);
			terragen.material_Slope.SetRoughness(0.5f);
			terragen.material_LowAltitude.SetRoughness(1);
			terragen.material_HighAltitude.SetRoughness(1);
			terragen.material_Base.textures[MaterialComponent::BASECOLORMAP].name = "terrain/base.jpg";
			terragen.material_Base.textures[MaterialComponent::NORMALMAP].name = "terrain/base_nor.jpg";
			terragen.material_Slope.textures[MaterialComponent::BASECOLORMAP].name = "terrain/slope.jpg";
			terragen.material_Slope.textures[MaterialComponent::NORMALMAP].name = "terrain/slope_nor.jpg";
			terragen.material_LowAltitude.textures[MaterialComponent::BASECOLORMAP].name = "terrain/low_altitude.jpg";
			terragen.material_HighAltitude.textures[MaterialComponent::BASECOLORMAP].name = "terrain/high_altitude.jpg";
			terragen.material_GrassParticle.textures[MaterialComponent::BASECOLORMAP].name = "terrain/grassparticle.png";
			terragen.material_GrassParticle.alphaRef = 0.75f;
			terragen.material_Base.CreateRenderData();
			terragen.material_Slope.CreateRenderData();
			terragen.material_LowAltitude.CreateRenderData();
			terragen.material_HighAltitude.CreateRenderData();
			terragen.material_GrassParticle.CreateRenderData();
			// Tree prop:
			{
				Scene props_scene;
				wi::scene::LoadModel(props_scene, "terrain/tree.wiscene");
				TerraGen::Prop& prop = terragen.props.emplace_back();
				prop.name = "tree";
				prop.min_count_per_chunk = 0;
				prop.max_count_per_chunk = 10;
				prop.region = 0;
				prop.region_power = 4;
				prop.noise_frequency = 0.01f;
				prop.threshold = 0.3f;
				prop.min_size = 2.0f;
				prop.max_size = 8.0f;
				prop.min_y_offset = -0.5f;
				prop.max_y_offset = -0.5f;
				prop.mesh_entity = props_scene.Entity_FindByName("tree_mesh");
				Entity object_entity = props_scene.Entity_FindByName("tree_object");
				ObjectComponent* object = props_scene.objects.GetComponent(object_entity);
				if (object != nullptr)
				{
					prop.object = *object;
					prop.object.lod_distance_multiplier = 0.05f;
					//prop.object.cascadeMask = 1; // they won't be rendered into the largest shadow cascade
				}
				props_scene.Entity_Remove(object_entity); // The objects will be placed by terrain generator, we don't need the default object that the scene has anymore
				wi::scene::GetScene().Merge(props_scene);
			}
			// Rock prop:
			{
				Scene props_scene;
				wi::scene::LoadModel(props_scene, "terrain/rock.wiscene");
				TerraGen::Prop& prop = terragen.props.emplace_back();
				prop.name = "rock";
				prop.min_count_per_chunk = 0;
				prop.max_count_per_chunk = 8;
				prop.region = 0;
				prop.region_power = 1;
				prop.noise_frequency = 10;
				prop.threshold = 0.7f;
				prop.min_size = 0.02f;
				prop.max_size = 3.0f;
				prop.min_y_offset = -2;
				prop.max_y_offset = 0.5f;
				prop.mesh_entity = props_scene.Entity_FindByName("rock_mesh");
				Entity object_entity = props_scene.Entity_FindByName("rock_object");
				ObjectComponent* object = props_scene.objects.GetComponent(object_entity);
				if (object != nullptr)
				{
					prop.object = *object;
					prop.object.lod_distance_multiplier = 0.02f;
					prop.object.cascadeMask = 1; // they won't be rendered into the largest shadow cascade
				}
				props_scene.Entity_Remove(object_entity); // The objects will be placed by terrain generator, we don't need the default object that the scene has anymore
				wi::scene::GetScene().Merge(props_scene);
			}
			terragen.init();
			editor->GetGUI().AddWidget(&terragen);
		}

		terragen.SetVisible(true);
		terragen.SetPos(XMFLOAT2(
			terrainGenButton.translation.x + terrainGenButton.scale.x + 10,
			terrainGenButton.translation.y)
		);

		terragen.Generation_Restart();


		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_ADD;
		editor->RecordSelection(archive);

		editor->RecordSelection(archive);
		editor->RecordAddedEntity(archive, terragen.terrainEntity);

		editor->RefreshSceneGraphView();


	});
	AddWidget(&terrainGenButton);


	morphTargetCombo.Create("Morph Target:");
	morphTargetCombo.SetSize(XMFLOAT2(100, hei));
	morphTargetCombo.SetPos(XMFLOAT2(x + 280, y += step));
	morphTargetCombo.OnSelect([&](wi::gui::EventArgs args) {
	    MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
	    if (mesh != nullptr && args.iValue < (int)mesh->targets.size())
	    {
			morphTargetSlider.SetValue(mesh->targets[args.iValue].weight);
	    }
	});
	morphTargetCombo.SetTooltip("Choose a morph target to edit weight.");
	AddWidget(&morphTargetCombo);

	morphTargetSlider.Create(0, 1, 0, 100000, "Weight: ");
	morphTargetSlider.SetTooltip("Set the weight for morph target");
	morphTargetSlider.SetSize(XMFLOAT2(100, hei));
	morphTargetSlider.SetPos(XMFLOAT2(x + 280, y += step));
	morphTargetSlider.OnSlide([&](wi::gui::EventArgs args) {
	    MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
	    if (mesh != nullptr && morphTargetCombo.GetSelected() < (int)mesh->targets.size())
	    {
			mesh->targets[morphTargetCombo.GetSelected()].weight = args.fValue;
			mesh->dirty_morph = true;
	    }
	});
	AddWidget(&morphTargetSlider);

	lodgenButton.Create("LOD Gen");
	lodgenButton.SetTooltip("Generate LODs (levels of detail).");
	lodgenButton.SetSize(XMFLOAT2(200, hei));
	lodgenButton.SetPos(XMFLOAT2(x + 180, y += step));
	lodgenButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (mesh->subsets_per_lod == 0)
			{
				// if there were no lods before, record the subset count without lods:
				mesh->subsets_per_lod = (uint32_t)mesh->subsets.size();
			}

			// https://github.com/zeux/meshoptimizer/blob/bedaaaf6e710d3b42d49260ca738c15d171b1a8f/demo/main.cpp
			size_t index_count = mesh->indices.size();
			size_t vertex_count = mesh->vertex_positions.size();

			const size_t lod_count = (size_t)lodCountSlider.GetValue();
			struct LOD
			{
				struct Subset
				{
					wi::vector<uint32_t> indices;
				};
				wi::vector<Subset> subsets;
			};
			wi::vector<LOD> lods(lod_count);

			const float target_error = lodErrorSlider.GetValue();

			for (size_t i = 0; i < lod_count; ++i)
			{
				lods[i].subsets.resize(mesh->subsets_per_lod);
				for (uint32_t subsetIndex = 0; subsetIndex < mesh->subsets_per_lod; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
					lods[i].subsets[subsetIndex].indices.resize(subset.indexCount);
					for (uint32_t ind = 0; ind < subset.indexCount; ++ind)
					{
						lods[i].subsets[subsetIndex].indices[ind] = mesh->indices[subset.indexOffset + ind];
					}
				}
			}

			for (uint32_t subsetIndex = 0; subsetIndex < mesh->subsets_per_lod; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];

				float threshold = wi::math::Lerp(0, 0.9f, wi::math::saturate(lodQualitySlider.GetValue()));
				for (size_t i = 1; i < lod_count; ++i)
				{
					wi::vector<unsigned int>& lod = lods[i].subsets[subsetIndex].indices;

					size_t target_index_count = size_t(mesh->indices.size() * threshold) / 3 * 3;

					// we can simplify all the way from base level or from the last result
					// simplifying from the base level sometimes produces better results, but simplifying from last level is faster
					//const wi::vector<unsigned int>& source = lods[0].subsets[subsetIndex].indices;
					const wi::vector<unsigned int>& source = lods[i - 1].subsets[subsetIndex].indices;

					if (source.size() < target_index_count)
						target_index_count = source.size();

					lod.resize(source.size());
					if (lodSloppyCheckBox.GetCheck())
					{
						lod.resize(meshopt_simplifySloppy(&lod[0], &source[0], source.size(), &mesh->vertex_positions[0].x, mesh->vertex_positions.size(), sizeof(XMFLOAT3), target_index_count, target_error));
					}
					else
					{
						lod.resize(meshopt_simplify(&lod[0], &source[0], source.size(), &mesh->vertex_positions[0].x, mesh->vertex_positions.size(), sizeof(XMFLOAT3), target_index_count, target_error));
					}

					threshold *= threshold;
				}

				// optimize each individual LOD for vertex cache & overdraw
				for (size_t i = 0; i < lod_count; ++i)
				{
					wi::vector<unsigned int>& lod = lods[i].subsets[subsetIndex].indices;

					meshopt_optimizeVertexCache(&lod[0], &lod[0], lod.size(), mesh->vertex_positions.size());
					meshopt_optimizeOverdraw(&lod[0], &lod[0], lod.size(), &mesh->vertex_positions[0].x, mesh->vertex_positions.size(), sizeof(XMFLOAT3), 1.0f);
				}
			}

			mesh->indices.clear();
			wi::vector<MeshComponent::MeshSubset> subsets;
			for (size_t i = 0; i < lod_count; ++i)
			{
				for (uint32_t subsetIndex = 0; subsetIndex < mesh->subsets_per_lod; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
					subsets.emplace_back();
					subsets.back() = subset;
					subsets.back().indexOffset = (uint32_t)mesh->indices.size();
					subsets.back().indexCount = (uint32_t)lods[i].subsets[subsetIndex].indices.size();
					for (auto& x : lods[i].subsets[subsetIndex].indices)
					{
						mesh->indices.push_back(x);
					}
				}
			}
			mesh->subsets = subsets;

			mesh->CreateRenderData();
			SetEntity(entity, subset);
		}
		});
	AddWidget(&lodgenButton);

	lodCountSlider.Create(2, 10, 6, 8, "LOD Count: ");
	lodCountSlider.SetTooltip("This is how many levels of detail will be created.");
	lodCountSlider.SetSize(XMFLOAT2(100, hei));
	lodCountSlider.SetPos(XMFLOAT2(x + 280, y += step));
	AddWidget(&lodCountSlider);

	lodQualitySlider.Create(0.1f, 1.0f, 0.5f, 10000, "LOD Quality: ");
	lodQualitySlider.SetTooltip("Lower values will make LODs more agressively simplified.");
	lodQualitySlider.SetSize(XMFLOAT2(100, hei));
	lodQualitySlider.SetPos(XMFLOAT2(x + 280, y += step));
	AddWidget(&lodQualitySlider);

	lodErrorSlider.Create(0.01f, 0.1f, 0.03f, 10000, "LOD Error: ");
	lodErrorSlider.SetTooltip("Lower values will make more precise levels of detail.");
	lodErrorSlider.SetSize(XMFLOAT2(100, hei));
	lodErrorSlider.SetPos(XMFLOAT2(x + 280, y += step));
	AddWidget(&lodErrorSlider);

	lodSloppyCheckBox.Create("Sloppy LOD: ");
	lodSloppyCheckBox.SetTooltip("Use the sloppy simplification algorithm, which is faster but doesn't preserve shape well.");
	lodSloppyCheckBox.SetSize(XMFLOAT2(hei, hei));
	lodSloppyCheckBox.SetPos(XMFLOAT2(x + 280, y += step));
	AddWidget(&lodSloppyCheckBox);

	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 1000, 80, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY, -1);
}

void MeshWindow::SetEntity(Entity entity, int subset)
{
	subset = std::max(0, subset);

	this->entity = entity;
	this->subset = subset;

	Scene& scene = wi::scene::GetScene();

	const MeshComponent* mesh = scene.meshes.GetComponent(entity);

	if (mesh != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		std::string ss;
		ss += "Mesh name: " + name.name + "\n";
		ss += "Vertex count: " + std::to_string(mesh->vertex_positions.size()) + "\n";
		ss += "Index count: " + std::to_string(mesh->indices.size()) + "\n";
		ss += "Subset count: " + std::to_string(mesh->subsets.size()) + " (" + std::to_string(mesh->GetLODCount()) + " LODs)\n";
		ss += "GPU memory: " + std::to_string((mesh->generalBuffer.GetDesc().size + mesh->streamoutBuffer.GetDesc().size) / 1024.0f / 1024.0f) + " MB\n";
		ss += "\nVertex buffers: ";
		if (mesh->vb_pos_nor_wind.IsValid()) ss += "position; ";
		if (mesh->vb_uvs.IsValid()) ss += "uvsets; ";
		if (mesh->vb_atl.IsValid()) ss += "atlas; ";
		if (mesh->vb_col.IsValid()) ss += "color; ";
		if (mesh->so_pre.IsValid()) ss += "previous_position; ";
		if (mesh->vb_bon.IsValid()) ss += "bone; ";
		if (mesh->vb_tan.IsValid()) ss += "tangent; ";
		if (mesh->so_pos_nor_wind.IsValid()) ss += "streamout_position; ";
		if (mesh->so_tan.IsValid()) ss += "streamout_tangents; ";
		if (mesh->IsTerrain()) ss += "\n\nTerrain will use 4 blend materials and blend by vertex colors, the default one is always the subset material and uses RED vertex color channel mask, the other 3 are selectable below.";
		meshInfoLabel.SetText(ss);

		terrainCheckBox.SetCheck(mesh->IsTerrain());

		subsetComboBox.ClearItems();
		for (size_t i = 0; i < mesh->subsets.size(); ++i)
		{
			subsetComboBox.AddItem(std::to_string(i));
		}
		if (subset >= 0)
		{
			subsetComboBox.SetSelected(subset);
		}

		subsetMaterialComboBox.ClearItems();
		subsetMaterialComboBox.AddItem("NO MATERIAL");
		terrainMat1Combo.ClearItems();
		terrainMat1Combo.AddItem("OFF (Use subset)");
		terrainMat2Combo.ClearItems();
		terrainMat2Combo.AddItem("OFF (Use subset)");
		terrainMat3Combo.ClearItems();
		terrainMat3Combo.AddItem("OFF (Use subset)");
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			Entity entity = scene.materials.GetEntity(i);
			const NameComponent& name = *scene.names.GetComponent(entity);
			subsetMaterialComboBox.AddItem(name.name);
			terrainMat1Combo.AddItem(name.name);
			terrainMat2Combo.AddItem(name.name);
			terrainMat3Combo.AddItem(name.name);

			if (subset >= 0 && subset < mesh->subsets.size() && mesh->subsets[subset].materialID == entity)
			{
				subsetMaterialComboBox.SetSelected((int)i + 1);
			}
			if (mesh->terrain_material1 == entity)
			{
				terrainMat1Combo.SetSelected((int)i + 1);
			}
			if (mesh->terrain_material2 == entity)
			{
				terrainMat2Combo.SetSelected((int)i + 1);
			}
			if (mesh->terrain_material3 == entity)
			{
				terrainMat3Combo.SetSelected((int)i + 1);
			}
		}

		doubleSidedCheckBox.SetCheck(mesh->IsDoubleSided());

		const ImpostorComponent* impostor = scene.impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostorDistanceSlider.SetValue(impostor->swapInDistance);
		}
		tessellationFactorSlider.SetValue(mesh->GetTessellationFactor());

		softbodyCheckBox.SetCheck(false);

		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			softbodyCheckBox.SetCheck(true);
			massSlider.SetValue(physicscomponent->mass);
			frictionSlider.SetValue(physicscomponent->friction);
			restitutionSlider.SetValue(physicscomponent->restitution);
		}

		uint8_t selected = morphTargetCombo.GetSelected();
		morphTargetCombo.ClearItems();
		for (size_t i = 0; i < mesh->targets.size(); i++)
		{
		    morphTargetCombo.AddItem(std::to_string(i).c_str());
		}
		if (selected < mesh->targets.size())
		{
		    morphTargetCombo.SetSelected(selected);
		}
		SetEnabled(true);

		if (mesh->targets.empty())
		{
			morphTargetCombo.SetEnabled(false);
			morphTargetSlider.SetEnabled(false);
		}
		else
		{
			morphTargetCombo.SetEnabled(true);
			morphTargetSlider.SetEnabled(true);
		}
	}
	else
	{
		meshInfoLabel.SetText("Select a mesh...");
		SetEnabled(false);
	}

	mergeButton.SetEnabled(true);

	terrainGenButton.SetEnabled(true);
	terragen.Generation_Update();
}
