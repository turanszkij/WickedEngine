#include "stdafx.h"
#include "TerrainGenerator.h"

#include "Utility/stb_image.h"

using namespace wi::ecs;
using namespace wi::scene;

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
	SetVisible(false);

	float xx = 150;
	float yy = 0;
	float stepstep = 25;
	float heihei = 20;

	centerToCamCheckBox.Create("Center to Cam: ");
	centerToCamCheckBox.SetTooltip("Automatically generate chunks around camera. This sets the center chunk to camera position.");
	centerToCamCheckBox.SetSize(XMFLOAT2(heihei, heihei));
	centerToCamCheckBox.SetPos(XMFLOAT2(xx, yy += stepstep));
	centerToCamCheckBox.SetCheck(true);
	AddWidget(&centerToCamCheckBox);

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

	bottomLevelSlider.Create(-100, 0, -60, 10000, "Bottom Level: ");
	bottomLevelSlider.SetTooltip("Terrain mesh grid lowest level");
	bottomLevelSlider.SetSize(XMFLOAT2(200, heihei));
	bottomLevelSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
	AddWidget(&bottomLevelSlider);

	topLevelSlider.Create(0, 5000, 380, 10000, "Top Level: ");
	topLevelSlider.SetTooltip("Terrain mesh grid topmost level");
	topLevelSlider.SetSize(XMFLOAT2(200, heihei));
	topLevelSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
	AddWidget(&topLevelSlider);

	perlinBlendSlider.Create(0, 1, 0.5f, 10000, "Perlin Blend: ");
	perlinBlendSlider.SetTooltip("Amount of perlin noise to use");
	perlinBlendSlider.SetSize(XMFLOAT2(200, heihei));
	perlinBlendSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
	AddWidget(&perlinBlendSlider);

	perlinFrequencySlider.Create(0.0001f, 0.01f, 0.0008f, 10000, "Perlin Frequency: ");
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

	voronoiFadeSlider.Create(0, 100, 2.59f, 10000, "Voronoi Fade: ");
	voronoiFadeSlider.SetTooltip("Fade out voronoi regions by distance from cell's center");
	voronoiFadeSlider.SetSize(XMFLOAT2(200, heihei));
	voronoiFadeSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
	AddWidget(&voronoiFadeSlider);

	voronoiShapeSlider.Create(0, 1, 0.7f, 10000, "Voronoi Shape: ");
	voronoiShapeSlider.SetTooltip("How much the voronoi shape will be kept");
	voronoiShapeSlider.SetSize(XMFLOAT2(200, heihei));
	voronoiShapeSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
	AddWidget(&voronoiShapeSlider);

	voronoiFalloffSlider.Create(0, 8, 6, 10000, "Voronoi Falloff: ");
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

void TerrainGenerator::Generation_Restart()
{
	Scene& scene = wi::scene::GetScene();

	// If these already exist, save them before recreating:
	//	(This is helpful if eg: someone edits them in the editor and then regenerates the terrain)
	if (materialEntity_Base != INVALID_ENTITY)
	{
		MaterialComponent* material = scene.materials.GetComponent(materialEntity_Base);
		if (material != nullptr)
		{
			material_Base = *material;
		}
	}
	if (materialEntity_Slope != INVALID_ENTITY)
	{
		MaterialComponent* material = scene.materials.GetComponent(materialEntity_Slope);
		if (material != nullptr)
		{
			material_Slope = *material;
		}
	}
	if (materialEntity_LowAltitude != INVALID_ENTITY)
	{
		MaterialComponent* material = scene.materials.GetComponent(materialEntity_LowAltitude);
		if (material != nullptr)
		{
			material_LowAltitude = *material;
		}
	}
	if (materialEntity_HighAltitude != INVALID_ENTITY)
	{
		MaterialComponent* material = scene.materials.GetComponent(materialEntity_HighAltitude);
		if (material != nullptr)
		{
			material_HighAltitude = *material;
		}
	}

	chunks.clear();

	scene.Entity_Remove(terrainEntity);
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
		weather.ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
		weather.SetRealisticSky(true);
		weather.SetVolumetricClouds(true);
		weather.volumetricCloudParameters.CoverageAmount = 0.4f;
		weather.volumetricCloudParameters.CoverageMinimum = 1.35f;
		//weather.SetOceanEnabled(true);
		weather.oceanParameters.waterHeight = -40;
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

void TerrainGenerator::Generation_Update(int allocated_timeframe_milliseconds)
{
	if (terrainEntity == INVALID_ENTITY)
		return;

	Scene& scene = wi::scene::GetScene();
	if (!scene.transforms.Contains(terrainEntity))
	{
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

	if (centerToCamCheckBox.GetCheck())
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

					const float noise = std::pow(perlin.compute(vertex_pos.x * prop.noise_frequency, vertex_pos.y * prop.noise_frequency, vertex_pos.z * prop.noise_frequency) * 0.5f + 0.5f, prop.noise_power);
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
