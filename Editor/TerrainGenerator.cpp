#include "stdafx.h"
#include "TerrainGenerator.h"

#include "Utility/stb_image.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

enum PRESET
{
	PRESET_HILLS,
	PRESET_ISLANDS,
	PRESET_MOUNTAINS,
	PRESET_ARCTIC,
};

void TerrainGenerator::init()
{
	terrainEntity = CreateEntity();

	indices.clear();
	lods.clear();
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

	RemoveWidgets();
	ClearTransform();

	wi::gui::Window::Create("TerraGen (Preview version)");
	SetSize(XMFLOAT2(420, 300));

	float x = 160;
	float y = 0;
	float step = 25;
	float hei = 20;

	centerToCamCheckBox.Create("Center to Cam: ");
	centerToCamCheckBox.SetTooltip("Automatically generate chunks around camera. This sets the center chunk to camera position.");
	centerToCamCheckBox.SetSize(XMFLOAT2(hei, hei));
	centerToCamCheckBox.SetPos(XMFLOAT2(x, y));
	centerToCamCheckBox.SetCheck(true);
	AddWidget(&centerToCamCheckBox);

	removalCheckBox.Create("Removal: ");
	removalCheckBox.SetTooltip("Automatically remove chunks that are farther than generation distance around center chunk.");
	removalCheckBox.SetSize(XMFLOAT2(hei, hei));
	removalCheckBox.SetPos(XMFLOAT2(x + 100, y));
	removalCheckBox.SetCheck(true);
	AddWidget(&removalCheckBox);

	grassCheckBox.Create("Grass: ");
	grassCheckBox.SetTooltip("Specify whether grass generation is enabled.");
	grassCheckBox.SetSize(XMFLOAT2(hei, hei));
	grassCheckBox.SetPos(XMFLOAT2(x + 200, y));
	grassCheckBox.SetCheck(true);
	AddWidget(&grassCheckBox);

	lodSlider.Create(0.0001f, 0.01f, 0.005f, 10000, "Mesh LOD Distance: ");
	lodSlider.SetTooltip("Set the LOD (Level Of Detail) distance multiplier.\nLow values increase LOD detail in distance");
	lodSlider.SetSize(XMFLOAT2(200, hei));
	lodSlider.SetPos(XMFLOAT2(x, y += step));
	lodSlider.OnSlide([this](wi::gui::EventArgs args) {
		for (auto& it : chunks)
		{
			const ChunkData& chunk_data = it.second;
			if (chunk_data.entity != INVALID_ENTITY)
			{
				ObjectComponent* object = scene->objects.GetComponent(chunk_data.entity);
				if (object != nullptr)
				{
					object->lod_distance_multiplier = args.fValue;
				}
			}
		}
		});
	AddWidget(&lodSlider);

	texlodSlider.Create(0.001f, 0.05f, 0.01f, 10000, "Texture LOD Distance: ");
	texlodSlider.SetTooltip("Set the LOD (Level Of Detail) distance multiplier.\nLow values increase LOD detail in distance");
	texlodSlider.SetSize(XMFLOAT2(200, hei));
	texlodSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&texlodSlider);

	generationSlider.Create(0, 16, 12, 16, "Generation Distance: ");
	generationSlider.SetTooltip("How far out chunks will be generated (value is in number of chunks)");
	generationSlider.SetSize(XMFLOAT2(200, hei));
	generationSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&generationSlider);

	propSlider.Create(0, 16, 10, 16, "Prop Distance: ");
	propSlider.SetTooltip("How far out props will be generated (value is in number of chunks)");
	propSlider.SetSize(XMFLOAT2(200, hei));
	propSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&propSlider);

	presetCombo.Create("Preset: ");
	presetCombo.SetTooltip("Select a terrain preset");
	presetCombo.SetSize(XMFLOAT2(200, hei));
	presetCombo.SetPos(XMFLOAT2(x, y += step));
	presetCombo.AddItem("Hills", PRESET_HILLS);
	presetCombo.AddItem("Islands", PRESET_ISLANDS);
	presetCombo.AddItem("Mountains", PRESET_MOUNTAINS);
	presetCombo.AddItem("Arctic", PRESET_ARCTIC);
	presetCombo.OnSelect([=](wi::gui::EventArgs args) {
		switch (args.userdata)
		{
		default:
		case PRESET_HILLS:
			seedSlider.SetValue(5333);
			bottomLevelSlider.SetValue(-60);
			topLevelSlider.SetValue(380);
			perlinBlendSlider.SetValue(0.5f);
			perlinFrequencySlider.SetValue(0.0008f);
			perlinOctavesSlider.SetValue(6);
			voronoiBlendSlider.SetValue(0.5f);
			voronoiFrequencySlider.SetValue(0.001f);
			voronoiFadeSlider.SetValue(2.59f);
			voronoiShapeSlider.SetValue(0.7f);
			voronoiFalloffSlider.SetValue(6);
			voronoiPerturbationSlider.SetValue(0.1f);
			region1Slider.SetValue(1);
			region2Slider.SetValue(2);
			region3Slider.SetValue(8);
			break;
		case PRESET_ISLANDS:
			seedSlider.SetValue(4691);
			bottomLevelSlider.SetValue(-79);
			topLevelSlider.SetValue(520);
			perlinBlendSlider.SetValue(0.5f);
			perlinFrequencySlider.SetValue(0.000991f);
			perlinOctavesSlider.SetValue(6);
			voronoiBlendSlider.SetValue(0.5f);
			voronoiFrequencySlider.SetValue(0.000317f);
			voronoiFadeSlider.SetValue(8.2f);
			voronoiShapeSlider.SetValue(0.126f);
			voronoiFalloffSlider.SetValue(1.392f);
			voronoiPerturbationSlider.SetValue(0.126f);
			region1Slider.SetValue(8);
			region2Slider.SetValue(0.7f);
			region3Slider.SetValue(8);
			break;
		case PRESET_MOUNTAINS:
			seedSlider.SetValue(8863);
			bottomLevelSlider.SetValue(0);
			topLevelSlider.SetValue(2960);
			perlinBlendSlider.SetValue(0.5f);
			perlinFrequencySlider.SetValue(0.00279f);
			perlinOctavesSlider.SetValue(8);
			voronoiBlendSlider.SetValue(0.5f);
			voronoiFrequencySlider.SetValue(0.000496f);
			voronoiFadeSlider.SetValue(5.2f);
			voronoiShapeSlider.SetValue(0.412f);
			voronoiFalloffSlider.SetValue(1.456f);
			voronoiPerturbationSlider.SetValue(0.092f);
			region1Slider.SetValue(1);
			region2Slider.SetValue(1);
			region3Slider.SetValue(0.8f);
			break;
		case PRESET_ARCTIC:
			seedSlider.SetValue(2124);
			bottomLevelSlider.SetValue(-50);
			topLevelSlider.SetValue(40);
			perlinBlendSlider.SetValue(1);
			perlinFrequencySlider.SetValue(0.002f);
			perlinOctavesSlider.SetValue(4);
			voronoiBlendSlider.SetValue(1);
			voronoiFrequencySlider.SetValue(0.004f);
			voronoiFadeSlider.SetValue(1.8f);
			voronoiShapeSlider.SetValue(0.518f);
			voronoiFalloffSlider.SetValue(0.2f);
			voronoiPerturbationSlider.SetValue(0.298f);
			region1Slider.SetValue(8);
			region2Slider.SetValue(8);
			region3Slider.SetValue(0);
			break;
		}
		Generation_Restart();
		});
	AddWidget(&presetCombo);

	scaleSlider.Create(1, 10, 1, 9, "Chunk Scale: ");
	scaleSlider.SetTooltip("Size of one chunk in horizontal directions.\nLarger chunk scale will cover larger distance, but will have less detail per unit.");
	scaleSlider.SetSize(XMFLOAT2(200, hei));
	scaleSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&scaleSlider);

	propDensitySlider.Create(1, 10, 1, 9, "Prop Density: ");
	propDensitySlider.SetTooltip("Modifies overall prop density.");
	propDensitySlider.SetSize(XMFLOAT2(200, hei));
	propDensitySlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&propDensitySlider);

	seedSlider.Create(1, 12345, 3926, 12344, "Seed: ");
	seedSlider.SetTooltip("Seed for terrain randomness");
	seedSlider.SetSize(XMFLOAT2(200, hei));
	seedSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&seedSlider);

	bottomLevelSlider.Create(-100, 0, -60, 10000, "Bottom Level: ");
	bottomLevelSlider.SetTooltip("Terrain mesh grid lowest level");
	bottomLevelSlider.SetSize(XMFLOAT2(200, hei));
	bottomLevelSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&bottomLevelSlider);

	topLevelSlider.Create(0, 5000, 380, 10000, "Top Level: ");
	topLevelSlider.SetTooltip("Terrain mesh grid topmost level");
	topLevelSlider.SetSize(XMFLOAT2(200, hei));
	topLevelSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&topLevelSlider);

	perlinBlendSlider.Create(0, 1, 0.5f, 10000, "Perlin Blend: ");
	perlinBlendSlider.SetTooltip("Amount of perlin noise to use");
	perlinBlendSlider.SetSize(XMFLOAT2(200, hei));
	perlinBlendSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&perlinBlendSlider);

	perlinFrequencySlider.Create(0.0001f, 0.01f, 0.0008f, 10000, "Perlin Frequency: ");
	perlinFrequencySlider.SetTooltip("Frequency for the perlin noise");
	perlinFrequencySlider.SetSize(XMFLOAT2(200, hei));
	perlinFrequencySlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&perlinFrequencySlider);

	perlinOctavesSlider.Create(1, 8, 6, 7, "Perlin Octaves: ");
	perlinOctavesSlider.SetTooltip("Octave count for the perlin noise");
	perlinOctavesSlider.SetSize(XMFLOAT2(200, hei));
	perlinOctavesSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&perlinOctavesSlider);

	voronoiBlendSlider.Create(0, 1, 0.5f, 10000, "Voronoi Blend: ");
	voronoiBlendSlider.SetTooltip("Amount of voronoi to use for elevation");
	voronoiBlendSlider.SetSize(XMFLOAT2(200, hei));
	voronoiBlendSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&voronoiBlendSlider);

	voronoiFrequencySlider.Create(0.0001f, 0.01f, 0.001f, 10000, "Voronoi Frequency: ");
	voronoiFrequencySlider.SetTooltip("Voronoi can create distinctly elevated areas, the more cells there are, smaller the consecutive areas");
	voronoiFrequencySlider.SetSize(XMFLOAT2(200, hei));
	voronoiFrequencySlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&voronoiFrequencySlider);

	voronoiFadeSlider.Create(0, 100, 2.59f, 10000, "Voronoi Fade: ");
	voronoiFadeSlider.SetTooltip("Fade out voronoi regions by distance from cell's center");
	voronoiFadeSlider.SetSize(XMFLOAT2(200, hei));
	voronoiFadeSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&voronoiFadeSlider);

	voronoiShapeSlider.Create(0, 1, 0.7f, 10000, "Voronoi Shape: ");
	voronoiShapeSlider.SetTooltip("How much the voronoi shape will be kept");
	voronoiShapeSlider.SetSize(XMFLOAT2(200, hei));
	voronoiShapeSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&voronoiShapeSlider);

	voronoiFalloffSlider.Create(0, 8, 6, 10000, "Voronoi Falloff: ");
	voronoiFalloffSlider.SetTooltip("Controls the falloff of the voronoi distance fade effect");
	voronoiFalloffSlider.SetSize(XMFLOAT2(200, hei));
	voronoiFalloffSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&voronoiFalloffSlider);

	voronoiPerturbationSlider.Create(0, 1, 0.1f, 10000, "Voronoi Perturbation: ");
	voronoiPerturbationSlider.SetTooltip("Controls the random look of voronoi region edges");
	voronoiPerturbationSlider.SetSize(XMFLOAT2(200, hei));
	voronoiPerturbationSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&voronoiPerturbationSlider);

	saveHeightmapButton.Create("Save Heightmap...");
	saveHeightmapButton.SetTooltip("Save a heightmap texture from the currently generated terrain, where the red channel corresponds to terrain height and the resolution to dimensions.\nThe heightmap will be normalized into 8bit PNG format which can result in precision loss!");
	saveHeightmapButton.SetSize(XMFLOAT2(200, hei));
	saveHeightmapButton.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&saveHeightmapButton);

	heightmapButton.Create("Load Heightmap...");
	heightmapButton.SetTooltip("Load a heightmap texture, where the red channel corresponds to terrain height and the resolution to dimensions.\nThe heightmap will be placed in the world center.");
	heightmapButton.SetSize(XMFLOAT2(200, hei));
	heightmapButton.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&heightmapButton);

	heightmapBlendSlider.Create(0, 1, 1, 10000, "Heightmap Blend: ");
	heightmapBlendSlider.SetTooltip("Amount of displacement coming from the heightmap texture");
	heightmapBlendSlider.SetSize(XMFLOAT2(200, hei));
	heightmapBlendSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&heightmapBlendSlider);

	region1Slider.Create(0, 8, 1, 10000, "Slope Region: ");
	region1Slider.SetTooltip("The region's falloff power");
	region1Slider.SetSize(XMFLOAT2(200, hei));
	region1Slider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&region1Slider);

	region2Slider.Create(0, 8, 2, 10000, "Low Altitude Region: ");
	region2Slider.SetTooltip("The region's falloff power");
	region2Slider.SetSize(XMFLOAT2(200, hei));
	region2Slider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&region2Slider);

	region3Slider.Create(0, 8, 8, 10000, "High Altitude Region: ");
	region3Slider.SetTooltip("The region's falloff power");
	region3Slider.SetSize(XMFLOAT2(200, hei));
	region3Slider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&region3Slider);


	auto generate_callback = [=](wi::gui::EventArgs args) {
		Generation_Restart();
	};
	scaleSlider.OnSlide(generate_callback);
	propDensitySlider.OnSlide(generate_callback);
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
	region1Slider.OnSlide(generate_callback);
	region2Slider.OnSlide(generate_callback);
	region3Slider.OnSlide(generate_callback);

	saveHeightmapButton.OnClick([=](wi::gui::EventArgs args) {

		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::SAVE;
		params.description = "PNG";
		params.extensions = { "PNG" };
		wi::helper::FileDialog(params, [=](std::string fileName) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

				wi::primitive::AABB aabb;
				for (auto& chunk : chunks)
				{
					const wi::primitive::AABB* object_aabb = scene->aabb_objects.GetComponent(chunk.second.entity);
					if (object_aabb != nullptr)
					{
						aabb = wi::primitive::AABB::Merge(aabb, *object_aabb);
					}
				}

				HeightmapTexture saved_heightmap;
				saved_heightmap.width = int(aabb.getHalfWidth().x * 2 + 1);
				saved_heightmap.height = int(aabb.getHalfWidth().z * 2 + 1);
				saved_heightmap.data.resize(saved_heightmap.width * saved_heightmap.height);
				std::fill(saved_heightmap.data.begin(), saved_heightmap.data.end(), 0u);

				for (auto& chunk : chunks)
				{
					const ObjectComponent* object = scene->objects.GetComponent(chunk.second.entity);
					if (object != nullptr)
					{
						const MeshComponent* mesh = scene->meshes.GetComponent(object->meshID);
						if (mesh != nullptr)
						{
							const XMMATRIX W = XMLoadFloat4x4(&object->worldMatrix);
							for (auto& x : mesh->vertex_positions)
							{
								XMVECTOR P = XMLoadFloat3(&x);
								P = XMVector3Transform(P, W);
								XMFLOAT3 p;
								XMStoreFloat3(&p, P);
								p.x -= aabb._min.x;
								p.z -= aabb._min.z;
								int coord = int(p.x) + int(p.z) * saved_heightmap.width;
								saved_heightmap.data[coord] = uint8_t(wi::math::InverseLerp(aabb._min.y, aabb._max.y, p.y) * 255u);
							}
						}
					}
				}

				wi::graphics::TextureDesc desc;
				desc.width = uint32_t(saved_heightmap.width);
				desc.height = uint32_t(saved_heightmap.height);
				desc.format = wi::graphics::Format::R8_UNORM;
				bool success = wi::helper::saveTextureToFile(saved_heightmap.data, desc, wi::helper::ReplaceExtension(fileName, "PNG"));
				assert(success);

				});
			});
		});

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

	SetPos(XMFLOAT2(50, 110));
	SetVisible(false);
	SetEnabled(true);

	presetCombo.SetSelectedByUserdata(PRESET_HILLS);
}

void TerrainGenerator::Generation_Restart()
{
	Generation_Cancel();
	generation_scene.Clear();

	chunks.clear();

	scene->Entity_Remove(terrainEntity);
	scene->transforms.Create(terrainEntity);
	scene->names.Create(terrainEntity) = "terrain";

	const uint32_t seed = (uint32_t)seedSlider.GetValue();
	perlin.init(seed);

	// Add some nice weather and lighting if there is none yet:
	if (scene->weathers.GetCount() == 0)
	{
		Entity weatherEntity = CreateEntity();
		WeatherComponent& weather = scene->weathers.Create(weatherEntity);
		scene->names.Create(weatherEntity) = "terrainWeather";
		scene->Component_Attach(weatherEntity, terrainEntity);
		weather.ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		weather.SetRealisticSky(true);
		weather.SetVolumetricClouds(true);
		weather.volumetricCloudParameters.CoverageAmount = 0.4f;
		weather.volumetricCloudParameters.CoverageMinimum = 1.35f;
		if (presetCombo.GetItemUserData(presetCombo.GetSelected()) == PRESET_ISLANDS)
		{
			weather.SetOceanEnabled(true);
		}
		else
		{
			scene->ocean = {};
		}
		weather.oceanParameters.waterHeight = -40;
		weather.oceanParameters.wave_amplitude = 120;
		weather.fogStart = 300;
		weather.fogEnd = 100000;
		weather.SetHeightFog(true);
		weather.fogHeightStart = 0;
		weather.fogHeightEnd = 100;
		weather.windDirection = XMFLOAT3(0.05f, 0.05f, 0.05f);
		weather.windSpeed = 4;
		weather.cloud_shadow_amount = 0.4f;
		weather.cloud_shadow_scale = 0.003f;
		weather.cloud_shadow_speed = 0.25f;
		weather.stars = 0.6f;
	}
	if (scene->lights.GetCount() == 0)
	{
		Entity sunEntity = scene->Entity_CreateLight("terrainSun");
		scene->Component_Attach(sunEntity, terrainEntity);
		LightComponent& light = *scene->lights.GetComponent(sunEntity);
		light.SetType(LightComponent::LightType::DIRECTIONAL);
		light.intensity = 16;
		light.SetCastShadow(true);
		//light.SetVolumetricsEnabled(true);
		TransformComponent& transform = *scene->transforms.GetComponent(sunEntity);
		transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV4, 0, XM_PIDIV4));
		transform.Translate(XMFLOAT3(0, 2, 0));
	}
}

void TerrainGenerator::Generation_Update(const wi::scene::CameraComponent& camera)
{
	// The generation task is always cancelled every frame so we are sure that generation is not running at this point
	Generation_Cancel();

	if (terrainEntity == INVALID_ENTITY || !scene->transforms.Contains(terrainEntity))
	{
		chunks.clear();
		return;
	}

	// What was generated, will be merged in to the main scene
	scene->Merge(generation_scene);

	const float chunk_scale = scaleSlider.GetValue();
	const float chunk_scale_rcp = 1.0f / chunk_scale;

	if (centerToCamCheckBox.GetCheck())
	{
		center_chunk.x = (int)std::floor((camera.Eye.x + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
		center_chunk.z = (int)std::floor((camera.Eye.z + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
	}

	const int removal_threshold = (int)generationSlider.GetValue() + 2;
	const float texlodMultiplier = texlodSlider.GetValue();
	GraphicsDevice* device = GetDevice();
	virtual_texture_updates.clear();
	virtual_texture_barriers_begin.clear();
	virtual_texture_barriers_end.clear();

	// Check whether there are any materials that would write to virtual textures:
	uint32_t max_texture_resolution = 0;
	bool virtual_texture_available[MaterialComponent::TEXTURESLOT_COUNT] = {};
	virtual_texture_available[MaterialComponent::SURFACEMAP] = true; // this is always needed to bake individual material properties
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
			switch (i)
			{
			case MaterialComponent::BASECOLORMAP:
			case MaterialComponent::NORMALMAP:
			case MaterialComponent::SURFACEMAP:
				if (material->textures[i].resource.IsValid())
				{
					virtual_texture_available[i] = true;
					const TextureDesc& desc = material->textures[i].resource.GetTexture().GetDesc();
					max_texture_resolution = std::max(max_texture_resolution, desc.width);
					max_texture_resolution = std::max(max_texture_resolution, desc.height);
				}
				break;
			default:
				break;
			}
		}
	}

	for (auto it = chunks.begin(); it != chunks.end();)
	{
		const Chunk& chunk = it->first;
		ChunkData& chunk_data = it->second;
		const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

		// chunk removal:
		if (removalCheckBox.GetCheck())
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
				if (chunk_data.grass.meshID != INVALID_ENTITY && (dist > 1 || !grassCheckBox.GetCheck()))
				{
					scene->Entity_Remove(chunk_data.grass_entity);
					chunk_data.grass_entity = INVALID_ENTITY; // grass can be generated here by generation thread...
				}

				// Prop removal:
				if (chunk_data.props_entity != INVALID_ENTITY && dist > int(propSlider.GetValue()))
				{
					scene->Entity_Remove(chunk_data.props_entity);
					chunk_data.props_entity = INVALID_ENTITY; // prop can be generated here by generation thread...
				}
			}
		}

		// Collect virtual texture update requests:
		if (max_texture_resolution > 0)
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

			uint32_t chunk_required_texture_resolution = uint32_t(max_texture_resolution / std::pow(2.0f, (float)std::max(0u, texture_lod)));
			chunk_required_texture_resolution = std::max(8u, chunk_required_texture_resolution);
			if (chunk_data.virtual_texture_resolution != chunk_required_texture_resolution)
			{
				chunk_data.virtual_texture_resolution = chunk_required_texture_resolution;

				MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);
				if (material != nullptr)
				{
					for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
					{
						if (virtual_texture_available[i])
						{
							TextureDesc desc;
							desc.width = chunk_required_texture_resolution;
							desc.height = chunk_required_texture_resolution;
							desc.format = Format::R8G8B8A8_UNORM;
							desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
							Texture texture;
							bool success = device->CreateTexture(&desc, nullptr, &texture);
							assert(success);

							material->textures[i].resource.SetTexture(texture);
							virtual_texture_barriers_begin.push_back(GPUBarrier::Image(&material->textures[i].resource.GetTexture(), desc.layout, ResourceState::UNORDERED_ACCESS));
							virtual_texture_barriers_end.push_back(GPUBarrier::Image(&material->textures[i].resource.GetTexture(), ResourceState::UNORDERED_ACCESS, desc.layout));
						}
					}

					virtual_texture_updates.push_back(chunk);
				}

			}
		}

		it++;
	}

	// Execute batched virtual texture updates:
	if (!virtual_texture_updates.empty())
	{
		CommandList cmd = device->BeginCommandList();
		device->EventBegin("TerrainVirtualTextureUpdate", cmd);
		auto range = wi::profiler::BeginRangeGPU("TerrainVirtualTextureUpdate", cmd);
		device->Barrier(virtual_texture_barriers_begin.data(), (uint32_t)virtual_texture_barriers_begin.size(), cmd);

		device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE), cmd);

		ShaderMaterial materials[4];
		material_Base.WriteShaderMaterial(&materials[0]);
		material_Slope.WriteShaderMaterial(&materials[1]);
		material_LowAltitude.WriteShaderMaterial(&materials[2]);
		material_HighAltitude.WriteShaderMaterial(&materials[3]);
		device->BindDynamicConstantBuffer(materials, 10, cmd);

		for (auto& chunk : virtual_texture_updates)
		{
			auto it = chunks.find(chunk);
			if (it == chunks.end())
				continue;
			ChunkData& chunk_data = it->second;

			const GPUResource* res[] = {
				&chunk_data.region_weights_texture,
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);
			if (material != nullptr)
			{
				// Shrink the uvs to avoid wrap sampling across edge by object rendering shaders:
				float virtual_texture_resolution_rcp = 1.0f / float(chunk_data.virtual_texture_resolution);
				material->texMulAdd.x = float(chunk_data.virtual_texture_resolution - 1) * virtual_texture_resolution_rcp;
				material->texMulAdd.y = float(chunk_data.virtual_texture_resolution - 1) * virtual_texture_resolution_rcp;
				material->texMulAdd.z = 0.5f * virtual_texture_resolution_rcp;
				material->texMulAdd.w = 0.5f * virtual_texture_resolution_rcp;

				for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
				{
					if (virtual_texture_available[i])
					{
						device->BindUAV(material->textures[i].GetGPUResource(), i, cmd);
					}
				}
			}

			device->Dispatch(chunk_data.virtual_texture_resolution / 8u, chunk_data.virtual_texture_resolution / 8u, 1, cmd);
		}

		device->Barrier(virtual_texture_barriers_end.data(), (uint32_t)virtual_texture_barriers_end.size(), cmd);
		wi::profiler::EndRange(range);
		device->EventEnd(cmd);
	}

	// Start the generation on a background thread and keep it running until the next frame
	wi::jobsystem::Execute(generation_workload, [=](wi::jobsystem::JobArgs args) {

		wi::Timer timer;
		const float lodMultiplier = lodSlider.GetValue();
		const int generation = (int)generationSlider.GetValue();
		const int prop_generation = (int)propSlider.GetValue();
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
		const float region1 = region1Slider.GetValue();
		const float region2 = region2Slider.GetValue();
		const float region3 = region3Slider.GetValue();
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

				chunk_data.entity = generation_scene.Entity_CreateObject("chunk_" + std::to_string(chunk.x) + "_" + std::to_string(chunk.z));
				ObjectComponent& object = *generation_scene.objects.GetComponent(chunk_data.entity);
				object.lod_distance_multiplier = lodMultiplier;
				generation_scene.Component_Attach(chunk_data.entity, terrainEntity);

				TransformComponent& transform = *generation_scene.transforms.GetComponent(chunk_data.entity);
				transform.ClearTransform();
				chunk_data.position = XMFLOAT3(float(chunk.x * (chunk_width - 1)) * chunk_scale, 0, float(chunk.z * (chunk_width - 1)) * chunk_scale);
				transform.Translate(chunk_data.position);
				transform.UpdateTransform();

				MaterialComponent& material = generation_scene.materials.Create(chunk_data.entity);
				// material params will be 1 because they will be created from only texture maps
				//	because region materials are blended together into one texture
				material.SetRoughness(1);
				material.SetMetalness(1);
				material.SetReflectance(1);

				MeshComponent& mesh = generation_scene.meshes.Create(chunk_data.entity);
				object.meshID = chunk_data.entity;
				mesh.indices = indices;
				for (auto& lod : lods)
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

				chunk_data.mesh_vertex_positions = mesh.vertex_positions.data();

				wi::HairParticleSystem grass = grass_properties;
				grass.vertex_lengths.resize(vertexCount);
				std::atomic<uint32_t> grass_valid_vertex_count{ 0 };

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
					const float region_slope = std::pow(1.0f - wi::math::saturate(normal.y), region1);
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
					const float grass_noise = perlin.compute(vertex_pos.x * grass_noise_frequency, vertex_pos.y * grass_noise_frequency, vertex_pos.z * grass_noise_frequency) * 0.5f + 0.5f;
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
					chunk_data.grass.viewDistance = chunk_width * chunk_scale;
				}

				// Create the blend weights texture for virtual texture update:
				{
					TextureDesc desc;
					desc.width = (uint32_t)chunk_width;
					desc.height = (uint32_t)chunk_width;
					desc.format = Format::R8G8B8A8_UNORM;
					desc.bind_flags = BindFlag::SHADER_RESOURCE;
					SubresourceData data;
					data.data_ptr = chunk_data.region_weights;
					data.row_pitch = chunk_width * sizeof(chunk_data.region_weights[0]);
					bool success = device->CreateTexture(&desc, &data, &chunk_data.region_weights_texture);
					assert(success);
				}

				wi::jobsystem::Wait(ctx); // wait until mesh.CreateRenderData() async task finishes
				generated_something = true;
			}

			const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

			// Grass patch placement:
			if (dist <= 1 && grassCheckBox.GetCheck())
			{
				it = chunks.find(chunk);
				if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
				{
					ChunkData& chunk_data = it->second;
					if (chunk_data.grass_entity == INVALID_ENTITY && chunk_data.grass.meshID != INVALID_ENTITY)
					{
						// add patch for this chunk
						chunk_data.grass_entity = CreateEntity();
						wi::HairParticleSystem& grass = generation_scene.hairs.Create(chunk_data.grass_entity);
						grass = chunk_data.grass;
						generation_scene.materials.Create(chunk_data.grass_entity) = material_GrassParticle;
						generation_scene.transforms.Create(chunk_data.grass_entity);
						generation_scene.names.Create(chunk_data.grass_entity) = "grass";
						generation_scene.Component_Attach(chunk_data.grass_entity, chunk_data.entity, true);
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

					if (chunk_data.props_entity == INVALID_ENTITY)
					{
						chunk_data.props_entity = CreateEntity();
						generation_scene.transforms.Create(chunk_data.props_entity);
						generation_scene.names.Create(chunk_data.props_entity) = "props";
						generation_scene.Component_Attach(chunk_data.props_entity, chunk_data.entity, true);

						int propDensity = int(propDensitySlider.GetValue());

						chunk_data.prop_rand.seed((uint32_t)chunk.compute_hash() ^ seed);
						for (auto& prop : props)
						{
							std::uniform_int_distribution<uint32_t> gen_distr(prop.min_count_per_chunk * propDensity, prop.max_count_per_chunk * propDensity);
							int gen_count = gen_distr(chunk_data.prop_rand);
							for (int i = 0; i < gen_count; ++i)
							{
								std::uniform_real_distribution<float> float_distr(0.0f, 1.0f);
								std::uniform_int_distribution<uint32_t> ind_distr(0, lods[0].indexCount / 3 - 1);
								uint32_t tri = ind_distr(chunk_data.prop_rand); // random triangle on the chunk mesh
								uint32_t ind0 = indices[tri * 3 + 0];
								uint32_t ind1 = indices[tri * 3 + 1];
								uint32_t ind2 = indices[tri * 3 + 2];
								const XMFLOAT3& pos0 = chunk_data.mesh_vertex_positions[ind0];
								const XMFLOAT3& pos1 = chunk_data.mesh_vertex_positions[ind1];
								const XMFLOAT3& pos2 = chunk_data.mesh_vertex_positions[ind2];
								const XMFLOAT4 region0 = chunk_data.region_weights[ind0];
								const XMFLOAT4 region1 = chunk_data.region_weights[ind1];
								const XMFLOAT4 region2 = chunk_data.region_weights[ind2];
								// random barycentric coords on the triangle:
								float f = float_distr(chunk_data.prop_rand);
								float g = float_distr(chunk_data.prop_rand);
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

								const float noise = std::pow(perlin.compute((vertex_pos.x + chunk_data.position.x) * prop.noise_frequency, vertex_pos.y * prop.noise_frequency, (vertex_pos.z + chunk_data.position.z) * prop.noise_frequency) * 0.5f + 0.5f, prop.noise_power);
								const float chance = std::pow(((float*)&region)[prop.region], prop.region_power) * noise;
								if (chance > prop.threshold)
								{
									Entity entity = generation_scene.Entity_CreateObject(prop.name + std::to_string(i));
									ObjectComponent* object = generation_scene.objects.GetComponent(entity);
									*object = prop.object;
									TransformComponent* transform = generation_scene.transforms.GetComponent(entity);
									XMFLOAT3 offset = vertex_pos;
									offset.y += wi::math::Lerp(prop.min_y_offset, prop.max_y_offset, float_distr(chunk_data.prop_rand));
									transform->Translate(offset);
									const float scaling = wi::math::Lerp(prop.min_size, prop.max_size, float_distr(chunk_data.prop_rand));
									transform->Scale(XMFLOAT3(scaling, scaling, scaling));
									transform->RotateRollPitchYaw(XMFLOAT3(0, XM_2PI * float_distr(chunk_data.prop_rand), 0));
									transform->UpdateTransform();
									generation_scene.Component_Attach(entity, chunk_data.props_entity, true);
									generated_something = true;
								}
							}
						}
					}
				}
			}

			if (generated_something && timer.elapsed_milliseconds() > generation_time_budget_milliseconds)
			{
				generation_cancelled.store(true);
			}

		};

		// generate center chunk first:
		request_chunk(0, 0);
		if (generation_cancelled.load()) return;

		// then generate neighbor chunks in outward spiral:
		for (int growth = 0; growth < generation; ++growth)
		{
			const int side = 2 * (growth + 1);
			int x = -growth - 1;
			int z = -growth - 1;
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (generation_cancelled.load()) return;
				x++;
			}
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (generation_cancelled.load()) return;
				z++;
			}
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (generation_cancelled.load()) return;
				x--;
			}
			for (int i = 0; i < side; ++i)
			{
				request_chunk(x, z);
				if (generation_cancelled.load()) return;
				z--;
			}
		}

		});

}

void TerrainGenerator::Generation_Cancel()
{
	generation_cancelled.store(true); // tell the generation thread that work must be stopped
	wi::jobsystem::Wait(generation_workload); // waits until generation thread exits
	generation_cancelled.store(false); // the next generation can run
}

void TerrainGenerator::BakeVirtualTexturesToFiles()
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
					if (tex.name.empty() && tex.GetGPUResource() != nullptr)
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

	wi::helper::messageBox("Baking terrain virtual textures, this could take a while!", "Attention!");

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

