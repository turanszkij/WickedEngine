#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MaterialWindow;

class HairParticleWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	void UpdateData();

	wiScene::wiHairParticle* GetHair();

	wiButton addButton;
	wiComboBox	meshComboBox;
	wiSlider lengthSlider;
	wiSlider stiffnessSlider;
	wiSlider randomnessSlider;
	wiSlider countSlider;
	wiSlider segmentcountSlider;
	wiSlider randomSeedSlider;
	wiSlider viewDistanceSlider;
	wiTextInputField framesXInput;
	wiTextInputField framesYInput;
	wiTextInputField frameCountInput;
	wiTextInputField frameStartInput;

};

