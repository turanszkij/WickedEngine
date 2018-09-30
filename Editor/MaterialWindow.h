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
	wiSlider*	normalMapSlider;
	wiSlider*	roughnessSlider;
	wiSlider*	reflectanceSlider;
	wiSlider*	metalnessSlider;
	wiSlider*	alphaSlider;
	wiSlider*	refractionIndexSlider;
	wiSlider*	emissiveSlider;
	wiSlider*	sssSlider;
	wiSlider*	pomSlider;
	wiSlider*	texAnimFrameRateSlider;
	wiSlider*	texAnimDirectionSliderU;
	wiSlider*	texAnimDirectionSliderV;
	wiSlider*	texMulSliderX;
	wiSlider*	texMulSliderY;
	wiColorPicker* colorPicker;
	wiSlider*	alphaRefSlider;
	wiComboBox* blendModeComboBox;
	wiComboBox* shaderTypeComboBox;

	wiLabel*	texture_baseColor_Label;
	wiLabel*	texture_normal_Label;
	wiLabel*	texture_surface_Label;
	wiLabel*	texture_displacement_Label;

	wiButton*	texture_baseColor_Button;
	wiButton*	texture_normal_Button;
	wiButton*	texture_surface_Button;
	wiButton*	texture_displacement_Button;
};

