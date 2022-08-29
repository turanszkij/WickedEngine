#include "stdafx.h"
#include "TerrainGenerator.h"

#include "Utility/stb_image.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;


struct PerlinModifier : public TerrainGenerator::Modifier
{
	wi::gui::Slider octavesSlider;
	wi::noise::Perlin perlin_noise;

	PerlinModifier() : Modifier("Perlin Noise")
	{
		octavesSlider.Create(1, 8, 6, 7, "Octaves: ");
		octavesSlider.SetTooltip("Octave count for the perlin noise");
		octavesSlider.SetSize(XMFLOAT2(100, 20));
		AddWidget(&octavesSlider);

		SetSize(XMFLOAT2(200, 140));
	}

	void ResizeLayout() override
	{
		Modifier::ResizeLayout();
		const float padding = 4;
		const float width = GetWidgetAreaSize().x;
		float y = padding;

		auto add = [&](wi::gui::Widget& widget) {
			const float margin_left = 100;
			const float margin_right = 50;
			widget.SetPos(XMFLOAT2(margin_left, y));
			widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
			y += widget.GetSize().y;
			y += padding;
		};

		add(blendCombo);
		add(blendSlider);
		add(frequencySlider);

		add(octavesSlider);
	}

	void Seed(uint32_t seed) override
	{
		perlin_noise.init(seed);
	}
	void SetCallback(std::function<void(wi::gui::EventArgs args)> func) override
	{
		Modifier::SetCallback(func);
		octavesSlider.OnSlide(func);
	}
	void Apply(const XMFLOAT2& world_pos, float& height) override
	{
		XMFLOAT2 p = world_pos;
		p.x *= frequencySlider.GetValue();
		p.y *= frequencySlider.GetValue();
		Blend(height, perlin_noise.compute(p.x, p.y, 0, (int)octavesSlider.GetValue()) * 0.5f + 0.5f);
	}
};
struct VoronoiModifier : public TerrainGenerator::Modifier
{
	wi::gui::Slider fadeSlider;
	wi::gui::Slider shapeSlider;
	wi::gui::Slider falloffSlider;
	wi::gui::Slider perturbationSlider;
	wi::noise::Perlin perlin_noise;
	float seed = 0;

	VoronoiModifier() : Modifier("Voronoi Noise")
	{
		fadeSlider.Create(0, 100, 2.59f, 10000, "Fade: ");
		fadeSlider.SetTooltip("Fade out voronoi regions by distance from cell's center");
		fadeSlider.SetSize(XMFLOAT2(100, 20));
		AddWidget(&fadeSlider);

		shapeSlider.Create(0, 1, 0.7f, 10000, "Shape: ");
		shapeSlider.SetTooltip("How much the voronoi shape will be kept");
		shapeSlider.SetSize(XMFLOAT2(100, 20));
		AddWidget(&shapeSlider);

		falloffSlider.Create(0, 8, 6, 10000, "Falloff: ");
		falloffSlider.SetTooltip("Controls the falloff of the voronoi distance fade effect");
		falloffSlider.SetSize(XMFLOAT2(100, 20));
		AddWidget(&falloffSlider);

		perturbationSlider.Create(0, 1, 0.1f, 10000, "Perturbation: ");
		perturbationSlider.SetTooltip("Controls the random look of voronoi region edges");
		perturbationSlider.SetSize(XMFLOAT2(100, 20));
		AddWidget(&perturbationSlider);

		SetSize(XMFLOAT2(200, 200));
	}

	void ResizeLayout() override
	{
		Modifier::ResizeLayout();
		const float padding = 4;
		const float width = GetWidgetAreaSize().x;
		float y = padding;

		auto add = [&](wi::gui::Widget& widget) {
			const float margin_left = 100;
			const float margin_right = 50;
			widget.SetPos(XMFLOAT2(margin_left, y));
			widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
			y += widget.GetSize().y;
			y += padding;
		};

		add(blendCombo);
		add(blendSlider);
		add(frequencySlider);

		add(fadeSlider);
		add(shapeSlider);
		add(falloffSlider);
		add(perturbationSlider);
	}

	void Seed(uint32_t seed) override
	{
		perlin_noise.init(seed);
		this->seed = (float)seed;
	}
	void SetCallback(std::function<void(wi::gui::EventArgs args)> func) override
	{
		Modifier::SetCallback(func);
		fadeSlider.OnSlide(func);
		shapeSlider.OnSlide(func);
		falloffSlider.OnSlide(func);
		perturbationSlider.OnSlide(func);
	}
	void Apply(const XMFLOAT2& world_pos, float& height) override
	{
		XMFLOAT2 p = world_pos;
		p.x *= frequencySlider.GetValue();
		p.y *= frequencySlider.GetValue();
		if (perturbationSlider.GetValue() > 0)
		{
			const float angle = perlin_noise.compute(p.x, p.y, 0, 6) * XM_2PI;
			p.x += std::sin(angle) * perturbationSlider.GetValue();
			p.y += std::cos(angle) * perturbationSlider.GetValue();
		}
		wi::noise::voronoi::Result res = wi::noise::voronoi::compute(p.x, p.y, seed);
		float weight = std::pow(1 - wi::math::saturate((res.distance - shapeSlider.GetValue()) * fadeSlider.GetValue()), std::max(0.0001f, falloffSlider.GetValue()));
		Blend(height, weight);
	}
};
struct HeightmapModifier : public TerrainGenerator::Modifier
{
	wi::gui::Slider scaleSlider;
	wi::gui::Button loadButton;

	wi::vector<uint8_t> data;
	int width = 0;
	int height = 0;

	HeightmapModifier() : Modifier("Heightmap")
	{
		blendSlider.SetValue(1);
		frequencySlider.SetValue(1);

		scaleSlider.Create(0, 1, 0.1f, 1000, "Scale: ");
		scaleSlider.SetSize(XMFLOAT2(100, 20));
		AddWidget(&scaleSlider);

		loadButton.Create("Load Heightmap...");
		loadButton.SetTooltip("Load a heightmap texture, where the red channel corresponds to terrain height and the resolution to dimensions.\nThe heightmap will be placed in the world center.");
		AddWidget(&loadButton);

		SetSize(XMFLOAT2(200, 180));
	}

	void ResizeLayout() override
	{
		Modifier::ResizeLayout();
		const float padding = 4;
		const float width = GetWidgetAreaSize().x;
		float y = padding;

		auto add = [&](wi::gui::Widget& widget) {
			const float margin_left = 100;
			const float margin_right = 50;
			widget.SetPos(XMFLOAT2(margin_left, y));
			widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
			y += widget.GetSize().y;
			y += padding;
		};

		add(blendCombo);
		add(blendSlider);
		add(frequencySlider);

		add(scaleSlider);
		add(loadButton);
	}

	void SetCallback(std::function<void(wi::gui::EventArgs args)> func) override
	{
		Modifier::SetCallback(func);

		scaleSlider.OnSlide(func);

		loadButton.OnClick([=](wi::gui::EventArgs args) {

			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

					data.clear();
					width = 0;
					height = 0;
					int bpp = 0;
					stbi_uc* rgba = stbi_load(fileName.c_str(), &width, &height, &bpp, 1);
					if (rgba != nullptr)
					{
						data.resize(width * height);
						for (int i = 0; i < width * height; ++i)
						{
							data[i] = rgba[i];
						}
						stbi_image_free(rgba);

						func(args); // callback after heightmap load confirmation
					}
					});
				});
			});
	}
	void Apply(const XMFLOAT2& world_pos, float& height) override
	{
		XMFLOAT2 p = world_pos;
		p.x *= frequencySlider.GetValue();
		p.y *= frequencySlider.GetValue();
		XMFLOAT2 pixel = XMFLOAT2(p.x + width * 0.5f, p.y + this->height * 0.5f);
		if (pixel.x >= 0 && pixel.x < width && pixel.y >= 0 && pixel.y < this->height)
		{
			const int idx = int(pixel.x) + int(pixel.y) * width;
			Blend(height, ((float)data[idx] / 255.0f) * scaleSlider.GetValue());
		}
	}
};

void TerrainGenerator::AddModifier(Modifier* modifier)
{
	modifiers.emplace_back().reset(modifier);
	auto generate_callback = [=](wi::gui::EventArgs args) {
		Generation_Restart();
	};
	modifier->SetCallback(generate_callback);
	AddWidget(modifier);

	modifier->OnClose([=](wi::gui::EventArgs args) {
		// Can't delete modifier in itself, so add to a deferred deletion queue:
		modifiers_to_remove.push_back(modifier);
		});

	wi::eventhandler::FireEvent(EVENT_THEME_RESET, 0);
}



enum PRESET
{
	PRESET_HILLS,
	PRESET_ISLANDS,
	PRESET_MOUNTAINS,
	PRESET_ARCTIC,
};

void TerrainGenerator::Create()
{
	RemoveWidgets();
	ClearTransform();

	wi::gui::Window::Create("Terrain Generator", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(420, 840));

	float x = 140;
	float y = 0;
	float step = 25;
	float hei = 20;
	float wid = 120;

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
	grassCheckBox.SetPos(XMFLOAT2(x, y += step));
	grassCheckBox.SetCheck(true);
	AddWidget(&grassCheckBox);

	lodSlider.Create(0.0001f, 0.01f, 0.005f, 10000, "Mesh LOD Distance: ");
	lodSlider.SetTooltip("Set the LOD (Level Of Detail) distance multiplier.\nLow values increase LOD detail in distance");
	lodSlider.SetSize(XMFLOAT2(wid, hei));
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

	texlodSlider.Create(0.001f, 0.05f, 0.01f, 10000, "Tex LOD Distance: ");
	texlodSlider.SetTooltip("Set the LOD (Level Of Detail) distance multiplier for virtual textures.\nLow values increase LOD detail in distance");
	texlodSlider.SetSize(XMFLOAT2(wid, hei));
	texlodSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&texlodSlider);

	generationSlider.Create(0, 16, 12, 16, "Generation Distance: ");
	generationSlider.SetTooltip("How far out chunks will be generated (value is in number of chunks)");
	generationSlider.SetSize(XMFLOAT2(wid, hei));
	generationSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&generationSlider);

	propSlider.Create(0, 16, 10, 16, "Prop Distance: ");
	propSlider.SetTooltip("How far out props will be generated (value is in number of chunks)");
	propSlider.SetSize(XMFLOAT2(wid, hei));
	propSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&propSlider);

	propDensitySlider.Create(0, 10, 1, 1000, "Prop Density: ");
	propDensitySlider.SetTooltip("Modifies overall prop density.");
	propDensitySlider.SetSize(XMFLOAT2(wid, hei));
	propDensitySlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&propDensitySlider);

	grassDensitySlider.Create(0, 4, 1, 1000, "Grass Density: ");
	grassDensitySlider.SetTooltip("Modifies overall grass density.");
	grassDensitySlider.SetSize(XMFLOAT2(wid, hei));
	grassDensitySlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&grassDensitySlider);

	presetCombo.Create("Preset: ");
	presetCombo.SetTooltip("Select a terrain preset");
	presetCombo.SetSize(XMFLOAT2(wid, hei));
	presetCombo.SetPos(XMFLOAT2(x, y += step));
	presetCombo.AddItem("Hills", PRESET_HILLS);
	presetCombo.AddItem("Islands", PRESET_ISLANDS);
	presetCombo.AddItem("Mountains", PRESET_MOUNTAINS);
	presetCombo.AddItem("Arctic", PRESET_ARCTIC);
	presetCombo.OnSelect([this](wi::gui::EventArgs args) {

		Generation_Cancel();
		for (auto& modifier : modifiers)
		{
			RemoveWidget(modifier.get());
		}
		modifiers.clear();
		AddModifier(new PerlinModifier);
		AddModifier(new VoronoiModifier);
		PerlinModifier& perlin = *(PerlinModifier*)modifiers[0].get();
		VoronoiModifier& voronoi = *(VoronoiModifier*)modifiers[1].get();
		perlin.blendCombo.SetSelectedByUserdata((uint64_t)Modifier::BlendMode::Additive);
		voronoi.blendCombo.SetSelectedByUserdata((uint64_t)Modifier::BlendMode::Multiply);

		switch (args.userdata)
		{
		default:
		case PRESET_HILLS:
			seedSlider.SetValue(5333);
			bottomLevelSlider.SetValue(-60);
			topLevelSlider.SetValue(380);
			perlin.blendSlider.SetValue(0.5f);
			perlin.frequencySlider.SetValue(0.0008f);
			perlin.octavesSlider.SetValue(6);
			voronoi.blendSlider.SetValue(0.5f);
			voronoi.frequencySlider.SetValue(0.001f);
			voronoi.fadeSlider.SetValue(2.59f);
			voronoi.shapeSlider.SetValue(0.7f);
			voronoi.falloffSlider.SetValue(6);
			voronoi.perturbationSlider.SetValue(0.1f);
			region1Slider.SetValue(1);
			region2Slider.SetValue(2);
			region3Slider.SetValue(8);
			break;
		case PRESET_ISLANDS:
			seedSlider.SetValue(4691);
			bottomLevelSlider.SetValue(-79);
			topLevelSlider.SetValue(520);
			perlin.blendSlider.SetValue(0.5f);
			perlin.frequencySlider.SetValue(0.000991f);
			perlin.octavesSlider.SetValue(6);
			voronoi.blendSlider.SetValue(0.5f);
			voronoi.frequencySlider.SetValue(0.000317f);
			voronoi.fadeSlider.SetValue(8.2f);
			voronoi.shapeSlider.SetValue(0.126f);
			voronoi.falloffSlider.SetValue(1.392f);
			voronoi.perturbationSlider.SetValue(0.126f);
			region1Slider.SetValue(8);
			region2Slider.SetValue(0.7f);
			region3Slider.SetValue(8);
			break;
		case PRESET_MOUNTAINS:
			seedSlider.SetValue(8863);
			bottomLevelSlider.SetValue(0);
			topLevelSlider.SetValue(2960);
			perlin.blendSlider.SetValue(0.5f);
			perlin.frequencySlider.SetValue(0.00279f);
			perlin.octavesSlider.SetValue(8);
			voronoi.blendSlider.SetValue(0.5f);
			voronoi.frequencySlider.SetValue(0.000496f);
			voronoi.fadeSlider.SetValue(5.2f);
			voronoi.shapeSlider.SetValue(0.412f);
			voronoi.falloffSlider.SetValue(1.456f);
			voronoi.perturbationSlider.SetValue(0.092f);
			region1Slider.SetValue(1);
			region2Slider.SetValue(1);
			region3Slider.SetValue(0.8f);
			break;
		case PRESET_ARCTIC:
			seedSlider.SetValue(2124);
			bottomLevelSlider.SetValue(-50);
			topLevelSlider.SetValue(40);
			perlin.blendSlider.SetValue(1);
			perlin.frequencySlider.SetValue(0.002f);
			perlin.octavesSlider.SetValue(4);
			voronoi.blendSlider.SetValue(1);
			voronoi.frequencySlider.SetValue(0.004f);
			voronoi.fadeSlider.SetValue(1.8f);
			voronoi.shapeSlider.SetValue(0.518f);
			voronoi.falloffSlider.SetValue(0.2f);
			voronoi.perturbationSlider.SetValue(0.298f);
			region1Slider.SetValue(8);
			region2Slider.SetValue(8);
			region3Slider.SetValue(0);
			break;
		}
		Generation_Restart();
		});
	AddWidget(&presetCombo);


	addModifierCombo.Create("Add Modifier: ");
	addModifierCombo.selected_font.anim.typewriter.looped = true;
	addModifierCombo.selected_font.anim.typewriter.time = 2;
	addModifierCombo.selected_font.anim.typewriter.character_start = 1;
	addModifierCombo.SetTooltip("Add a new modifier that will affect terrain generation.");
	addModifierCombo.SetSize(XMFLOAT2(wid, hei));
	addModifierCombo.SetPos(XMFLOAT2(x, y += step));
	addModifierCombo.SetInvalidSelectionText("...");
	addModifierCombo.AddItem("Perlin Noise");
	addModifierCombo.AddItem("Voronoi Noise");
	addModifierCombo.AddItem("Heightmap Image");
	addModifierCombo.OnSelect([this](wi::gui::EventArgs args) {

		addModifierCombo.SetSelectedWithoutCallback(-1);
		Generation_Cancel();
		switch (args.iValue)
		{
		default:
			break;
		case 0:
			AddModifier(new PerlinModifier);
			break;
		case 1:
			AddModifier(new VoronoiModifier);
			break;
		case 2:
			AddModifier(new HeightmapModifier);
			break;
		}
		Generation_Restart();

		});
	AddWidget(&addModifierCombo);

	scaleSlider.Create(1, 10, 1, 9, "Chunk Scale: ");
	scaleSlider.SetTooltip("Size of one chunk in horizontal directions.\nLarger chunk scale will cover larger distance, but will have less detail per unit.");
	scaleSlider.SetSize(XMFLOAT2(wid, hei));
	scaleSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&scaleSlider);

	seedSlider.Create(1, 12345, 3926, 12344, "Seed: ");
	seedSlider.SetTooltip("Seed for terrain randomness");
	seedSlider.SetSize(XMFLOAT2(wid, hei));
	seedSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&seedSlider);

	bottomLevelSlider.Create(-100, 0, -60, 10000, "Bottom Level: ");
	bottomLevelSlider.SetTooltip("Terrain mesh grid lowest level");
	bottomLevelSlider.SetSize(XMFLOAT2(wid, hei));
	bottomLevelSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&bottomLevelSlider);

	topLevelSlider.Create(0, 5000, 380, 10000, "Top Level: ");
	topLevelSlider.SetTooltip("Terrain mesh grid topmost level");
	topLevelSlider.SetSize(XMFLOAT2(wid, hei));
	topLevelSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&topLevelSlider);

	saveHeightmapButton.Create("Save Heightmap...");
	saveHeightmapButton.SetTooltip("Save a heightmap texture from the currently generated terrain, where the red channel corresponds to terrain height and the resolution to dimensions.\nThe heightmap will be normalized into 8bit PNG format which can result in precision loss!");
	saveHeightmapButton.SetSize(XMFLOAT2(wid, hei));
	saveHeightmapButton.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&saveHeightmapButton);

	region1Slider.Create(0, 8, 1, 10000, "Slope Region: ");
	region1Slider.SetTooltip("The region's falloff power");
	region1Slider.SetSize(XMFLOAT2(wid, hei));
	region1Slider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&region1Slider);

	region2Slider.Create(0, 8, 2, 10000, "Low Altitude Region: ");
	region2Slider.SetTooltip("The region's falloff power");
	region2Slider.SetSize(XMFLOAT2(wid, hei));
	region2Slider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&region2Slider);

	region3Slider.Create(0, 8, 8, 10000, "High Altitude Region: ");
	region3Slider.SetTooltip("The region's falloff power");
	region3Slider.SetSize(XMFLOAT2(wid, hei));
	region3Slider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&region3Slider);


	auto generate_callback = [=](wi::gui::EventArgs args) {
		Generation_Restart();
	};
	scaleSlider.OnSlide(generate_callback);
	seedSlider.OnSlide(generate_callback);
	bottomLevelSlider.OnSlide(generate_callback);
	topLevelSlider.OnSlide(generate_callback);
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

				wi::vector<uint8_t> data;
				int width = int(aabb.getHalfWidth().x * 2 + 1);
				int height = int(aabb.getHalfWidth().z * 2 + 1);
				data.resize(width * height);
				std::fill(data.begin(), data.end(), 0u);

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
								int coord = int(p.x) + int(p.z) * width;
								data[coord] = uint8_t(wi::math::InverseLerp(aabb._min.y, aabb._max.y, p.y) * 255u);
							}
						}
					}
				}

				wi::graphics::TextureDesc desc;
				desc.width = uint32_t(width);
				desc.height = uint32_t(height);
				desc.format = wi::graphics::Format::R8_UNORM;
				bool success = wi::helper::saveTextureToFile(data, desc, wi::helper::ReplaceExtension(fileName, "PNG"));
				assert(success);

				});
			});
		});


	SetCollapsed(true);
}

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

	uint32_t seed = (uint32_t)seedSlider.GetValue();
	perlin_noise.init(seed);
	for (auto& modifier : modifiers)
	{
		modifier->Seed(seed);
	}

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
		weather.volumetricCloudParameters.CoverageAmount = 0.5f;
		weather.volumetricCloudParameters.CoverageMinimum = 0.05f;
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

	// Check whether any modifiers were "closed", and we will really remove them here if so:
	if (!modifiers_to_remove.empty())
	{
		for (auto& modifier : modifiers_to_remove)
		{
			RemoveWidget(modifier);
			for (auto it = modifiers.begin(); it != modifiers.end(); ++it)
			{
				if (it->get() == modifier)
				{
					modifiers.erase(it);
					break;
				}
			}
		}
		Generation_Restart();
		modifiers_to_remove.clear();
	}

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
				if (chunk_data.props_entity != INVALID_ENTITY && (dist > int(propSlider.GetValue()) || std::abs(chunk_data.prop_density_current - propDensitySlider.GetValue()) > std::numeric_limits<float>::epsilon()))
				{
					scene->Entity_Remove(chunk_data.props_entity);
					chunk_data.props_entity = INVALID_ENTITY; // prop can be generated here by generation thread...
				}
			}
		}

		// Grass density modification:
		if (chunk_data.grass_entity != INVALID_ENTITY && std::abs(chunk_data.grass_density_current - grassDensitySlider.GetValue()) > std::numeric_limits<float>::epsilon())
		{
			wi::HairParticleSystem* grass = scene->hairs.GetComponent(chunk_data.grass_entity);
			if (grass != nullptr)
			{
				chunk_data.grass_density_current = grassDensitySlider.GetValue();
				grass->strandCount = uint32_t(chunk_data.grass.strandCount * chunk_data.grass_density_current);
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

			chunk_data.required_texture_resolution = uint32_t(max_texture_resolution / std::pow(2.0f, (float)std::max(0u, texture_lod)));
			chunk_data.required_texture_resolution = std::max(8u, chunk_data.required_texture_resolution);
			MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);

			if (material != nullptr)
			{
				bool need_update = false;
				for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
				{
					if (virtual_texture_available[i])
					{
						uint32_t current_resolution = 0;
						if (material->textures[i].resource.IsValid())
						{
							current_resolution = material->textures[i].resource.GetTexture().GetDesc().width;
						}

						if (current_resolution != chunk_data.required_texture_resolution)
						{
							need_update = true;
							TextureDesc desc;
							desc.width = chunk_data.required_texture_resolution;
							desc.height = chunk_data.required_texture_resolution;
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
				}

				if (need_update)
				{
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
				float virtual_texture_resolution_rcp = 1.0f / float(chunk_data.required_texture_resolution);
				material->texMulAdd.x = float(chunk_data.required_texture_resolution - 1) * virtual_texture_resolution_rcp;
				material->texMulAdd.y = float(chunk_data.required_texture_resolution - 1) * virtual_texture_resolution_rcp;
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

			device->Dispatch(chunk_data.required_texture_resolution / 8u, chunk_data.required_texture_resolution / 8u, 1, cmd);
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
		const uint32_t seed = (uint32_t)seedSlider.GetValue();
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
						for (auto& modifier : modifiers)
						{
							modifier->Apply(world_pos, height);
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
					const float grass_noise = perlin_noise.compute(vertex_pos.x * grass_noise_frequency, vertex_pos.y * grass_noise_frequency, vertex_pos.z * grass_noise_frequency) * 0.5f + 0.5f;
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
						chunk_data.grass_density_current = grassDensitySlider.GetValue();
						grass.strandCount = uint32_t(grass.strandCount * chunk_data.grass_density_current);
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
						chunk_data.prop_density_current = propDensitySlider.GetValue();

						chunk_data.prop_rand.seed((uint32_t)chunk.compute_hash() ^ seed);
						for (auto& prop : props)
						{
							std::uniform_int_distribution<uint32_t> gen_distr(
								uint32_t(prop.min_count_per_chunk * chunk_data.prop_density_current),
								uint32_t(prop.max_count_per_chunk * chunk_data.prop_density_current)
							);
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

								const float noise = std::pow(perlin_noise.compute((vertex_pos.x + chunk_data.position.x) * prop.noise_frequency, vertex_pos.y * prop.noise_frequency, (vertex_pos.z + chunk_data.position.z) * prop.noise_frequency) * 0.5f + 0.5f, prop.noise_power);
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


void TerrainGenerator::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;

	auto add = [&] (wi::gui::Widget& widget) {
		const float margin_left = 150;
		const float margin_right = 45;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_checkbox = [&](wi::gui::CheckBox& widget) {
		const float margin_right = 45;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_window = [&](wi::gui::Window& widget) {
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add_checkbox(centerToCamCheckBox);
	add_checkbox(removalCheckBox);
	add_checkbox(grassCheckBox);
	add(lodSlider);
	add(texlodSlider);
	add(generationSlider);
	add(propSlider);
	add(propDensitySlider);
	add(grassDensitySlider);
	add(presetCombo);
	add(scaleSlider);
	add(seedSlider);
	add(bottomLevelSlider);
	add(topLevelSlider);
	add(region1Slider);
	add(region2Slider);
	add(region3Slider);
	add(saveHeightmapButton);
	add(addModifierCombo);

	for (auto& modifier : modifiers)
	{
		add_window(*modifier);
	}

}
