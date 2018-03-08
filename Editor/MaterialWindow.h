#pragma once

struct Material;
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

	void SetMaterial(Material* mat);

	wiGUI* GUI;

	Material* material;

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
	wiSlider*	movingTexSliderU;
	wiSlider*	movingTexSliderV;
	wiSlider*	texMulSliderX;
	wiSlider*	texMulSliderY;
	wiColorPicker* colorPicker;
	wiSlider*	alphaRefSlider;
	wiComboBox* blendModeComboBox;

	wiLabel*	texture_baseColor_Label;
	wiLabel*	texture_normal_Label;
	wiLabel*	texture_surface_Label;
	wiLabel*	texture_displacement_Label;

	wiButton*	texture_baseColor_Button;
	wiButton*	texture_normal_Button;
	wiButton*	texture_surface_Button;
	wiButton*	texture_displacement_Button;
};

