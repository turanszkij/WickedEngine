#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;
class wiButton;

class MaterialWindow;

class HairParticleWindow
{
public:
	HairParticleWindow(wiGUI* gui);
	~HairParticleWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	void UpdateData();

	wiSceneSystem::wiHairParticle* GetHair();

	wiGUI* GUI;

	wiWindow*	hairWindow;

	wiButton* addButton;
	wiComboBox*	meshComboBox;
	wiSlider* lengthSlider;
	wiSlider* stiffnessSlider;
	wiSlider* randomnessSlider;
	wiSlider* countSlider;
	wiSlider* segmentcountSlider;
	wiSlider* randomSeedSlider;

};

