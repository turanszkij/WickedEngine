#pragma once
class EditorComponent;

class LightWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void SetLightType(wi::scene::LightComponent::LightType type);

	wi::gui::Slider intensitySlider;
	wi::gui::Slider rangeSlider;
	wi::gui::Slider radiusSlider;
	wi::gui::Slider lengthSlider;
	wi::gui::Slider outerConeAngleSlider;
	wi::gui::Slider innerConeAngleSlider;
	wi::gui::CheckBox shadowCheckBox;
	wi::gui::CheckBox haloCheckBox;
	wi::gui::CheckBox volumetricsCheckBox;
	wi::gui::CheckBox staticCheckBox;
	wi::gui::CheckBox volumetricCloudsCheckBox;
	wi::gui::ColorPicker colorPicker;
	wi::gui::ComboBox typeSelectorComboBox;
	wi::gui::ComboBox shadowResolutionComboBox;

	wi::gui::Label lensflare_Label;
	wi::gui::Button lensflare_Button[7];

	struct CascadeConfig
	{
		wi::gui::Slider distanceSlider;
		wi::gui::Button removeButton;
	};
	wi::vector<CascadeConfig> cascades;
	wi::gui::Button addCascadeButton;
	void RefreshCascades();

	void ResizeLayout() override;
};

