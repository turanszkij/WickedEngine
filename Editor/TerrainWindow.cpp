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
	SetCollapsed(true);
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
	SetCollapsed(true);
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
	SetCollapsed(true);
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

PropWindow::PropWindow(wi::terrain::Prop* prop, wi::scene::Scene* scene)
	:prop(prop)
	,scene(scene)
{
	std::string windowName = "Prop: ";
	std::string propName = "NONE";
	Entity entity = INVALID_ENTITY;

	if(!prop->data.empty()) // extract object name
	{
		wi::Archive archive = wi::Archive(prop->data.data(), prop->data.size());
		EntitySerializer serializer;
		entity = scene->Entity_Serialize(
			archive,
			serializer,
			INVALID_ENTITY,
			wi::scene::Scene::EntitySerializeFlags::RECURSIVE |
			wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
		);

		const NameComponent* name = scene->names.GetComponent(entity);
		if (name != nullptr)
		{
			propName = name->name;
			windowName += propName;
		}

		scene->Entity_Remove(entity);
	}

	wi::gui::Window::Create(windowName, wi::gui::Window::WindowControls::CLOSE_AND_COLLAPSE);

	constexpr auto elementSize = XMFLOAT2(100, 20);

	meshCombo.Create("Object: ");
	meshCombo.SetTooltip("Select object component");
	meshCombo.SetSize(elementSize);
	meshCombo.AddItem(propName, entity);
	for (size_t i = 0; i < scene->objects.GetCount(); ++i)
	{
		const Entity ent = scene->objects.GetEntity(i);
		const auto* name = scene->names.GetComponent(ent);
		meshCombo.AddItem(name != nullptr ? name->name : std::to_string(ent), ent);
	}
	AddWidget(&meshCombo);

	meshCombo.OnSelect([=](wi::gui::EventArgs args) {
		if(args.userdata == entity)
		{
			return;
		}

		const auto name = "Prop: " + args.sValue;
		SetName(name);
		SetText(name);
		label.SetText(name);

		const wi::ecs::Entity ent = static_cast<wi::ecs::Entity>(args.userdata);

		wi::Archive archive;
		EntitySerializer serializer;
		scene->Entity_Serialize(
			archive,
			serializer,
			ent,
			wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
		);
		archive.WriteData(prop->data);

		generation_callback();
	});

	minCountPerChunkInput.Create("");
	minCountPerChunkInput.SetDescription("Min count per chunk: ");
	minCountPerChunkInput.SetSize(elementSize);
	minCountPerChunkInput.SetValue(0);
	minCountPerChunkInput.SetTooltip("A chunk will try to generate min this many props of this type");
	minCountPerChunkInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		prop->min_count_per_chunk = std::min(prop->max_count_per_chunk, args.iValue);
		generation_callback();
	});
	AddWidget(&minCountPerChunkInput);

	maxCountPerChunkInput.Create("");
	maxCountPerChunkInput.SetDescription("Max count per chunk: ");
	maxCountPerChunkInput.SetSize(elementSize);
	maxCountPerChunkInput.SetValue(5);
	maxCountPerChunkInput.SetTooltip("A chunk will try to generate max this many props of this type");
	maxCountPerChunkInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		prop->max_count_per_chunk = std::max(prop->min_count_per_chunk, args.iValue);
		generation_callback();
	});
	AddWidget(&maxCountPerChunkInput);

	regionCombo.Create("Region: ");
	regionCombo.SetTooltip("Select a terrain region");
	regionCombo.SetSize(elementSize);
	regionCombo.AddItem("Base", 0);
	regionCombo.AddItem("Slopes", 1);
	regionCombo.AddItem("Low altitude ", 2);
	regionCombo.AddItem("High altitude", 3);
	regionCombo.OnSelect([=](wi::gui::EventArgs args) {
		prop->region = static_cast<int>(args.userdata);
		generation_callback();
	});
	AddWidget(&regionCombo);

	regionPowerSlider.Create(0.0f, 1.0f, 1.0f, 1000, "Region power: ");
	regionPowerSlider.SetSize(elementSize);
	regionPowerSlider.SetTooltip("Region weight affection power factor");
	regionPowerSlider.OnSlide([=](wi::gui::EventArgs args) {
		prop->region_power = args.fValue;
		generation_callback();
		});
	AddWidget(&regionPowerSlider);

	noiseFrequencySlider.Create(0.0f, 1.0f, 1.0f, 1000, "Noise frequency: ");
	noiseFrequencySlider.SetSize(elementSize);
	noiseFrequencySlider.SetTooltip("Perlin noise's frequency for placement factor");
	noiseFrequencySlider.OnSlide([=](wi::gui::EventArgs args) {
		prop->noise_frequency = args.fValue;
		generation_callback();
		});
	AddWidget(&noiseFrequencySlider);

	noisePowerSlider.Create(0.0f, 1.0f, 1.0f, 1000, "Noise pwer: ");
	noisePowerSlider.SetSize(elementSize);
	noisePowerSlider.SetTooltip("Perlin noise's power");
	noisePowerSlider.OnSlide([=](wi::gui::EventArgs args) {
		prop->noise_power = args.fValue;
		generation_callback();
		});
	AddWidget(&noisePowerSlider);

	thresholdSlider.Create(0.0f, 1.0f, 0.5f, 1000, "Threshold: ");
	thresholdSlider.SetSize(elementSize);
	thresholdSlider.SetTooltip("The chance of placement (higher is less chance)");
	thresholdSlider.OnSlide([=](wi::gui::EventArgs args) {
		prop->threshold = args.fValue;
		generation_callback();
	});
	AddWidget(&thresholdSlider);

	minSizeSlider.Create(0.0f, 1.0f, 1.0f, 1000, "Min size: ");
	minSizeSlider.SetSize(elementSize);
	minSizeSlider.SetTooltip("Scaling randomization range min");
	minSizeSlider.OnSlide([=](wi::gui::EventArgs args) {
		prop->min_size = std::min(args.fValue, prop->max_size);
		generation_callback();
	});
	AddWidget(&minSizeSlider);

	maxSizeSlider.Create(0.0f, 1.0f, 1.0f, 1000, "Max size: ");
	maxSizeSlider.SetSize(elementSize);
	maxSizeSlider.SetTooltip("Scaling randomization range max");
	maxSizeSlider.OnSlide([=](wi::gui::EventArgs args) {
		prop->max_size = std::max(args.fValue, prop->min_size);
		generation_callback();
	});
	AddWidget(&maxSizeSlider);

	minYOffsetInput.Create("");
	minYOffsetInput.SetDescription("Min vertical offset: ");
	minYOffsetInput.SetSize(elementSize);
	minYOffsetInput.SetValue(0);
	minYOffsetInput.SetTooltip("Minimal randomized offset on vertical axis");
	minYOffsetInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		prop->min_y_offset = std::min(args.fValue, prop->max_y_offset);
		generation_callback();
	});
	AddWidget(&minYOffsetInput);

	maxYOffsetInput.Create("");
	maxYOffsetInput.SetDescription("Max vertical offset: ");
	maxYOffsetInput.SetSize(elementSize);
	maxYOffsetInput.SetValue(0);
	maxYOffsetInput.SetTooltip("Maximum randomized offset on vertical axis");
	maxYOffsetInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		prop->max_y_offset = std::max(args.fValue, prop->min_y_offset);
		generation_callback();
	});
	AddWidget(&maxYOffsetInput);

	SetSize(XMFLOAT2(200, 312));
	SetCollapsed(true);
}

void PropWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	constexpr float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;

	auto add = [&](wi::gui::Widget& widget) {
		constexpr float margin_left = 150;
		constexpr float margin_right = 50;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add(meshCombo);
	add(minCountPerChunkInput);
	add(maxCountPerChunkInput);
	add(regionCombo);
	add(regionPowerSlider);
	add(noiseFrequencySlider);
	add(noisePowerSlider);
	add(thresholdSlider);
	add(minSizeSlider);
	add(maxSizeSlider);
	add(minYOffsetInput);
	add(maxYOffsetInput);
}

PropsWindow::PropsWindow(EditorComponent* editor)
	:editor(editor)
{
	wi::gui::Window::Create("Props", wi::gui::Window::WindowControls::COLLAPSE);

	SetCollapsed(true);

	addButton.Create("Add prop");
	addButton.SetSize(XMFLOAT2(100, 20));
	addButton.OnClick([this](wi::gui::EventArgs args) {
		terrain->props.emplace_back();
		AddWindow(terrain->props.back());
	});
	AddWidget(&addButton);

	SetSize(XMFLOAT2(420, 332));
}

void PropsWindow::Rebuild()
{
	for(auto& window : windows)
	{
		RemoveWidget(window.get());
	}

	windows.clear();
	windows_to_remove.clear();

	if(terrain == nullptr)
	{
		return;
	}

	generation_callback = [&] {
		terrain->Generation_Restart();
	};

	for(auto i = terrain->props.begin(); i != terrain->props.end(); ++i)
	{
		AddWindow(*i);
	}
}

void PropsWindow::AddWindow(wi::terrain::Prop& prop)
{
	PropWindow* wnd = new PropWindow(&prop, &editor->GetCurrentScene());
	wnd->generation_callback = generation_callback;
	wnd->OnClose([&, wnd](wi::gui::EventArgs args) {
		windows_to_remove.push_back(wnd);
	});
	AddWidget(wnd);

	windows.emplace_back().reset(wnd);

	editor->generalWnd.themeCombo.SetSelected(editor->generalWnd.themeCombo.GetSelected()); // theme refresh
}

void PropsWindow::Update(const wi::Canvas& canvas, float dt)
{
	if(windows.size() != terrain->props.size())
	{
		// recreate all windows
		Rebuild();
	}
	else
	{
		if(!windows_to_remove.empty())
		{
			for(const auto& window : windows_to_remove)
			{
				for (size_t i = 0; i < windows.size(); ++i)
				{
					if (windows[i].get() == window)
					{
						RemoveWidget(window);

						terrain->props.erase(terrain->props.begin() + i);
						windows.erase(windows.begin() + i);

						break;
					}
				}
			}

			// updating props pointers
			for(size_t i = 0; i < windows.size(); ++i)
			{
				windows[i]->prop = &terrain->props[i];
			}

			windows_to_remove.clear();
			generation_callback();
		}
	}

	Window::Update(canvas, dt);
}

void PropsWindow::ResizeLayout()
{
	constexpr float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;

	auto add = [&](wi::gui::Widget& widget) {
		constexpr float margin_left = 150;
		constexpr float margin_right = 50;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
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

	add_fullwidth(addButton);

	for(auto& window : windows)
	{
		add_window(*window);
	}

	SetSize(XMFLOAT2(GetScale().x, control_size + y));

	wi::gui::Window::ResizeLayout();
}

void TerrainWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	RemoveWidgets();
	ClearTransform();

	wi::gui::Window::Create(ICON_TERRAIN " Terrain", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(420, 1000));

	closeButton.SetTooltip("Delete Terrain.");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().terrains.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
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
		wi::HairParticleSystem* hair = terrain->scene->hairs.GetComponent(terrain->grassEntity);
		if (hair != nullptr)
		{
			hair->length = args.fValue;
		}
		});
	AddWidget(&grassLengthSlider);

	grassDistanceSlider.Create(10, 100, 60, 1000, "Grass Distance: ");
	grassDistanceSlider.SetTooltip("Modifies grass view distance.");
	grassDistanceSlider.SetSize(XMFLOAT2(wid, hei));
	grassDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	grassDistanceSlider.OnSlide([this](wi::gui::EventArgs args) {
		terrain->grass_properties.viewDistance = args.fValue;
		wi::HairParticleSystem* hair = terrain->scene->hairs.GetComponent(terrain->grassEntity);
		if (hair != nullptr)
		{
			hair->viewDistance = args.fValue;
		}
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
	presetCombo.AddItem("Desert", PRESET_DESERT);
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
			region1Slider.SetValue(0.8f);
			region2Slider.SetValue(1.2f);
			region3Slider.SetValue(100);
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
			region1Slider.SetValue(1);
			region2Slider.SetValue(0.2f);
			region3Slider.SetValue(100);
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
			region1Slider.SetValue(1);
			region2Slider.SetValue(1);
			region3Slider.SetValue(0);
			break;
		case PRESET_DESERT:
			terrain->weather.SetOceanEnabled(false);
			seedSlider.SetValue(1597);
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
			region2Slider.SetValue(0);
			region3Slider.SetValue(100);
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

		editor->componentsWnd.RefreshEntityTree();

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

	region1Slider.Create(0, 2, 1, 1000, "Slope Region: ");
	region1Slider.SetTooltip("The region's falloff power");
	region1Slider.SetSize(XMFLOAT2(wid, hei));
	region1Slider.SetPos(XMFLOAT2(x, y += step));
	region1Slider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->region1 = args.fValue;
		generate_callback();
		});
	AddWidget(&region1Slider);

	region2Slider.Create(0, 2, 1, 1000, "Low Altitude Region: ");
	region2Slider.SetTooltip("The region's falloff power");
	region2Slider.SetSize(XMFLOAT2(wid, hei));
	region2Slider.SetPos(XMFLOAT2(x, y += step));
	region2Slider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->region2 = args.fValue;
		generate_callback();
		});
	AddWidget(&region2Slider);

	region3Slider.Create(0, 2, 1, 1000, "High Altitude Region: ");
	region3Slider.SetTooltip("The region's falloff power");
	region3Slider.SetSize(XMFLOAT2(wid, hei));
	region3Slider.SetPos(XMFLOAT2(x, y += step));
	region3Slider.OnSlide([=](wi::gui::EventArgs args) {
		terrain->region3 = args.fValue;
		generate_callback();
		});
	AddWidget(&region3Slider);

	materialCombos[wi::terrain::MATERIAL_BASE].Create("Base material: ");
	materialCombos[wi::terrain::MATERIAL_SLOPE].Create("Slope material: ");
	materialCombos[wi::terrain::MATERIAL_LOW_ALTITUDE].Create("Low altitude material: ");
	materialCombos[wi::terrain::MATERIAL_HIGH_ALTITUDE].Create("High altitude material: ");

	for (size_t i = 0; i < arraysize(materialCombos); ++i)
	{
		materialCombos[i].SetTooltip("Select material entity");
		materialCombos[i].SetSize(XMFLOAT2(wid, hei));
		materialCombos[i].SetPos(XMFLOAT2(x, y += step));
		materialCombos[i].OnSelect([&, i](wi::gui::EventArgs args) {
			const Scene& scene = editor->GetCurrentScene();
			wi::ecs::Entity entity = static_cast<wi::ecs::Entity>(args.userdata);
			if (entity != INVALID_ENTITY && scene.materials.Contains(entity))
			{
				if (terrain->materialEntities[i] != entity)
				{
					terrain->materialEntities[i] = entity;
					terrain->Generation_Restart();
				}
			}
			else
			{
				terrain->materialEntities[i] = INVALID_ENTITY;
			}
			editor->paintToolWnd.RecreateTerrainMaterialButtons();
		});

		AddWidget(&materialCombos[i]);
	}

	materialCombo_GrassParticle.Create("Grass material: ");
	materialCombo_GrassParticle.SetTooltip("Select material entity");
	materialCombo_GrassParticle.SetSize(XMFLOAT2(wid, hei));
	materialCombo_GrassParticle.SetPos(XMFLOAT2(x, y += step));
	materialCombo_GrassParticle.OnSelect([&](wi::gui::EventArgs args) {
		const Scene& scene = editor->GetCurrentScene();
		wi::ecs::Entity entity = static_cast<wi::ecs::Entity>(args.userdata);
		if (entity != INVALID_ENTITY && scene.materials.Contains(entity))
		{
			if (terrain->grassEntity != entity)
			{
				terrain->grassEntity = entity;
				terrain->Generation_Restart();
			}
		}
		else
		{
			terrain->grassEntity = INVALID_ENTITY;
		}
	});
	AddWidget(&materialCombo_GrassParticle);

	propsWindow.reset(new PropsWindow(editor));
	AddWidget(propsWindow.get());

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
								dest[coord] = wi::Color(
									chunk_data.blendmap_layers[0].pixels[i],
									chunk_data.blendmap_layers[1].pixels[i],
									chunk_data.blendmap_layers[2].pixels[i],
									chunk_data.blendmap_layers[3].pixels[i]
								);
								i++;
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

	if (this->entity == entity)
		return;

	this->entity = entity;

	terrain = scene.terrains.GetComponent(entity);
	propsWindow->terrain = terrain;
	if (terrain == nullptr)
		return;

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

	auto fillMaterialCombo = [&](wi::gui::ComboBox& comboBox, Entity selected) {
		comboBox.ClearItems();
		comboBox.AddItem("NO MATERIAL", INVALID_ENTITY);
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			Entity entity = scene.materials.GetEntity(i);
			if (scene.names.Contains(entity))
			{
				const NameComponent& name = *scene.names.GetComponent(entity);
				comboBox.AddItem(name.name, entity);
			}
			else
			{
				comboBox.AddItem(std::to_string(entity), entity);
			}

			if (selected == entity)
			{
				comboBox.SetSelectedWithoutCallback(int(i + 1));
			}
		}
	};

	for (size_t i = 0; i < arraysize(materialCombos); ++i)
	{
		fillMaterialCombo(materialCombos[i], terrain->materialEntities[i]);
	}
	fillMaterialCombo(materialCombo_GrassParticle, terrain->grassEntity);

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

	propsWindow->Rebuild();

	editor->paintToolWnd.RecreateTerrainMaterialButtons();
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

	editor->generalWnd.themeCombo.SetSelected(editor->generalWnd.themeCombo.GetSelected()); // theme refresh
}
void TerrainWindow::SetupAssets()
{
	// Customize terrain generator before it's initialized:
	Scene& currentScene = editor->GetCurrentScene();

	Entity terrainEntity = CreateEntity();
	wi::terrain::Terrain& terrain_preset = currentScene.terrains.Create(terrainEntity);
	currentScene.names.Create(terrainEntity) = "terrain";

	SetEntity(terrainEntity);

	terrain_preset.materialEntities.clear();
	terrain_preset.materialEntities.resize(wi::terrain::MATERIAL_COUNT);
	for (int i = 0; i < wi::terrain::MATERIAL_COUNT; ++i)
	{
		terrain_preset.materialEntities[i] = CreateEntity();
		currentScene.materials.Create(terrain_preset.materialEntities[i]);
		currentScene.Component_Attach(terrain_preset.materialEntities[i], entity);
	}

	MaterialComponent* material_Base = currentScene.materials.GetComponent(terrain_preset.materialEntities[wi::terrain::MATERIAL_BASE]);
	MaterialComponent* material_Slope = currentScene.materials.GetComponent(terrain_preset.materialEntities[wi::terrain::MATERIAL_SLOPE]);
	MaterialComponent* material_LowAltitude = currentScene.materials.GetComponent(terrain_preset.materialEntities[wi::terrain::MATERIAL_LOW_ALTITUDE]);
	MaterialComponent* material_HighAltitude = currentScene.materials.GetComponent(terrain_preset.materialEntities[wi::terrain::MATERIAL_HIGH_ALTITUDE]);

	material_Base->SetRoughness(1);
	material_Base->SetReflectance(0.005f);
	material_Slope->SetRoughness(0.1f);
	material_LowAltitude->SetRoughness(1);
	material_HighAltitude->SetRoughness(1);

	std::string asset_path = wi::helper::GetCurrentPath() + "/Content/terrain/";
	if (!wi::helper::DirectoryExists(asset_path))
	{
		// Usually in source download, the assets are one level outside of Editor:
		asset_path = wi::helper::GetCurrentPath() + "/../Content/terrain/";
	}
	if (!wi::helper::DirectoryExists(asset_path))
	{
		// In UWP or older Editor content, it's not in Content folder:
		asset_path = wi::helper::GetCurrentPath() + "/terrain/";
	}

	material_Base->textures[MaterialComponent::BASECOLORMAP].name = asset_path + "base.jpg";
	material_Base->textures[MaterialComponent::NORMALMAP].name = asset_path + "base_nor.jpg";
	material_Slope->textures[MaterialComponent::BASECOLORMAP].name = asset_path + "slope.jpg";
	material_Slope->textures[MaterialComponent::NORMALMAP].name = asset_path + "slope_nor.jpg";
	material_LowAltitude->textures[MaterialComponent::BASECOLORMAP].name = asset_path + "low_altitude.jpg";
	material_LowAltitude->textures[MaterialComponent::NORMALMAP].name = asset_path + "low_altitude_nor.jpg";
	material_HighAltitude->textures[MaterialComponent::BASECOLORMAP].name = asset_path + "high_altitude.jpg";
	material_HighAltitude->textures[MaterialComponent::NORMALMAP].name = asset_path + "high_altitude_nor.jpg";

	material_Base->SetTextureStreamingDisabled();
	material_Slope->SetTextureStreamingDisabled();
	material_LowAltitude->SetTextureStreamingDisabled();
	material_HighAltitude->SetTextureStreamingDisabled();

	// Extra material: rock
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Rock";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "rock.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "rock_nor.jpg";
		mat.roughness = 0.9f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: ground
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Ground";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "ground.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "ground_nor.jpg";
		mat.roughness = 0.9f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: ground2
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Ground2";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "ground2.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "ground2_nor.jpg";
		mat.roughness = 0.9f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: bricks
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Bricks";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "bricks.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "bricks_nor.jpg";
		mat.roughness = 0.9f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: darkrock
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Dark Rock";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "darkrock.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "darkrock_nor.jpg";
		mat.roughness = 0.8f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: metalplate
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Metal Plate";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "metalplate.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "metalplate_nor.jpg";
		mat.metalness = 1;
		mat.roughness = 0.5f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: foil
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Foil";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "foil.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "foil_nor.jpg";
		mat.metalness = 1;
		mat.roughness = 0.01f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: pavingstone
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Paving Stone";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "pavingstone.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "pavingstone_nor.jpg";
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: tactilepaving
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Tactile Paving";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "tactilepaving.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "tactilepaving_nor.jpg";
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}
	// Extra material: lava
	{
		Entity materialEntity = CreateEntity();
		MaterialComponent& mat = currentScene.materials.Create(materialEntity);
		currentScene.names.Create(materialEntity) = "Lava";
		currentScene.Component_Attach(materialEntity, entity);
		mat.textures[MaterialComponent::BASECOLORMAP].name = asset_path + "lava.jpg";
		mat.textures[MaterialComponent::NORMALMAP].name = asset_path + "lava_nor.jpg";
		mat.textures[MaterialComponent::EMISSIVEMAP].name = asset_path + "lava_emi.jpg";
		mat.roughness = 0.8f;
		mat.SetTextureStreamingDisabled();
		terrain_preset.materialEntities.push_back(materialEntity);
	}

	wi::jobsystem::context ctx;
	wi::jobsystem::Dispatch(ctx, (uint32_t)terrain_preset.materialEntities.size(), 1, [&](wi::jobsystem::JobArgs args) {
		Entity entity = terrain_preset.materialEntities[args.jobIndex];
		MaterialComponent* material = currentScene.materials.GetComponent(entity);
		if (material == nullptr)
			return;
		material->CreateRenderData();
	});

	wi::config::File config;
	config.Open(std::string(asset_path + "props.ini").c_str());
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
				wi::scene::LoadModel(prop_scenes[text], asset_path + text);
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

	wi::jobsystem::Wait(ctx);

	for (auto& it : prop_scenes)
	{
		editor->GetCurrentScene().Merge(it.second);
	}

	// Grass config:
	terrain_preset.grassEntity = CreateEntity();
	currentScene.Component_Attach(terrain_preset.grassEntity, entity);
	currentScene.materials.Create(terrain_preset.grassEntity);
	currentScene.hairs.Create(terrain_preset.grassEntity) = terrain_preset.grass_properties;
	MaterialComponent* material_Grass = currentScene.materials.GetComponent(terrain_preset.grassEntity);
	wi::HairParticleSystem* grass = currentScene.hairs.GetComponent(terrain_preset.grassEntity);
	wi::config::File grass_config;
	grass_config.Open(std::string(asset_path + "grass.ini").c_str());
	if (grass_config.Has("texture"))
	{
		material_Grass->textures[MaterialComponent::BASECOLORMAP].name = asset_path + grass_config.GetText("texture");
		material_Grass->CreateRenderData();
	}
	if (grass_config.Has("alphaRef"))
	{
		material_Grass->alphaRef = grass_config.GetFloat("alphaRef");
	}
	if (grass_config.Has("length"))
	{
		grass->length = grass_config.GetFloat("length");
	}
	if (grass_config.Has("frameCount"))
	{
		grass->frameCount = grass_config.GetInt("frameCount");
	}
	if (grass_config.Has("framesX"))
	{
		grass->framesX = grass_config.GetInt("framesX");
	}
	if (grass_config.Has("framesY"))
	{
		grass->framesY = grass_config.GetInt("framesY");
	}
	if (grass_config.Has("frameCount"))
	{
		grass->frameStart = grass_config.GetInt("frameStart");
	}

	{
		Entity sunEntity = currentScene.Entity_CreateLight("sun");
		LightComponent& light = *currentScene.lights.GetComponent(sunEntity);
		light.SetType(LightComponent::LightType::DIRECTIONAL);
		light.intensity = 16;
		light.SetCastShadow(true);
		TransformComponent& transform = *currentScene.transforms.GetComponent(sunEntity);
		transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV4, 0, XM_PIDIV4));
		transform.Translate(XMFLOAT3(0, 4, 0));
	}

	presetCombo.SetSelected(0);

	editor->paintToolWnd.RecreateTerrainMaterialButtons();
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
	for (size_t i = 0; i < arraysize(materialCombos); ++i)
	{
		add(materialCombos[i]);
	}
	add(materialCombo_GrassParticle);
	add(saveHeightmapButton);
	add(saveRegionButton);
	add(addModifierCombo);

	for (auto& modifier : modifiers)
	{
		add_window(*modifier);
	}

	add_window(*propsWindow.get());

	SetSize(XMFLOAT2(GetScale().x, y + control_size));

	wi::gui::Window::ResizeLayout();
}
