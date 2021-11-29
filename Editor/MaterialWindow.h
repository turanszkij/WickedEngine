#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MaterialWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::TextInputField materialNameField;
	wi::widget::Button newMaterialButton;
	wi::widget::CheckBox shadowReceiveCheckBox;
	wi::widget::CheckBox shadowCasterCheckBox;
	wi::widget::CheckBox useVertexColorsCheckBox;
	wi::widget::CheckBox specularGlossinessCheckBox;
	wi::widget::CheckBox occlusionPrimaryCheckBox;
	wi::widget::CheckBox occlusionSecondaryCheckBox;
	wi::widget::CheckBox windCheckBox;
	wi::widget::CheckBox doubleSidedCheckBox;
	wi::widget::Slider normalMapSlider;
	wi::widget::Slider roughnessSlider;
	wi::widget::Slider reflectanceSlider;
	wi::widget::Slider metalnessSlider;
	wi::widget::Slider emissiveSlider;
	wi::widget::Slider transmissionSlider;
	wi::widget::Slider refractionSlider;
	wi::widget::Slider pomSlider;
	wi::widget::Slider displacementMappingSlider;
	wi::widget::Slider subsurfaceScatteringSlider;
	wi::widget::Slider texAnimFrameRateSlider;
	wi::widget::Slider texAnimDirectionSliderU;
	wi::widget::Slider texAnimDirectionSliderV;
	wi::widget::Slider texMulSliderX;
	wi::widget::Slider texMulSliderY;
	wi::widget::Slider alphaRefSlider;
	wi::widget::ComboBox shaderTypeComboBox;
	wi::widget::ComboBox blendModeComboBox;
	wi::widget::ComboBox shadingRateComboBox;
	wi::widget::Slider sheenRoughnessSlider;
	wi::widget::Slider clearcoatSlider;
	wi::widget::Slider clearcoatRoughnessSlider;

	wi::widget::ComboBox colorComboBox;
	wi::widget::ColorPicker colorPicker;

	wi::widget::ComboBox textureSlotComboBox;
	wi::widget::Button textureSlotButton;
	wi::widget::Label textureSlotLabel;
	wi::widget::TextInputField textureSlotUvsetField;
};

