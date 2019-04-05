#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiColorPicker;
class wiButton;
class wiComboBox;
class wiTextInputField;

class MaterialWindow
{
public:
	MaterialWindow(wiGUI* gui);
	~MaterialWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow*	materialWindow;
	wiTextInputField*	materialNameField;
	wiCheckBox* waterCheckBox;
	wiCheckBox* planarReflCheckBox;
	wiCheckBox* shadowCasterCheckBox;
	wiCheckBox* flipNormalMapCheckBox;
	wiCheckBox* useVertexColorsCheckBox;
	wiCheckBox* specularGlossinessCheckBox;
	wiCheckBox* occlusionPrimaryCheckBox;
	wiCheckBox* occlusionSecondaryCheckBox;
	wiSlider*	normalMapSlider;
	wiSlider*	roughnessSlider;
	wiSlider*	reflectanceSlider;
	wiSlider*	metalnessSlider;
	wiSlider*	alphaSlider;
	wiSlider*	refractionIndexSlider;
	wiSlider*	emissiveSlider;
	wiSlider*	sssSlider;
	wiSlider*	pomSlider;
	wiSlider*	displacementMappingSlider;
	wiSlider*	texAnimFrameRateSlider;
	wiSlider*	texAnimDirectionSliderU;
	wiSlider*	texAnimDirectionSliderV;
	wiSlider*	texMulSliderX;
	wiSlider*	texMulSliderY;
	wiColorPicker* baseColorPicker;
	wiColorPicker* emissiveColorPicker;
	wiSlider*	alphaRefSlider;
	wiComboBox* blendModeComboBox;
	wiComboBox* shaderTypeComboBox;

	wiLabel*	texture_baseColor_Label;
	wiLabel*	texture_normal_Label;
	wiLabel*	texture_surface_Label;
	wiLabel*	texture_displacement_Label;
	wiLabel*	texture_emissive_Label;
	wiLabel*	texture_occlusion_Label;

	wiButton*	texture_baseColor_Button;
	wiButton*	texture_normal_Button;
	wiButton*	texture_surface_Button;
	wiButton*	texture_displacement_Button;
	wiButton*	texture_emissive_Button;
	wiButton*	texture_occlusion_Button;

	wiTextInputField*	texture_baseColor_uvset_Field;
	wiTextInputField*	texture_normal_uvset_Field;
	wiTextInputField*	texture_surface_uvset_Field;
	wiTextInputField*	texture_displacement_uvset_Field;
	wiTextInputField*	texture_emissive_uvset_Field;
	wiTextInputField*	texture_occlusion_uvset_Field;
};

