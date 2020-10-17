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
	wiSlider refractionIndexSlider;
	wiSlider emissiveSlider;
	wiSlider pomSlider;
	wiSlider displacementMappingSlider;
	wiSlider texAnimFrameRateSlider;
	wiSlider texAnimDirectionSliderU;
	wiSlider texAnimDirectionSliderV;
	wiSlider texMulSliderX;
	wiSlider texMulSliderY;
	wiSlider alphaRefSlider;
	wiComboBox shaderTypeComboBox;
	wiComboBox blendModeComboBox;
	wiComboBox sssComboBox;
	wiComboBox shadingRateComboBox;

	wiLabel texture_baseColor_Label;
	wiLabel texture_normal_Label;
	wiLabel texture_surface_Label;
	wiLabel texture_displacement_Label;
	wiLabel texture_emissive_Label;
	wiLabel texture_occlusion_Label;

	wiButton texture_baseColor_Button;
	wiButton texture_normal_Button;
	wiButton texture_surface_Button;
	wiButton texture_displacement_Button;
	wiButton texture_emissive_Button;
	wiButton texture_occlusion_Button;

	wiTextInputField texture_baseColor_uvset_Field;
	wiTextInputField texture_normal_uvset_Field;
	wiTextInputField texture_surface_uvset_Field;
	wiTextInputField texture_displacement_uvset_Field;
	wiTextInputField texture_emissive_uvset_Field;
	wiTextInputField texture_occlusion_uvset_Field;

	wiComboBox colorComboBox;
	wiColorPicker colorPicker;
};

