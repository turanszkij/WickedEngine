#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;
class wiButton;
class wiTextInputField;

class EditorComponent;

class MaterialWindow;

class HairParticleWindow
{
public:
	HairParticleWindow(EditorComponent* editor);
	~HairParticleWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	void UpdateData();

	wiScene::wiHairParticle* GetHair();

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
	wiSlider* viewDistanceSlider;
	wiTextInputField* framesXInput;
	wiTextInputField* framesYInput;
	wiTextInputField* frameCountInput;
	wiTextInputField* frameStartInput;
	wiComboBox* shadingRateComboBox;

};

