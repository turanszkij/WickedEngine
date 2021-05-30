#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MaterialWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiTextInputField materialNameField;
	wiButton newMaterialButton;
	wiCheckBox shadowReceiveCheckBox;
	wiCheckBox shadowCasterCheckBox;
	wiCheckBox useVertexColorsCheckBox;
	wiCheckBox specularGlossinessCheckBox;
	wiCheckBox occlusionPrimaryCheckBox;
	wiCheckBox occlusionSecondaryCheckBox;
	wiCheckBox windCheckBox;
	wiCheckBox doubleSidedCheckBox;
	wiSlider normalMapSlider;
	wiSlider roughnessSlider;
	wiSlider reflectanceSlider;
	wiSlider metalnessSlider;
	wiSlider emissiveSlider;
	wiSlider transmissionSlider;
	wiSlider refractionSlider;
	wiSlider pomSlider;
	wiSlider displacementMappingSlider;
	wiSlider subsurfaceScatteringSlider;
	wiSlider texAnimFrameRateSlider;
	wiSlider texAnimDirectionSliderU;
	wiSlider texAnimDirectionSliderV;
	wiSlider texMulSliderX;
	wiSlider texMulSliderY;
	wiSlider alphaRefSlider;
	wiComboBox shaderTypeComboBox;
	wiComboBox blendModeComboBox;
	wiComboBox shadingRateComboBox;
	wiSlider sheenRoughnessSlider;
	wiSlider clearcoatSlider;
	wiSlider clearcoatRoughnessSlider;

	wiComboBox colorComboBox;
	wiColorPicker colorPicker;

	wiComboBox textureSlotComboBox;
	wiButton textureSlotButton;
	wiLabel textureSlotLabel;
	wiTextInputField textureSlotUvsetField;
};

