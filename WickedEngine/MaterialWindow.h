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
	wiSlider*	normalMapSlider;
	wiSlider*	roughnessSlider;
	wiSlider*	reflectanceSlider;
	wiSlider*	metalnessSlider;
	wiSlider*	alphaSlider;
	wiSlider*	refractionIndexSlider;
	wiSlider*	emissiveSlider;
	wiSlider*	sssSlider;
	wiSlider*	pomSlider;
	wiButton*	colorPickerToggleButton;
	wiColorPicker* colorPicker;
};

