#pragma once
class EditorComponent;

struct ModifierWindow : public wi::gui::Window
{
	wi::terrain::Modifier* modifier = nullptr;
	std::function<void()> generation_callback;
	wi::gui::ComboBox blendCombo;
	wi::gui::Slider weightSlider;
	wi::gui::Slider frequencySlider;

	virtual ~ModifierWindow() = default;

	ModifierWindow(const std::string& name);
	void Bind(wi::terrain::Modifier* ptr);
	void From(wi::terrain::Modifier* ptr);
};
struct PerlinModifierWindow : public ModifierWindow
{
	wi::gui::Slider octavesSlider;

	PerlinModifierWindow();
	void ResizeLayout() override;
	void Bind(wi::terrain::PerlinModifier* ptr);
	void From(wi::terrain::PerlinModifier* ptr);
};
struct VoronoiModifierWindow : public ModifierWindow
{
	wi::gui::Slider fadeSlider;
	wi::gui::Slider shapeSlider;
	wi::gui::Slider falloffSlider;
	wi::gui::Slider perturbationSlider;

	VoronoiModifierWindow();
	void ResizeLayout() override;
	void Bind(wi::terrain::VoronoiModifier* ptr);
	void From(wi::terrain::VoronoiModifier* ptr);
};
struct HeightmapModifierWindow : public ModifierWindow
{
	wi::gui::Slider scaleSlider;
	wi::gui::Button loadButton;

	HeightmapModifierWindow();
	void ResizeLayout() override;
	void Bind(wi::terrain::HeightmapModifier* ptr);
	void From(wi::terrain::HeightmapModifier* ptr);
};

struct PropWindow : public wi::gui::Window
{
	wi::gui::ComboBox meshCombo;
	wi::gui::TextInputField minCountPerChunkInput;
	wi::gui::TextInputField maxCountPerChunkInput;
	wi::gui::ComboBox regionCombo;
	wi::gui::Slider regionPowerSlider;
	wi::gui::Slider noiseFrequencySlider;
	wi::gui::Slider noisePowerSlider;
	wi::gui::Slider thresholdSlider;
	wi::gui::Slider minSizeSlider;
	wi::gui::Slider maxSizeSlider;
	wi::gui::TextInputField minYOffsetInput;
	wi::gui::TextInputField maxYOffsetInput;

	PropWindow(wi::terrain::Prop* prop, wi::scene::Scene* scene);
	void ResizeLayout() override;

	wi::terrain::Prop* prop = nullptr;
	wi::scene::Scene* scene = nullptr;

	std::function<void()> generation_callback;
};

struct PropsWindow : public wi::gui::Window
{
	wi::gui::Button addButton;

	PropsWindow(EditorComponent* editor);

	void Rebuild();
	void AddWindow(wi::terrain::Prop& prop);

	void Update(const wi::Canvas& canvas, float dt) override;

	void ResizeLayout() override;

	EditorComponent* editor = nullptr;
	wi::terrain::Terrain* terrain = nullptr;

	wi::vector<std::unique_ptr<PropWindow>> windows;
	wi::vector<PropWindow*> windows_to_remove;

	std::function<void()> generation_callback;
};

class TerrainWindow : public wi::gui::Window
{
public:
	wi::gui::CheckBox centerToCamCheckBox;
	wi::gui::CheckBox removalCheckBox;
	wi::gui::CheckBox grassCheckBox;
	wi::gui::CheckBox physicsCheckBox;
	wi::gui::CheckBox tessellationCheckBox;
	wi::gui::Slider lodSlider;
	wi::gui::Slider generationSlider;
	wi::gui::Slider propGenerationSlider;
	wi::gui::Slider physicsGenerationSlider;
	wi::gui::Slider propDensitySlider;
	wi::gui::Slider grassDensitySlider;
	wi::gui::Slider grassLengthSlider;
	wi::gui::Slider grassDistanceSlider;
	wi::gui::ComboBox presetCombo;
	wi::gui::Slider scaleSlider;
	wi::gui::Slider seedSlider;
	wi::gui::Slider bottomLevelSlider;
	wi::gui::Slider topLevelSlider;
	wi::gui::Button saveHeightmapButton;
	wi::gui::Button saveRegionButton;
	wi::gui::ComboBox addModifierCombo;

	wi::gui::Slider region1Slider;
	wi::gui::Slider region2Slider;
	wi::gui::Slider region3Slider;

	wi::gui::ComboBox materialCombos[wi::terrain::MATERIAL_COUNT];
	wi::gui::ComboBox materialCombo_GrassParticle;

	std::unique_ptr<PropsWindow> propsWindow;

	enum PRESET
	{
		PRESET_HILLS,
		PRESET_ISLANDS,
		PRESET_MOUNTAINS,
		PRESET_ARCTIC,
		PRESET_DESERT,
	};

	wi::vector<std::unique_ptr<ModifierWindow>> modifiers;
	wi::vector<ModifierWindow*> modifiers_to_remove;

	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	wi::terrain::Terrain* terrain = nullptr;
	void SetEntity(wi::ecs::Entity entity);
	void AddModifier(ModifierWindow* modifier_window);
	void SetupAssets();

	void Update(const wi::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};

