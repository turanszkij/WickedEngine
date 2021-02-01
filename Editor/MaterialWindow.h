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

	wiLabel texture_baseColor_Label;
	wiLabel texture_normal_Label;
	wiLabel texture_surface_Label;
	wiLabel texture_displacement_Label;
	wiLabel texture_emissive_Label;
	wiLabel texture_occlusion_Label;
	wiLabel texture_transmission_Label;
	wiLabel texture_sheenColor_Label;
	wiLabel texture_sheenRoughness_Label;
	wiLabel texture_clearcoat_Label;
	wiLabel texture_clearcoatRoughness_Label;
	wiLabel texture_clearcoatNormal_Label;

	wiButton texture_baseColor_Button;
	wiButton texture_normal_Button;
	wiButton texture_surface_Button;
	wiButton texture_displacement_Button;
	wiButton texture_emissive_Button;
	wiButton texture_occlusion_Button;
	wiButton texture_transmission_Button;
	wiButton texture_sheenColor_Button;
	wiButton texture_sheenRoughness_Button;
	wiButton texture_clearcoat_Button;
	wiButton texture_clearcoatRoughness_Button;
	wiButton texture_clearcoatNormal_Button;

	wiTextInputField texture_baseColor_uvset_Field;
	wiTextInputField texture_normal_uvset_Field;
	wiTextInputField texture_surface_uvset_Field;
	wiTextInputField texture_displacement_uvset_Field;
	wiTextInputField texture_emissive_uvset_Field;
	wiTextInputField texture_occlusion_uvset_Field;
	wiTextInputField texture_transmission_uvset_Field;
	wiTextInputField texture_sheenColor_uvset_Field;
	wiTextInputField texture_sheenRoughness_uvset_Field;
	wiTextInputField texture_clearcoat_uvset_Field;
	wiTextInputField texture_clearcoatRoughness_uvset_Field;
	wiTextInputField texture_clearcoatNormal_uvset_Field;

	wiComboBox colorComboBox;
	wiColorPicker colorPicker;
};

