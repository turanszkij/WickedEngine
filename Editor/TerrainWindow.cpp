#include "stdafx.h"
#include "TerrainWindow.h"

#include "Utility/stb_image.h"

using namespace wi::ecs;
using namespace wi::scene;

ModifierWindow::ModifierWindow(const std::string& name)
{
	wi::gui::Window::Create(name, wi::gui::Window::WindowControls::CLOSE_AND_COLLAPSE);
	SetSize(XMFLOAT2(200, 100));

	blendCombo.Create("Blend: ");
	blendCombo.SetSize(XMFLOAT2(100, 20));
	blendCombo.AddItem("Normal", (uint64_t)wi::terrain::Modifier::BlendMode::Normal);
	blendCombo.AddItem("Additive", (uint64_t)wi::terrain::Modifier::BlendMode::Additive);
	blendCombo.AddItem("Multiply", (uint64_t)wi::terrain::Modifier::BlendMode::Multiply);
	blendCombo.OnSelect([=](wi::gui::EventArgs args) {
		modifier->blend = (wi::terrain::Modifier::BlendMode)args.userdata;
		generation_callback();
		});
	AddWidget(&blendCombo);

	weightSlider.Create(0, 1, 0.5f, 10000, "Weight: ");
	weightSlider.SetSize(XMFLOAT2(100, 20));
	weightSlider.SetTooltip("Blending amount");
	weightSlider.OnSlide([=](wi::gui::EventArgs args) {
		modifier->weight = args.fValue;
		generation_callback();
		});
	AddWidget(&weightSlider);

	frequencySlider.Create(0.0001f, 0.01f, 0.0008f, 10000, "Frequency: ");
	frequencySlider.SetSize(XMFLOAT2(100, 20));
	frequencySlider.SetTooltip("Frequency for the tiling");
	frequencySlider.OnSlide([=](wi::gui::EventArgs args) {
		modifier->frequency = args.fValue;
		generation_callback();
		});
	AddWidget(&frequencySlider);
}
void ModifierWindow::Bind(wi::terrain::Modifier* ptr)
{
	modifier = ptr;
	modifier->blend = (wi::terrain::Modifier::BlendMode)blendCombo.GetItemUserData(blendCombo.GetSelected());
	modifier->weight = weightSlider.GetValue();
	modifier->frequency = frequencySlider.GetValue();
}
void ModifierWindow::From(wi::terrain::Modifier* ptr)
{
	modifier = ptr;
	blendCombo.SetSelectedByUserdataWithoutCallback((uint64_t)ptr->blend);
	weightSlider.SetValue(ptr->weight);
	frequencySlider.SetValue(ptr->frequency);
}

PerlinModifierWindow::PerlinModifierWindow() : ModifierWindow("Perlin Noise")
{
	octavesSlider.Create(1, 8, 6, 7, "Octaves: ");
	octavesSlider.SetTooltip("Octave count for the perlin noise");
	octavesSlider.SetSize(XMFLOAT2(100, 20));
	octavesSlider.OnSlide([=](wi::gui::EventArgs args) {
		((wi::terrain::PerlinModifier*)modifier)->octaves = args.iValue;
		generation_callback();
		});
	AddWidget(&octavesSlider);

	SetSize(XMFLOAT2(200, 140));
}
void PerlinModifierWindow::ResizeLayout()
{
	ModifierWindow::ResizeLayout();
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
	add(weightSlider);
	add(frequencySlider);

	add(octavesSlider);
}
void PerlinModifierWindow::Bind(wi::terrain::PerlinModifier* ptr)
{
	ModifierWindow::Bind(ptr);
	ptr->octaves = (int)octavesSlider.GetValue();
}
void PerlinModifierWindow::From(wi::terrain::PerlinModifier* ptr)
{
	ModifierWindow::From(ptr);
	octavesSlider.SetValue((float)ptr->octaves);
}

VoronoiModifierWindow::VoronoiModifierWindow() : ModifierWindow("Voronoi Noise")
{
	fadeSlider.Create(0, 100, 2.59f, 10000, "Fade: ");
	fadeSlider.SetTooltip("Fade out voronoi regions by distance from cell's center");
	fadeSlider.SetSize(XMFLOAT2(100, 20));
	fadeSlider.OnSlide([=](wi::gui::EventArgs args) {
		((wi::terrain::VoronoiModifier*)modifier)->fade = args.fValue;
		generation_callback();
		});
	AddWidget(&fadeSlider);

	shapeSlider.Create(0, 1, 0.7f, 10000, "Shape: ");
	shapeSlider.SetTooltip("How much the voronoi shape will be kept");
	shapeSlider.SetSize(XMFLOAT2(100, 20));
	shapeSlider.OnSlide([=](wi::gui::EventArgs args) {
		((wi::terrain::VoronoiModifier*)modifier)->shape = args.fValue;
		generation_callback();
		});
	AddWidget(&shapeSlider);

	falloffSlider.Create(0, 8, 6, 10000, "Falloff: ");
	falloffSlider.SetTooltip("Controls the falloff of the voronoi distance fade effect");
	falloffSlider.SetSize(XMFLOAT2(100, 20));
	falloffSlider.OnSlide([=](wi::gui::EventArgs args) {
		((wi::terrain::VoronoiModifier*)modifier)->falloff = args.fValue;
		generation_callback();
		});
	AddWidget(&falloffSlider);

	perturbationSlider.Create(0, 1, 0.1f, 10000, "Perturbation: ");
	perturbationSlider.SetTooltip("Controls the random look of voronoi region edges");
	perturbationSlider.SetSize(XMFLOAT2(100, 20));
	perturbationSlider.OnSlide([=](wi::gui::EventArgs args) {
		((wi::terrain::VoronoiModifier*)modifier)->perturbation = args.fValue;
		generation_callback();
		});
	AddWidget(&perturbationSlider);

	SetSize(XMFLOAT2(200, 200));
}
void VoronoiModifierWindow::ResizeLayout()
{
	ModifierWindow::ResizeLayout();
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
	add(weightSlider);
	add(frequencySlider);

	add(fadeSlider);
	add(shapeSlider);
	add(falloffSlider);
	add(perturbationSlider);
}
void VoronoiModifierWindow::Bind(wi::terrain::VoronoiModifier* ptr)
{
	ModifierWindow::Bind(ptr);
	ptr->fade = fadeSlider.GetValue();
	ptr->shape = shapeSlider.GetValue();
	ptr->falloff = falloffSlider.GetValue();
	ptr->perturbation = perturbationSlider.GetValue();
}
void VoronoiModifierWindow::From(wi::terrain::VoronoiModifier* ptr)
{
	ModifierWindow::From(ptr);
	fadeSlider.SetValue(ptr->fade);
	shapeSlider.SetValue(ptr->shape);
	falloffSlider.SetValue(ptr->falloff);
	perturbationSlider.SetValue(ptr->perturbation);
}

HeightmapModifierWindow::HeightmapModifierWindow() : ModifierWindow("Heightmap")
{
	weightSlider.SetValue(1);
	frequencySlider.SetValue(1);

	scaleSlider.Create(0, 1, 0.1f, 1000, "Scale: ");
	scaleSlider.SetSize(XMFLOAT2(100, 20));
	scaleSlider.OnSlide([=](wi::gui::EventArgs args) {
		((wi::terrain::HeightmapModifier*)modifier)->scale = args.fValue;
		generation_callback();
		});
	AddWidget(&scaleSlider);

	loadButton.Create("Load Heightmap...");
	loadButton.SetTooltip("Load a heightmap texture, where the red channel corresponds to terrain height and the resolution to dimensions.\nThe heightmap will be placed in the world center.\nIt is recommended to use a 16-bit PNG for heightmaps.");
	loadButton.OnClick([=](wi::gui::EventArgs args) {

		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::OPEN;
		params.description = "*.png";
		params.extensions = { "PNG" };
		wi::helper::FileDialog(params, [=](std::string fileName) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

				auto* heightmap_modifier = (wi::terrain::HeightmapModifier*)modifier;
				heightmap_modifier->data.clear();
				heightmap_modifier->width = 0;
				heightmap_modifier->height = 0;
				int bpp = 0;
				if (stbi_is_16_bit(fileName.c_str()))
				{
					stbi_us* rgba = stbi_load_16(fileName.c_str(), &heightmap_modifier->width, &heightmap_modifier->height, &bpp, 1); if (rgba != nullptr)
					{
						heightmap_modifier->data.resize(heightmap_modifier->width * heightmap_modifier->height * sizeof(uint16_t));
						std::memcpy(heightmap_modifier->data.data(), rgba, heightmap_modifier->data.size());
						stbi_image_free(rgba);
						generation_callback(); // callback after heightmap load confirmation
					}
				}
				else
				{
					stbi_uc* rgba = stbi_load(fileName.c_str(), &heightmap_modifier->width, &heightmap_modifier->height, &bpp, 1);
					if (rgba != nullptr)
					{
						heightmap_modifier->data.resize(heightmap_modifier->width * heightmap_modifier->height * sizeof(uint8_t));
						std::memcpy(heightmap_modifier->data.data(), rgba, heightmap_modifier->data.size());
						generation_callback(); // callback after heightmap load confirmation
					}
				}
			});
		});
	});
	AddWidget(&loadButton);

	SetSize(XMFLOAT2(200, 180));
}
void HeightmapModifierWindow::ResizeLayout()
{
	ModifierWindow::ResizeLayout();
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
	add(weightSlider);
	add(frequencySlider);

	add(scaleSlider);
	add(loadButton);
}
void HeightmapModifierWindow::Bind(wi::terrain::HeightmapModifier* ptr)
{
	ModifierWindow::Bind(ptr);
	ptr->scale = scaleSlider.GetValue();
}
void HeightmapModifierWindow::From(wi::terrain::HeightmapModifier* ptr)
{
	ModifierWindow::From(ptr);
	scaleSlider.SetValue(ptr->scale);
}


void TerrainWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	RemoveWidgets();
	ClearTransform();

	wi::gui::Window::Create(ICON_TERRAIN " Terrain", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(420, 980));

	closeButton.SetTooltip("Delete Terrain.");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().terrains.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 140;
	float y = 0;
	float step = 25;
	float hei = 20;
	float wid = 120;

	auto generate_callback = [&]() {

		terrain->SetCenterToCamEnabled(centerToCamCheckBox.GetCheck());
		terrain->SetRemovalEnabled(removalCheckBox.GetCheck());
		terrain->SetGrassEnabled(grassCheckBox.GetCheck());
		terrain->lod_multiplier = lodSlider.GetValue();
		terrain->generation = (int)generationSlider.GetValue();
		terrain->prop_generation = (int)propGenerationSlider.GetValue();
		terrain->physics_generation = (int)physicsGenerationSlider.GetValue();
		terrain->prop_density = propDensitySlider.GetValue();
		terrain->grass_density = grassDensitySlider.GetValue();
		terrain->grass_properties.length = grassLengthSlider.GetValue();
		terrain->grass_properties.viewDistance = grassDistanceSlider.GetValue();
		terrain->chunk_scale = scaleSlider.GetValue();
		terrain->seed = (uint32_t)seedSlider.GetValue();
		terrain->bottomLevel = bottomLevelSlider.GetValue();
		terrain->topLevel = topLevelSlider.GetValue();
		terrain->region1 = region1Slider.GetValue();
		terrain->region2 = region2Slider.GetValue();
		terrain->region3 = region3Slider.GetValue();

		terrain->SetGenerationStarted(false);
	};

	centerToCamCheckBox.Create("Center to Cam: ");
	centerToCamCheckBox.SetTooltip("Automatically generate chunks around camera. This sets the center chunk to camera position.");
	centerToCamCheckBox.SetSize(XMFLOAT2(hei, hei));
	centerToCamCheckBox.SetPos(XMFLOAT2(x, y));
	centerToCamCheckBox.SetCheck(true);
	centerToCamCheckBox.OnClick([=](wi::gui::EventArgs args) {
		terrain->SetCenterToCamEnabled(args.bValue);
		});
	AddWidget(&centerToCamCheckBox);

	removalCheckBox.Create("Removal: ");
	removalCheckBox.SetTooltip("Automatically remove chunks that are farther than generation distance around center chunk.");
	removalCheckBox.SetSize(XMFLOAT2(hei, hei));
	removalCheckBox.SetPos(XMFLOAT2(x + 100, y));
	removalCheckBox.SetCheck(true);
	removalCheckBox.OnClick([=](wi::gui::EventArgs args) {
		terrain->SetRemovalEnabled(args.bValue);
		});
	AddWidget(&removalCheckBox);

	grassCheckBox.Create("Grass: ");
	grassCheckBox.SetTooltip("Specify whether grass generation is enabled.");
	grassCheckBox.SetSize(XMFLOAT2(hei, hei));
	grassCheckBox.SetPos(XMFLOAT2(x, y += step));
	grassCheckBox.SetCheck(true);
	grassCheckBox.OnClick([=](wi::gui::EventArgs args) {
		terrain->SetGrassEnabled(args.bValue);
		});
	AddWidget(&grassCheckBox);

	physicsCheckBox.Create("Physics: ");
	physicsCheckBox.SetTooltip("Specify whether physics is enabled for newly generated terrain chunks.");
	physicsCheckBox.SetSize(XMFLOAT2(hei, hei));
	physicsCheckBox.SetPos(XMFLOAT2(x, y += step));
	physicsCheckBox.SetCheck(true);
	physicsCheckBox.OnClick([=](wi::gui::EventArgs args) {
		terrain->SetPhysicsEnabled(args.bValue);
		generate_callback();
		});
	AddWidget(&physicsCheckBox);

	tessellationCheckBox.Create("Tessellation: ");
	tessellationCheckBox.SetTooltip("Specify whether tessellation is enabled for terrain surface.\nTessellation requires GPU hardware support\nTessellation doesn't work with raytracing effects or Visibility Compute Shading rendering mode.");
	tessellationCheckBox.SetSize(XMFLOAT2(hei, hei));
	tessellationCheckBox.SetPos(XMFLOAT2(x, y += step));
	tessellationCheckBox.SetCheck(true);
	tessellationCheckBox.OnClick([=](wi::gui::EventArgs args) {
		terrain->SetTessellationEnabled(args.bValue);
		});
	AddWidget(&tessellationCheckBox);

	lodSlider.Create(0.0001f, 0.01f, 0.005f, 10000, "Mesh LOD Distance: ");
	lodSlider.SetTooltip("Set the LOD (Level Of Detail) distance multiplier.\nLow values increase LOD detail in distance");
	lodSlider.SetSize(XMFLOAT2(wid, hei));
	lodSlider.SetPos(XMFLOAT2(x, y += step));
	lodSlider.OnSlide([this](wi::gui::EventArgs args) {
		for (auto& it : terrain->chunks)
		{
			const wi::terrain::ChunkData& chunk_data = it.second;
			if (chunk_data.entity != INVALID_ENTITY)
			{
				ObjectComponent* object = terrain->scene->objects.GetComponent(chunk_data.entity);
				if (object != nullptr)
				{
					object->lod_distance_multiplier = args.fValue;
				}
			}
		}
		terrain->lod_multiplier = args.fValue;
		});
	AddWidget(&lodSlider);

	generationSlider.Create(0, 16, 12, 16, "Generation Distance: ");
	generationSlider.SetTooltip("How far out chunks will be generated (value is in number of chunks)");
	generationSlider.SetSize(XMFLOAT2(wid, hei));
	generationSlider.SetPos(XMFLOAT2(x, y += step));
	generationSlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->generation = args.iValue;
		});
	AddWidget(&generationSlider);

	propGenerationSlider.Create(0, 16, 10, 16, "Prop Distance: ");
	propGenerationSlider.SetTooltip("How far out props will be generated (value is in number of chunks)");
	propGenerationSlider.SetSize(XMFLOAT2(wid, hei));
	propGenerationSlider.SetPos(XMFLOAT2(x, y += step));
	propGenerationSlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->prop_generation = args.iValue;
		});
	AddWidget(&propGenerationSlider);

	physicsGenerationSlider.Create(0, 16, 3, 16, "Physics Distance: ");
	physicsGenerationSlider.SetTooltip("How far out physics meshes will be generated (value is in number of chunks)");
	physicsGenerationSlider.SetSize(XMFLOAT2(wid, hei));
	physicsGenerationSlider.SetPos(XMFLOAT2(x, y += step));
	physicsGenerationSlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->physics_generation = args.iValue;
		});
	AddWidget(&physicsGenerationSlider);

	propDensitySlider.Create(0, 10, 1, 1000, "Prop Density: ");
	propDensitySlider.SetTooltip("Modifies overall prop density.");
	propDensitySlider.SetSize(XMFLOAT2(wid, hei));
	propDensitySlider.SetPos(XMFLOAT2(x, y += step));
	propDensitySlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->prop_density = args.fValue;
		});
	AddWidget(&propDensitySlider);

	grassDensitySlider.Create(0, 4, 2, 1000, "Grass Density: ");
	grassDensitySlider.SetTooltip("Modifies overall grass density.");
	grassDensitySlider.SetSize(XMFLOAT2(wid, hei));
	grassDensitySlider.SetPos(XMFLOAT2(x, y += step));
	grassDensitySlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->grass_density = args.fValue;
		});
	AddWidget(&grassDensitySlider);

	grassLengthSlider.Create(0, 8, 2, 1000, "Grass Length: ");
	grassLengthSlider.SetTooltip("Modifies overall grass length.");
	grassLengthSlider.SetSize(XMFLOAT2(wid, hei));
	grassLengthSlider.SetPos(XMFLOAT2(x, y += step));
	grassLengthSlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->grass_properties.length = args.fValue;
		});
	AddWidget(&grassLengthSlider);

	grassDistanceSlider.Create(10, 100, 60, 1000, "Grass Distance: ");
	grassDistanceSlider.SetTooltip("Modifies grass view distance.");
	grassDistanceSlider.SetSize(XMFLOAT2(wid, hei));
	grassDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	grassDistanceSlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->grass_properties.viewDistance = args.fValue;
		});
	AddWidget(&grassDistanceSlider);

	presetCombo.Create("Preset: ");
	presetCombo.SetTooltip("Select a terrain preset");
	presetCombo.SetSize(XMFLOAT2(wid, hei));
	presetCombo.SetPos(XMFLOAT2(x, y += step));
	presetCombo.AddItem("Hills", PRESET_HILLS);
	presetCombo.AddItem("Islands", PRESET_ISLANDS);
	presetCombo.AddItem("Mountains", PRESET_MOUNTAINS);
	presetCombo.AddItem("Arctic", PRESET_ARCTIC);
	presetCombo.OnSelect([=](wi::gui::EventArgs args) {

		terrain->Generation_Cancel();
		for (auto& modifier : modifiers)
		{
			RemoveWidget(modifier.get());
		}
		modifiers.clear();
		modifiers_to_remove.clear();
		terrain->modifiers.clear();
		terrain->modifiers_to_remove.clear();
		PerlinModifierWindow* perlin = new PerlinModifierWindow;
		VoronoiModifierWindow* voronoi = new VoronoiModifierWindow;
		perlin->blendCombo.SetSelectedByUserdataWithoutCallback((uint64_t)wi::terrain::Modifier::BlendMode::Additive);
		voronoi->blendCombo.SetSelectedByUserdataWithoutCallback((uint64_t)wi::terrain::Modifier::BlendMode::Multiply);

		switch (args.userdata)
		{
		default:
		case PRESET_HILLS:
			terrain->weather.SetOceanEnabled(false);
			seedSlider.SetValue(5415);
			bottomLevelSlider.SetValue(-60);
			topLevelSlider.SetValue(380);
			perlin->weightSlider.SetValue(0.5f);
			perlin->frequencySlider.SetValue(0.0008f);
			perlin->octavesSlider.SetValue(6);
			voronoi->weightSlider.SetValue(0.5f);
			voronoi->frequencySlider.SetValue(0.001f);
			voronoi->fadeSlider.SetValue(2.59f);
			voronoi->shapeSlider.SetValue(0.7f);
			voronoi->falloffSlider.SetValue(6);
			voronoi->perturbationSlider.SetValue(0.1f);
			region1Slider.SetValue(1);
			region2Slider.SetValue(2);
			region3Slider.SetValue(8);
			break;
		case PRESET_ISLANDS:
			terrain->weather.SetOceanEnabled(true);
			seedSlider.SetValue(8526);
			bottomLevelSlider.SetValue(-79);
			topLevelSlider.SetValue(520);
			perlin->weightSlider.SetValue(0.5f);
			perlin->frequencySlider.SetValue(0.000991f);
			perlin->octavesSlider.SetValue(6);
			voronoi->weightSlider.SetValue(0.5f);
			voronoi->frequencySlider.SetValue(0.000317f);
			voronoi->fadeSlider.SetValue(8.2f);
			voronoi->shapeSlider.SetValue(0.126f);
			voronoi->falloffSlider.SetValue(1.392f);
			voronoi->perturbationSlider.SetValue(0.126f);
			region1Slider.SetValue(8);
			region2Slider.SetValue(0.7f);
			region3Slider.SetValue(8);
			break;
		case PRESET_MOUNTAINS:
			terrain->weather.SetOceanEnabled(false);
			seedSlider.SetValue(5213);
			bottomLevelSlider.SetValue(0);
			topLevelSlider.SetValue(2960);
			perlin->weightSlider.SetValue(0.5f);
			perlin->frequencySlider.SetValue(0.00279f);
			perlin->octavesSlider.SetValue(8);
			voronoi->weightSlider.SetValue(0.5f);
			voronoi->frequencySlider.SetValue(0.000496f);
			voronoi->fadeSlider.SetValue(5.2f);
			voronoi->shapeSlider.SetValue(0.412f);
			voronoi->falloffSlider.SetValue(1.456f);
			voronoi->perturbationSlider.SetValue(0.092f);
			region1Slider.SetValue(1);
			region2Slider.SetValue(1);
			region3Slider.SetValue(0.8f);
			break;
		case PRESET_ARCTIC:
			terrain->weather.SetOceanEnabled(false);
			seedSlider.SetValue(11597);
			bottomLevelSlider.SetValue(-50);
			topLevelSlider.SetValue(40);
			perlin->weightSlider.SetValue(1);
			perlin->frequencySlider.SetValue(0.002f);
			perlin->octavesSlider.SetValue(4);
			voronoi->weightSlider.SetValue(1);
			voronoi->frequencySlider.SetValue(0.004f);
			voronoi->fadeSlider.SetValue(1.8f);
			voronoi->shapeSlider.SetValue(0.518f);
			voronoi->falloffSlider.SetValue(0.2f);
			voronoi->perturbationSlider.SetValue(0.298f);
			region1Slider.SetValue(8);
			region2Slider.SetValue(8);
			region3Slider.SetValue(0);
			break;
		}

		std::shared_ptr<wi::terrain::PerlinModifier> terrain_perlin = std::make_shared<wi::terrain::PerlinModifier>();
		terrain->modifiers.emplace_back() = terrain_perlin;
		std::shared_ptr<wi::terrain::VoronoiModifier> terrain_voronoi = std::make_shared<wi::terrain::VoronoiModifier>();
		terrain->modifiers.emplace_back() = terrain_voronoi;

		perlin->Bind(terrain_perlin.get());
		voronoi->Bind(terrain_voronoi.get());
		AddModifier(perlin);
		AddModifier(voronoi);

		generate_callback();

		editor->optionsWnd.RefreshEntityTree();

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
	addModifierCombo.OnSelect([=](wi::gui::EventArgs args) {

		addModifierCombo.SetSelectedWithoutCallback(-1);
		terrain->Generation_Cancel();
		switch (args.iValue)
		{
		default:
			break;
		case 0:
			{
				PerlinModifierWindow* ptr = new PerlinModifierWindow;
				std::shared_ptr<wi::terrain::PerlinModifier> modifier = std::make_shared<wi::terrain::PerlinModifier>();
				terrain->modifiers.push_back(modifier);
				ptr->Bind(modifier.get());
				AddModifier(ptr);
			}
			break;
		case 1:
			{
				VoronoiModifierWindow* ptr = new VoronoiModifierWindow;
				std::shared_ptr<wi::terrain::VoronoiModifier> modifier = std::make_shared<wi::terrain::VoronoiModifier>();
				terrain->modifiers.push_back(modifier);
				ptr->Bind(modifier.get());
				AddModifier(ptr);
			}
			break;
		case 2:
			{
				HeightmapModifierWindow* ptr = new HeightmapModifierWindow;
				std::shared_ptr<wi::terrain::HeightmapModifier> modifier = std::make_shared<wi::terrain::HeightmapModifier>();
				terrain->modifiers.push_back(modifier);
				ptr->Bind(modifier.get());
				AddModifier(ptr);
			}
			break;
		}
		generate_callback();

		});
	AddWidget(&addModifierCombo);

	scaleSlider.Create(1, 10, 1, 9, "Chunk Scale: ");
	scaleSlider.SetTooltip("Size of one chunk in horizontal directions.\nLarger chunk scale will cover larger distance, but will have less detail per unit.");
	scaleSlider.SetSize(XMFLOAT2(wid, hei));
	scaleSlider.SetPos(XMFLOAT2(x, y += step));
	scaleSlider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->chunk_scale = args.fValue;
		generate_callback();
		});
	AddWidget(&scaleSlider);

	seedSlider.Create(1, 12345, 3926, 12344, "Seed: ");
	seedSlider.SetTooltip("Seed for terrain randomness");
	seedSlider.SetSize(XMFLOAT2(wid, hei));
	seedSlider.SetPos(XMFLOAT2(x, y += step));
	seedSlider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->seed = (uint32_t)args.iValue;
		generate_callback();
		});
	AddWidget(&seedSlider);

	bottomLevelSlider.Create(-100, 0, -60, 10000, "Bottom Level: ");
	bottomLevelSlider.SetTooltip("Terrain mesh grid lowest level");
	bottomLevelSlider.SetSize(XMFLOAT2(wid, hei));
	bottomLevelSlider.SetPos(XMFLOAT2(x, y += step));
	bottomLevelSlider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->bottomLevel = args.fValue;
		generate_callback();
		});
	AddWidget(&bottomLevelSlider);

	topLevelSlider.Create(0, 5000, 380, 10000, "Top Level: ");
	topLevelSlider.SetTooltip("Terrain mesh grid topmost level");
	topLevelSlider.SetSize(XMFLOAT2(wid, hei));
	topLevelSlider.SetPos(XMFLOAT2(x, y += step));
	topLevelSlider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->topLevel = args.fValue;
		generate_callback();
		});
	AddWidget(&topLevelSlider);

	saveHeightmapButton.Create("Save Heightmap...");
	saveHeightmapButton.SetTooltip("Save a heightmap texture from the currently generated terrain, where the red channel corresponds to terrain height and the resolution to dimensions.\nThe image will be saved as a single channel 16-bit PNG.");
	saveHeightmapButton.SetSize(XMFLOAT2(wid, hei));
	saveHeightmapButton.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&saveHeightmapButton);

	saveRegionButton.Create("Save Blendmap...");
	saveRegionButton.SetTooltip("Save a color texture from the currently generated terrain where RGBA channels indicate terrain property blend weights.\nNote that you can get a completely transparent image easily if the alpha channel weights are zero and you export to PNG.");
	saveRegionButton.SetSize(XMFLOAT2(wid, hei));
	saveRegionButton.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&saveRegionButton);

	region1Slider.Create(0, 8, 1, 10000, "Slope Region: ");
	region1Slider.SetTooltip("The region's falloff power");
	region1Slider.SetSize(XMFLOAT2(wid, hei));
	region1Slider.SetPos(XMFLOAT2(x, y += step));
	region1Slider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->region1 = args.fValue;
		generate_callback();
		});
	AddWidget(&region1Slider);

	region2Slider.Create(0, 8, 2, 10000, "Low Altitude Region: ");
	region2Slider.SetTooltip("The region's falloff power");
	region2Slider.SetSize(XMFLOAT2(wid, hei));
	region2Slider.SetPos(XMFLOAT2(x, y += step));
	region2Slider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->region2 = args.fValue;
		generate_callback();
		});
	AddWidget(&region2Slider);

	region3Slider.Create(0, 8, 8, 10000, "High Altitude Region: ");
	region3Slider.SetTooltip("The region's falloff power");
	region3Slider.SetSize(XMFLOAT2(wid, hei));
	region3Slider.SetPos(XMFLOAT2(x, y += step));
	region3Slider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->region3 = args.fValue;
		generate_callback();
		});
	AddWidget(&region3Slider);

	saveHeightmapButton.OnClick([=](wi::gui::EventArgs args) {

		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::SAVE;
		params.description = "PNG";
		params.extensions = { "PNG" };
		wi::helper::FileDialog(params, [=](std::string fileName) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

				wi::primitive::AABB aabb;
				for (auto& chunk : terrain->chunks)
				{
					const size_t index = terrain->scene->objects.GetIndex(chunk.second.entity);
					if (index == ~0ull)
						continue;
					const wi::primitive::AABB& object_aabb = terrain->scene->aabb_objects[index];
					aabb = wi::primitive::AABB::Merge(aabb, object_aabb);
				}

				wi::vector<uint8_t> data;
				int width = int(aabb.getHalfWidth().x * 2 + 1);
				int height = int(aabb.getHalfWidth().z * 2 + 1);
				data.resize(width * height * sizeof(uint16_t));
				std::fill(data.begin(), data.end(), 0u);
				uint16_t* dest = (uint16_t*)data.data();

				for (auto& chunk : terrain->chunks)
				{
					const ObjectComponent* object = terrain->scene->objects.GetComponent(chunk.second.entity);
					if (object != nullptr)
					{
						const MeshComponent* mesh = terrain->scene->meshes.GetComponent(object->meshID);
						if (mesh != nullptr)
						{
							size_t objectIndex = terrain->scene->objects.GetIndex(chunk.second.entity);
							const XMMATRIX W = XMLoadFloat4x4(&terrain->scene->matrix_objects[objectIndex]);
							for (auto& x : mesh->vertex_positions)
							{
								XMVECTOR P = XMLoadFloat3(&x);
								P = XMVector3Transform(P, W);
								XMFLOAT3 p;
								XMStoreFloat3(&p, P);
								p.x -= aabb._min.x;
								p.z -= aabb._min.z;
								int coord = int(p.x) + int(p.z) * width;
								dest[coord] = uint16_t(wi::math::InverseLerp(aabb._min.y, aabb._max.y, p.y) * 65535u);
							}
						}
					}
				}

				std::string extension = wi::helper::GetExtensionFromFileName(fileName);
				std::string filename_replaced = fileName;
				if (extension != "PNG")
				{
					filename_replaced = wi::helper::ReplaceExtension(fileName, "PNG");
				}

				wi::graphics::TextureDesc desc;
				desc.width = uint32_t(width);
				desc.height = uint32_t(height);
				desc.format = wi::graphics::Format::R16_UNORM;
				bool success = wi::helper::saveTextureToFile(data, desc, filename_replaced);
				assert(success);

				if (success)
				{
					editor->PostSaveText("Exported terrain height map: ", filename_replaced);
				}

				});
			});
		});


	saveRegionButton.OnClick([=](wi::gui::EventArgs args) {

		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::SAVE;
		params.description = "JPG, PNG";
		params.extensions = { "JPG", "PNG" };
		wi::helper::FileDialog(params, [=](std::string fileName) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

				wi::primitive::AABB aabb;
				for (auto& chunk : terrain->chunks)
				{
					const size_t index = terrain->scene->objects.GetIndex(chunk.second.entity);
					if (index == ~0ull)
						continue;
					const wi::primitive::AABB& object_aabb = terrain->scene->aabb_objects[index];
					aabb = wi::primitive::AABB::Merge(aabb, object_aabb);
				}

				wi::vector<uint8_t> data;
				int width = int(aabb.getHalfWidth().x * 2 + 1);
				int height = int(aabb.getHalfWidth().z * 2 + 1);
				data.resize(width * height * sizeof(wi::Color));
				std::fill(data.begin(), data.end(), 0u);
				wi::Color* dest = (wi::Color*)data.data();

				for (auto& chunk : terrain->chunks)
				{
					const wi::terrain::ChunkData& chunk_data = chunk.second;
					const ObjectComponent* object = terrain->scene->objects.GetComponent(chunk.second.entity);
					if (object != nullptr)
					{
						const MeshComponent* mesh = terrain->scene->meshes.GetComponent(object->meshID);
						if (mesh != nullptr)
						{
							size_t objectIndex = terrain->scene->objects.GetIndex(chunk.second.entity);
							const XMMATRIX W = XMLoadFloat4x4(&terrain->scene->matrix_objects[objectIndex]);
							int i = 0;
							for (auto& x : mesh->vertex_positions)
							{
								XMVECTOR P = XMLoadFloat3(&x);
								P = XMVector3Transform(P, W);
								XMFLOAT3 p;
								XMStoreFloat3(&p, P);
								p.x -= aabb._min.x;
								p.z -= aabb._min.z;
								int coord = int(p.x) + int(p.z) * width;
								dest[coord] = chunk_data.region_weights[i++];
							}
						}
					}
				}

				std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(fileName));
				std::string filename_replaced = fileName;
				if (extension != "JPG" && extension != "PNG")
				{
					filename_replaced = wi::helper::ReplaceExtension(fileName, "JPG");
				}

				wi::graphics::TextureDesc desc;
				desc.width = uint32_t(width);
				desc.height = uint32_t(height);
				desc.format = wi::graphics::Format::R8G8B8A8_UNORM;
				bool success = wi::helper::saveTextureToFile(data, desc, filename_replaced);
				assert(success);

				if (success)
				{
					editor->PostSaveText("Exported terrain blend map: ", filename_replaced);
				}

				});
			});
		});

	SetCollapsed(true);
}
void TerrainWindow::SetEntity(Entity entity)
{
	wi::scene::Scene& scene = editor->GetCurrentScene();
	terrain = scene.terrains.GetComponent(entity);
	if (terrain == nullptr)
	{
		entity = INVALID_ENTITY;
		terrain = &terrain_preset;
	}

	if (this->entity == entity)
		return;

	this->entity = entity;

	for (auto& x : modifiers)
	{
		modifiers_to_remove.push_back(x.get());
	}


	centerToCamCheckBox.SetCheck(terrain->IsCenterToCamEnabled());
	removalCheckBox.SetCheck(terrain->IsRemovalEnabled());
	grassCheckBox.SetCheck(terrain->IsGrassEnabled());
	physicsCheckBox.SetCheck(terrain->IsPhysicsEnabled());
	tessellationCheckBox.SetCheck(terrain->IsTessellationEnabled());
	lodSlider.SetValue(terrain->lod_multiplier);
	generationSlider.SetValue((float)terrain->generation);
	propGenerationSlider.SetValue((float)terrain->prop_generation);
	physicsGenerationSlider.SetValue((float)terrain->physics_generation);
	propDensitySlider.SetValue(terrain->prop_density);
	grassDensitySlider.SetValue(terrain->grass_density);
	grassLengthSlider.SetValue(terrain->grass_properties.length);
	grassDistanceSlider.SetValue(terrain->grass_properties.viewDistance);
	scaleSlider.SetValue(terrain->chunk_scale);
	seedSlider.SetValue((float)terrain->seed);
	bottomLevelSlider.SetValue(terrain->bottomLevel);
	topLevelSlider.SetValue(terrain->topLevel);
	region1Slider.SetValue(terrain->region1);
	region2Slider.SetValue(terrain->region2);
	region3Slider.SetValue(terrain->region3);

	for (auto& x : terrain->modifiers)
	{
		switch (x->type)
		{
		default:
		case wi::terrain::Modifier::Type::Perlin:
		{
			PerlinModifierWindow* modifier = new PerlinModifierWindow;
			modifier->From((wi::terrain::PerlinModifier*)x.get());
			AddModifier(modifier);
		}
		break;
		case wi::terrain::Modifier::Type::Voronoi:
		{
			VoronoiModifierWindow* modifier = new VoronoiModifierWindow;
			modifier->From((wi::terrain::VoronoiModifier*)x.get());
			AddModifier(modifier);
		}
		break;
		case wi::terrain::Modifier::Type::Heightmap:
		{
			HeightmapModifierWindow* modifier = new HeightmapModifierWindow;
			modifier->From((wi::terrain::HeightmapModifier*)x.get());
			AddModifier(modifier);
		}
		break;
		}
	}
}
void TerrainWindow::AddModifier(ModifierWindow* modifier_window)
{
	modifier_window->generation_callback = [=]() {
		terrain->Generation_Restart();
	};
	modifiers.emplace_back().reset(modifier_window);
	AddWidget(modifier_window);

	modifier_window->OnClose([=](wi::gui::EventArgs args) {
		// Can't delete modifier in itself, so add to a deferred deletion queue:
		terrain->modifiers_to_remove.push_back(modifier_window->modifier);
		modifiers_to_remove.push_back(modifier_window);
		});

	editor->optionsWnd.generalWnd.themeCombo.SetSelected(editor->optionsWnd.generalWnd.themeCombo.GetSelected()); // theme refresh
}
void TerrainWindow::SetupAssets()
{
	if (!terrain_preset.props.empty())
		return;

	// Customize terrain generator before it's initialized:
	terrain_preset.material_Base.SetRoughness(1);
	terrain_preset.material_Base.SetReflectance(0.005f);
	terrain_preset.material_Slope.SetRoughness(0.1f);
	terrain_preset.material_LowAltitude.SetRoughness(1);
	terrain_preset.material_HighAltitude.SetRoughness(1);
	terrain_preset.material_Base.textures[MaterialComponent::BASECOLORMAP].name = wi::helper::GetCurrentPath() + "/terrain/base.jpg";
	terrain_preset.material_Base.textures[MaterialComponent::NORMALMAP].name = wi::helper::GetCurrentPath() + "/terrain/base_nor.jpg";
	terrain_preset.material_Slope.textures[MaterialComponent::BASECOLORMAP].name = wi::helper::GetCurrentPath() + "/terrain/slope.jpg";
	terrain_preset.material_Slope.textures[MaterialComponent::NORMALMAP].name = wi::helper::GetCurrentPath() + "/terrain/slope_nor.jpg";
	terrain_preset.material_LowAltitude.textures[MaterialComponent::BASECOLORMAP].name = wi::helper::GetCurrentPath() + "/terrain/low_altitude.jpg";
	terrain_preset.material_LowAltitude.textures[MaterialComponent::NORMALMAP].name = wi::helper::GetCurrentPath() + "/terrain/low_altitude_nor.jpg";
	terrain_preset.material_HighAltitude.textures[MaterialComponent::BASECOLORMAP].name = wi::helper::GetCurrentPath() + "/terrain/high_altitude.jpg";
	terrain_preset.material_HighAltitude.textures[MaterialComponent::NORMALMAP].name = wi::helper::GetCurrentPath() + "/terrain/high_altitude_nor.jpg";
	terrain_preset.material_Base.CreateRenderData();
	terrain_preset.material_Slope.CreateRenderData();
	terrain_preset.material_LowAltitude.CreateRenderData();
	terrain_preset.material_HighAltitude.CreateRenderData();

	std::string terrain_path = wi::helper::GetCurrentPath() + "/terrain/";
	wi::config::File config;
	config.Open(std::string(terrain_path + "props.ini").c_str());
	std::unordered_map<std::string, Scene> prop_scenes;

	for (const auto& it : config)
	{
		const std::string& section_name = it.first;
		const wi::config::Section& section = it.second;
		Entity entity = INVALID_ENTITY;
		Scene* scene = &editor->GetCurrentScene();

		if (section.Has("file"))
		{
			std::string text = section.GetText("file");
			if (prop_scenes.count(text) == 0)
			{
				wi::scene::LoadModel(prop_scenes[text], terrain_path + text);
			}
			if (prop_scenes.count(text) != 0)
			{
				scene = &prop_scenes[text];
			}
		}
		if (section.Has("entity"))
		{
			std::string text = section.GetText("entity");
			entity = scene->Entity_FindByName(text);
		}
		if (entity == INVALID_ENTITY)
		{
			continue;
		}

		wi::terrain::Prop& prop = terrain_preset.props.emplace_back();

		if (section.Has("min_count_per_chunk"))
		{
			prop.min_count_per_chunk = section.GetInt("min_count_per_chunk");
		}
		if (section.Has("max_count_per_chunk"))
		{
			prop.max_count_per_chunk = section.GetInt("max_count_per_chunk");
		}
		if (section.Has("region"))
		{
			prop.region = section.GetInt("region");
		}
		if (section.Has("region_power"))
		{
			prop.region_power = section.GetFloat("region_power");
		}
		if (section.Has("noise_frequency"))
		{
			prop.noise_frequency = section.GetFloat("noise_frequency");
		}
		if (section.Has("noise_power"))
		{
			prop.noise_power = section.GetFloat("noise_power");
		}
		if (section.Has("threshold"))
		{
			prop.threshold = section.GetFloat("threshold");
		}
		if (section.Has("min_size"))
		{
			prop.min_size = section.GetFloat("min_size");
		}
		if (section.Has("max_size"))
		{
			prop.max_size = section.GetFloat("max_size");
		}
		if (section.Has("min_y_offset"))
		{
			prop.min_y_offset = section.GetFloat("min_y_offset");
		}
		if (section.Has("max_y_offset"))
		{
			prop.max_y_offset = section.GetFloat("max_y_offset");
		}

		wi::Archive archive;
		EntitySerializer seri;
		scene->Entity_Serialize(
			archive,
			seri,
			entity,
			wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
		);
		archive.WriteData(prop.data);
		scene->Entity_Remove(entity); // The entities will be placed by terrain generator, we don't need the default object that the scene has anymore
	}

	for (auto& it : prop_scenes)
	{
		editor->GetCurrentScene().Merge(it.second);
	}

	// Grass config:
	terrain_preset.material_GrassParticle.alphaRef = 0.75f;
	terrain_preset.grass_properties.length = 2;
	terrain_preset.grass_properties.frameCount = 2;
	terrain_preset.grass_properties.framesX = 1;
	terrain_preset.grass_properties.framesY = 2;
	terrain_preset.grass_properties.frameStart = 0;

	wi::config::File grass_config;
	grass_config.Open(std::string(terrain_path + "grass.ini").c_str());
	if (grass_config.Has("texture"))
	{
		terrain_preset.material_GrassParticle.textures[MaterialComponent::BASECOLORMAP].name = terrain_path + grass_config.GetText("texture");
		terrain_preset.material_GrassParticle.CreateRenderData();
	}
	if (grass_config.Has("alphaRef"))
	{
		terrain_preset.material_GrassParticle.alphaRef = grass_config.GetFloat("alphaRef");
	}
	if (grass_config.Has("length"))
	{
		terrain_preset.grass_properties.length = grass_config.GetFloat("length");
	}
	if (grass_config.Has("frameCount"))
	{
		terrain_preset.grass_properties.frameCount = grass_config.GetInt("frameCount");
	}
	if (grass_config.Has("framesX"))
	{
		terrain_preset.grass_properties.framesX = grass_config.GetInt("framesX");
	}
	if (grass_config.Has("framesY"))
	{
		terrain_preset.grass_properties.framesY = grass_config.GetInt("framesY");
	}
	if (grass_config.Has("frameCount"))
	{
		terrain_preset.grass_properties.frameStart = grass_config.GetInt("frameStart");
	}

	terrain = &terrain_preset;
	presetCombo.SetSelected(0);
}

void TerrainWindow::Update(const wi::Canvas& canvas, float dt)
{
	// Check whether any modifiers were "closed", and we will really remove them here if so:
	if (!modifiers_to_remove.empty())
	{
		for (auto& modifier : modifiers_to_remove)
		{
			for (auto it = modifiers.begin(); it != modifiers.end(); ++it)
			{
				if (it->get() == modifier)
				{
					modifiers.erase(it);
					RemoveWidget(modifier);
					break;
				}
			}
		}
		modifiers_to_remove.clear();
	}

	wi::gui::Window::Update(canvas, dt);
}
void TerrainWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;

	auto add = [&](wi::gui::Widget& widget) {
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
		widget.SetEnabled(true);
	};

	add_checkbox(removalCheckBox);
	centerToCamCheckBox.SetPos(XMFLOAT2(removalCheckBox.GetPos().x - 100, removalCheckBox.GetPos().y));
	add_checkbox(grassCheckBox);
	physicsCheckBox.SetPos(XMFLOAT2(grassCheckBox.GetPos().x - 100, grassCheckBox.GetPos().y));
	add_checkbox(tessellationCheckBox);
	add(lodSlider);
	add(generationSlider);
	add(propGenerationSlider);
	add(physicsGenerationSlider);
	add(propDensitySlider);
	add(grassDensitySlider);
	add(grassLengthSlider);
	add(grassDistanceSlider);
	add(presetCombo);
	add(scaleSlider);
	add(seedSlider);
	add(bottomLevelSlider);
	add(topLevelSlider);
	add(region1Slider);
	add(region2Slider);
	add(region3Slider);
	add(saveHeightmapButton);
	add(saveRegionButton);
	add(addModifierCombo);

	for (auto& modifier : modifiers)
	{
		add_window(*modifier);
	}

}
