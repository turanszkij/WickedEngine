#pragma once
class EditorComponent;

class MaterialWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::TextInputField materialNameField;
	wi::gui::CheckBox shadowReceiveCheckBox;
	wi::gui::CheckBox shadowCasterCheckBox;
	wi::gui::CheckBox useVertexColorsCheckBox;
	wi::gui::CheckBox specularGlossinessCheckBox;
	wi::gui::CheckBox occlusionPrimaryCheckBox;
	wi::gui::CheckBox occlusionSecondaryCheckBox;
	wi::gui::CheckBox vertexAOCheckBox;
	wi::gui::CheckBox windCheckBox;
	wi::gui::CheckBox doubleSidedCheckBox;
	wi::gui::CheckBox outlineCheckBox;
	wi::gui::CheckBox preferUncompressedCheckBox;
	wi::gui::ComboBox shaderTypeComboBox;
	wi::gui::ComboBox blendModeComboBox;
	wi::gui::ComboBox shadingRateComboBox;
	wi::gui::Slider normalMapSlider;
	wi::gui::Slider roughnessSlider;
	wi::gui::Slider reflectanceSlider;
	wi::gui::Slider metalnessSlider;
	wi::gui::Slider emissiveSlider;
	wi::gui::Slider transmissionSlider;
	wi::gui::Slider refractionSlider;
	wi::gui::Slider pomSlider;
	wi::gui::Slider anisotropyStrengthSlider;
	wi::gui::Slider anisotropyRotationSlider;
	wi::gui::Slider displacementMappingSlider;
	wi::gui::Slider subsurfaceScatteringSlider;
	wi::gui::Slider texAnimFrameRateSlider;
	wi::gui::Slider texAnimDirectionSliderU;
	wi::gui::Slider texAnimDirectionSliderV;
	wi::gui::Slider texMulSliderX;
	wi::gui::Slider texMulSliderY;
	wi::gui::Slider alphaRefSlider;
	wi::gui::Slider sheenRoughnessSlider;
	wi::gui::Slider clearcoatSlider;
	wi::gui::Slider clearcoatRoughnessSlider;
	wi::gui::Slider blendTerrainSlider;

	wi::gui::ComboBox colorComboBox;
	wi::gui::ColorPicker colorPicker;

	wi::gui::ComboBox textureSlotComboBox;
	wi::gui::Button textureSlotButton;
	wi::gui::Label textureSlotLabel;
	wi::gui::TextInputField textureSlotUvsetField;

	void ResizeLayout() override;
};

