#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiColorPicker;
class wiButton;

class MaterialWindow
{
public:
	MaterialWindow(wiGUI* gui);
	~MaterialWindow();

	void SetMaterial(Material* mat);

	wiGUI* GUI;

	Material* material;

	wiWindow*	materialWindow;
	wiLabel*	materialLabel;
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
	wiButton*	colorPickerToggleButton;
	wiColorPicker* colorPicker;
	wiSlider*	alphaRefSlider;
};

