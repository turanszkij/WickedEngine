#pragma once
class EditorComponent;

class ObjectWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox meshCombo;
	wi::gui::CheckBox renderableCheckBox;
	wi::gui::CheckBox shadowCheckBox;
	wi::gui::CheckBox navmeshCheckBox;
	wi::gui::CheckBox foregroundCheckBox;
	wi::gui::CheckBox notVisibleInMainCameraCheckBox;
	wi::gui::CheckBox notVisibleInReflectionsCheckBox;
	wi::gui::Slider ditherSlider;
	wi::gui::Slider cascadeMaskSlider;
	wi::gui::Slider lodSlider;
	wi::gui::Slider drawdistanceSlider;
	wi::gui::Slider sortPrioritySlider;

	wi::gui::ComboBox colorComboBox;
	wi::gui::ColorPicker colorPicker;

	wi::gui::Slider lightmapResolutionSlider;
	wi::gui::CheckBox lightmapBlockCompressionCheckBox;
	wi::gui::ComboBox lightmapSourceUVSetComboBox;
	wi::gui::Button generateLightmapButton;
	wi::gui::Button stopLightmapGenButton;
	wi::gui::Button clearLightmapButton;

	void ResizeLayout() override;
};

