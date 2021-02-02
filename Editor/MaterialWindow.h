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
	wiCheckBox shadowCasterCheckBox;
	wiCheckBox useVertexColorsCheckBox;
	wiCheckBox specularGlossinessCheckBox;
	wiCheckBox occlusionPrimaryCheckBox;
	wiCheckBox occlusionSecondaryCheckBox;
	wiCheckBox windCheckBox;
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

	struct TextureSlotControls
	{
		std::string name;
		wiLabel label;
		wiButton button;
		wiTextInputField uvsetField;

		void SetEnabled(bool value)
		{
			label.SetEnabled(value);
			button.SetEnabled(value);
			uvsetField.SetEnabled(value);
		}
	};
	TextureSlotControls slots[wiScene::MaterialComponent::TEXTURESLOT_COUNT];

	wiComboBox colorComboBox;
	wiColorPicker colorPicker;
};

